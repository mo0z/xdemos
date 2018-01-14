
// point.c

#include <stdio.h>

#include "point.h"

int main(void) {
	char buf1[32], buf2[32], buf3[32];
	struct point p = {15, 16}, q = {5, 17};
	point_str(p, buf1, sizeof buf1);
	point_str(q, buf2, sizeof buf2);
	point_str(point_add(p, q), buf3, sizeof buf3);
	printf("%s + %s = %s\n", buf1, buf2, buf3);
	point_str(p, buf1, sizeof buf1);
	point_str(q, buf2, sizeof buf2);
	point_str(point_sub(p, q), buf3, sizeof buf3);
	printf("%s - %s = %s\n", buf1, buf2, buf3);
	point_str(p, buf1, sizeof buf1);
	point_str(q, buf2, sizeof buf2);
	point_str(point_mul(p, q), buf3, sizeof buf3);
	printf("%s * %s = %s\n", buf1, buf2, buf3);
	point_str(p, buf1, sizeof buf1);
	point_str(q, buf2, sizeof buf2);
	point_str(point_div(p, q), buf3, sizeof buf3);
	printf("%s / %s = %s\n", buf1, buf2, buf3);
	printf("abs(%s) = %f\n", buf1, point_abs(p));
	printf("abs(%s) = %f\n", buf2, point_abs(q));
	printf("angle(%s) = %f\n", buf1, point_angle(p));
	printf("angle(%s) = %f\n", buf2, point_angle(q));
}
