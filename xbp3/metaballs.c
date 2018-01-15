
/* metaballs.c
 *
 * Copyright (c) 2017, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <X11/XKBlib.h>

#include "hsv.h"
#include "xbp.h"
#include "xbp_time.h"

#define LIMIT(x, l) (((x) > (l)) ? (l) : (x))
#define DIST_MULT(w) (w)
#define NUM_BALLS 10
#define RADIUS_LOW(w)  ((w) / 20)
#define RADIUS_HIGH(w) ((w) / 8)
#define MAX_SPEED(w) (w / 100)
#define MAX_DIST (256.0 * NUM_BALLS)

struct metaballs {
	struct xbp_time xt;
	struct {
		float radius;
		int x, y;
		int speed_x, speed_y;
	} balls[NUM_BALLS];
	float *dist_cache;
	uint32_t rgb_cache[256];
	struct timespec total_frametime, total_runtime;
	size_t num_frames;
};

#define ABS(x) ((x) < 0 ? -(x) : (x))

static inline int metaballs_dist(struct metaballs *m, int x, int y, int width) {
	register int a, b;
	register size_t i;
	register float dist = 0;
	for(i = 0; i < NUM_BALLS; i++) {
		a = x - m->balls[i].x;
		b = y - m->balls[i].y;
		dist += m->dist_cache[ABS(a) + width * ABS(b)] * m->balls[i].radius;
	}
	if(dist > 255)
		return 255;
	return dist;
}

static inline float rsqrt(float n) {
	void *v;
	int i;
	float x2;
	x2 = n * 0.5F;
	v  = &n;
	i  = *(int*)v;                      // evil floating point bit level hacking
	i  = 0x5f3759df - (i >> 1);         // what the fuck?
	v  = &i;
	n  = *(float*)v;
	n  = n * (1.5F - (x2 * n * n));     // 1st iteration
	return n;
}

int action(struct xbp *x, XEvent *ev) {
	struct metaballs *m = xbp_get_data(x);
	size_t i;
	float rnd, mult, hue, rgb[3] = { 0.0, 0.0, 0.0 };
	KeySym keysym = XK_space;
	if(ev != NULL)
		keysym = XkbKeycodeToKeysym(x->disp, ev->xkey.keycode, 0, 0);
	if(keysym == XK_f) {
		if(x->fullscreen)
			return xbp_fullscreen_leave(x);
		else
			return xbp_fullscreen(x);
	}
	if(keysym != XK_space)
		return 0;
	rnd = (float)rand() / RAND_MAX * 2.0;
	mult = .25 + (float)rand() / RAND_MAX * 1.75;
	for(i = 0; i < 256; i++) {
		if(rnd > 1.0)
			hue = (float)i / 255.0 - (rnd - 1.0);
		else
			hue = (float)i / 255.0 + rnd;
		hue *= mult;
		hsv_to_rgb(rgb, hue - floor(hue), 1.0, .8);
		((unsigned char*)(m->rgb_cache + i))[0] = rgb[0] * 255;
		((unsigned char*)(m->rgb_cache + i))[1] = rgb[1] * 255;
		((unsigned char*)(m->rgb_cache + i))[2] = rgb[2] * 255;
		((unsigned char*)(m->rgb_cache + i))[3] =          255;
	}
	return 0;
	(void)x;
	(void)ev;
}

int update(struct xbp *x) {
	register struct metaballs *m = xbp_get_data(x);
	register size_t i;
	register int px, py;
	xbp_time_frame_start(&m->xt);
	for(i = 0; i < NUM_BALLS; i++) {
		m->balls[i].x += m->balls[i].speed_x;
		if(m->balls[i].x < 0 || m->balls[i].x >= XBP_WIDTH(x)) {
			m->balls[i].speed_x *= -1;
			m->balls[i].x += m->balls[i].speed_x;
		}
		m->balls[i].y += m->balls[i].speed_y;
		if(m->balls[i].y < 0 || m->balls[i].y >= XBP_HEIGHT(x)) {
			m->balls[i].speed_y *= -1;
			m->balls[i].y += m->balls[i].speed_y;
		}
	}
	for(py = 0; py < XBP_HEIGHT(x); py++)
		for(px = 0; px < XBP_WIDTH(x); px++)
			xbp_set_pixel(x, px, py, m->rgb_cache[
				metaballs_dist(m, px, py, XBP_WIDTH(x))
			]);
	xbp_time_frame_end(&m->xt);
	return 0;
}

int resize(struct xbp *x) {
	struct metaballs *m = xbp_get_data(x);
	float fx, fy, dist;
	size_t i, j, l;
	m->dist_cache = realloc(
		m->dist_cache,
		XBP_WIDTH(x) * XBP_HEIGHT(x) * sizeof *m->dist_cache * NUM_BALLS
	);
	if(m->dist_cache == NULL) {
		perror("realloc");
		return -1;
	}
	l = XBP_WIDTH(x) * XBP_HEIGHT(x);
	for(j = 0; j < NUM_BALLS; j++)
		m->dist_cache[j * l] = (float)INT_MAX;
	for(i = 1; i < l; i++) {
		fx = i % XBP_WIDTH(x);
		fy = i / XBP_HEIGHT(x);
		dist = rsqrt(fx * fx + fy * fy);
		for(j = 0; j < NUM_BALLS; j++)
			m->dist_cache[j * l + i] = dist * DIST_MULT(XBP_WIDTH(x));
	}
	for(i = 0; i < NUM_BALLS; i++) {
		if(m->balls[i].x > XBP_WIDTH(x))
			m->balls[i].x = XBP_WIDTH(x) - 1;
		if(m->balls[i].y > XBP_HEIGHT(x))
			m->balls[i].y = XBP_HEIGHT(x) - 1;
	}
	return 0;
}

int main(void) {
	struct xbp x = {
		.config = {
			.width = 800,
			.height = 600,
			.max_fps = 0,
			.fullscreen = 0,
			.alpha = 1,
			.defaultkeys = 1,
			.image = 1,
		},
		.callbacks = {
			.update = update,
			.resize = resize,
			.listeners = (struct xbp_listener*[2]){
				&(struct xbp_listener){
					.event = KeyPress,
					.callback = action,
				},
				NULL,
			},
		},
	};
	struct metaballs m;
	int speed_mod, ret = EXIT_FAILURE;
	size_t i;
	srand(time(NULL));
	if(xbp_time_init(&m.xt) < 0)
		return EXIT_FAILURE;
	if(xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	xbp_set_data(&x, &m);
	m.dist_cache = NULL;
	speed_mod = MAX_SPEED(XBP_WIDTH(&x));
	for(i = 0; i < NUM_BALLS; i++) {
		m.balls[i].radius = (float)(
			RADIUS_LOW(XBP_WIDTH(&x)) + random() % (
				RADIUS_HIGH(XBP_WIDTH(&x)) -
				RADIUS_LOW(XBP_WIDTH(&x))
			)
		) / NUM_BALLS;
		m.balls[i].x = random() % XBP_WIDTH(&x);
		m.balls[i].y = random() % XBP_HEIGHT(&x);
		m.balls[i].speed_x = random() % speed_mod;
		m.balls[i].speed_y = random() % speed_mod;
	}
	if(resize(&x) < 0)
		goto error;
	action(&x, NULL);
	if(clock_gettime(CLOCK_MONOTONIC, &m.total_runtime) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		goto error;
	}
	if(xbp_main(&x) == 0 && xbp_time_print_stats(&m.xt) == 0)
		ret = EXIT_SUCCESS;
error:
	free(m.dist_cache);
	xbp_cleanup(&x);
	return ret;
}
