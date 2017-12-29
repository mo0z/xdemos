
// voronoi.c

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <X11/XKBlib.h>

#include "hsv.h"
#include "xbp.h"
#include "point.h"

#define NUM_POINTS 7

struct voronoi {
	struct point points[NUM_POINTS];
	unsigned long colors[NUM_POINTS];

	size_t num_perpendiculars, num_crosses, num_triangles, *triangles;
	struct vertical {
		struct point p[2];
		size_t ref[2]; // offset in points
	} *perpendiculars;
	struct cross {
		struct point p;
		size_t ref[2]; // offset in perpendiculars
	} *crosses;
	size_t num_frames;
	struct timespec total_frametime, start_time;
};

static inline struct timespec timespec_add(struct timespec ts1,
                                           struct timespec ts2) {
	register struct timespec result = ts1;
	register long sec;

	result.tv_nsec += ts2.tv_nsec;
	result.tv_sec += ts2.tv_sec;

	if(result.tv_nsec >= XBP_BILLION) {
		result.tv_sec += result.tv_nsec / XBP_BILLION;
		result.tv_nsec %= XBP_BILLION;
	}
	if(result.tv_nsec < 0) {
		sec = 1 + -(result.tv_nsec / XBP_BILLION);
		result.tv_nsec += XBP_BILLION * sec;
		result.tv_sec -= sec;
	}
	return result;
}

static inline struct timespec timespec_diff(struct timespec ts1,
                                            struct timespec ts2) {
	ts2.tv_sec *= -1;
	ts2.tv_nsec *= -1;
	return timespec_add(ts1, ts2);
}

static inline int in_points(const struct point p, struct point *points,
                            size_t len, size_t skip) {
	size_t i;
	for(i = 0; i < len; i++) {
		if(i == skip)
			continue;
		if(fabs(p.x - points[i].x) < .5 && fabs(p.y - points[i].y) < .5)
			return 1;
	}
	return 0;
}

void wiggle_points(struct xbp *x, struct voronoi *v, size_t i) {
	struct point new;
	do {
		new.x = v->points[i].x + (random() % 3 - 1);
		new.y = v->points[i].y + (random() % 3 - 1);
	} while(new.x < .5 || new.x >= XBP_WIDTH(x) - .5 ||
	  new.y < .5 || new.y >= XBP_HEIGHT(x) - .5 ||
	  in_points(new, v->points, NUM_POINTS, i) == 1);
	v->points[i] = new;
}

static inline void random_points_colors(struct xbp *x, struct voronoi *v) {
	float rgb[4] = { .0 };
	size_t i;
	for(i = 0; i < NUM_POINTS;
	  i += in_points(v->points[i], v->points, i, i) == 0) {
		v->points[i].x = rand() % XBP_WIDTH(x);
		v->points[i].y = rand() % XBP_HEIGHT(x);
	}
	for(i = 0; i < NUM_POINTS; i++) {
		hsv_to_rgb(rgb, random(), 1.0, 1.0);
		v->colors[i] = xbp_rgb8(x,
			(int)(rgb[0] * 0xff), (int)(rgb[1] * 0xff),
			((int)(rgb[2] * 0xff) << 16)
		);
	}
}

static inline int create_verticals(struct voronoi *v, double diagonal) {
	struct point delta, center, dest;
	size_t p1, p2, idx;
	for(p1 = 0, idx = 0; p1 < NUM_POINTS; p1++)
		for(p2 = p1 + 1; p2 < NUM_POINTS; p2++) {
			delta = point_sub(v->points[p2], v->points[p1]);
			center = point_add(v->points[p1], point_div(
				delta, (struct point){ 2, 2 }
			));
			dest = polar_to_point((struct polar){
				diagonal, point_angle(delta) + 90
			});
			v->perpendiculars[idx++] = (struct vertical){
				.p = { point_add(center, dest), point_sub(center, dest), },
				.ref = { p1, p2, },
			};
		}
	return 0;
}

static inline int ref_in_array(size_t ref, size_t *refs, size_t num) {
	size_t i;
	for(i = 0; i < num; i++)
		if(refs[i] == ref)
			return 1;
	return 0;
}

