
/* maze.c
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

#define TIMESPECLD(t) ((long double)(t).tv_nsec / 1000000000 + (t).tv_sec)

struct maze {
	struct xbp x;
	struct xbp_win w;
	uint16_t *buf;
	Pixmap p;
};

struct life_mask {
	uint16_t stay_alive;
	uint16_t come_alive;
} mazelife = { 0x3f, 0x08 },
  mazectric = { 0x1f, 0x08 },
  conway = { 0x0c, 0x08};

static int keypressed(struct xbp *x) {
	char kr[32];
	XQueryKeymap(x->disp, kr);
	// exit on LSHIFT|LCONTROL
	return (kr[4] & 0x20) && (kr[6] & 0x04);
}

static int maze_args(int argc, char **argv, bool *help, bool *root,
              struct life_mask **lm) {
	int i;
	for(i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			*help = true;
			return 0;
		} else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--root") == 0)
			*root = true;
		else if(strcmp(argv[i], "--mazectric") == 0)
			*lm = &mazectric;
		else if(strcmp(argv[i], "--conway") == 0)
			*lm = &conway;
		else {
			fprintf(stderr, "Error: unknown argument: `%s'.\n", argv[i]);
			return -1;
		}
	}
	return 0;
}

static void maze_usage(const char *argv0) {
	printf("usage: %s [-h|--help] | [-r|--root] [--mazectric|--conway]\n",
	  argv0);
}

static int maze_init(struct maze *m, size_t *size, bool root) {
	m->buf = NULL;
	if(xbp_connect(&m->x, NULL) < 0)
		return -1;
	if(root == true)
		xbp_getrootwin(&m->x, &m->w);
	else {
		xbp_getfullscreenwin(&m->x, &m->w);
		xbp_cursorinvisible(&m->x, &m->w);
	}
	m->p = XCreatePixmap(m->x.disp, m->w.win,
	  m->w.attr.width, m->w.attr.height, m->w.attr.depth);
	*size = ATTR_SIZE(&m->w);
	m->buf = malloc(*size * sizeof *m->buf);
	if(m->buf == NULL) {
		perror("malloc");
		return -1;
	}
	memset(m->buf, 0, *size * sizeof *m->buf);
	XSetForeground(m->x.disp, m->w.gc, XBlackPixel(m->x.disp, m->x.scr));
	XFillRectangle(m->x.disp, m->p, m->w.gc, 0, 0,
	  m->w.attr.width, m->w.attr.height);
	XSetForeground(m->x.disp, m->w.gc, XWhitePixel(m->x.disp, m->x.scr));
	XSetForeground(m->x.disp, m->w.gc, XBlackPixel(m->x.disp, m->x.scr));
	xbp_setpixmap(&m->x, &m->w, &m->p);
	return 0;
}

void maze_seed(struct maze *m, size_t w, size_t h) {
	size_t i, c, n = w * h;
	for(i = 0; i < n; i++) {
		if((rand() & 1) == 0)
			continue;
		c = m->w.attr.width * ((m->w.attr.height - h) / 2 + (i / w)) +
		  m->w.attr.width / 2 - w / 2 + i % w;
		m->buf[c] = ALIVE;
		XDrawPoint(m->x.disp, m->p, m->w.gc,
		  c % m->w.attr.width, c / m->w.attr.width);
	}
}

bool life_func(struct maze *m, size_t i, char nsum, struct life_mask lm) {
	if(NZ(m->buf[i] & ALIVE) && (lm.stay_alive & (1 << nsum)) == 0) {
		m->buf[i] &= ~ALIVE;
		m->buf[i] |= DIED;
		return true;
	} else if((lm.come_alive & (1 << nsum)) != 0) {
		m->buf[i] |= ALIVE;
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

static int maze_step(struct maze *m, size_t size[], struct life_mask lm) {
	register size_t i, n = size[0] * size[1];
	register bool alive;
	size_t a, b, l, r, x, y;
	bool white, update = false;
	for(i = 0; i < n; i++) {
		if((m->buf[i] & (ALIVE|DIED)) == 0)
			continue;
		a = (FIRSTROW(i, size) * size[1] + Y(i, size) - 1) * size[0];
		l = FIRSTCOL(i, size) * size[0] + X(i, size) - 1;
		alive = NZ(m->buf[i] & ALIVE);
		if(NZ(m->buf[l + a] & RIGHT_BOTTOM) == alive)
			continue;
		b = ((Y(i, size) + 1) % size[1]) * size[0];
		r = (X(i, size) + 1) % size[0];
		x = X(i, size);
		y = Y(i, size) * size[0];
		SET_BIT(m->buf[l + a], RIGHT_BOTTOM, alive);
		SET_BIT(m->buf[x + a], BOTTOM, alive);
		SET_BIT(m->buf[r + a], LEFT_BOTTOM, alive);
		SET_BIT(m->buf[l + y], RIGHT, alive);
		SET_BIT(m->buf[r + y], LEFT, alive);
		SET_BIT(m->buf[l + b], RIGHT_TOP, alive);
		SET_BIT(m->buf[x + b], TOP, alive);
		SET_BIT(m->buf[r + b], LEFT_TOP, alive);
	}
	for(i = 0; i < n; i++) {
		m->buf[i] &= ~DIED;
		if(m->buf[i] == 0)
			continue;
		update = life_func(m, i, popcount(m->buf[i] & 0xff), lm);
		if(update == false)
			continue;
		if(i == 0 || white != NZ(m->buf[i] & ALIVE)) {
			white = NZ(m->buf[i] & ALIVE);
			XSetForeground(m->x.disp, m->w.gc, white ?
			  XWhitePixel(m->x.disp, m->x.scr) :
			  XBlackPixel(m->x.disp, m->x.scr));
		}
		XDrawPoint(m->x.disp, m->p, m->w.gc,
		  i % m->w.attr.width, i / m->w.attr.width);
	}
	xbp_setpixmap(&m->x, &m->w, &m->p);
	return 0;
}

static void maze_cleanup(struct maze *m) {
	free(m->buf);
	xbp_destroywin(&m->x, &m->w);
	XFreePixmap(m->x.disp, m->p);
	xbp_disconnect(&m->x);
}

int main(int argc, char *argv[]) {
	struct maze m;
	struct timespec tp, tq;
	size_t size, numframes = 0;
	int ret = EXIT_FAILURE;
	struct life_mask *lm = &mazelife;
	bool timerec = true, root = false, help = false;
	srand(time(NULL));
	if(maze_args(argc - 1, argv + 1, &help, &root, &lm) < 0)
		return EXIT_FAILURE;
	if(help == true) {
		maze_usage(argv[0]);
		return 0;
	}
	if(maze_init(&m, &size, root) < 0)
		goto error;
	if(lm == &mazelife || lm == &mazectric)
		maze_seed(&m, 10, 10);
	else if(lm == &conway)
		maze_seed(&m, 100, 100);
	if(clock_gettime(CLOCK_MONOTONIC, &tq) < 0) {
		perror("clock_gettime");
		timerec = false;
	}
	do {
		if(maze_step(&m, (size_t[]){
			m.w.attr.width, m.w.attr.height
		  }, *lm) < 0)
			break;
		numframes++;
	} while(keypressed(&m.x) == 0);
	if(clock_gettime(CLOCK_MONOTONIC, &tp) < 0) {
		perror("clock_gettime");
		timerec = false;
	}
	fprintf(stderr, "Calculated %zu frames in ", numframes);
	if(timerec == true) {
		if(tp.tv_nsec < tq.tv_nsec) {
			tp.tv_sec--;
			tp.tv_nsec += 1000000000;
		}
		tp.tv_nsec -= tq.tv_nsec;
		tp.tv_sec -= tq.tv_sec;
		fprintf(stderr, "%.3Lf\n", TIMESPECLD(tp));
		fprintf(stderr, "FPS: %.3Lf\n",
		  (long double)numframes / TIMESPECLD(tp));
	} else
		fprintf(stderr, "unknown time.\n");
	ret = EXIT_SUCCESS;
error:
	maze_cleanup(&m);
	return ret;
}
