
// metaballs.c

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "demo.h"

#define LIMIT(x, l) (((x) > (l)) ? (l) : (x))

#define NUM_BALLS 10
#define RADIUS_LOW  (d.w.attr.width / 40)
#define RADIUS_HIGH (d.w.attr.width / 16)
#define MAX_SPEED (d.w.attr.width / 40)
#define DIST_MULT (d.w.attr.width)
#define MAX_DIST (256.0 * NUM_BALLS)

struct metaballs {
	char *data;
	XImage *img;
	struct {
		int radius;
		int x, y;
		int speed_x, speed_y;
	} balls[NUM_BALLS];
	float *dist_cache;
	unsigned char rgb_cache[256 * 3];
};

static inline int metaballs_dist(int width, struct metaballs *m, int x, int y) {
	float dist = 0;
	size_t i;
	for(i = 0; i < NUM_BALLS; i++)
		dist += m->dist_cache[
			abs(x - m->balls[i].x) + abs(y - m->balls[i].y) * width
		] * m->balls[i].radius;
	if(dist < 0 || dist > MAX_DIST)
		return 0;
	return dist / NUM_BALLS;
}

int update(struct demo *d, void *data) {
	register int w = d->w.attr.width, h = d->w.attr.height;
	struct metaballs *m = (struct metaballs*)data;
	size_t i, l = w * h;
	unsigned char *bitmap = (unsigned char*)m->data;
	for(i = 0; i < NUM_BALLS; i++) {
		m->balls[i].x += m->balls[i].speed_x;
		if(m->balls[i].x < 0 || m->balls[i].x >= d->w.attr.width) {
			m->balls[i].speed_x *= -1;
			m->balls[i].x += m->balls[i].speed_x * 2;
		}
		m->balls[i].y += m->balls[i].speed_y;
		if(m->balls[i].y < 0 || m->balls[i].y >= d->w.attr.height) {
			m->balls[i].speed_y *= -1;
			m->balls[i].y += m->balls[i].speed_y * 2;
		}
	}
	for(i = 0; i < l; i++)
		memcpy(bitmap + 4 * i, m->rgb_cache + 3 * LIMIT(
			metaballs_dist(w, m, i % w, i / w), 255
		), 3);
	XPutImage(d->x.disp, d->w.win, d->w.gc, m->img,
	          0, 0, 0, 0, d->w.attr.width, d->w.attr.height);
	return 0;
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

static inline void hsv_to_rgb(float result[], float h, float s, float v) {
	float i, f, p, q, t;
	if(s == 0) {
		result[0] = v;
		result[1] = v;
		result[2] = v;
		return;
	}
	i = floorl(h * 6.0);
	f = (h * 6.0) - i;
	p = v * (1.0 - s);
	q = v * (1.0 - s * f);
	t = v * (1.0 - s * (1.0-f));
	switch((int)i % 6) {
	case 0:
		result[0] = v;
		result[1] = t;
		result[2] = p;
		return;
	case 1:
		result[0] = q;
		result[1] = v;
		result[2] = p;
		return;
	case 2:
		result[0] = p;
		result[1] = v;
		result[2] = t;
		return;
	case 3:
		result[0] = p;
		result[1] = q;
		result[2] = v;
		return;
	case 4:
		result[0] = t;
		result[1] = p;
		result[2] = v;
		return;
	case 5:
		result[0] = v;
		result[1] = p;
		result[2] = q;
		return;
	}
}

int main(void) {
	struct demo d;
	struct metaballs m;
	int i, ret = EXIT_FAILURE;
	float fx, fy, rgb[3] = { 0.0, 0.0, 0.0 };
	if(sizeof(float) != sizeof(int))
		return EXIT_FAILURE;
	srand(time(NULL));
	if(demo_init(&d) < 0)
		return EXIT_FAILURE;
	for(i = 0; i < NUM_BALLS; i++) {
		m.balls[i].radius = RADIUS_LOW + random() % (RADIUS_HIGH - RADIUS_LOW);
		m.balls[i].x = random() % d.w.attr.width;
		m.balls[i].y = random() % d.w.attr.height;
		m.balls[i].speed_x = random() % (2 * MAX_SPEED) - MAX_SPEED;
		m.balls[i].speed_y = random() % (2 * MAX_SPEED) - MAX_SPEED;
	}
	m.dist_cache = malloc(sizeof m.dist_cache[0]
	             * d.w.attr.width * d.w.attr.height);
	if(m.dist_cache == NULL) {
		perror("malloc");
		goto error;
	}
	m.data = malloc(4 * d.w.attr.width * d.w.attr.height);
	if(m.data == NULL) {
		perror("malloc");
		goto error;
	}
	// prepare distance LUT
	for(i = 0; i < d.w.attr.width * d.w.attr.height; i++) {
		fx = i % d.w.attr.width;
		fy = i / d.w.attr.height;
		m.dist_cache[i] = rsqrt(fx * fx + fy * fy) * DIST_MULT;
		((unsigned char*)m.data)[4 * i + 3] = 255;
	}
	m.dist_cache[0] = (float)INT_MAX;
	// prepare rgb LUT
	for(i = 0; i < 256; i++) {
		hsv_to_rgb(rgb, i / 255.0, 1.0, .8);
		m.rgb_cache[3 * i + 0] = LIMIT((int)(rgb[0] * 255), 255);
		m.rgb_cache[3 * i + 1] = LIMIT((int)(rgb[1] * 255), 255);
		m.rgb_cache[3 * i + 2] = LIMIT((int)(rgb[2] * 255), 255);
	}
	m.img = XCreateImage(d.x.disp, d.x.vinfo.visual, d.x.vinfo.depth, ZPixmap,
	                      0, m.data, d.w.attr.width, d.w.attr.height, 32,
	                      4 * d.w.attr.width);
	if(demo_main(&d, update, &m) == 0)
		ret = EXIT_SUCCESS;
	XDestroyImage(m.img);
error:
	free(m.dist_cache);
	demo_cleanup(&d);
	return ret;
}
