
// animations.c

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>

#include "animations.h"


int reflecting_box(struct xconn *x);
int wolfram1(struct xconn *x);
int wolfram2(struct xconn *x);

int (*animations[])(struct xconn *x) = {
	reflecting_box,
	wolfram1,
	wolfram2,
	NULL,
};

void cleanup_pixmap(struct xconn *x, void *p) {
	XFreePixmap(x->d, *(Pixmap*)p);
}

static XWindowAttributes a = { 0 };

#define INTS(x, ...) (int[]){ x, __VA_ARG__ }
#define BITS(a, b) ((a) | ((b) << 1))
#define LOWER(x, s) ((x) - (s) < 0)
#define UPPER(x, s, m, d) ((x) + (s) >= (m) - (d))
int reflecting_box(struct xconn *x) {
	static Pixmap p;
	static const int size[2] = { 100, 100 }, speed = 10;
	static int direction = 1, pos[2] = { 50, 50 };
	if(a.height == 0) {
		XGetWindowAttributes(x->d, x->r, &a);
		if(a.height == 0)
			return -1;
		p = XCreatePixmap(x->d, x->r, a.width, a.height, a.depth);
		xrootgen_cleanup_add(x, cleanup_pixmap, (void*)&p);
		XFillRectangle(x->d, p, x->gc, 0, 0, a.width, a.height);
	}
	XSetForeground(x->d, x->gc, XBlackPixel(x->d, x->s));
	XFillRectangle(x->d, p, x->gc, pos[0], pos[1], size[0], size[1]);
	switch(direction) {
	case 0:
		pos[0] += speed;
		pos[1] -= speed;
		direction = (int[]){ 0, 3, 1, 2 }[
			BITS(UPPER(pos[0], speed, a.width, size[0]), LOWER(pos[1], speed))
		];
		break;
	case 1:
		pos[0] += speed;
		pos[1] += speed;
		direction = (int[]){ 1, 2, 0, 3 }[
			BITS(UPPER(pos[0], speed, a.width, size[0]),
				UPPER(pos[1], speed, a.height, size[1]))
		];
		break;
	case 2:
		pos[0] -= speed;
		pos[1] += speed;
		direction = (int[]){ 2, 1, 3, 0 }[
			BITS(LOWER(pos[0], speed), UPPER(pos[1], speed, a.height, size[1]))
		];
		break;
	case 3:
		pos[0] -= speed;
		pos[1] -= speed;
		direction = (int[]){ 3, 0, 2, 1 }[
			BITS(LOWER(pos[0], speed), LOWER(pos[1], speed))
		];
		break;
	}
	XSetForeground(x->d, x->gc, XWhitePixel(x->d, x->s));
	XFillRectangle(x->d, p, x->gc, pos[0], pos[1], size[0], size[1]);
	XSetWindowBackgroundPixmap(x->d, x->r, p);
	XClearWindow(x->d, x->r);
	XSync(x->d, False);
	return 0;
}

void cleanup_free(struct xconn *x, void *p) {
	free(p);
	(void)x;
}

int wolfram(struct xconn *x, int (*rule)(bool, int, char*));

int rule24(bool init, int len, char *base) {
	int i;
	if(init == true) {
		for(i = 0; i < len; i++)
			base[i] = rand() & 1;
		return 0;
	}
	i = (base[0] != 0) + (base[1] != 0) + (base[2] != 0) +
		(base[3] != 0) + (base[4] != 0);
	return i == 2 || i == 4;
}

int wolfram1(struct xconn *x) {
	return wolfram(x, rule24);
}

int rule126(bool init, int len, char *base) {
	int i;
	if(init == true) {
		memset(base, 0, len);
		base[len / 3] = 1;
		base[(len / 3) * 2] = 1;
		return 0;
	}
	i = (base[1] != 0) + (base[2] != 0) + (base[3] != 0);
	return i == 1 || i == 2;
}

int wolfram2(struct xconn *x) {
	return wolfram(x, rule126);
}

#define MAX_COL 0x10000
int wolfram(struct xconn *x, int (*rule)(bool, int, char*)) {
	static XColor c;
	static Pixmap p;
	static char *last_row, left[3] = { 0 };
	static int line = 20;
	int i, sum = 0;
	if(a.height == 0) {
		XGetWindowAttributes(x->d, x->r, &a);
		if(a.height == 0)
			return -1;
		last_row = malloc(a.width);
		if(last_row == NULL)
			return -1;
		rule(true, a.width, last_row);
		xrootgen_cleanup_add(x, cleanup_free, last_row);
		p = XCreatePixmap(x->d, x->r, a.width, a.height, a.depth);
		xrootgen_cleanup_add(x, cleanup_pixmap, (void*)&p);
		XSetForeground(x->d, x->gc, XBlackPixel(x->d, x->s));
		XFillRectangle(x->d, p, x->gc, 0, 0, a.width, a.height);
		c.red = MAX_COL / 2 + rand() % (MAX_COL / 2);
		c.green = MAX_COL / 2 + rand() % (MAX_COL / 2);
		c.blue = MAX_COL / 2 + rand() % (MAX_COL / 2);
		c.flags = DoRed|DoGreen|DoBlue;
		XAllocColor(x->d, x->cm, &c);
		XFreeColors(x->d, x->cm, &c.pixel, 1, 0);
		XSetForeground(x->d, x->gc, c.pixel);
	} else for(i = 0; i < a.width; i++) {
		left[2] = left[1];
		left[1] = left[0];
		left[0] = last_row[i];
		last_row[i] = rule(false, 5, (char[]){
			(i > 1) * left[2],
			(i > 0) * left[1],
			last_row[i],
			(i < a.width - 1) * last_row[i + (i < a. width - 1)],
			(i < a.width - 2) * last_row[i + 2 * (i < a. width - 2)],
		});
	}
	if(line >= a.height)
		return -1;
	for(i = 0; i < a.width; i++)
		if(last_row[i] != 0) {
			XDrawPoint(x->d, p, x->gc, i, line);
			sum++;
		}
	line++;
	XSetWindowBackgroundPixmap(x->d, x->r, p);
	XClearWindow(x->d, x->r);
	XSync(x->d, False);
	return -1 * (sum == 0);
}
