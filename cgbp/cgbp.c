
// cgbp.c

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "cgbp.h"

#define CGBP_FPS 30

static inline ssize_t fbdev_write_term(const char *str, size_t len) {
	ssize_t ret;
	if(len == 0)
		len = strlen(str);
	do {
		errno = 0;
		ret = write(STDOUT_FILENO, str, len);
	} while(ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));
	return ret;
}

static inline int fbdev_setup_term(struct cgbp *c) {
	tcflag_t lflag_orig;
	// turn off cursor
	fbdev_write_term("\x1b[?25l", 6);
	c->old_fl = fcntl(STDIN_FILENO, F_GETFL, 0);
	if(fcntl(STDIN_FILENO, F_SETFL, c->old_fl|O_NONBLOCK) < 0) {
		perror("fcntl");
		return -1;
	}
	if(tcgetattr(STDIN_FILENO, &c->tc) < 0) {
		perror("tcgetattr");
		return -1;
	}
	c->tc_set = 1;
	lflag_orig = c->tc.c_lflag;
	c->tc.c_lflag &= ~(ICANON|ECHO);
	if(tcsetattr(STDIN_FILENO, TCSANOW, &c->tc) < 0) {
		perror("tcsetattr");
		return -1;
	}
	c->tc.c_lflag = lflag_orig;
	return 0;
}

int cgbp_init(struct cgbp *c) {
	if(fbdev_setup_term(c) < 0)
		return -1;

	c->fbfd = open("/dev/fb0", O_RDWR);
	if(c->fbfd < 0) {
		perror("open");
		return -1;
	}
	ioctl(c->fbfd, FBIOGET_VSCREENINFO, &c->vinfo);
	ioctl(c->fbfd, FBIOGET_FSCREENINFO, &c->finfo);

	// unneeded - need to no longer assume a pixel in *fbmm to be uint32_t
	c->fbmm = mmap(0, cgbp_buffer_size(c), PROT_READ|PROT_WRITE,
	               MAP_SHARED, c->fbfd, 0);
	if(c->fbmm == MAP_FAILED) {
		perror("mmap");
		c->fbmm = NULL;
		return -1;
	}
	c->data = malloc(cgbp_buffer_size(c));
	if(c->data == NULL) {
		perror("malloc");
		return -1;
	}
	memset(c->data, 0, cgbp_buffer_size(c));

	if(timer_create(CLOCK_MONOTONIC, NULL, &c->timerid) < 0) {
		perror("timer_create");
		return -1;
	}
	c->timer_set = 1;
	return 0;
}

static char cgbp_ticked = 0;
void cgbp_alarm(int sig) {
	if(sig == SIGALRM)
		cgbp_ticked = 1;
}

int cgbp_main(struct cgbp *c, void *data, cgbp_updatecb *cb) {
	struct sigaction sa = {
		.sa_flags = 0,
		.sa_handler = cgbp_alarm,
	};
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);
	timer_settime(c->timerid, 0, &(struct itimerspec){
		.it_value = { 0, 1e9 / CGBP_FPS },
		.it_interval = { 0, 1e9 / CGBP_FPS },
	}, NULL);

	do {
		if(cgbp_ticked == 0)
			sleep(1);
		if(cb(c, data) < 0)
			return -1;
		memcpy(c->fbmm, c->data, cgbp_buffer_size(c));
	} while(c->running);

	timer_settime(c->timerid, 0, &(struct itimerspec){
		.it_value = { 0, 0 },
		.it_interval = { 0, 0 },
	}, NULL);
	return 0;
}

static inline void fbdev_cleanup_term(struct cgbp *c) {
	// turn on cursor
	fbdev_write_term("\x1b[?25h", 6);
	if(fcntl(STDIN_FILENO, F_SETFL, c->old_fl) < 0)
		perror("fcntl");
	if(c->tc_set == 1 && tcsetattr(STDIN_FILENO, TCSANOW, &c->tc) < 0)
		perror("tcsetattr");
}

void cgbp_cleanup(struct cgbp *c) {
	if(c->fbmm != NULL) {
		memset(c->fbmm, 0, cgbp_buffer_size(c));
		munmap(c->fbmm, cgbp_buffer_size(c));
	}
	if(c->fbfd >= 0)
		close(c->fbfd);
	if(c->data != NULL)
		free(c->data);
	if(c->timer_set == 1)
		timer_delete(c->timerid);
	fbdev_cleanup_term(c);
}
