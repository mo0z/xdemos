
// demo.c

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <X11/XKBlib.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

#include "demo.h"

#define MILLION (1000 * 1000)
#define BILLION (1000 * 1000 * 1000)

static inline struct timespec timespec_diff(struct timespec ts1,
                                            struct timespec ts2) {
	register struct timespec result;
	register long sec;

	if(ts1.tv_sec > ts2.tv_sec) {
		result.tv_nsec = ts1.tv_nsec + (ts1.tv_sec - ts2.tv_sec) * BILLION;
		result.tv_sec = ts2.tv_sec;
	} else {
		result.tv_nsec = ts1.tv_nsec;
		result.tv_sec = ts1.tv_sec;
	}
	result.tv_nsec -= ts2.tv_nsec;
	result.tv_sec -= ts2.tv_sec;

	if(result.tv_nsec >= BILLION) {
		result.tv_nsec %= BILLION;
		result.tv_sec += result.tv_nsec / BILLION;
	}
	if(result.tv_nsec < 0) {
		sec = 1 + -(result.tv_nsec / BILLION);
		result.tv_nsec += BILLION * sec;
		result.tv_sec -= sec;
	}
	return result;
}

static inline void demo_event(struct demo *d) {
	XEvent ev;
	KeySym keysym;
	if(XNextEvent(d->x.disp, &ev))
		return;
	if(ev.type == KeyPress) {
		keysym = XkbKeycodeToKeysym(d->x.disp, ev.xkey.keycode, 0, 0);
		if(keysym == XK_q || keysym == XK_Escape)
			d->running = false;
	}
}

static inline int demo_select(struct demo *d) {
	fd_set fds;
	register int sel, fd = ConnectionNumber(d->x.disp);
	struct timespec frame_end;
again:
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	if(clock_gettime(CLOCK_MONOTONIC, &frame_end) < 0) {
		fprintf(stderr, "Error: clock_dettime()\n");
		return -1;
	}

	frame_end = timespec_diff(frame_end, d->frame_start);
	if(d->frames.max_wait.tv_sec < frame_end.tv_sec
	  || (d->frames.max_wait.tv_sec == frame_end.tv_sec
	   && d->frames.max_wait.tv_nsec < frame_end.tv_nsec))
		d->frames.max_wait = frame_end;
	frame_end = timespec_diff((struct timespec){ 0, 50 * MILLION }, frame_end);
	if(frame_end.tv_sec < 0) {
		d->frames.missed++;
		return 0;
	}

	sel = pselect(fd + 1, &fds, NULL, NULL, &frame_end, NULL);
	if(sel < 0) {
		if(errno == EINTR)
			goto again;
		return -1;
	}
	return 0;
}

int demo_init(struct demo *d) {
	if(xbp_connect(&d->x, NULL) < 0)
		return -1;
	xbp_getfullscreenwin(&d->x, &d->w);
	xbp_cursorinvisible(&d->x, &d->w);
	d->frames.missed = 0;
	d->frames.max_wait.tv_sec = 0;
	d->frames.max_wait.tv_nsec = 0;
	d->running = true;
	return 0;
}

int demo_main(struct demo *d, int (*update_cb)(struct demo*, void*),
              void *data) {
	XGrabKeyboard(d->x.disp, d->w.win, 0, GrabModeAsync, GrabModeAsync,
	              CurrentTime);
	while(d->running) {
		if(clock_gettime(CLOCK_MONOTONIC, &d->frame_start) < 0) {
			fprintf(stderr, "Error: clock_gettime()\n");
			return -1;
		}
		if(update_cb(d, data) < 0)
			return -1;
		XFlush(d->x.disp);
		if(demo_select(d) < 0)
			return -1;
		while(XPending(d->x.disp) > 0)
			demo_event(d);
	}
	XUngrabKeyboard(d->x.disp, CurrentTime);
	fprintf(stderr, "Missed: %"PRIuMAX"\n", d->frames.missed);
	fprintf(stderr, "Max frame time: %ld.%09ld\n",
	        d->frames.max_wait.tv_sec, d->frames.max_wait.tv_nsec);
	return 0;
}

void demo_cleanup(struct demo *d) {
	xbp_destroywin(&d->x, &d->w);
	xbp_disconnect(&d->x);
}
