
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
	for(py = 0; py < x->img->height; py++)
		for(px = 0; px < x->img->width; px++) {
			xor_value = (px ^ py) + (*offset & 0xff);
			hsv_to_rgb(rgb, (float)*offset / OFFSET_MAX, 1.0,
			           (float)xor_value / 255);
			xbp_set_pixel(x->img, px, py,
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
			.fullscreen = true,
			.alpha = false,
			.defaultkeys = true,
		},
	};
	int ret = EXIT_FAILURE;
	OFFSET_TYPE offset = 0;
	xbp_set_data(&x, &offset);
	if(xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	if(xbp_main(&x, (struct xbp_callbacks){.update = update}) == 0)
		ret = EXIT_SUCCESS;
	xbp_cleanup(&x);
	return ret;
}
