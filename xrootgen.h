
// xrootgen.h

#ifndef XROOTGEN_H
#define XROOTGEN_H

#include <X11/Xlib.h>

#define MAX_CLEANUP 32

struct xconn {
	Display *d;
	int s;
	Window w;
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

#endif // XROOTGEN_H
