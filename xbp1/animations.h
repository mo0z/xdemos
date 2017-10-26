
/* animations.h
 *
 * Copyright (c) 2016, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "xrootgen.h"

extern int (*animations[])(struct xconn*, void*);

static inline size_t animations_count(
  int (*animations[])(struct xconn*, void*)) {
	size_t i;
	for(i = 0; animations[i] != NULL; i++);
	return i;
}

struct animations_data {
	int dir;
};

#endif // ANIMATIONS_H
