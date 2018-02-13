
// langtonsant.c

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

struct langtonsant {
	size_t x, y, bytesize;
	uint32_t *data;
	char direction;
};

int nonneg_mod(int num, int div) {
	num = num % div;
	if(num < 0)
		num += div;
	return num;
}

int langtonsant_setup(struct langtonsant *l, size_t x, size_t y) {
	l->x = x / 2;
	l->y = y / 2;
	l->bytesize = x * y * sizeof *l->data;
	l->data = malloc(l->bytesize);
	if(l->data == NULL) {
		perror("malloc");
		return -1;
	}
	l->direction = 0;
	return 0;
}

int langtonsant_update(struct langtonsant *l, size_t x, size_t y) {
	if(l->data[l->y * x + l->x] == 0) {
		l->direction++;
		l->data[l->y * x + l->x] = 0xffffff;
	} else {
		l->direction--;
		l->data[l->y * x + l->x] = 0;
	}
	switch(nonneg_mod(l->direction, 4)) {
	case 0:
		if(l->y == 0)
			l->y = y - 1;
		else
			l->y--;
		break;
	case 1:
		if(l->x == x - 1)
			l->x = 0;
		else
			l->x++;
		break;
	case 2:
		if(l->y == y - 1)
			l->y = 0;
		else
			l->y++;
		break;
	case 3:
		if(l->x == 0)
			l->x = x - 1;
		else
			l->x--;
		break;
	}
	return 0;
}

void langtonsant_cleanup(struct langtonsant *l) {
	return;
	(void)l;
}

int fbdev_update(struct langtonsant *l,
                 uint32_t *fbmm, size_t x, size_t y) {
	size_t i;
	for(i = 0; i < 1000; i++)
		if(langtonsant_update(l, x, y) < 0)
			return -1;
	memcpy(fbmm, l->data, l->bytesize);
	return 0;
}

static char fbdev_ticked = 0;
void fbdev_alarm(int sig) {
	if(sig == SIGALRM)
		fbdev_ticked = 1;
}

int main(void) {
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	struct langtonsant l;
	struct sigaction sa = {
		.sa_flags = 0,
		.sa_handler = fbdev_alarm,
	};
	struct termios tc;
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = 1e9 / 30,
	};
	tcflag_t lflag_orig;
	timer_t timerid = 0;
	size_t screensize;
	int fbfd = open("/dev/fb0", O_RDWR), old_fl, ret;
	uint32_t *fbmm = NULL;
	char c, tc_set = 0;
	old_fl = fcntl(STDIN_FILENO, F_GETFL, 0);

	fputs("\x1b[?25l", stdout);
	fflush(stdout);
	if(fbfd < 0) {
		perror("open");
		goto error;
	}

	if(tcgetattr(STDIN_FILENO, &tc) < 0) {
		perror("tcgetattr");
		goto error;
	}
	tc_set = 1;
	lflag_orig = tc.c_lflag;
	tc.c_lflag &= ~(ICANON|ECHO);
	if(tcsetattr(STDIN_FILENO, TCSANOW, &tc) < 0) {
		perror("tcsetattr");
		goto error;
	}
	tc.c_lflag = lflag_orig;

	if(fcntl(STDIN_FILENO, F_SETFL, old_fl|O_NONBLOCK) < 0)
		goto error;

	//Get variable screen information
	ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.grayscale = 0;
	vinfo.bits_per_pixel = 32;
	ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo);
	ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);
	ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);

	screensize = vinfo.yres_virtual * finfo.line_length;
	fbmm = mmap(0, screensize, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
	if(fbmm == MAP_FAILED) {
		perror("mmap");
		fbmm = NULL;
		goto error;
	}

	memset(fbmm, 0, vinfo.xres * vinfo.yres * sizeof *fbmm);
	langtonsant_setup(&l, vinfo.xres, vinfo.yres);

	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);
	if(timer_create(CLOCK_MONOTONIC, NULL, &timerid) < 0) {
		perror("timer_create");
		goto error;
	}
	timer_settime(timerid, 0, &(struct itimerspec){
		.it_value = ts,
		.it_interval = ts,
	}, NULL);

	do {
		if(fbdev_ticked == 0)
			sleep(1);
		if(fbdev_update(&l, fbmm, vinfo.xres, vinfo.yres) < 0)
			goto error;
	} while(read(STDIN_FILENO, &c, 1) <= 0);
	ret = EXIT_SUCCESS;
error:
	timer_delete(timerid);
	langtonsant_cleanup(&l);
	if(fbmm != NULL) {
		memset(fbmm, 0, screensize);
		munmap(fbmm, screensize);
	}
	close(fbfd);
	if(tc_set == 1 && tcsetattr(STDIN_FILENO, TCSANOW, &tc) < 0) {
		perror("tcsetattr");
		ret = EXIT_FAILURE;
	}
	if(fcntl(STDIN_FILENO, F_SETFL, old_fl) < 0) {
		perror("fcntl");
		ret = EXIT_FAILURE;
	}
	fputs("\x1b[?25h", stdout);
	return ret;
}
