
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
#define BITS(hc, b) ((hc) | ((b) << 1))
#define LOWER(x, s) ((x) - (s) < 0)
#define UPPER(x, s, m, d) ((x) + (s) >= (m) - (d))
int reflecting_box(struct xbp *x, struct hacks_collection *hc) {
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
	(void)hc;
}

int wolfram(struct xbp *x, struct hacks_collection *hc,
            int (*rule)(bool, int, char*));

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

int wolfram1(struct xbp *x, struct hacks_collection *hc) {
	return wolfram(x, hc, rule24);
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

int wolfram2(struct xbp *x, struct hacks_collection *hc) {
	return wolfram(x, hc, rule126);
}

int wolfram(struct xbp *x, struct hacks_collection *hc, int (*rule)(bool, int, char*)) {
	char left[5] = { 0 };
	size_t i;
	if(hc->row_current == 0) {
		if(hc->dir == HACKS_COLLECTION_UP || hc->dir == HACKS_COLLECTION_DOWN) {
			hc->row_len = XBP_WIDTH(x);
			hc->num_rows = XBP_HEIGHT(x);
		} else {
			hc->row_len = XBP_HEIGHT(x);
			hc->num_rows = XBP_WIDTH(x);
		}
		hc->row_prev = malloc(hc->row_len * sizeof *hc->row_prev);
		if(hc->row_prev == NULL)
			return -1;
		rule(true, hc->row_len, hc->row_prev);
		XSetForeground(x->disp, x->gc, XBlackPixel(x->disp, x->scr));
		XFillRectangle(x->disp, x->win, x->gc, 0, 0,
		               XBP_WIDTH(x), XBP_HEIGHT(x));
		XSetForeground(x->disp, x->gc, xbp_rgb8(x,
			128 + random() % 127, 128 + random() % 127, 128 + random() % 127
		));
		goto first;
	} else if(hc->row_current >= hc->num_rows)
		return 0;
	if(hc->row_current > 0) {
		left[0] = hc->row_prev[hc->row_len - 1];
		left[1] = hc->row_prev[hc->row_len - 2];
		left[3] = hc->row_prev[0];
		left[4] = hc->row_prev[1];
		for(i = 0; i < hc->row_len; i++) {
			memmove(left + 1, left, 2 * sizeof *left);
			left[0] = hc->row_prev[i];
			hc->row_prev[i] = rule(false, 5, (char[]){
				left[2],
				left[1],
				hc->row_prev[i],
				i + 1 >= hc->row_len ? left[4 + i - hc->row_len] : hc->row_prev[i + 1],
				i + 2 >= hc->row_len ? left[5 + i - hc->row_len] : hc->row_prev[i + 2],
			});
		}
	}
first:
	for(i = 0; i < hc->row_len; i++) {
		if(hc->row_prev[i] == 0)
			continue;
		switch(hc->dir) {
		case HACKS_COLLECTION_UP:
			XDrawPoint(x->disp, x->win, x->gc, i,
			           XBP_HEIGHT(x) - 1 - hc->row_current);
			break;
		case HACKS_COLLECTION_DOWN:
			XDrawPoint(x->disp, x->win, x->gc, i, hc->row_current);
			break;
		case HACKS_COLLECTION_LEFT:
			XDrawPoint(x->disp, x->win, x->gc,
			           XBP_WIDTH(x) - 1 - hc->row_current, i);
			break;
		case HACKS_COLLECTION_RIGHT:
			XDrawPoint(x->disp, x->win, x->gc, hc->row_current, i);
			break;
		default:
			break;
		}
	}
	hc->row_current++;
	return 0;
}

void hacks_collection_cleanup(struct hacks_collection *hc) {
	free(hc->row_prev);
	(void)hacks_list;
}
