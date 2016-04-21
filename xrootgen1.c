
// xrootgen1.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

struct xmeta {
	Display *d;
	int s;
	Window r;
	Colormap cm;
	GC gc;
};

void xrootgen1_draw1(struct xmeta *x) {
	XWindowAttributes a;
	Pixmap p;
	XGetWindowAttributes(x->d, x->r, &a);
	p = XCreatePixmap(x->d, x->r, a.width, a.height, a.depth);
	XSetForeground(x->d, x->gc, XBlackPixel(x->d, x->s));
	XFillRectangle(x->d, p, x->gc, 0, 0, a.width, a.height);
	XSetForeground(x->d, x->gc, XWhitePixel(x->d, x->s));
	XFillRectangle(x->d, p, x->gc, 50, 50, 100, 100);
	XSetWindowBackgroundPixmap(x->d, x->r, p);
	XFreePixmap(x->d, p);
	XFreeColors(x->d, x->cm, (unsigned long[]){ c.pixel }, 1, 0);
}

void xrootgen1_draw2(struct xmeta *x) {
	XWindowAttributes a;
	Pixmap p;
	struct timespec start, current;
	int off = 50, move = 5;
	XGetWindowAttributes(x->d, x->r, &a);
	if(clock_gettime(CLOCK_MONOTONIC, &start) < 0)
		return;
	current = start;
	p = XCreatePixmap(x->d, x->r, a.width, a.height, a.depth);
	XSetGraphicsExposures(x->d, x->gc, True);
	while(current.tv_sec - 4 <= start.tv_sec) {
		XSetForeground(x->d, x->gc, XBlackPixel(x->d, x->s));
		XFillRectangle(x->d, p, x->gc, 0, 0, a.width, a.height);
		XSetForeground(x->d, x->gc, XWhitePixel(x->d, x->s));
		XFillRectangle(x->d, p, x->gc, off, off, 100, 100);
		XSetWindowBackgroundPixmap(x->d, x->r, p);
		XClearWindow(x->d, x->r);
		XSync(x->d, False);
		usleep(50000);
		off += move;
		if(off >= 200 || off <= 50)
			move *= -1;
		if(clock_gettime(CLOCK_MONOTONIC, &current) < 0)
			break;
	}
	XSetForeground(x->d, x->gc, XBlackPixel(x->d, x->s));
	XFillRectangle(x->d, p, x->gc, 0, 0, a.width, a.height);
	XSetWindowBackgroundPixmap(x->d, x->r, p);
	XClearWindow(x->d, x->r);
	XSync(x->d, False);
	XFreePixmap(x->d, p);
}

int main(int argc, char *argv[]) {
	struct xmeta x = {
		.d = XOpenDisplay(NULL),
	};
	int ret = EXIT_SUCCESS;
	if(x.d == NULL) {
		fprintf(stderr, "Error: failed opening Display.\n");
		return EXIT_FAILURE;
	}
	x.s = DefaultScreen(x.d);
	x.r = RootWindow(x.d, x.s);
	x.cm = XDefaultColormap(x.d, x.s);
	x.gc = XCreateGC(x.d, x.r, 0, NULL);
	xrootgen1_draw2(&x);
	XSync(x.d, False);
	XFreeGC(x.d, x.gc);
	XFreeColormap(x.d, x.cm);
	XCloseDisplay(x.d);
	return ret;
	(void)argc;
	(void)argv;
}
