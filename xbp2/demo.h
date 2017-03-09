
// demo.h

#ifndef DEMO_H
#define DEMO_H

#include <stdbool.h>
#include <stdint.h>

#include "xbp.h"

struct demo {
	struct xbp x;
	struct xbp_win w;
	struct timespec frame_start;
	struct {
		uintmax_t missed;
		struct timespec max_wait;
	} frames;
	bool running;
};

void demo_cleanup(struct demo *d);
int demo_main(struct demo *d, int (*update_cb)(struct demo*, void*),
              void *data);
int demo_init(struct demo *d);

#endif // DEMO_H
