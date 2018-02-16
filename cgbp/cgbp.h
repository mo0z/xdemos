
// cgbp.h

#ifndef CGBP_H
#define CGBP_H

// cgbp: the common graphics boilerplate

#include <limits.h>
#include <stdint.h>
#include <termios.h>
#include <linux/fb.h>


struct cgbp {
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	struct termios tc;
	timer_t timerid;
	int fbfd, old_fl;
	uint8_t *fbmm, *data, tc_set: 1, running: 1, timer_set: 1;
};

#define EMPTY_CGBP { \
	.fbfd = -1, \
	.fbmm = NULL, \
	.data = NULL, \
	.tc_set = 0, \
	.running = 1, \
	.timer_set = 0, \
}

#define CGBP_BYTES_PER_PIXEL(c) ((c)->vinfo.bits_per_pixel / CHAR_BIT)

typedef int cgbp_updatecb(struct cgbp*, void*);

int cgbp_init(struct cgbp *c);
int cgbp_main(struct cgbp *c, void *data, cgbp_updatecb *update);
void cgbp_cleanup(struct cgbp *c);

static inline size_t cgbp_bytes_per_line(struct cgbp *c) {
	return c->finfo.line_length;
}

static inline size_t cgbp_buffer_size(struct cgbp *c) {
	return c->vinfo.yres * c->finfo.line_length;
}

static inline size_t cgbp_width(struct cgbp *c) {
	return c->vinfo.xres;
}

static inline size_t cgbp_height(struct cgbp *c) {
	return c->vinfo.yres;
}

static inline uint32_t cgbp_get_pixel(struct cgbp *c, size_t x, size_t y) {
	size_t i, base = y * c->finfo.line_length + CGBP_BYTES_PER_PIXEL(c) * x;
	uint32_t value = c->data[base];
	for(i = 1; i < CGBP_BYTES_PER_PIXEL(c); i++)
		value = (value << 8) | c->data[base + i];
	return value;
}

static inline void cgbp_set_pixel(struct cgbp *c,
                                  size_t x, size_t y, uint32_t color) {
	size_t i, base = y * c->finfo.line_length + CGBP_BYTES_PER_PIXEL(c) * x;
	for(i = 0; i < CGBP_BYTES_PER_PIXEL(c); i++)
		c->data[base + i] = color >> (8 * i);
}

#endif // CGBP_H
