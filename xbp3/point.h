
// point.h

#ifndef POINT_H
#define POINT_H

#include <float.h>
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

#define POINT_ROUND(a) ((int)round(a))

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

static inline struct point point_add(const struct point a,
                                     const struct point b) {
	return (struct point){ a.x + b.x, a.y + b.y };
}

static inline struct point point_sub(const struct point a,
                                     const struct point b) {
	return (struct point){ a.x - b.x, a.y - b.y };
}

static inline struct point point_mul(const struct point a,
                                     const struct point b) {
	return (struct point){ a.x * b.x, a.y * b.y };
}

static inline struct point point_div(const struct point a,
                                     const struct point b) {
	return (struct point){ a.x / b.x, a.y / b.y };
}

static inline double point_abs(const struct point a) {
	return sqrt(a.x * a.x + a.y * a.y);
}

static inline double point_angle(const struct point a) {
	return atan2(a.y, a.x) * 180 / PI;
}

static inline int point_str(const struct point a, char *s, size_t l) {
	return snprintf(s, l, "(%f, %f)", a.x, a.y);
}

static inline int point_exact(const struct point a, char *s, size_t l) {
	return snprintf(s, l, "(%a, %a)", a.x, a.y);
}

static inline struct polar point_to_polar(const struct point a) {
	return (struct polar){ point_abs(a), point_angle(a) };
}

static inline struct point polar_to_point(const struct polar a) {
	return (struct point){
		cos(a.a * PI / 180) * a.r,
		sin(a.a * PI / 180) * a.r,
	};
}

static inline struct point point_rotated_at(const struct point center,
                                            const struct point other,
                                            double d) {
	struct polar p = point_to_polar(point_sub(other, center));
	p.a += d;
	return point_add(center, polar_to_point(p));
}

static inline int point_cross(const struct point p1, const struct point p2,
                              const struct point q1, const struct point q2,
                              struct point *result) {
	struct polar p = point_to_polar(point_sub(p2, p1));
	struct point delta, rq1 = point_rotated_at(p1, q1, -p.a),
		rq2 = point_rotated_at(p1, q2, -p.a);
	double x, factor = 0;
	delta = point_sub(rq1, rq2);
	if(between(p1.y, rq1.y, rq2.y) != 0) {
		x = rq1.x;
		if(fabs(delta.y) > DBL_EPSILON) {
			factor = (rq1.y - p1.y) / delta.y;
			x -= delta.x * factor;
		}
		if(between(x - p1.x, 0, p.r) == 0)
			return 0;
		if(result != NULL)
			*result = point_rotated_at(p1, (struct point){ x, p1.y }, p.a);
		return 1;
	}
	return 0;
}

static inline struct point point_limit(const struct point p, double left,
                                       double top, double right,
                                       double bottom) {
	return (struct point){
		p.x < left ? left : (p.x > right ? right : p.x),
		p.y < top ? top : (p.y > bottom ? bottom : p.y),
	};
}

static inline double angle_diff(double a, const double b) {
	a = fmod(a - b, 360.);
	if(a < .0)
		a += 360.;
	if(a > 180.)
		return 360. - a;
	return a;
}

#endif // POINT_H
