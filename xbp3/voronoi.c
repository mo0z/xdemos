
// voronoi.c

#include <stdlib.h>
#include <time.h>
#include <X11/XKBlib.h>

#include "point.h"
#include "xbp.h"
#include "hsv.h"

#define NUM_POINTS 10

struct voronoi {
	struct point points[NUM_POINTS];
	unsigned long colors[NUM_POINTS];
	size_t num_frames;
	struct timespec total_frametime, start_time;
};

static inline struct timespec timespec_add(struct timespec ts1,
                                           struct timespec ts2) {
	register struct timespec result = ts1;
	register long sec;

	result.tv_nsec += ts2.tv_nsec;
	result.tv_sec += ts2.tv_sec;

	if(result.tv_nsec >= XBP_BILLION) {
		result.tv_sec += result.tv_nsec / XBP_BILLION;
		result.tv_nsec %= XBP_BILLION;
	}
	if(result.tv_nsec < 0) {
		sec = 1 + -(result.tv_nsec / XBP_BILLION);
		result.tv_nsec += XBP_BILLION * sec;
		result.tv_sec -= sec;
	}
	return result;
}

static inline struct timespec timespec_diff(struct timespec ts1,
                                            struct timespec ts2) {
	ts2.tv_sec *= -1;
	ts2.tv_nsec *= -1;
	return timespec_add(ts1, ts2);
}

static inline int in_points(const struct point p, struct point *points,
                            size_t len, size_t skip) {
	size_t i;
	for(i = 0; i < len; i++) {
		if(i == skip)
			continue;
		if(fabs(p.x - points[i].x) < .5 && fabs(p.y - points[i].y) < .5)
			return 1;
	}
	return 0;
}

void wiggle_points(struct xbp *x, struct voronoi *v, size_t i) {
	struct point new;
	do {
		new.x = v->points[i].x + (random() % 3 - 1);
		new.y = v->points[i].y + (random() % 3 - 1);
	} while(new.x < .5 || new.x >= XBP_WIDTH(x) - .5 ||
	  new.y < .5 || new.y >= XBP_HEIGHT(x) - .5 ||
	  in_points(new, v->points, NUM_POINTS, i) == 1);
	v->points[i] = new;
}

int update(struct xbp *x) {
	struct voronoi *v = xbp_get_data(x);
	struct timespec frame_start, frame_end;
	size_t i, j;
	if(clock_gettime(CLOCK_MONOTONIC, &frame_start) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	XClearWindow(x->disp, x->win);
	XSetForeground(x->disp, x->gc, WhitePixel(x->disp, x->scr));
	for(i = 0; i < NUM_POINTS; i++)
		wiggle_points(x, v, i);
	for(i = 0; i < NUM_POINTS; i++)
		for(j = i + 1; j < NUM_POINTS; j++)
			XDrawLine(x->disp, x->win, x->gc, v->points[i].x, v->points[i].y,
				v->points[j].x, v->points[j].y);
	if(clock_gettime(CLOCK_MONOTONIC, &frame_end) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	frame_end = timespec_diff(frame_end, frame_start);
	v->total_frametime = timespec_add(v->total_frametime, frame_end);
	v->num_frames++;
	XFlush(x->disp);
	return 0;
}

static inline void random_points_colors(struct voronoi *v, size_t w, size_t h) {
	float rgb[4] = { .0 };
	size_t i;
	for(i = 0; i < NUM_POINTS;
	  i += in_points(v->points[i], v->points, i, i) == 0) {
		v->points[i].x = rand() % w;
		v->points[i].y = rand() % h;
	}
	for(i = 0; i < NUM_POINTS; i++) {
		hsv_to_rgb(rgb, random(), 1.0, 1.0);
		v->colors[i] = (int)(rgb[0] * 0xff) | ((int)(rgb[1] * 0xff) << 8) |
			((int)(rgb[2] * 0xff) << 16) | (0xff << 24);
	}
}

int action(struct xbp *x, XEvent *ev) {
	KeySym keysym = XK_space;
	if(ev != NULL)
		keysym = XkbKeycodeToKeysym(x->disp, ev->xkey.keycode, 0, 0);
	if(keysym == XK_space)
		random_points_colors(xbp_get_data(x), XBP_WIDTH(x), XBP_HEIGHT(x));
	return 0;
}

static inline int print_stats(struct voronoi *v) {
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	v->start_time = timespec_diff(ts, v->start_time);
	fprintf(stderr, "total runtime: %.2f\n", ((double)v->start_time.tv_sec
	        + (double)v->start_time.tv_nsec / XBP_BILLION));
	fprintf(stderr, "num frames: %zu\n", v->num_frames);
	fprintf(stderr, "FPS: %.2f\n",
		v->num_frames / ((double)v->start_time.tv_sec
		+ (double)v->start_time.tv_nsec / XBP_BILLION));
	fprintf(stderr, "time spent for calulation in total: %ld.%09ld\n",
	        v->total_frametime.tv_sec, v->total_frametime.tv_nsec);
	return 0;
}

int main(void) {
	struct voronoi v = {
		.total_frametime = { 0, 0 },
		.start_time = { 0, 0 },
		.num_frames = 0,
	};
	struct xbp x = {
		.config = {
			.fullscreen = true,
			.alpha = false,
			.defaultkeys = true,
			.image = false,
		},
		.callbacks = {
			.update = update,
			.listeners = (struct xbp_listener*[3]){
				&(struct xbp_listener){
					.event = KeyPress,
					.callback = action,
				},
				NULL,
			},
		},
	};
	int ret = EXIT_FAILURE;
	srand(time(NULL));
	xbp_set_data(&x, &v);
	if(xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	random_points_colors(&v, XBP_WIDTH(&x), XBP_HEIGHT(&x));
	if(clock_gettime(CLOCK_MONOTONIC, &v.start_time) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		goto error;
	}
	if(xbp_main(&x) == 0 &&
	  print_stats(&v) == 0)
		ret = EXIT_SUCCESS;
error:
	xbp_cleanup(&x);
	return ret;
}
