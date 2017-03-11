
/* life1d.c
 *
 * Copyright (c) 2016, mar77i <mysatyre at gmail dot com>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

// TODO: this thing seems buggy as shit

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "time_stat.h"
#include "xbp.h"

#define WIDTH(l) (l)->w.attr.width
#define HEIGHT(l) (l)->w.attr.height
#define ATTR_SIZE(l) ((size_t)(WIDTH(l) * HEIGHT(l)))
#define X(l, n) ((n) % WIDTH(l))
#define Y(l, n) ((n) / WIDTH(l))
#define DIRX(l, p, o) \
	((l)->vert ? (p) : (l)->dir == LEFT ? (size_t)(WIDTH(l) - 1 - (o)) : (o))
#define DIRY(l, p, o) \
	((l)->vert ? ((l)->dir == UP ? (size_t)(HEIGHT(l) - 1 - (o)) : (o)) : (p))

#define NUM_BITS(n) ((n + CHAR_BIT - 1) / CHAR_BIT)
//#define BIT_AT(l, i) ((l)->buf[(i) / CHAR_BIT] & (1 << ((i) % CHAR_BIT)))
#define SIGN(x) (((x) >= 0) * 2 - 1)

struct life1d {
	struct xbp x;
	struct xbp_win w;
	Pixmap p;
	size_t pixels, row, maxrow;
	uint8_t *buf;
	enum direction { UP, DOWN, LEFT, RIGHT } dir;
	bool vert;
};

struct life1d_rule {
	int8_t neighbor; // neighbor negative value means uncompressed rule
	uint8_t rule;
} rule1771476584 = { 2, 0x14 },
  customrule = { -1, 0 };

static int keypressed(struct xbp *x) {
	char kr[32];
	XQueryKeymap(x->disp, kr);
	// exit on LSHIFT|LCONTROL
	return (kr[4] & 0x20) && (kr[6] & 0x04);
}

static int life1d_args(int argc, char **argv, bool *root, enum direction *dir,
                       struct life1d_rule **lr) {
	int i;
	for(i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			return 1;
		else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--root") == 0)
			*root = true;
		else if(strcmp(argv[i], "--up") == 0)
			*dir = UP;
		else if(strcmp(argv[i], "--down") == 0)
			*dir = DOWN;
		else if(strcmp(argv[i], "--left") == 0)
			*dir = LEFT;
		else if(strcmp(argv[i], "--right") == 0)
			*dir = RIGHT;
		else if(strcmp(argv[i], "--rule1771476584") == 0)
			*lr = &rule1771476584;
		else if(strncmp(argv[i], "--", 2) == 0 && strlen(argv[i]) > 2) {
			*lr = &customrule;
			(*lr)->rule = strtoul(argv[i] + 2, NULL, 10);
		} else {
			fprintf(stderr, "Error: unknown argument: `%s'.\n", argv[i]);
			return -1;
		}
	}
	return 0;
}

static void life1d_usage(const char *argv0) {
	printf("usage: %s [-h|--help] | [-r|--root] [--up|--down|--left|--right] "
	  "[--rule1771476584|--<rulenumber>]\n",
	  argv0);
}

static int life1d_init(struct life1d *l, bool root) {
	l->buf = NULL;
	if(xbp_connect(&l->x, NULL) < 0)
		return -1;
	if(root == true)
		xbp_getrootwin(&l->x, &l->w);
	else {
		xbp_getfullscreenwin(&l->x, &l->w);
		xbp_cursorinvisible(&l->x, &l->w);
	}
	l->p = XCreatePixmap(l->x.disp, l->w.win, WIDTH(l), HEIGHT(l),
	                     l->w.attr.depth);
	l->vert = l->dir == UP || l->dir == DOWN;
	if(l->vert) {
		l->pixels = WIDTH(l);
		l->maxrow = HEIGHT(l);
	} else {
		l->pixels = HEIGHT(l);
		l->maxrow = WIDTH(l);
	}
	l->row = 0;
	l->buf = malloc(NUM_BITS(l->pixels));
	if(l->buf == NULL) {
		perror("malloc");
		return -1;
	}
	memset(l->buf, 0, NUM_BITS(l->pixels));
	XSetForeground(l->x.disp, l->w.gc, XBlackPixel(l->x.disp, l->x.scr));
	XFillRectangle(l->x.disp, l->p, l->w.gc, 0, 0, WIDTH(l), HEIGHT(l));
	XSetForeground(l->x.disp, l->w.gc, XWhitePixel(l->x.disp, l->x.scr));
	return 0;
}

static void life1d_seed(struct life1d *l) {
	size_t i;
	for(i = 0; i < l->pixels; i++) {
		if(i % CHAR_BIT == 0)
			l->buf[i / CHAR_BIT] = rand();
		if((l->buf[i / CHAR_BIT] & (1 << (i % CHAR_BIT))) != 0)
			XDrawPoint(l->x.disp, l->p, l->w.gc,
			  DIRX(l, i, l->row), DIRY(l, i, l->row));
	}
	// clear leftover bits
	if(i % CHAR_BIT > 0) {
		printf("clearing %d\n", (1 << (i % CHAR_BIT)) - 1);
		l->buf[i / CHAR_BIT] &= (1 << (i % CHAR_BIT)) - 1;
	}
	xbp_setpixmap(&l->x, &l->w, &l->p);
	l->row++;
}

static uint8_t popcount(uint8_t c) {
#define B(x) x, x + 1, x + 1, x + 2
#define B16(x) B(x), B(x + 1), B(x + 1), B(x + 2)
#define B64(x) B16(x), B16(x + 1), B16(x + 1), B16(x + 2)
	return (uint8_t[]){ B64(0), B64(1), B64(1), B64(2) }[c];
}

bool life_func(register uint8_t accu, struct life1d_rule lr) {
	if(lr.neighbor < 0)
		return (lr.rule & (1 << accu)) != 0;
	return (lr.rule & (1 << popcount(accu))) != 0;
}

static inline bool life_set(struct life1d *l, size_t byte, size_t bit, bool b) {
	uint8_t set = 1 << bit;
	l->buf[byte] = (l->buf[byte] & ~set) | (set * b);
	return (l->buf[byte] & set) != 0;
}

void binout(uint8_t n, FILE *fh) {
	uint8_t i;
	for(i = CHAR_BIT; i > 0; i--)
		fputc('0' + ((n & (1 << (i - 1))) > 0), fh);
}

int life1d_step(struct life1d *l, struct life1d_rule lr) {
	register size_t i;
	register uint8_t accu;
	uint8_t n, first;
	if(l->row >= l->maxrow)
		return 1;
	n = lr.neighbor * SIGN(lr.neighbor);
	first = l->buf[0];
	accu = ((first & ((1 << (n + 1)) - 1)) << n) |
	 (l->buf[(l->pixels - 1) / CHAR_BIT] >> (((l->pixels - 1) % CHAR_BIT) - n));
/*
	fprintf(stdout, "%zu: first: ", l->row);
	binout(first, stdout);
	fprintf(stdout, "; buf[%zu]: ", (l->pixels - 1) / CHAR_BIT);
	binout(l->buf[(l->pixels - 1) / CHAR_BIT], stdout);
	fputs("; accu: ", stdout);
	binout(accu, stdout);
	fputc('\n', stdout);
*/
	if(life_set(l, 0, 0, life_func(accu, lr)))
		XDrawPoint(l->x.disp, l->p, l->w.gc,
		  DIRX(l, 0, l->row), DIRY(l, 0, l->row));
	for(i = 1; i < l->pixels; i++) {
		accu >>= 1;
		if(i < l->pixels - n)
			accu |= (1 << (2 * n)) *
			  ((l->buf[(i + n) / CHAR_BIT] & (1 << ((i + n) % CHAR_BIT))) != 0);
		else {
			accu |= (1 << (2 * n)) *
			  ((first & (1 << (l->pixels - i - 1))) != 0);
/*
			fprintf(stdout, "%zu (end): first: ", l->row);
			binout(first, stdout);
			fputs("; accu: ", stdout);
			binout(accu, stdout);
			fputc('\n', stdout);
*/
		}
		if(life_set(l, i / CHAR_BIT, i % CHAR_BIT, life_func(accu, lr)))
			XDrawPoint(l->x.disp, l->p, l->w.gc,
			  DIRX(l, i, l->row), DIRY(l, i, l->row));
	}
	xbp_setpixmap(&l->x, &l->w, &l->p);
	l->row++;
	return 0;
}

