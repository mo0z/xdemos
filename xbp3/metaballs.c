
/* metaballs.c
 *
 * Copyright (c) 2017, mar77i <mysatyre at gmail dot com>
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

#include "hsv.h"
#include "xbp.h"

#define BILLION (1000 * 1000 * 1000)
#define LIMIT(x, l) (((x) > (l)) ? (l) : (x))
#define DIST_MULT(w) (w)
#define NUM_BALLS 10
#define RADIUS_LOW(w)  ((w) / 40)
#define RADIUS_HIGH(w) ((w) / 16)
#define MAX_SPEED(w) (w / 80)
#define MAX_DIST (256.0 * NUM_BALLS)

struct metaballs {
	struct {
		float radius;
		int x, y;
		int speed_x, speed_y;
	} balls[NUM_BALLS];
	float *dist_cache;
	uint32_t rgb_cache[256];
	struct timespec total_frametime, total_runtime;
	size_t num_frames, height, width;
};

#define timespec_diff(ts1, ts2) ( \
	(ts2).tv_sec *= -1, \
	(ts2).tv_nsec *= -1, \
	timespec_add((ts1), (ts2)) \
)

static inline struct timespec timespec_add(struct timespec ts1,
                                           struct timespec ts2) {
	register struct timespec result = ts1;
	register long sec;

	result.tv_nsec += ts2.tv_nsec;
	result.tv_sec += ts2.tv_sec;

	if(result.tv_nsec >= BILLION) {
		result.tv_sec += result.tv_nsec / BILLION;
		result.tv_nsec %= BILLION;
	}
	if(result.tv_nsec < 0) {
		sec = 1 + -(result.tv_nsec / BILLION);
		result.tv_nsec += BILLION * sec;
		result.tv_sec -= sec;
	}
	return result;
}

#define ABS(x) ((x) < 0 ? -(x) : (x))

static inline int metaballs_dist(struct metaballs *m, int x, int y) {
	register int a, b;
	register size_t i;
	register float dist = 0;
	for(i = 0; i < NUM_BALLS; i++) {
		a = x - m->balls[i].x;
		b = y - m->balls[i].y;
		dist += m->dist_cache[ABS(a) + ABS(b)] * m->balls[i].radius;
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
	i  = *(int*)v;                              // evil floating point bit level hacking
	i  = 0x5f3759df - (i >> 1);                 // what the fuck?
	v  = &i;
	n  = *(float*)v;
	n  = n * (1.5F - (x2 * n * n));             // 1st iteration
	return n;
}

static inline void action(void *data) {
	struct metaballs *m = data;
	size_t i;
	float rnd, mult, hue, rgb[3] = { 0.0, 0.0, 0.0 };
	m->dist_cache[0] = (float)INT_MAX;
	rnd = (float)rand() / RAND_MAX * 2.0;
	mult = .25 + (float)rand() / RAND_MAX * 1.75;
	for(i = 0; i < 256; i++) {
		if(rnd > 1.0)
			hue = (float)i / 255.0 - (rnd - 1.0);
		else
			hue = (float)i / 255.0 + rnd;
		hue *= mult;
		hsv_to_rgb(rgb, hue - floor(hue), 1.0, .8);
		((unsigned char*)m->rgb_cache)[4 * i + 0] = rgb[0] * 255;
		((unsigned char*)m->rgb_cache)[4 * i + 1] = rgb[1] * 255;
		((unsigned char*)m->rgb_cache)[4 * i + 2] = rgb[2] * 255;
		((unsigned char*)m->rgb_cache)[4 * i + 3] =          255;
	}
}

int update(struct xbp *x, void *data) {
	register struct metaballs *m = data;
	register size_t i;
	register int px, py, rows = x->attr.width * x->attr.height;
	struct timespec frame_start, frame_end;
	if(clock_gettime(CLOCK_MONOTONIC, &frame_start) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	for(i = 0; i < NUM_BALLS; i++) {
		m->balls[i].x += m->balls[i].speed_x;
		if(m->balls[i].x < 0 || m->balls[i].x >= x->attr.width) {
			m->balls[i].speed_x *= -1;
			m->balls[i].x += m->balls[i].speed_x;
		}
		m->balls[i].y += m->balls[i].speed_y * x->attr.width;
		if(m->balls[i].y < 0 || m->balls[i].y >= rows) {
			m->balls[i].speed_y *= -1;
			m->balls[i].y += m->balls[i].speed_y * x->attr.width;
		}
	}
	for(py = 0; py < rows; py += x->attr.width)
		for(px = 0; px < x->attr.width; px++)
			((uint32_t*)x->data)[py + px] = m->rgb_cache[
				metaballs_dist(m, px, py)
			];
	if(clock_gettime(CLOCK_MONOTONIC, &frame_end) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	frame_end = timespec_diff(frame_end, frame_start);
	m->total_frametime = timespec_add(m->total_frametime, frame_end);
	m->num_frames++;
	return 0;
}

static inline int print_stats(struct metaballs *m) {
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	m->total_runtime = timespec_diff(ts, m->total_runtime);
	fprintf(stderr, "total runtime: %.2f\n", ((double)m->total_runtime.tv_sec
	        + (double)m->total_runtime.tv_nsec / BILLION));
	fprintf(stderr, "num frames: %zu\n", m->num_frames);
	fprintf(stderr, "FPS: %.2f\n",
		m->num_frames / ((double)m->total_runtime.tv_sec
		+ (double)m->total_runtime.tv_nsec / BILLION));
	fprintf(stderr, "time spent for calulation in total: %ld.%09ld\n",
	        m->total_frametime.tv_sec, m->total_frametime.tv_nsec);
	return 0;
}

int main(void) {
	struct xbp x;
	struct metaballs m = {
		.total_frametime = { 0, 0 },
		.total_runtime = { 0, 0 },
		.num_frames = 0,
	};
	float fx, fy;
	int speed_mod, ret = EXIT_FAILURE;
	size_t i, l;
	srand(time(NULL));
	if(xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	m.width = x.attr.width;
	m.height = x.attr.height;
	m.dist_cache = malloc(
		x.attr.width * x.attr.height * sizeof *m.dist_cache * NUM_BALLS
	);
	if(m.dist_cache == NULL) {
		perror("malloc");
		goto error;
	}
	speed_mod = (2 * MAX_SPEED(m.width)) - MAX_SPEED(m.width);
	for(i = 0; i < NUM_BALLS; i++) {
		m.balls[i].radius = (float)(
			RADIUS_LOW(m.width) + random()
			% (RADIUS_HIGH(m.width) - RADIUS_LOW(m.width))
		) / NUM_BALLS;
		m.balls[i].x = random() % m.width;
		m.balls[i].y = (random() % m.height) * m.width;
		m.balls[i].speed_x = random() % speed_mod;
		m.balls[i].speed_y = random() % speed_mod;
	}
	l = m.width * m.height;
	for(i = 1; i < l; i++) {
		fx = i % m.width;
		fy = i / m.height;
		m.dist_cache[i] = rsqrt(fx * fx + fy * fy)
		                 * DIST_MULT(m.width);
	}
	action(&m);
	if(clock_gettime(CLOCK_MONOTONIC, &m.total_runtime) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		goto error;
	}
	if(xbp_main(&x, update, action, &m) == 0 && print_stats(&m) == 0)
		ret = EXIT_SUCCESS;
error:
	free(m.dist_cache);
	xbp_cleanup(&x);
	return ret;
}
