
// xor.c

#include <stdint.h>
#include <stdlib.h>

#include "hsv.h"
#include "xbp.h"

#define OFFSET_TYPE uint16_t
#define OFFSET_MAX 1023

int update(struct xbp *x) {
	float rgb[4] = { .0 };
	int px, py;
	OFFSET_TYPE *offset = xbp_get_data(x);
	uint8_t xor_value;
	for(py = 0; py < xbp_ximage(x)->height; py++)
		for(px = 0; px < xbp_ximage(x)->width; px++) {
			xor_value = (px ^ py) + (*offset & 0xff);
			hsv_to_rgb(rgb, (float)*offset / OFFSET_MAX, 1.0,
			           (float)xor_value / 255);
			xbp_set_pixel(x, px, py,
				(int)(rgb[0] * 0xff) | ((int)(rgb[1] * 0xff) << 8) |
				((int)(rgb[2] * 0xff) << 16) | (0xff << 24)
			);
		}
	if(*offset >= OFFSET_MAX)
		*offset = 0;
	else
		(*offset)++;
	return 0;
}

int main(void) {
	struct xbp x = {
		.config = {
			.fullscreen = 1,
			.alpha = 0,
			.defaultkeys = 1,
			.image = 1,
		},
		.callbacks = {
			.update = update,
		},
	};
	int ret = EXIT_FAILURE;
	OFFSET_TYPE offset = 0;
	xbp_set_data(&x, &offset);
	if(xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	if(xbp_main(&x) == 0)
		ret = EXIT_SUCCESS;
	xbp_cleanup(&x);
	return ret;
}
