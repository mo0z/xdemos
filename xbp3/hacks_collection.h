
/* hacks_collection.h
 *
 * Copyright (c) 2016, 2018, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef HACKS_COLLECTION_H
#define HACKS_COLLECTION_H

#include "xbp.h"

struct hacks_collection {
	enum hacks_collection_direction {
		HACKS_COLLECTION_NODIR,
		HACKS_COLLECTION_UP,
		HACKS_COLLECTION_DOWN,
		HACKS_COLLECTION_LEFT,
		HACKS_COLLECTION_RIGHT,
	} dir;
	size_t row_len, num_rows, row_current;
	char *row_prev;
};

int reflecting_box(struct xbp *x, struct hacks_collection *hc);
int wolfram1(struct xbp *x, struct hacks_collection *hc);
int wolfram2(struct xbp *x, struct hacks_collection *hc);
void hacks_collection_cleanup(struct hacks_collection *hc);

static int (*hacks_list[])(struct xbp*, struct hacks_collection*) = {
	reflecting_box,
	wolfram1,
	wolfram2,
};

#define NUM_ANIMATIONS (sizeof hacks_list / sizeof *hacks_list)

#endif // HACKS_COLLECTION_h
