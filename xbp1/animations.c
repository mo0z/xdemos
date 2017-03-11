
/* animations.c
 *
 * Copyright (c) 2016, mar77i <mysatyre at gmail dot com>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>

#include "animations.h"


int reflecting_box(struct xconn *x, void *d);
int wolfram1(struct xconn *x, void *d);
int wolfram2(struct xconn *x, void *d);

int (*animations[])(struct xconn*, void*) = {
	reflecting_box,
	wolfram1,
	wolfram2,
	NULL,
};

#define INTS(x, ...) (int[]){ x, __VA_ARG__ }
#define BITS(a, b) ((a) | ((b) << 1))
#define LOWER(x, s) ((x) - (s) < 0)
#define UPPER(x, s, m, d) ((x) + (s) >= (m) - (d))
int reflecting_box(struct xconn *x, void *data) {
	static Pixmap p;
	static const int size[2] = { 100, 100 }, speed = 10;
	static int direction = -1, pos[2] = { 50, 50 };
	if(direction < 0) {
		p = XCreatePixmap(x->d, x->w, x->a.width, x->a.height, x->a.depth);
		xrootgen_cleanup_add(x, cleanup_pixmap, (void*)&p);
		XFillRectangle(x->d, p, x->gc, 0, 0, x->a.width, x->a.height);
		direction = (direction + 1) * -1;
	}
	XSetForeground(x->d, x->gc, XBlackPixel(x->d, x->s));
	XFillRectangle(x->d, p, x->gc, pos[0], pos[1], size[0], size[1]);
	switch(direction) {
	case 0:
		pos[0] += speed;
		pos[1] -= speed;
		direction = (int[]){ 0, 3, 1, 2 }[
			BITS(UPPER(pos[0], speed, x->a.width, size[0]),
			     LOWER(pos[1], speed))
		];
		break;
	case 1:
		pos[0] += speed;
		pos[1] += speed;
		direction = (int[]){ 1, 2, 0, 3 }[
			BITS(UPPER(pos[0], speed, x->a.width, size[0]),
			     UPPER(pos[1], speed, x->a.height, size[1]))
		];
		break;
	case 2:
		pos[0] -= speed;
		pos[1] += speed;
		direction = (int[]){ 2, 1, 3, 0 }[
			BITS(LOWER(pos[0], speed),
			     UPPER(pos[1], speed, x->a.height, size[1]))
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
	xrootgen_setpixmap(x, &p);
	return 0;
	(void)data;
}

int wolfram(struct xconn *x, struct animations_data *data,
            int (*rule)(bool, int, char*));

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

int wolfram1(struct xconn *x, void *data) {
	return wolfram(x, data, rule24);
}

int rule126(bool init, int len, char *base) {
	int i;
	if(init == true) {
		memset(base, 0, len);
		base[len / 4] = 1;
		base[(len / 4) * 3] = 1;
		return 0;
	}
	i = (base[1] != 0) + (base[2] != 0) + (base[3] != 0);
	return i == 1 || i == 2;
}

int wolfram2(struct xconn *x, void *data) {
	return wolfram(x, data, rule126);
}

int wolfram(struct xconn *x, struct animations_data *data,
            int (*rule)(bool, int, char*)) {
	static Pixmap p;
	static char *last_row, left[5] = { 0 };
	static int line = 0, l;
	int i, sum = 0;
	if(line == 0) {
		l = (data->dir & 2) == 0 ? x->a.width : x->a.height;
		last_row = malloc(l);
		if(last_row == NULL)
			return -1;
		xrootgen_cleanup_add(x, cleanup_free, last_row);
		rule(true, l, last_row);
		p = XCreatePixmap(x->d, x->w, x->a.width, x->a.height, x->a.depth);
		xrootgen_cleanup_add(x, cleanup_pixmap, (void*)&p);
		XSetForeground(x->d, x->gc, XBlackPixel(x->d, x->s));
		XFillRectangle(x->d, p, x->gc, 0, 0, x->a.width, x->a.height);
		XSetForeground(x->d, x->gc, xrootgen_rgb(x,
			MAX_COLOR / 2 + rand() % (MAX_COLOR / 2),
			MAX_COLOR / 2 + rand() % (MAX_COLOR / 2),
			MAX_COLOR / 2 + rand() % (MAX_COLOR / 2)));
		goto first;
	}
	left[0] = last_row[l - 1];
	left[1] = last_row[l - 2];
	left[3] = last_row[0];
	left[4] = last_row[1];
	for(i = 0; i < l; i++) {
		memmove(left + 1, left, 2 * sizeof *left);
		left[0] = last_row[i];
		last_row[i] = rule(false, 5, (char[]){
			left[2],
			left[1],
			last_row[i],
			i + 1 >= l ? left[4 + i - l] : last_row[i + 1],
			i + 2 >= l ? left[5 + i - l] : last_row[i + 2],
		});
	}
	if((data->dir & 2) == 0 ? (line >= x->a.height) : (line >= x->a.width))
		return -1;
first:
	for(i = 0; i < l; i++) {
		switch((last_row[i] == 0) * 4 + data->dir) {
		case 0:
			XDrawPoint(x->d, p, x->gc, i, x->a.height - 1 - line);
			break;
		case 1:
			XDrawPoint(x->d, p, x->gc, i, line);
			break;
		case 2:
			XDrawPoint(x->d, p, x->gc, x->a.width - 1 - line, i);
			break;
		case 3:
			XDrawPoint(x->d, p, x->gc, line, i);
			break;
		default:
			continue;
		}
		sum++;
	}
	line++;
	xrootgen_setpixmap(x, &p);
	return -1 * (sum == 0);
}
