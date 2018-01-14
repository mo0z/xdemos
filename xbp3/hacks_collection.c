
/* hacks_collection.c
 *
 * Copyright (c) 2016, 2018, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>

#include "hacks_collection.h"

#define INTS(x, ...) (int[]){ x, __VA_ARG__ }
#define BITS(a, b) ((a) | ((b) << 1))
#define LOWER(x, s) ((x) - (s) < 0)
#define UPPER(x, s, m, d) ((x) + (s) >= (m) - (d))
int reflecting_box(struct xbp *x, struct animation *a) {
	static const int size[2] = { 100, 100 }, speed = 10;
	static int direction = -1, pos[2] = { 50, 50 };
	if(direction < 0) {
		XFillRectangle(x->disp, x->win, x->gc, 0, 0, XBP_WIDTH(x), XBP_HEIGHT(x));
		direction = (direction + 1) * -1;
	}
	XSetForeground(x->disp, x->gc, XBlackPixel(x->disp, x->scr));
	XFillRectangle(x->disp, x->win, x->gc, pos[0], pos[1], size[0], size[1]);
	switch(direction) {
	case 0:
		pos[0] += speed;
		pos[1] -= speed;
		direction = (int[]){ 0, 3, 1, 2 }[
			BITS(UPPER(pos[0], speed, XBP_WIDTH(x), size[0]),
			     LOWER(pos[1], speed))
		];
		break;
	case 1:
		pos[0] += speed;
		pos[1] += speed;
		direction = (int[]){ 1, 2, 0, 3 }[
			BITS(UPPER(pos[0], speed, XBP_WIDTH(x), size[0]),
			     UPPER(pos[1], speed, XBP_HEIGHT(x), size[1]))
		];
		break;
	case 2:
		pos[0] -= speed;
		pos[1] += speed;
		direction = (int[]){ 2, 1, 3, 0 }[
			BITS(LOWER(pos[0], speed),
			     UPPER(pos[1], speed, XBP_HEIGHT(x), size[1]))
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
	XSetForeground(x->disp, x->gc, XWhitePixel(x->disp, x->scr));
	XFillRectangle(x->disp, x->win, x->gc, pos[0], pos[1], size[0], size[1]);
	return 0;
	(void)a;
}

int wolfram(struct xbp *x, struct animation *a, int (*rule)(bool, int, char*));

int rule24(bool init, int len, char *base) {
	int i;
	if(init == true) {
		for(i = 0; i < len; i++)
			base[i] = random() & 1;
		return 0;
	}
	i = (base[0] != 0) + (base[1] != 0) + (base[2] != 0) +
		(base[3] != 0) + (base[4] != 0);
	return i == 2 || i == 4;
}

int wolfram1(struct xbp *x, struct animation *a) {
	return wolfram(x, a, rule24);
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

int wolfram2(struct xbp *x, struct animation *a) {
	return wolfram(x, a, rule126);
}

int wolfram(struct xbp *x, struct animation *a, int (*rule)(bool, int, char*)) {
	char left[5] = { 0 };
	size_t i;
	if(a->row_current == 0) {
		if(a->dir == ANIMATION_UP || a->dir == ANIMATION_DOWN) {
			a->row_len = XBP_WIDTH(x);
			a->num_rows = XBP_HEIGHT(x);
		} else {
			a->row_len = XBP_HEIGHT(x);
			a->num_rows = XBP_WIDTH(x);
		}
		a->row_prev = malloc(a->row_len * sizeof *a->row_prev);
		if(a->row_prev == NULL)
			return -1;
		rule(true, a->row_len, a->row_prev);
		XSetForeground(x->disp, x->gc, XBlackPixel(x->disp, x->scr));
		XFillRectangle(x->disp, x->win, x->gc, 0, 0,
		               XBP_WIDTH(x), XBP_HEIGHT(x));
		XSetForeground(x->disp, x->gc, xbp_rgb8(x,
			128 + random() % 127, 128 + random() % 127, 128 + random() % 127
		));
		goto first;
	} else if(a->row_current >= a->num_rows)
		return 0;
	if(a->row_current > 0) {
		left[0] = a->row_prev[a->row_len - 1];
		left[1] = a->row_prev[a->row_len - 2];
		left[3] = a->row_prev[0];
		left[4] = a->row_prev[1];
		for(i = 0; i < a->row_len; i++) {
			memmove(left + 1, left, 2 * sizeof *left);
			left[0] = a->row_prev[i];
			a->row_prev[i] = rule(false, 5, (char[]){
				left[2],
				left[1],
				a->row_prev[i],
				i + 1 >= a->row_len ? left[4 + i - a->row_len] : a->row_prev[i + 1],
				i + 2 >= a->row_len ? left[5 + i - a->row_len] : a->row_prev[i + 2],
			});
		}
	}
first:
	for(i = 0; i < a->row_len; i++) {
		if(a->row_prev[i] == 0)
			continue;
		switch(a->dir) {
		case ANIMATION_UP:
			XDrawPoint(x->disp, x->win, x->gc, i,
			           XBP_HEIGHT(x) - 1 - a->row_current);
			break;
		case ANIMATION_DOWN:
			XDrawPoint(x->disp, x->win, x->gc, i, a->row_current);
			break;
		case ANIMATION_LEFT:
			XDrawPoint(x->disp, x->win, x->gc,
			           XBP_WIDTH(x) - 1 - a->row_current, i);
			break;
		case ANIMATION_RIGHT:
			XDrawPoint(x->disp, x->win, x->gc, a->row_current, i);
			break;
		default:
			break;
		}
	}
	a->row_current++;
	return 0;
}

void animation_cleanup(struct animation *a) {
	free(a->row_prev);
	(void)animations;
}
