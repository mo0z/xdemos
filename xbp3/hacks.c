
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

struct xrootgen {
	struct xbp_time xt;
	size_t animation_idx;
	struct animation a;
};

/*
void xrootgen_invisible_cursor(struct xconn *x, Cursor *c) {
	Pixmap p;
	XColor d = { .pixel = XBlackPixel(x->d, x->s) };
	p = XCreatePixmap(x->d, x->w, 1, 1, 1);
	*c = XCreatePixmapCursor(x->d, p, p, &d, &d, 0, 0);
	XFreePixmap(x->d, p);
}
*/

int update(struct xbp *x) {
	struct xrootgen *xr = xbp_get_data(x);
	xbp_time_frame_start(&xr->xt);
	if(animations[xr->animation_idx](x, &xr->a) < 0)
		return -1;
	xbp_time_frame_end(&xr->xt);
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
	struct xrootgen xr = {
		.a = {
			.dir = ANIMATION_NODIR,
			.row_len = 0,
			.row_current = 0,
			.row_prev = NULL,
		},
	};
	int ret = EXIT_FAILURE;
	srand(time(NULL));

	while(argc > 2) {
		if(strcmp(argv[1], "--up") == 0) {
			xr.a.dir = ANIMATION_UP;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "--down") == 0) {
			xr.a.dir = ANIMATION_DOWN;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "--left") == 0) {
			xr.a.dir = ANIMATION_LEFT;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "--right") == 0) {
			xr.a.dir = ANIMATION_RIGHT;
			argc--;
			argv++;
		} else {
			fprintf(stderr, "Error: invalid argument: %s\n", argv[1]);
			return EXIT_FAILURE;
		}
	}
	if(xr.a.dir == ANIMATION_NODIR)
		xr.a.dir = (enum animation_direction[]){
			ANIMATION_UP, ANIMATION_DOWN, ANIMATION_LEFT, ANIMATION_RIGHT,
		}[random() % 4];
	if(argc == 2) {
		xr.animation_idx = strtol(argv[1], NULL, 0);
		if(errno == ERANGE) {
			fprintf(stderr, "Warning: could not parse `%s' as a number.\n",
			  argv[1]);
			argc = 1;
		}
	} else
		xr.animation_idx = random() % NUM_ANIMATIONS;

	if(xbp_time_init(&xr.xt) < 0 || xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	xbp_set_data(&x, &xr);
	if(xbp_main(&x) == 0 && xbp_time_print_stats(&xr.xt) == 0)
		ret = EXIT_SUCCESS;
	animation_cleanup(&xr.a);
	xbp_cleanup(&x);
	return ret;
}
