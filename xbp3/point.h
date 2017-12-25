// point.h

#ifndef POINT_H
#define POINT_H

#include <math.h>
#include <stdio.h>

#ifndef PI
#ifdef M_PI
#define PI M_PI
#else // M_PI
#define PI acos(-1.0)
#endif // M_PI
#endif // PI

struct point {
	double x, y;
};

struct polar {
	double r, a;
};

static inline int between(double a, double b, double c) {
	double tmp;
	if(b > c) {
		tmp = c;
		c = b;
		b = tmp;
	}
	return a >= b && a <= c;
}

static inline struct point point_add(struct point a, struct point b) {
	return (struct point){a.x + b.x, a.y + b.y};
}

static inline struct point point_sub(struct point a, struct point b) {
	return (struct point){a.x - b.x, a.y - b.y};
}

static inline struct point point_mul(struct point a, struct point b) {
	return (struct point){a.x * b.x, a.y * b.y};
}

static inline struct point point_div(struct point a, struct point b) {
	return (struct point){a.x / b.x, a.y / b.y};
}

static inline double point_abs(struct point a) {
	return sqrt(a.x * a.x + a.y * a.y);
}

static inline double point_angle(struct point a) {
	return atan2(a.y, a.x) * 180 / PI;
}

static inline int point_str(struct point a, char *s, size_t l) {
	return snprintf(s, l, "(%f, %f)", a.x, a.y);
}

static inline int point_exact(struct point a, char *s, size_t l) {
	return snprintf(s, l, "(%a, %a)", a.x, a.y);
}

static inline struct polar to_polar(struct point a) {
	return (struct polar){point_abs(a), point_angle(a)};
}

static inline struct point to_cartesian(struct polar a) {
	return (struct point){cos(a.a * PI / 180) * a.r, sin(a.a * PI / 180) * a.r};
}

#endif // POINT_H
