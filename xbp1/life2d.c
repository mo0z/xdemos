
/* life2d.c
 *
 * Copyright (c) 2016, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "time_stat.h"
#include "xbp.h"

#define SET_BIT(a, v, l) do { \
	if(NZ((a) & (v)) != NZ(l)) \
		(a) ^= (v); \
} while(0)

#define LEFT_TOP     (1 << 0)
#define TOP          (1 << 1)
#define RIGHT_TOP    (1 << 2)
#define LEFT         (1 << 3)
#define RIGHT        (1 << 4)
#define LEFT_BOTTOM  (1 << 5)
#define BOTTOM       (1 << 6)
#define RIGHT_BOTTOM (1 << 7)
#define ALIVE        (1 << 8)
#define DIED         (1 << 9)

#define WIDTH(l) (l)->w.attr.width
#define HEIGHT(l) (l)->w.attr.height
#define ATTR_SIZE(l) ((size_t)(WIDTH(l) * HEIGHT(l)))
#define X(l, n) ((n) % WIDTH(l))
#define Y(l, n) ((n) / WIDTH(l))

#define NZ(x) ((x) != 0)
#define NSUM(b, i) (NZ((b)[i] & LEFT_TOP) + NZ((b)[i] & TOP) + \
	NZ((b)[i] & RIGHT_TOP) + NZ((b)[i] & LEFT) + NZ((b)[i] & RIGHT) + \
	NZ((b)[i] & LEFT_BOTTOM) + NZ((b)[i] & BOTTOM) + NZ((b)[i] & RIGHT_BOTTOM))

struct life2d {
	struct xbp x;
	struct xbp_win w;
	size_t size;
	uint16_t *buf;
	Pixmap p;
};

struct life2d_rule {
	uint16_t stay_alive;
	uint16_t come_alive;
} maze = { 0x3f, 0x08 },
  mazectric = { 0x1f, 0x08 },
  conway = { 0x0c, 0x08};

static int keypressed(struct xbp *x) {
	char kr[32];
	XQueryKeymap(x->disp, kr);
	// exit on LSHIFT|LCONTROL
	return (kr[4] & 0x20) && (kr[6] & 0x04);
}

static int life2d_args(int argc, char **argv, bool *root,
                       struct life2d_rule **lr) {
	int i;
	for(i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			return 1;
		else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--root") == 0)
			*root = true;
		else if(strcmp(argv[i], "--maze") == 0)
			*lr = &maze;
		else if(strcmp(argv[i], "--mazectric") == 0)
			*lr = &mazectric;
		else if(strcmp(argv[i], "--conway") == 0)
			*lr = &conway;
		else {
			fprintf(stderr, "Error: unknown argument: `%s'.\n", argv[i]);
			return -1;
		}
	}
	return 0;
}

static void life2d_usage(const char *argv0) {
	printf("usage: %s [-h|--help] | "
	  "[-r|--root] [--maze|--mazectric|--conway]\n",
	  argv0);
}

static int life2d_init(struct life2d *l, bool root) {
	l->buf = NULL;
	if(xbp_connect(&l->x, NULL) < 0)
		return -1;
	if(root == true)
		xbp_getrootwin(&l->x, &l->w);
	else {
		xbp_getfullscreenwin(&l->x, &l->w);
		xbp_cursorinvisible(&l->x, &l->w);
	}
	l->p = XCreatePixmap(l->x.disp, l->w.win,
	  WIDTH(l), HEIGHT(l), l->w.attr.depth);
	l->size = ATTR_SIZE(l);
	l->buf = malloc(l->size * sizeof *l->buf);
	if(l->buf == NULL) {
		perror("malloc");
		return -1;
	}
	memset(l->buf, 0, l->size * sizeof *l->buf);
	XClearWindow(l->x.disp, l->w.win);
	XSetForeground(l->x.disp, l->w.gc, XWhitePixel(l->x.disp, l->x.scr));
	return 0;
}

static void life2d_seed(struct life2d *l, size_t w, size_t h) {
	size_t i, c, n = w * h;
	for(i = 0; i < n; i++) {
		if((rand() & 1) == 0)
			continue;
		c = WIDTH(l) * ((HEIGHT(l) - h) / 2 + (i / w)) +
		  WIDTH(l) / 2 - w / 2 + i % w;
		l->buf[c] = ALIVE;
		XDrawPoint(l->x.disp, l->p, l->w.gc, c % WIDTH(l), c / WIDTH(l));
	}
	xbp_setpixmap(&l->x, &l->w, &l->p);
}

static bool life_func(struct life2d *l, size_t i, uint8_t nsum,
                      struct life2d_rule lr) {
	if(NZ(l->buf[i] & ALIVE) && (lr.stay_alive & (1 << nsum)) == 0) {
		l->buf[i] &= ~ALIVE;
		l->buf[i] |= DIED;
		return true;
	} else if((lr.come_alive & (1 << nsum)) != 0) {
		l->buf[i] |= ALIVE;
		return true;
	}
	return false;
}

static uint8_t popcount(uint8_t c) {
#define B(x) x, x + 1, x + 1, x + 2
#define B16(x) B(x), B(x + 1), B(x + 1), B(x + 2)
#define B64(x) B16(x), B16(x + 1), B16(x + 1), B16(x + 2)
	return (uint8_t[]){ B64(0), B64(1), B64(1), B64(2) }[c];
}

static int life2d_step(struct life2d *l, struct life2d_rule lr) {
	register size_t i;
	register bool alive;
	size_t above, beneath, left, right, x, y;
	bool white, update = false;
	for(i = 0; i < l->size; i++) {
		if((l->buf[i] & (ALIVE|DIED)) == 0)
			continue;
		above = ((Y(l, i) == 0) * HEIGHT(l) + Y(l, i) - 1) * WIDTH(l);
		left = (X(l, i) == 0) * WIDTH(l) + X(l, i) - 1;
		alive = NZ(l->buf[i] & ALIVE);
		if(NZ(l->buf[left + above] & RIGHT_BOTTOM) == alive)
			continue;
		beneath = ((Y(l, i) + 1) % HEIGHT(l)) * WIDTH(l);
		right = (X(l, i) + 1) % WIDTH(l);
		x = X(l, i);
		y = Y(l, i) * WIDTH(l);
		SET_BIT(l->buf[left + above], RIGHT_BOTTOM, alive);
		SET_BIT(l->buf[x + above], BOTTOM, alive);
		SET_BIT(l->buf[right + above], LEFT_BOTTOM, alive);
		SET_BIT(l->buf[left + y], RIGHT, alive);
		SET_BIT(l->buf[right + y], LEFT, alive);
		SET_BIT(l->buf[left + beneath], RIGHT_TOP, alive);
		SET_BIT(l->buf[x + beneath], TOP, alive);
		SET_BIT(l->buf[right + beneath], LEFT_TOP, alive);
	}
	for(i = 0; i < l->size; i++) {
		l->buf[i] &= ~DIED;
		if(l->buf[i] == 0)
			continue;
		update = life_func(l, i, popcount(l->buf[i] & 0xff), lr);
		if(update == false)
			continue;
		if(i == 0 || white != NZ(l->buf[i] & ALIVE)) {
			white = NZ(l->buf[i] & ALIVE);
			XSetForeground(l->x.disp, l->w.gc, white ?
			  XWhitePixel(l->x.disp, l->x.scr) :
			  XBlackPixel(l->x.disp, l->x.scr));
		}
		XDrawPoint(l->x.disp, l->p, l->w.gc,
		  i % l->w.attr.width, i / l->w.attr.width);
	}
	xbp_setpixmap(&l->x, &l->w, &l->p);
	return 0;
}

static void life2d_cleanup(struct life2d *l) {
	free(l->buf);
	xbp_destroywin(&l->x, &l->w);
	XFreePixmap(l->x.disp, l->p);
	xbp_disconnect(&l->x);
}

int main(int argc, char *argv[]) {
	struct life2d l;
	struct time_stat t;
	struct life2d_rule *lr;
	int ret = EXIT_FAILURE;
	bool root = false;
	srand(time(NULL));
	lr = (struct life2d_rule*[]){ &maze, &mazectric, &conway }[rand() % 3];
	switch(life2d_args(argc - 1, argv + 1, &root, &lr)) {
	case -1:
		return EXIT_FAILURE;
	case 1:
		life2d_usage(argv[0]);
		return EXIT_SUCCESS;
	}
	if(life2d_init(&l, root) < 0)
		goto error;
	if(lr == &maze || lr == &mazectric)
		life2d_seed(&l, 10, 10);
	else if(lr == &conway)
		life2d_seed(&l, 100, 100);
	time_stat_start(&t);
	do {
		if(life2d_step(&l, *lr) < 0)
			break;
		t.numframes++;
	} while(keypressed(&l.x) == 0);
	time_stat_end(&t);
	time_stat_status(&t);
	ret = EXIT_SUCCESS;
error:
	life2d_cleanup(&l);
	return ret;
}
