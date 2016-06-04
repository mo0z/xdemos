
// maze2.c

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xbp.h"

#define ATTR_SIZE(w) ((size_t)((w)->attr.width * (w)->attr.height))

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

int keypressed(struct xbp *x) {
	char kr[32];
	XQueryKeymap(x->disp, kr);
	// exit on LSHIFT|LCONTROL
	return (kr[4] & 0x20) && (kr[6] & 0x04);
}

int maze_init(struct maze *m, size_t *size) {
	size_t i, c;
	m->buf = NULL;
	if(xbp_connect(&m->x, NULL) < 0)
		return -1;
	xbp_getfullscreenwin(&m->x, &m->w);
	xbp_cursorinvisible(&m->x, &m->w);
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
	for(i = 0; i < 100; i++) {
		if((rand() & 1) == 0)
			continue;
		c = m->w.attr.width * ((m->w.attr.height / 2 - 5) + (i / 10)) +
		  m->w.attr.width / 2 - 5 + i % 10;
		m->buf[c] = ALIVE;
		XDrawPoint(m->x.disp, m->p, m->w.gc,
		  c % m->w.attr.width, c / m->w.attr.width);
	}
	XSetForeground(m->x.disp, m->w.gc, XBlackPixel(m->x.disp, m->x.scr));
	xbp_setpixmap(&m->x, &m->w, &m->p);
	return 0;
}

char popcount(uint8_t c) {
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

#define SET_BIT(a, v, l) do { \
	if(NZ((a) & (v)) != NZ(l)) \
		(a) ^= (v); \
} while(0)
int maze_step(struct maze *m, size_t size[], uint8_t alive_max) {
	register size_t i, n = size[0] * size[1], nsum;
	register bool alive;
	size_t a, b, l, r, x, y;
	enum {
		WHITE = 1,
		UPDATE = 2,
	} drw = 0;
	for(i = 0; i < n; i++) {
		if(m->buf[i] == 0)
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
		nsum = popcount(m->buf[i] & 0xff);
		if(NZ(m->buf[i] & ALIVE) && nsum > alive_max) {
			m->buf[i] &= ~ALIVE;
			m->buf[i] |= DIED;
			drw |= UPDATE;
		} else if(nsum == 3) {
			m->buf[i] |= ALIVE;
			drw |= UPDATE;
		}
		if(NZ(drw & UPDATE) == 0)
			continue;
		if(i == 0 || (drw & WHITE) != NZ(m->buf[i] & ALIVE)) {
			drw = NZ(m->buf[i] & ALIVE);
			XSetForeground(m->x.disp, m->w.gc, NZ(m->buf[i] & ALIVE) ?
			  XWhitePixel(m->x.disp, m->x.scr) :
			  XBlackPixel(m->x.disp, m->x.scr));
		} else
			drw &= ~UPDATE;
		XDrawPoint(m->x.disp, m->p, m->w.gc,
		  i % m->w.attr.width, i / m->w.attr.width);
	}
	xbp_setpixmap(&m->x, &m->w, &m->p);
	return 0;
}

void maze_cleanup(struct maze *m) {
	free(m->buf);
	xbp_destroywin(&m->x, &m->w);
	XFreePixmap(m->x.disp, m->p);
	xbp_disconnect(&m->x);
}

int main(void) {
	struct maze m;
	struct timespec tp, tq;
	size_t size, numframes = 0;
	int ret = EXIT_FAILURE;
	uint8_t alive_max = 5;
	bool timerec = true;
	srand(time(NULL));
	if(maze_init(&m, &size) < 0)
		goto error;
	if(clock_gettime(CLOCK_MONOTONIC, &tq) < 0) {
		perror("clock_gettime");
		timerec = false;
	}
	do {
		if(maze_step(&m, (size_t[]){ m.w.attr.width, m.w.attr.height },
		  alive_max) < 0)
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
