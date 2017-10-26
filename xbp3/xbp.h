
/* xbp.h
 *
 * Copyright (c) 2017, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef XBP_H
#define XBP_H

#include <limits.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define XBP_ERRPRINT(msg) do { \
	fprintf(stderr, "Error: "msg"\n"); \
	fprintf(stderr, "%s:%d: %s()\n", __FILE__, __LINE__, __func__); \
} while(0);

#define XBP_ERRPRINTF(msg, ...) do { \
	fprintf(stderr, "Error: "msg"\n", __VA_ARGS__); \
	fprintf(stderr, "%s:%d: %s()\n", __FILE__, __LINE__, __func__); \
} while(0);

struct xbp {
	Display *disp;
	int scr;
	XVisualInfo vinfo;
	Colormap cmap;

	Window win;
	GC gc;

	XImage *img;
	char init;
	bool running;
	struct xbp_sizehint {
		int width, height;
	} *sizehint;
};

int xbp_init(struct xbp *x, const char *display_name);
int xbp_main(struct xbp *x, int (*cb)(struct xbp*, void*),
             void (*action)(void*), int (*resize)(struct xbp*, void*),
             void *data);
static inline void xbp_set_pixel(XImage *img, int x, int y,
                                 unsigned long color) {
	int i, px;
	if(x < 0 || x > img->width || y < 0 || y > img->height)
		return;
	px = img->bytes_per_line * y + x * img->bits_per_pixel / CHAR_BIT;
	for(i = 0; i < img->bits_per_pixel / CHAR_BIT; i++)
		img->data[px + i] = color >> (8 * i);
}

void xbp_cleanup(struct xbp *x);

#endif // XBP_H
