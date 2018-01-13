
// voronoi.c

#include <stdlib.h>
#include <X11/XKBlib.h>

#include "hsv.h"
#include "point.h"
#include "xbp.h"
#include "xbp_time.h"

#define NUM_POINTS 10
#define DRAW_CIRCLE(xx, p, r) \
	(XFillArc((xx)->disp, (xx)->win, (xx)->gc, POINT_ROUND((p).x) - (r), \
	          POINT_ROUND((p).y) - (r), 2 * (r), 2 * (r), 0, 360 * 64))
#define DRAW_PERPENDICULAR(xx, pp) \
	(XDrawLine((xx)->disp, (xx)->win, (xx)->gc, \
	           POINT_ROUND((pp).p[0].x), POINT_ROUND((pp).p[0].y), \
	           POINT_ROUND((pp).p[1].x), POINT_ROUND((pp).p[1].y)))


struct voronoi {
	struct xbp_time xt;

	struct point points[NUM_POINTS];
	double directions[NUM_POINTS];
	unsigned long colors[NUM_POINTS];

	struct perpendicular {
		struct point p[2];
		size_t ref[2]; // offset in points
	} *perpendiculars;
};

int in_distance(struct point points[], size_t num, struct point p, double thr) {
	size_t i;
	for(i = 1; i < num; i++)
		if(point_abs(point_sub(p, points[i])) <= thr)
			return 1;
	return 0;
}

static inline void voronoi_init(struct xbp *x, struct voronoi *v) {
	float rgb[4] = { .0 };
	size_t i;
	for(i = 0; i < NUM_POINTS; i++) {
		do
			v->points[i] = (struct point){
				random() % XBP_WIDTH(x),
				random() % XBP_HEIGHT(x),
			};
		while(in_distance(v->points, i, v->points[i], 3));
		hsv_to_rgb(rgb, (float)random() / RAND_MAX, 1.0, 1.0);
		v->colors[i] = xbp_rgb8(x, rgb[0] * 0xff, rgb[1] * 0xff, rgb[2] * 0xff);
		v->directions[i] = random() % 360;
	}
}

int action(struct xbp *x, XEvent *ev) {
	KeySym keysym = XK_space;
	if(ev != NULL)
		keysym = XkbKeycodeToKeysym(x->disp, ev->xkey.keycode, 0, 0);
	if(keysym == XK_space)
		voronoi_init(x, xbp_get_data(x));
	return 0;
}

static inline void move_points(int size[2], double *direction,
                               struct point *p) {
	*p = point_add(*p, polar_to_point((struct polar){1, *direction}));
	if(p->x < 0)
		p->x = 0;
	if(p->x > size[0] - 1)
		p->x = size[0] - 1;
	if(p->y < 0)
		p->y = 0;
	if(p->y > size[1] - 1)
		p->y = size[1] - 1;
	*direction += random() % 10 - 5;
}

static inline void perpendiculars_create(struct point size,
                                       struct point points[], size_t num_points,
                                       struct perpendicular perpendiculars[]) {
	struct point delta, center, dest;
	size_t i, j, k;
	for(i = 0, k = 0; i < num_points; i++)
		for(j = i + 1; j < num_points; j++) {
			delta = point_sub(points[j], points[i]);
			center = point_add(points[i],
			                   point_div(delta, (struct point){ 2, 2 }));
			dest = polar_to_point((struct polar){
				point_abs(size) + 4, point_angle(delta) + 90
			});
			perpendiculars[k++] = (struct perpendicular){
				.p = { point_add(center, dest), point_sub(center, dest), },
				.ref = { i, j },
			};
		}
}

int update(struct xbp *x) {
	struct voronoi *v = xbp_get_data(x);
	size_t i, j, k;
	xbp_time_frame_start(&v->xt);
	XClearWindow(x->disp, x->win);
	for(i = 0; i < NUM_POINTS; i++)
		move_points((int[2]){ XBP_WIDTH(x), XBP_HEIGHT(x) }, &v->directions[i],
		            &v->points[i]);
	for(i = 0; i < NUM_POINTS; i++) {
		XSetForeground(x->disp, x->gc, v->colors[i]);
		DRAW_CIRCLE(x, v->points[i], 3);
	}
	perpendiculars_create(
		(struct point){ XBP_WIDTH(x), XBP_HEIGHT(x), },
		v->points, NUM_POINTS, v->perpendiculars);
	XSetForeground(x->disp, x->gc, xbp_rgb8(x, 0xff, 0, 0));
	for(i = 0, k = 0; i < NUM_POINTS; i++) {
		for(j = i + 1; j < NUM_POINTS; j++, k++)
			DRAW_PERPENDICULAR(x, v->perpendiculars[k]);
		break;
	}
	xbp_time_frame_end(&v->xt);
	return 0;
}

static inline size_t triangular(size_t stop) {
	size_t i, result = 0;
	for(i = 1; i < stop; i++)
		result += i;
	return result;
}

int main(void) {
	struct xbp x = {
		.config = {
			.fullscreen = true,
			.alpha = false,
			.defaultkeys = true,
			.image = false,
		},
		.callbacks = {
			.update = update,
			.listeners = (struct xbp_listener*[3]){
				&(struct xbp_listener){
					.event = KeyPress,
					.callback = action,
				},
				NULL,
			},
		},
	};
	struct voronoi v;
	int ret = EXIT_FAILURE;
	if(xbp_time_init(&v.xt) < 0)
		return EXIT_FAILURE;

	v.perpendiculars = malloc(
		triangular(NUM_POINTS) * sizeof *v.perpendiculars);
	if(v.perpendiculars == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	if(xbp_init(&x, NULL) < 0)
		goto error;
	srand(time(NULL));
	voronoi_init(&x, &v);
	xbp_set_data(&x, &v);
	if(xbp_main(&x) == 0 && xbp_time_print_stats(&v.xt) == 0)
		ret = EXIT_SUCCESS;
error:
	free(v.perpendiculars);
	xbp_cleanup(&x);
	return ret;
}
