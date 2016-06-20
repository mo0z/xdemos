
/* life2d.c
 *
 * Copyright (c) 2016, mar77i <mar77i at mar77i dot ch>
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

#define ATTR_SIZE(w) ((size_t)((w)->attr.width * (w)->attr.height))
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

#define X(n, s) ((n) % (s)[0])
#define Y(n, s) ((n) / (s)[0])
#define FIRSTROW(n, s) (Y((n), (s)) == 0)
#define FIRSTCOL(n, s) (X((n), (s)) == 0)

#define NZ(x) ((x) != 0)
#define NSUM(b, i) (NZ((b)[i] & LEFT_TOP) + NZ((b)[i] & TOP) + \
	NZ((b)[i] & RIGHT_TOP) + NZ((b)[i] & LEFT) + NZ((b)[i] & RIGHT) + \
	NZ((b)[i] & LEFT_BOTTOM) + NZ((b)[i] & BOTTOM) + NZ((b)[i] & RIGHT_BOTTOM))

struct life2d {
	struct xbp x;
	struct xbp_win w;
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

static int life2d_init(struct life2d *l, size_t *size, bool root) {
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
	  l->w.attr.width, l->w.attr.height, l->w.attr.depth);
	*size = ATTR_SIZE(&l->w);
	l->buf = malloc(*size * sizeof *l->buf);
	if(l->buf == NULL) {
		perror("malloc");
		return -1;
	}
	memset(l->buf, 0, *size * sizeof *l->buf);
	XSetForeground(l->x.disp, l->w.gc, XBlackPixel(l->x.disp, l->x.scr));
	XFillRectangle(l->x.disp, l->p, l->w.gc, 0, 0,
	  l->w.attr.width, l->w.attr.height);
	XSetForeground(l->x.disp, l->w.gc, XWhitePixel(l->x.disp, l->x.scr));
	XSetForeground(l->x.disp, l->w.gc, XBlackPixel(l->x.disp, l->x.scr));
	xbp_setpixmap(&l->x, &l->w, &l->p);
	return 0;
}

void life2d_seed(struct life2d *l, size_t w, size_t h) {
	size_t i, c, n = w * h;
	for(i = 0; i < n; i++) {
		if((rand() & 1) == 0)
			continue;
		c = l->w.attr.width * ((l->w.attr.height - h) / 2 + (i / w)) +
		  l->w.attr.width / 2 - w / 2 + i % w;
		l->buf[c] = ALIVE;
		XDrawPoint(l->x.disp, l->p, l->w.gc,
		  c % l->w.attr.width, c / l->w.attr.width);
	}
}

bool life_func(struct life2d *l, size_t i, char nsum, struct life2d_rule lr) {
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

static char popcount(uint8_t c) {
	static char popc[256] = {
		0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
		0,
	};
	size_t i;
	if(popc[16] == 0)
		for(i = 16; i < 256; i++) {
			popc[i] = popc[i % 16] + popc[i / 16];
		}
	return popc[c];
}

static int life2d_step(struct life2d *l, size_t size[], struct life2d_rule lr) {
	register size_t i, n = size[0] * size[1];
	register bool alive;
	size_t above, beneath, left, right, x, y;
	bool white, update = false;
	for(i = 0; i < n; i++) {
		if((l->buf[i] & (ALIVE|DIED)) == 0)
			continue;
		above = (FIRSTROW(i, size) * size[1] + Y(i, size) - 1) * size[0];
		left = FIRSTCOL(i, size) * size[0] + X(i, size) - 1;
		alive = NZ(l->buf[i] & ALIVE);
		if(NZ(l->buf[left + above] & RIGHT_BOTTOM) == alive)
			continue;
		beneath = ((Y(i, size) + 1) % size[1]) * size[0];
		right = (X(i, size) + 1) % size[0];
		x = X(i, size);
		y = Y(i, size) * size[0];
		SET_BIT(l->buf[left + above], RIGHT_BOTTOM, alive);
		SET_BIT(l->buf[x + above], BOTTOM, alive);
		SET_BIT(l->buf[right + above], LEFT_BOTTOM, alive);
		SET_BIT(l->buf[left + y], RIGHT, alive);
		SET_BIT(l->buf[right + y], LEFT, alive);
		SET_BIT(l->buf[left + beneath], RIGHT_TOP, alive);
		SET_BIT(l->buf[x + beneath], TOP, alive);
		SET_BIT(l->buf[right + beneath], LEFT_TOP, alive);
	}
	for(i = 0; i < n; i++) {
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
	size_t size;
	int ret = EXIT_FAILURE;
	struct life2d_rule *lr = &maze;
	bool root = false;
	srand(time(NULL));
	switch(life2d_args(argc - 1, argv + 1, &root, &lr)) {
	case -1:
		return EXIT_FAILURE;
	case 1:
		life2d_usage(argv[0]);
		return EXIT_SUCCESS;
	}
	if(life2d_init(&l, &size, root) < 0)
		goto error;
	if(lr == &maze || lr == &mazectric)
		life2d_seed(&l, 10, 10);
	else if(lr == &conway)
		life2d_seed(&l, 100, 100);
	time_stat_start(&t);
	do {
		if(life2d_step(&l, (size_t[]){
			l.w.attr.width, l.w.attr.height
		  }, *lr) < 0)
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
