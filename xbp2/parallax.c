
// parallax.c

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "demo.h"

#define NUM_POINTS 20
#define NUM_PARALLAX 3
#define MAX_CLOUDS 256

struct parallax {
	const char *color;
	unsigned threshold, range[2];
	int points[NUM_POINTS + 1];
	uint8_t off, speed;
};

struct cloud {
	const char *color;
	size_t num_clouds;
	unsigned max_width, range[2], yvari;
	uint8_t yvari_wait, speed;
	struct {
		unsigned x, y, width, height;
		uint8_t yvari_count;
	} clouds[MAX_CLOUDS];
};

static inline unsigned limit(unsigned value, unsigned lo, unsigned hi) {
	if(value < lo)
		value = lo;
	if(value > hi)
		value = hi;
	return value;
}

static inline int random_centered(int interval) {
	if(interval == 0) {
		fprintf(stderr, "derp\n");
		exit(1);
	}
	return random() % interval - interval / 2;
}

static inline void set_color(struct demo *d, const char *color, XColor *c) {
	XParseColor(d->x.disp, d->x.cmap, color, c);
	XAllocColor(d->x.disp, d->x.cmap, c);
	XFreeColors(d->x.disp, d->x.cmap, &c->pixel, 1, 0);
	XSetForeground(d->x.disp, d->w.gc, c->pixel);
}

void parallax_init(struct parallax *p, const char *color, unsigned threshold,
                   unsigned range_lo, unsigned range_hi, uint8_t speed) {
	size_t i;
	unsigned range;
	p->color = color;
	p->threshold = threshold;
	p->range[0] = range_lo < range_hi ? range_lo : range_hi;
	p->range[1] = range_lo < range_hi ? range_hi : range_lo;
	range = p->range[1] - p->range[0];
	p->points[0] = p->range[0] + random() % range;
	p->points[1] = limit(p->points[0] + random_centered(range * threshold / 100),
	                     p->range[0], p->range[1]);
	for(i = 2; i <= NUM_POINTS; i++) {
		p->points[i] = limit(p->points[i - 1]
			+ random_centered(range * p->threshold / 100)
			+ (p->points[i - 1] - p->points[i - 2]) / 2,
			p->range[0], p->range[1]
		);
	}
	p->speed = speed;
	p->off = 0;
}

static inline void parallax_update(struct demo *d, struct parallax *p) {
	register size_t i;
	XColor color;
	unsigned section = d->w.attr.width / (NUM_POINTS - 1);
	set_color(d, p->color, &color);
	for(i = 0; i < NUM_POINTS; i++) {
		XFillPolygon(d->x.disp, d->w.win, d->w.gc, (XPoint[]){
			(XPoint){
				i * section - section * p->off / p->speed,
				d->w.attr.height
			},
			(XPoint){
				i * section - section * p->off / p->speed,
				d->w.attr.height - p->points[i],
			},
			(XPoint){
				(i + 1) * section - section * p->off / p->speed,
				d->w.attr.height - p->points[i + 1],
			},
			(XPoint){
				(i + 1) * section - section * p->off / p->speed,
				d->w.attr.height
			},
		}, 4, Convex, CoordModeOrigin);
	}
	if(++p->off == p->speed) {
		p->off = 0;
		memmove(p->points, p->points + 1, NUM_POINTS * sizeof p->points[0]);
		p->points[NUM_POINTS] = limit(p->points[NUM_POINTS - 1]
			+ random_centered((p->range[1] - p->range[0]) * p->threshold / 100)
			+ (p->points[NUM_POINTS - 1] - p->points[NUM_POINTS - 2]) / 2,
			p->range[0], p->range[1]
		);
	}
}

void cloud_init(struct cloud *c, const char *color, unsigned max_width,
                unsigned range_lo, unsigned range_hi, unsigned yvari,
                uint8_t yvari_wait, uint8_t speed, unsigned win_width) {
	//register size_t i;
	c->color = color;
	c->max_width = max_width;
	c->range[0] = range_lo < range_hi ? range_lo : range_hi;
	c->range[1] = range_lo < range_hi ? range_hi : range_lo;
	c->yvari = yvari;
	c->yvari_wait = yvari_wait;
	c->speed = speed;
	c->num_clouds = 0;
	/* TODO: initialise first clouds
	 * every cloud spawning should start with the left edge
	 * just past the screen's right edge
	 */
	return;
	(void)win_width;
}

void cloud_update(struct demo *d, struct cloud *c) {
	(void)d;
	(void)c;
}

int update(struct demo *d, void *data) {
	register size_t i;
	XColor color;
	set_color(d, "#00ffcc", &color);
	XFillRectangle(d->x.disp, d->w.win, d->w.gc, 0, 0,
				   d->w.attr.width, d->w.attr.height);
	for(i = 0; i < NUM_PARALLAX; i++)
		parallax_update(d, &((struct parallax*)*(void**)data)[i]);
	return 0;
	(void)data;
}

int main(void) {
	struct demo d;
	struct parallax p[NUM_PARALLAX];
	struct cloud c;
	int ret = EXIT_FAILURE;
	srand(time(NULL));
	if(demo_init(&d) < 0)
		return EXIT_FAILURE;
	parallax_init(&p[0], "#33CCAA", 40,
	              d.w.attr.height * 6 / 10, d.w.attr.height, 7);
	parallax_init(&p[1], "#00CC20", 25,
	              d.w.attr.height * 4 / 10, d.w.attr.height * 6 / 10, 5);
	parallax_init(&p[2], "#00FF00", 20,
	              d.w.attr.height * 15 / 100, d.w.attr.height * 35 / 100, 4);
	cloud_init(&c, "#FFFFFF", d.w.attr.width / 3, d.w.attr.height * 75 / 100,
	  d.w.attr.height * 98 / 100, 1, 8, d.w.attr.width / 120, d.w.attr.width);
	if(demo_main(&d, update, (void*[]){ &p, &c}) == 0)
		ret = EXIT_SUCCESS;
	demo_cleanup(&d);
	return ret;
}
