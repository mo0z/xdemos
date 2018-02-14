
// draw.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hsv.h"
#include "xbp.h"

#define NUMDOTS 64
#define WAIT_FRAMES 10

#define SIGN(x) ((x) < 0 ? -1 : 1)
#define ABS(x) ((x) < 0 ? -(x) : (x))

struct draw {
	size_t wait, step;
};

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
		xbp_set_pixel(x, pos, other, color);
	}
	return;
use_y:
	for(pos = start_y; pos != end_y; pos += SIGN(delta_y)) {
		other = start_x + delta_x * ABS(pos - start_y) / ABS(delta_y);
		xbp_set_pixel(x, other, pos, color);
	}
	return;
}

static inline void get_pos(size_t w, size_t h, size_t step, size_t *x, size_t *y) {
	step %= NUMDOTS * 4;
	switch(step / NUMDOTS) {
	case 0:
		*x = w * step / NUMDOTS;
		*y = 0;
		break;
	case 1:
		*x = w - 1;
		*y = h * (step - NUMDOTS) / NUMDOTS;
		break;
	case 2:
		*x = w - w * (step - 2 * NUMDOTS) / NUMDOTS - 1;
		*y = h - 1;
		break;
	case 3:
		*x = 0;
		*y = h - h * (step - 3 * NUMDOTS) / NUMDOTS - 1;
		break;
	}
}

int update(struct xbp *x) {
	struct draw *d = xbp_get_data(x);
	size_t w = XBP_WIDTH(x), h = XBP_HEIGHT(x), x1, x2, y1, y2;
	double rgb[3] = { 0.0, 0.0, 0.0 };
	if(++d->wait < WAIT_FRAMES)
		return 0;
	get_pos(w, h, d->step, &x1, &y1);
	get_pos(w, h, d->step + NUMDOTS + 1, &x2, &y2);
	hsv_to_rgb(rgb, (double)rand() / RAND_MAX, 1.0, .8);
	draw_line(x, xbp_rgb8(x, rgb[0] * 255, rgb[1] * 255, rgb[2] * 255),
	          x1, y1, x2, y2);
	if(d->step > NUMDOTS + 1) {
		x2 = x1;
		y2 = y1;
		get_pos(w, h, d->step - NUMDOTS - 1, &x1, &y1);
		draw_line(x, BlackPixel(x->disp, x->scr), x1, y1, x2, y2);
	}
	d->step++;
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
	struct draw d = { 0, 0 };
	srand(time(NULL));
	if(xbp_init(&x, NULL) < 0)
		return EXIT_FAILURE;
	xbp_set_data(&x, &d);
	if(xbp_main(&x) == 0)
		ret = EXIT_SUCCESS;
	xbp_cleanup(&x);
	return ret;
}
