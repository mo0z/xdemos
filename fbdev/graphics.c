
// graphics.c

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

#include "graphics.h"

#define GRAPHICS_FPS 30

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

static inline int fbdev_setup_term(struct graphics *g) {
	tcflag_t lflag_orig;
	// turn off cursor
	fbdev_write_term("\x1b[?25l", 6);
	g->old_fl = fcntl(STDIN_FILENO, F_GETFL, 0);
	if(fcntl(STDIN_FILENO, F_SETFL, g->old_fl|O_NONBLOCK) < 0) {
		perror("fcntl");
		return -1;
	}
	if(tcgetattr(STDIN_FILENO, &g->tc) < 0) {
		perror("tcgetattr");
		return -1;
	}
	g->tc_set = 1;
	lflag_orig = g->tc.c_lflag;
	g->tc.c_lflag &= ~(ICANON|ECHO);
	if(tcsetattr(STDIN_FILENO, TCSANOW, &g->tc) < 0) {
		perror("tcsetattr");
		return -1;
	}
	g->tc.c_lflag = lflag_orig;
	return 0;
}

int graphics_init(struct graphics *g) {
	if(fbdev_setup_term(g) < 0)
		return -1;

	g->fbfd = open("/dev/fb0", O_RDWR);
	if(g->fbfd < 0) {
		perror("open");
		return -1;
	}
	ioctl(g->fbfd, FBIOGET_VSCREENINFO, &g->vinfo);
	ioctl(g->fbfd, FBIOGET_FSCREENINFO, &g->finfo);

	// unneeded - need to no longer assume a pixel in *fbmm to be uint32_t
	g->vinfo.grayscale = 0;
	g->vinfo.bits_per_pixel = 32;
	ioctl(g->fbfd, FBIOPUT_VSCREENINFO, &g->vinfo);
	ioctl(g->fbfd, FBIOGET_VSCREENINFO, &g->vinfo);

	g->fbmm = mmap(0, graphics_buffer_size(g), PROT_READ|PROT_WRITE,
	               MAP_SHARED, g->fbfd, 0);
	if(g->fbmm == MAP_FAILED) {
		perror("mmap");
		g->fbmm = NULL;
		return -1;
	}
	g->data = malloc(graphics_buffer_size(g));
	if(g->data == NULL) {
		perror("malloc");
		return -1;
	}
	memset(g->data, 0, graphics_buffer_size(g));

	if(timer_create(CLOCK_MONOTONIC, NULL, &g->timerid) < 0) {
		perror("timer_create");
		return -1;
	}
	g->timer_set = 1;
	return 0;
}

static char graphics_ticked = 0;
void graphics_alarm(int sig) {
	if(sig == SIGALRM)
		graphics_ticked = 1;
}

int graphics_main(struct graphics *g, void *data, graphics_updatecb *cb) {
	struct sigaction sa = {
		.sa_flags = 0,
		.sa_handler = graphics_alarm,
	};
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);
	timer_settime(g->timerid, 0, &(struct itimerspec){
		.it_value = { 0, 1e9 / GRAPHICS_FPS },
		.it_interval = { 0, 1e9 / GRAPHICS_FPS },
	}, NULL);

	do {
		if(graphics_ticked == 0)
			sleep(1);
		if(cb(g, data) < 0)
			return -1;
		memcpy(g->fbmm, g->data, graphics_buffer_size(g));
	} while(g->running);

	timer_settime(g->timerid, 0, &(struct itimerspec){
		.it_value = { 0, 0 },
		.it_interval = { 0, 0 },
	}, NULL);
	return 0;
}

void graphics_cleanup(struct graphics *g) {
	if(g->fbmm != NULL) {
		memset(g->fbmm, 0, graphics_buffer_size(g));
		munmap(g->fbmm, graphics_buffer_size(g));
	}
	if(g->fbfd >= 0)
		close(g->fbfd);
	// turn on cursor
	fbdev_write_term("\x1b[?25h", 6);
	if(fcntl(STDIN_FILENO, F_SETFL, g->old_fl) < 0)
		perror("fcntl");
	if(g->tc_set == 1 && tcsetattr(STDIN_FILENO, TCSANOW, &g->tc) < 0)
		perror("tcsetattr");
	if(g->data != NULL)
		free(g->data);
	if(g->timer_set == 1)
		timer_delete(g->timerid);
}
