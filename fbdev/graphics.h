
// graphics.h

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <limits.h>
#include <stdint.h>
#include <termios.h>
#include <linux/fb.h>


struct graphics {
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	struct termios tc;
	timer_t timerid;
	int fbfd, old_fl;
	uint8_t *fbmm, *data, tc_set: 1, running: 1, timer_set: 1;
};

#define EMPTY_GRAPHICS { \
	.fbfd = -1, \
	.fbmm = NULL, \
	.data = NULL, \
	.tc_set = 0, \
	.running = 1, \
	.timer_set = 0, \
}

#define GRAPHICS_BYTES_PER_PIXEL(g) ((g)->vinfo.bits_per_pixel / CHAR_BIT)

typedef int graphics_updatecb(struct graphics*, void*);

int graphics_init(struct graphics *g);
int graphics_main(struct graphics *g, void *data, graphics_updatecb *cb);
void graphics_cleanup(struct graphics *g);

static inline size_t graphics_buffer_size(struct graphics *g) {
	return g->vinfo.yres_virtual * g->finfo.line_length;
}

static inline size_t graphics_width(struct graphics *g) {
	return g->vinfo.xres;
}

static inline size_t graphics_height(struct graphics *g) {
	return g->vinfo.yres;
}

static inline uint32_t graphics_get_pixel(struct graphics *g,
                                          size_t x, size_t y) {
	size_t base = GRAPHICS_BYTES_PER_PIXEL(g) * (y * g->vinfo.xres + x), i;
	uint32_t value = g->data[base];
	for(i = 1; i < GRAPHICS_BYTES_PER_PIXEL(g); i++)
		value = (value << 8) | g->data[base + i];
	return value;
}

static inline void graphics_set_pixel(struct graphics *g,
                                      size_t x, size_t y, uint32_t color) {
	size_t base = GRAPHICS_BYTES_PER_PIXEL(g) * (y * g->vinfo.xres + x), i;
	for(i = 0; i < GRAPHICS_BYTES_PER_PIXEL(g); i++)
		g->data[base + i] = color >> (8 * i);
}

#endif // GRAPHICS_H
