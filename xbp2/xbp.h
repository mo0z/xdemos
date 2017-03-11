
/* xbp.h
 *
 * Copyright (c) 2016, mar77i <mysatyre at gmail dot com>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef XBP_H
#define XBP_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_COLOR UINT16_MAX

struct xbp {
	Display *disp;
	int scr;
	XVisualInfo vinfo;
	Colormap cmap;
};

struct xbp_win {
	Window win;
	XWindowAttributes attr;
	GC gc;
};

void xbp_cleanup_pixmap(struct xbp *x, void *p);
void xbp_cleanup_free(struct xbp *x, void *p);
int xbp_cleanup_add(struct xbp *x,
  void (*func)(struct xbp*, void*), void *data);
int xbp_connect(struct xbp *x, char *display_name);
unsigned long xbp_rgb(struct xbp *x, unsigned short red,
                      unsigned short green, unsigned short blue);
void xbp_cursorinvisible(struct xbp *x, struct xbp_win *w);
int xbp_getrootwin(struct xbp *x, struct xbp_win *w);
int xbp_getfullscreenwin(struct xbp *x, struct xbp_win *w);
void xbp_setpixmap(struct xbp *x, struct xbp_win *w, Pixmap *p);
void xbp_destroywin(struct xbp *x, struct xbp_win *w);
void xbp_disconnect(struct xbp *x);

#endif // XBP_H