static void life1d_cleanup(struct life1d *l) {
	free(l->buf);
	xbp_destroywin(&l->x, &l->w);
	XFreePixmap(l->x.disp, l->p);
	xbp_disconnect(&l->x);
}

int main(int argc, char *argv[]) {
	struct life1d l;
	struct time_stat t;
	size_t i;
	struct life1d_rule *lr;
	int ret = EXIT_FAILURE;
	bool root = false, running = true;
	srand(time(NULL));
/*
	if(rand() % 2 == 0)
		lr = &rule1771476584;
	else {
*/
		lr = &customrule;
		lr->rule = (uint8_t[]){
			 13,  18,  22,  26,  28,  30,  45,  50,
			 54,  57,  58,  60,  62,  69,  70,  73,
			 75,  77,  78,  79,  82,  86,  89,  90,
			 92,  93,  94,  99, 101, 102, 105, 107,
			109, 110, 114, 118, 121, 122, 124, 126,
			129, 131, 133, 135, 137, 141, 145, 146,
			147, 149, 150, 153, 154, 156, 157, 158,
			161, 163, 165, 167, 169, 177, 178, 179,
			181, 182, 186, 188, 190, 193, 195, 197,
			198, 199, 206, 210, 214, 218, 220, 222,
			225, 230, 238, 242, 246, 250, 252, 254,
		}[random() % 88];
//	}
	l.dir = (enum direction[]){ UP, DOWN, LEFT, RIGHT }[rand() % 4];
	switch(life1d_args(argc - 1, argv + 1, &root, &l.dir, &lr)) {
	case -1:
		return EXIT_FAILURE;
	case 1:
		life1d_usage(argv[0]);
		return EXIT_SUCCESS;
	}
	if(life1d_init(&l, root) < 0)
		goto error;
	if(lr != &rule1771476584) {
		l.buf[l.pixels / 2 / CHAR_BIT] = 1 << ((l.pixels / 2) % CHAR_BIT);
		XDrawPoint(l.x.disp, l.p, l.w.gc,
		  DIRX(&l, l.pixels / 2, l.row), DIRY(&l, l.pixels / 2, l.row));
		l.row++;
		xbp_setpixmap(&l.x, &l.w, &l.p);
	} else
		life1d_seed(&l);
	time_stat_start(&t);
	do {
		for(i = 0; i < 100 && running; i++)
			switch(life1d_step(&l, *lr)) {
			case -1:
				running = false;
				break;
			case 0:
				t.numframes++;
			}
	} while(keypressed(&l.x) == 0 && running == true);
	time_stat_end(&t);
	time_stat_status(&t);
	ret = EXIT_SUCCESS;
error:
	life1d_cleanup(&l);
	return ret;
}
