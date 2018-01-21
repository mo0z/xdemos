
// utils.h

#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static inline void *emalloc(size_t size) {
	void *np = malloc(size);
	if(np == NULL)
		perror("malloc");
	return np;
}

#define ALLOCATE(x, n) ((x) = emalloc((n) * sizeof *(x)))

static inline void *erealloc_fof(void *ptr, size_t size) {
	// error-reporting realloc with free on fail
	void *np = realloc(ptr, size);
	if(np == NULL) {
		free(ptr);
		perror("realloc");
	}
	return np;
}

#define REALLOCATE(x, n) erealloc_fof((x), (n) * sizeof *(x))

static inline size_t estrtoul(const char *s) {
	size_t result;
	errno = 0;
	result = strtoul(s, NULL, 0);
	if(errno != 0) {
		perror("strtoul");
		return SIZE_MAX;
	}
	return result;
}

/*
static inline size_t triangular(size_t n) {
	size_t i, result = 0;
	for(i = 1; i < n; i++)
		result += i;
	return result;
}

static inline size_t four_d_figurate(size_t n) {
	 * implementation of A002419: 4-dimensional figurate numbers
	 * offset by -3, basically predicting the deduplicated amount of crossing
	 * points of perpendiculars.
	 *
	 * if AB-BC is the crossing of two perpendiculars relating to a common
	 * triangle ABC's center of circumcircle, that is the same circumcircle
	 * as AC-BC and AB-AC.
	 *
	 * To only calculate the point once, create_crosses keeps a list of
	 * triangles. From 3 points onward, there are as many triangles as points.
	 *
	size_t m[5] = { 6, 1, 1, 1, 1 }, i;
	if(n < 3)
		return 0;
	n -= 3;
	for(i = 0; i < n; i++) {
		m[1] += m[0];
		m[2] += m[1];
		m[3] += m[2];
		m[4] += m[3];
	}
	return m[4];
}
*/

// return a random integer between min and max inclusively
#define RANDINT(min, max) ((min) + rand() % ((max) - (min) + 1))

#endif // UTILS_H
