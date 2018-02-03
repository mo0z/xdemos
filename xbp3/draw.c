
// draw.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hsv.h"
#include "xbp.h"

#define SIGN(x) ((x) < 0 ? -1 : 1)
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define COORDCHECK(w, h, x, y) ( \
	(x) >= 0 && (x) < (w) && (y) >= 0 && (y) < (h) \
)

void draw_line(struct xbp *x, unsigned long color,
               int start_x, int start_y, int end_x, int end_y) {
	int delta_x, delta_y, pos, other;
	if(x == NULL)
		return;
	delta_x = end_x - start_x;
	delta_y = end_y - start_y;
	xbp_set_pixel(x, end_x, end_y, color);
	if(ABS(delta_x) < ABS(delta_y))
		goto use_y;
	for(pos = start_x; pos != end_x; pos += SIGN(delta_x)) {
		other = start_y + delta_y * ABS(pos - start_x) / ABS(delta_x);
		if(!COORDCHECK(XBP_WIDTH(x), XBP_HEIGHT(x), pos, other))
			break;
		xbp_set_pixel(x, pos, other, color);
	}
	return;
use_y:
	for(pos = start_y; pos != end_y; pos += SIGN(delta_y)) {
		other = start_x + delta_x * ABS(pos - start_y) / ABS(delta_y);
		if(!COORDCHECK(XBP_WIDTH(x), XBP_HEIGHT(x), other, pos))
			break;
		xbp_set_pixel(x, other, pos, color);
	}
	return;
}

int update(struct xbp *x) {
	size_t i;
	double rgb[3] = { 0.0, 0.0, 0.0 };
	memset(xbp_ximage_data(x), 0,
	       xbp_ximage_allo(x) * xbp_ximage_bytes_per_pixel(x));
	for(i = 0; i < 1000; i++) {
		hsv_to_rgb(rgb, (double)rand() / RAND_MAX, 1.0, .8);
		draw_line(x, xbp_rgb8(x, rgb[0] * 255, rgb[1] * 255, rgb[2] * 255),
		          rand() % XBP_WIDTH(x), rand() % XBP_HEIGHT(x),
		          rand() % XBP_WIDTH(x), rand() % XBP_HEIGHT(x)
		);
	}
	return 0;
}

int main(void) {
	struct xbp x = {
		.config = {
			.fullscreen = 1,
			.defaultkeys = 1,
			.image = 1,
		},
		.callbacks = {
			.update = update,
			.resize = NULL,
			.listeners = NULL,
		},
	};
	int ret = EXIT_FAILURE;
	srand(time(NULL));
	if(xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	if(xbp_main(&x) == 0)
		ret = EXIT_SUCCESS;
	xbp_cleanup(&x);
	return ret;
}
