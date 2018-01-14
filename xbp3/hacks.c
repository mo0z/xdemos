
/* hacks.c
 *
 * Copyright (c) 2016, 2018, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <X11/Xatom.h>

#include "hacks_collection.h"
#include "xbp_time.h"
#include "xbp.h"

struct hacks {
	struct xbp_time xt;
	size_t hacks_idx;
	struct hacks_collection hc;
};

void invisible_cursor(struct xbp *x) {
	Pixmap p;
	XSetWindowAttributes xa;
	XColor d = { .pixel = XBlackPixel(x->disp, x->scr) };
	p = XCreatePixmap(x->disp, x->win, 1, 1, 1);
	xa.cursor = XCreatePixmapCursor(x->disp, p, p, &d, &d, 0, 0);
	XChangeWindowAttributes(x->disp, x->win, CWCursor, &xa);
	XFreeCursor(x->disp, xa.cursor);
	XFreePixmap(x->disp, p);
}

int update(struct xbp *x) {
	struct hacks *h = xbp_get_data(x);
	xbp_time_frame_start(&h->xt);
	if(hacks_list[h->hacks_idx](x, &h->hc) < 0)
		return -1;
	xbp_time_frame_end(&h->xt);
	return 0;
}

int main(int argc, char *argv[]) {
	struct xbp x = {
		.config = {
			.max_fps = 0,
			.fullscreen = 1,
			.alpha = 0,
			.defaultkeys = 1,
			.image = 0,
		},
		.callbacks = {
			.update = update,
		},
	};
	struct hacks h = {
		.hc = {
			.dir = HACKS_COLLECTION_NODIR,
			.row_len = 0,
			.row_current = 0,
			.row_prev = NULL,
		},
	};
	int ret = EXIT_FAILURE;
	srand(time(NULL));

	while(argc > 2) {
		if(strcmp(argv[1], "--up") == 0) {
			h.hc.dir = HACKS_COLLECTION_UP;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "--down") == 0) {
			h.hc.dir = HACKS_COLLECTION_DOWN;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "--left") == 0) {
			h.hc.dir = HACKS_COLLECTION_LEFT;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "--right") == 0) {
			h.hc.dir = HACKS_COLLECTION_RIGHT;
			argc--;
			argv++;
		} else {
			fprintf(stderr, "Error: invalid argument: %s\n", argv[1]);
			return EXIT_FAILURE;
		}
	}
	if(h.hc.dir == HACKS_COLLECTION_NODIR)
		h.hc.dir = (enum hacks_collection_direction[]){
			HACKS_COLLECTION_UP, HACKS_COLLECTION_DOWN,
			HACKS_COLLECTION_LEFT, HACKS_COLLECTION_RIGHT,
		}[random() % 4];
	if(argc == 2) {
		h.hacks_idx = strtol(argv[1], NULL, 0);
		if(errno == ERANGE) {
			fprintf(stderr, "Warning: could not parse `%s' as a number.\n",
			  argv[1]);
			argc = 1;
		}
	} else
		h.hacks_idx = random() % NUM_ANIMATIONS;

	if(xbp_time_init(&h.xt) < 0 || xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	invisible_cursor(&x);
	xbp_set_data(&x, &h);
	if(xbp_main(&x) == 0 && xbp_time_print_stats(&h.xt) == 0)
		ret = EXIT_SUCCESS;
	hacks_collection_cleanup(&h.hc);
	xbp_cleanup(&x);
	return ret;
}
