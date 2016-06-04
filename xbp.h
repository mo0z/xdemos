
// xbp.h

#ifndef XBP_H
#define XBP_H

#include <stdint.h>
#include <bulk77i/bulk.h>
#include <X11/Xlib.h>

#define MAX_COLOR UINT16_MAX

struct xbp {
	Display *disp;
	int scr;
	Colormap cmap;
	struct bulk cleanup;
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
