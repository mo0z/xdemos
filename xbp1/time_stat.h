
/* time_stat.h
 *
 * Copyright (c) 2016, mar77i <mar77i at mar77i dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef TIME_STAT_H
#define TIME_STAT_H

#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

struct time_stat {
	struct timespec start, end;
	size_t numframes;
	bool failed;
};

void time_stat_start(struct time_stat *t);
void time_stat_end(struct time_stat *t);
void time_stat_status(struct time_stat *t);

#endif // TIME_STAT_H