static inline int in_triangle(size_t x, size_t triangle[]) {
	return triangle[0] == x || triangle[1] == x || triangle[2] == x;
}

static inline int skip_triangle(size_t triangle[], size_t triangles[],
                                size_t num_triangles) {
	size_t i;
	for(i = 0; i < num_triangles; i++)
		if(in_triangle(triangle[0], &triangles[i * 3]) &&
		  in_triangle(triangle[1], &triangles[i * 3]) &&
		  in_triangle(triangle[2], &triangles[i * 3]))
			return 1;
	return 0;
}

static inline size_t create_crosses(struct voronoi *v, size_t p1, size_t p2,
                                    size_t crosses_idx, size_t *triangles_idx) {
	struct point c;
	size_t tidx, triangle[3] = {
		v->perpendiculars[p1].ref[0],
		v->perpendiculars[p1].ref[1],
		v->perpendiculars[p2].ref[
			v->perpendiculars[p1].ref[0] == v->perpendiculars[p2].ref[0] ||
			v->perpendiculars[p1].ref[1] == v->perpendiculars[p2].ref[0]
		],
	};
	if(in_triangle(v->perpendiculars[p2].ref[1], triangle) != 0) {
		if(skip_triangle(triangle, v->triangles, *triangles_idx) != 0)
			return 0;
		tidx = 3 * *triangles_idx;
		v->triangles[tidx] = triangle[0];
		v->triangles[tidx + 1] = triangle[1];
		v->triangles[tidx + 2] = triangle[2];
		(*triangles_idx)++;
	}
	if(crosses_idx >= v->num_crosses) {
		fprintf(stderr, "Warning: Too many crosses?\n");
		return 0;
	}

	if(point_cross(v->perpendiculars[p1].p[0], v->perpendiculars[p1].p[1],
	  v->perpendiculars[p2].p[0], v->perpendiculars[p2].p[1], &c)) {
		v->crosses[crosses_idx].p = c;
		v->crosses[crosses_idx].ref[0] = p1;
		v->crosses[crosses_idx].ref[1] = p2;
		return 1;
	}
	return 0;
}

