
/* xbp.h
 *
 * Copyright (c) 2017, mar77i <mysatyre at gmail dot com>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef XBP_H
#define XBP_H

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
	XWindowAttributes attr;
	GC gc;

	char *data;
	XImage *img;
	char init;
	bool running;
};

int xbp_init(struct xbp *x, const char *display_name);
int xbp_main(struct xbp *x, int (*cb)(struct xbp*, void*),
             void (*action)(void*), void *data);
void xbp_cleanup(struct xbp *x);

#endif // XBP_H
