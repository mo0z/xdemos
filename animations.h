
// animations.h

#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "xrootgen.h"

extern int (*animations[])(struct xconn *x);

static inline size_t animations_count(int (*animations[])(struct xconn*)) {
	size_t i;
	for(i = 0; animations[i] != NULL; i++);
	return i;
}

struct animations_data {
	int dir;
};

#endif // ANIMATIONS_H
