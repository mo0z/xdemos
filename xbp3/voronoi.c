
// voronoi.c

#include <stdlib.h>
#include <X11/XKBlib.h>

#include "point.h"
#include "xbp.h"
#include "xbp_time.h"

#define NUM_POINTS 10

struct voronoi {
	struct xbp_time xt;
	struct point points[NUM_POINTS];
};

int action(struct xbp *x, XEvent *ev) {
	return 0;
	(void)x;
	(void)ev;
}

int update(struct xbp *x) {
	struct voronoi *v = xbp_get_data(x);
	xbp_time_frame_start(&v->xt);
	XClearWindow(x->disp, x->win);
	xbp_time_frame_end(&v->xt);
	return 0;
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
	int ret = EXIT_FAILURE;
	struct voronoi v;
	if(xbp_time_init(&v.xt) < 0)
		return EXIT_FAILURE;
	if(xbp_init(&x, NULL) < 0)
		goto error;
	xbp_set_data(&x, &v);
	srand(time(NULL));
	if(xbp_main(&x) == 0 && xbp_time_print_stats(&v.xt) == 0)
		ret = EXIT_SUCCESS;
error:
	xbp_cleanup(&x);
	return ret;
}
