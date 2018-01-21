
// voronoi.c

#include <stdlib.h>
#include <string.h>
#include <X11/XKBlib.h>

#include "hsv.h"
#include "point.h"
#include "utils.h"
#include "xbp.h"
#include "xbp_time.h"

#define DRAW_CIRCLE(xx, p, r) \
	(XFillArc((xx)->disp, (xx)->win, (xx)->gc, POINT_ROUND((p).x) - (r), \
	          POINT_ROUND((p).y) - (r), 2 * (r), 2 * (r), 0, 360 * 64))
#define DRAW_PERPENDICULAR(xx, pp) \
	(XDrawLine((xx)->disp, (xx)->win, (xx)->gc, \
	           POINT_ROUND((pp).p[0].x), POINT_ROUND((pp).p[0].y), \
	           POINT_ROUND((pp).p[1].x), POINT_ROUND((pp).p[1].y)))

struct voronoi {
	struct xbp_time xt;
	size_t num_points, allo_points;
	struct voronoi_point {
		struct point p;
		double direction;
		unsigned long color;
	} *points;
	unsigned long black;
};

static inline int in_distance(struct voronoi_point points[], size_t num,
                              struct point p, double thr) {
	size_t i;
	for(i = 0; i < num; i++)
		if(point_abs(point_sub(p, points[i].p)) <= thr)
			return 1;
	return 0;
}

static inline int voronoi_init(struct xbp *x, struct voronoi *v) {
	double color[3] = { 0 };
	size_t i, allo = v->allo_points + (v->allo_points == 0);
	while(allo < v->num_points)
		allo *= 2;
	if(allo > v->allo_points) {
		v->points = REALLOCATE(v->points, allo);
		v->allo_points = allo;
	}
	if(v->points == NULL)
		return -1;
	for(i = 0; i < v->num_points; i++) {
		do {
			v->points[i].p = (struct point){
				RANDINT(0, XBP_WIDTH(x)),
				RANDINT(0, XBP_HEIGHT(x)),
			};
		} while(in_distance(v->points, i, v->points[i].p, 3) != 0);
		hsv_to_rgb(color, (double)rand() / RAND_MAX, 1.0, 1.0);
		v->points[i].color = xbp_rgb8(
			x, color[0] * 0xff, color[1] * 0xff, color[2] * 0xff);
		v->points[i].direction = RANDINT(0, 360);
	}
	v->black = BlackPixel(x->disp, x->scr);
	return 0;
}

int action(struct xbp *x, XEvent *ev) {
	if(ev == NULL)
		return 0;
	if(XkbKeycodeToKeysym(x->disp, ev->xkey.keycode, 0, 0) == XK_space)
		voronoi_init(x, xbp_get_data(x));
	return 0;
}

static inline struct voronoi_point *voronoi_find(const struct voronoi *v,
                                                 const struct point p) {
	double dist, min_dist = DBL_MAX;
	struct voronoi_point *vp = NULL;
	size_t i;
	for(i = 0; i < v->num_points; i++) {
		dist = point_abs(point_sub(p, v->points[i].p));
		if(dist >= min_dist)
			continue;
		vp = &v->points[i];
		min_dist = dist;
	}
	return vp;
}

static inline int voronoi_add(struct xbp *x, struct voronoi *v,
     const struct point p) {
	double color[3] = { 0 };
	if(in_distance(v->points, v->num_points, p, 3) != 0)
		return 0;
	if(v->num_points >= v->allo_points) {
		v->points = REALLOCATE(v->points, v->allo_points * 2);
		if(v->points == NULL)
			return -1;
		v->allo_points *= 2;
	}
	v->points[v->num_points].p = p;
	hsv_to_rgb(color, (double)rand() / RAND_MAX, 1.0, 1.0);
	v->points[v->num_points].color = xbp_rgb8(
		x, color[0] * 0xff, color[1] * 0xff, color[2] * 0xff);
	v->points[v->num_points].direction = RANDINT(0, 360);
	v->num_points++;
	return 0;
}

static inline void voronoi_remove(struct voronoi *v, struct voronoi_point *vp) {
	size_t index;
	if(vp == NULL || v->num_points == 1)
		return;
	index = vp - v->points;
	if(index < v->num_points - 1)
		memmove(v->points + index, v->points + index + 1,
		        (v->num_points - index - 1) * sizeof *v->points);
	v->num_points--;
}

int click(struct xbp *x, XEvent *ev) {
	struct voronoi *v = xbp_get_data(x);
	if(ev == NULL)
		return 0;
	if(ev->xbutton.button == 1 &&
	  voronoi_add(x, v, (struct point){ev->xbutton.x, ev->xbutton.y}) < 0)
		return -1;
	else if(ev->xbutton.button == 3)
		voronoi_remove(v, voronoi_find(v, (struct point){
			ev->xbutton.x, ev->xbutton.y
		}));
	return 0;
}

int update(struct xbp *x) {
	struct voronoi *v = xbp_get_data(x);
	struct voronoi_point *vp;
	size_t i, y;
	xbp_time_frame_start(&v->xt);
	for(i = 0; i < v->num_points; i++) {
		v->points[i].p = point_limit(point_add(v->points[i].p, polar_to_point(
			(struct polar){1, v->points[i].direction}
		)), 0, 0, XBP_WIDTH(x) - 1, XBP_HEIGHT(x) - 1);
		v->points[i].direction += RANDINT(-5, 5);
	}
	for(i = 0; (int)i < XBP_WIDTH(x); i++)
		for(y = 0; (int)y < XBP_HEIGHT(x); y++) {
			vp = voronoi_find(v, (struct point){ i, y });
			if(vp == NULL ||
			  point_abs(point_sub((struct point){ i, y }, vp->p)) < 2.5)
				xbp_set_pixel(x, i, y, v->black);
			else
				xbp_set_pixel(x, i, y, vp->color);
		}
	xbp_time_frame_end(&v->xt);
	return 0;
}

int main(int argc, char *argv[]) {
	struct xbp x = {
		.config = {
			.fullscreen = 1,
			.alpha = 0,
			.defaultkeys = 1,
			.image = 1,
			.event_mask = KeyPressMask|ButtonPressMask|ButtonReleaseMask,
		},
		.callbacks = {
			.update = update,
			.listeners = (struct xbp_listener*[3]){
				&(struct xbp_listener){
					.event = KeyPress,
					.callback = action,
				},
				&(struct xbp_listener){
					.event = ButtonPress,
					.callback = click,
				},
				NULL,
			},
		},
	};
	struct voronoi v = {
		.num_points = 15,
		.allo_points = 0,
		.points = NULL,
	};
	int ret = EXIT_FAILURE;

	if(argc == 2) {
		errno = 0;
		v.num_points = estrtoul(argv[1]);
		if(v.num_points > 256 || v.num_points == 0) {
			fprintf(stderr, "Error: argument invalid or out of range.\n");
			return EXIT_FAILURE;
		}
		argc--;
	}
	if(argc > 1) {
		fprintf(stderr, "Error: invalid arguments.\n");
		return EXIT_FAILURE;
	}

	if(xbp_time_init(&v.xt) < 0 || xbp_init(&x, NULL) < 0 ||
	  voronoi_init(&x, &v) < 0)
		goto error;

	srand(time(NULL));
	xbp_set_data(&x, &v);
	if(xbp_main(&x) == 0 && xbp_time_print_stats(&v.xt) == 0)
		ret = EXIT_SUCCESS;
error:
	free(v.points);
	xbp_cleanup(&x);
	return ret;
}
