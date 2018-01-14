
/* animations.h
 *
 * Copyright (c) 2016, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "xbp.h"

struct animation {
	enum animation_direction {
		ANIMATION_NODIR,
		ANIMATION_UP,
		ANIMATION_DOWN,
		ANIMATION_LEFT,
		ANIMATION_RIGHT,
	} dir;
	size_t row_len, num_rows, row_current;
	char *row_prev;
};

int reflecting_box(struct xbp *x, struct animation *a);
int wolfram1(struct xbp *x, struct animation *a);
int wolfram2(struct xbp *x, struct animation *a);
void animation_cleanup(struct animation *a);

static int (*animations[])(struct xbp*, struct animation*) = {
	reflecting_box,
	wolfram1,
	wolfram2,
};

#define NUM_ANIMATIONS (sizeof animations / sizeof *animations)

#endif // ANIMATIONS_H