int update(struct xbp *x) {
	struct voronoi *v = xbp_get_data(x);
	struct timespec frame_start, frame_end;
	double diagonal = point_abs((struct point){XBP_WIDTH(x), XBP_HEIGHT(x)});
	size_t i, j, k, triangles_idx;
	if(clock_gettime(CLOCK_MONOTONIC, &frame_start) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	XClearWindow(x->disp, x->win);
	for(i = 0; i < NUM_POINTS; i++)
		wiggle_points(x, v, i);
	create_verticals(v, diagonal);
	XSetForeground(x->disp, x->gc, WhitePixel(x->disp, x->scr));
	for(i = 0; i < NUM_POINTS; i++)
		for(j = i + 1; j < NUM_POINTS; j++)
			XDrawLine(x->disp, x->win, x->gc,
				POINT_ROUND(v->points[i].x), POINT_ROUND(v->points[i].y),
				POINT_ROUND(v->points[j].x), POINT_ROUND(v->points[j].y));
	XSetForeground(x->disp, x->gc, xbp_rgb8(x, 0xff, 0, 0));
	for(i = 0, k = 0; i < NUM_POINTS; i++)
		for(j = i + 1; j < NUM_POINTS; j++, k++)
			XDrawLine(x->disp, x->win, x->gc,
				POINT_ROUND(v->perpendiculars[k].p[0].x),
				POINT_ROUND(v->perpendiculars[k].p[0].y),
				POINT_ROUND(v->perpendiculars[k].p[1].x),
				POINT_ROUND(v->perpendiculars[k].p[1].y)
			);
	for(i = 0, k = 0, triangles_idx = 0; i < v->num_perpendiculars; i++)
		for(j = i + 1; j < v->num_perpendiculars; j++)
			k += create_crosses(v, i, j, k, &triangles_idx);
	XSetForeground(x->disp, x->gc, xbp_rgb8(x, 0xff, 0xff, 0));
	for(i = 0; i < k; i++)
		XFillArc(x->disp, x->win, x->gc, POINT_ROUND(v->crosses[i].p.x) - 2,
			POINT_ROUND(v->crosses[i].p.y) - 2, 4, 4, 0, 360 * 64);
	if(clock_gettime(CLOCK_MONOTONIC, &frame_end) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	frame_end = timespec_diff(frame_end, frame_start);
	v->total_frametime = timespec_add(v->total_frametime, frame_end);
	v->num_frames++;
	XFlush(x->disp);
	return 0;
}

int action(struct xbp *x, XEvent *ev) {
	KeySym keysym = XK_space;
	if(ev != NULL)
		keysym = XkbKeycodeToKeysym(x->disp, ev->xkey.keycode, 0, 0);
	if(keysym == XK_space)
		random_points_colors(x, xbp_get_data(x));
	return 0;
}

static inline int print_stats(struct voronoi *v) {
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	v->start_time = timespec_diff(ts, v->start_time);
	fprintf(stderr, "total runtime: %.2f\n", ((double)v->start_time.tv_sec
	        + (double)v->start_time.tv_nsec / XBP_BILLION));
	fprintf(stderr, "num frames: %zu\n", v->num_frames);
	fprintf(stderr, "FPS: %.2f\n",
		v->num_frames / ((double)v->start_time.tv_sec
		+ (double)v->start_time.tv_nsec / XBP_BILLION));
	fprintf(stderr, "time spent for calulation in total: %ld.%09ld\n",
	        v->total_frametime.tv_sec, v->total_frametime.tv_nsec);
	return 0;
}

static inline size_t range_sum(size_t stop) {
	size_t i, result = 0;
	for(i = 1; i < stop; i++)
		result += i;
	return result;
}

static inline size_t four_d_figurate(size_t stop) {
	/* implementation of A002419: 4-dimensional figurate numbers
	 * offset by -3, basically predicting the deduplicated amount of crossing
	 * points of perpendiculars.
	 *
	 * if AB-BC is the crossing of two perpendiculars relating to a common
	 * triangle ABC's center of circumcircle, that is the same circumcircle
	 * as AC-BC and AB-AC.
	 *
	 * To only calculate the point once, create_crosses keeps a list of
	 * triangles. From 3 points onward, there are as many triangles as points.
	 */
	size_t m[5] = { 6, 1, 1, 1, 1 }, i;
	if(stop < 3)
		return 0;
	stop -= 3;
	for(i = 0; i < stop; i++) {
		m[1] += m[0];
		m[2] += m[1];
		m[3] += m[2];
		m[4] += m[3];
	}
	return m[4];
}

static inline size_t tetrahedal(size_t stop) {
	// as per A000292 - Tetrahedral (or triangular pyramidal) numbers
	return stop * (stop + 1) * (stop + 2) / 6;
}

int main(void) {
	struct voronoi v = {
		.num_perpendiculars = range_sum(NUM_POINTS),
		.num_crosses = four_d_figurate(NUM_POINTS),
		.num_triangles = tetrahedal(NUM_POINTS),
		.total_frametime = { 0, 0 },
		.start_time = { 0, 0 },
		.num_frames = 0,
	};
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
	int ret = EXIT_FAILURE;
	srand(time(NULL));
	xbp_set_data(&x, &v);
	v.perpendiculars = malloc(v.num_perpendiculars * sizeof *v.perpendiculars);
	if(v.perpendiculars == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	v.crosses = malloc(v.num_crosses * sizeof *v.crosses);
	if(v.crosses == NULL) {
		perror("malloc");
		free(v.perpendiculars);
		return EXIT_FAILURE;
	}
	v.triangles = malloc(3 * v.num_triangles * sizeof *v.triangles);
	if(v.triangles == NULL) {
		perror("malloc");
		free(v.perpendiculars);
		free(v.crosses);
		return EXIT_FAILURE;
	}

	if(xbp_init(&x, NULL) < 0)
		goto error;
	random_points_colors(&x, &v);
	if(clock_gettime(CLOCK_MONOTONIC, &v.start_time) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		goto error;
	}
	if(xbp_main(&x) == 0 &&
	  print_stats(&v) == 0)
		ret = EXIT_SUCCESS;
error:
	free(v.perpendiculars);
	free(v.crosses);
	free(v.triangles);
	xbp_cleanup(&x);
	return ret;
}
