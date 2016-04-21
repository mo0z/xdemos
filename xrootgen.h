
// xrootgen.h

#ifndef XROOTGEN_H
#define XROOTGEN_H

#include <X11/Xlib.h>

#define MAX_CLEANUP 32
#define MAX_COLOR 0x10000

struct xconn {
	Display *d;
	int s;
	Window w;
	XWindowAttributes a;
	Colormap cm;
	GC gc;
	struct {
		size_t num;
		void (*func[MAX_CLEANUP])(struct xconn*, void*);
		void *data[MAX_CLEANUP];
	} cleanup;
};

int xrootgen_cleanup_add(struct xconn *x,
  void (*func)(struct xconn*, void*), void *data);
void cleanup_pixmap(struct xconn *x, void *p);
void cleanup_free(struct xconn *x, void *p);
unsigned long xrootgen_rgb(struct xconn *x, unsigned short red,
                           unsigned short green, unsigned short blue);
void xrootgen_setpixmap(struct xconn *x, Pixmap *p);

#endif // XROOTGEN_H
