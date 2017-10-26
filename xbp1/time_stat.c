
/* time_stat.c
 *
 * Copyright (c) 2016, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <stdio.h>

#include "time_stat.h"

#define TIMESPECLD(t) ((long double)(t).tv_nsec / 1000000000 + (t).tv_sec)

void time_stat_start(struct time_stat *t) {
	t->numframes = 0;
	t->failed = false;
	if(clock_gettime(CLOCK_MONOTONIC, &t->start) < 0) {
		perror("clock_gettime");
		t->failed = true;
	}
}

void time_stat_end(struct time_stat *t) {
	if(clock_gettime(CLOCK_MONOTONIC, &t->end) < 0) {
		perror("clock_gettime");
		t->failed = true;
	}
}

void time_stat_status(struct time_stat *t) {
	fprintf(stderr, "Calculated %zu frames in ", t->numframes);
	if(t->failed == false) {
		if(t->end.tv_nsec < t->start.tv_nsec) {
			t->end.tv_sec--;
			t->end.tv_nsec += 1000000000;
		}
		t->end.tv_nsec -= t->start.tv_nsec;
		t->end.tv_sec -= t->start.tv_sec;
		fprintf(stderr, "%.3Lfs\n", TIMESPECLD(t->end));
		fprintf(stderr, "FPS: %.3Lf\n",
		  (long double)t->numframes / TIMESPECLD(t->end));
	} else
		fprintf(stderr, "unknown time.\n");
}
