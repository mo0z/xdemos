
// xrootgen.c

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <X11/Xatom.h>

#include "animations.h"
#include "xrootgen.h"

void xrootgen_restnsleep(struct timespec ts, long nsec) {
	struct timespec now;
	if(clock_gettime(CLOCK_MONOTONIC, &now) < 0)
		return;
	ts.tv_nsec -= now.tv_nsec;
	ts.tv_sec -= now.tv_sec;
	if(nsec < 0)
		return;
	ts.tv_nsec += nsec;
	while(ts.tv_nsec >= 1000000000L) {
		ts.tv_sec += 1L;
		ts.tv_nsec -= 1000000000L;
	}
	while(ts.tv_nsec < 0) {
		ts.tv_sec -= 1L;
		ts.tv_nsec += 1000000000L;
	}
	if(ts.tv_sec < 0 || ts.tv_nsec < 0)
		return;
	nanosleep(&ts, NULL);
}

int xrootgen_keypressed(Display *d) {
	char kr[32];
	XQueryKeymap(d, kr);
	// exit on LSHIFT|LCONTROL
	return (kr[4] & 0x20) && (kr[6] & 0x04);
}

void cleanup_pixmap(struct xconn *x, void *p) {
	XFreePixmap(x->d, *(Pixmap*)p);
}

void cleanup_free(struct xconn *x, void *p) {
	free(p);
	(void)x;
}

int xrootgen_cleanup_add(struct xconn *x,
  void (*func)(struct xconn*, void*), void *data) {
	if(x->cleanup.num == MAX_CLEANUP) {
		fprintf(stderr, "Error: could not add data to cleanup stack.\n");
		return -1;
	}
	x->cleanup.func[x->cleanup.num] = func;
	x->cleanup.data[x->cleanup.num] = data;
	x->cleanup.num++;
	return 0;
}

unsigned long xrootgen_rgb(struct xconn *x, unsigned short red,
                           unsigned short green, unsigned short blue) {
	XColor c = {
		.red = red,
		.green = green,
		.blue = blue,
	};
	XAllocColor(x->d, x->cm, &c);
	XFreeColors(x->d, x->cm, &c.pixel, 1, 0);
	return c.pixel;
}

void xrootgen_invisible_cursor(struct xconn *x, Cursor *c) {
	static char empty[8] = { 0 };
	static Pixmap p = { 0 };
	static bool already = false;
	XColor b = { 0 };
	if(already == false) {
		p = XCreateBitmapFromData(x->d, x->w, empty, 8, 8);
		already = true;
	}
	*c = XCreatePixmapCursor(x->d, p, None, &b, &b, 0, 0);
	xrootgen_cleanup_add(x, cleanup_pixmap, &p);
}

int main(int argc, char *argv[]) {
	Cursor cursor;
	struct timespec ts;
	struct xconn x;
	size_t j;
	int i;
	bool root = false;
	x.cleanup.num = 0;
	if(argc > 1 && strcmp(argv[1], "-r") == 0) {
		root = true;
		argc--;
		argv++;
	}
	if(argc > 2) {
		fprintf(stderr, "Error: invalid arguments.\n");
		return EXIT_FAILURE;
	} else if(argc == 2) {
		i = strtol(argv[1], NULL, 0);
		if(errno == ERANGE) {
			fprintf(stderr, "Warning: could not parse `%s' as a number.\n",
			  argv[1]);
			argc = 1;
		}
	}
	srand(time(NULL));
	if(argc == 1)
		i = rand() % animations_count(animations);
	x.d = XOpenDisplay(NULL);
	if(x.d == NULL) {
		fprintf(stderr, "Error: failed opening Display.\n");
		return EXIT_FAILURE;
	}
	x.s = DefaultScreen(x.d);
	x.cm = XDefaultColormap(x.d, x.s);
	x.w = RootWindow(x.d, x.s);
	XGetWindowAttributes(x.d, x.w, &x.a);
	if(root == false) {
		xrootgen_invisible_cursor(&x, &cursor);
		x.w = XCreateWindow(x.d, x.w, 0, 0,
		  x.a.width, x.a.height, 0, CopyFromParent, CopyFromParent,
		  CopyFromParent, CWColormap|CWOverrideRedirect|CWCursor,
		  &(XSetWindowAttributes){
			.colormap = x.cm,
			.override_redirect = True,
			.cursor = cursor,
		});
		XChangeProperty(x.d, x.w, XInternAtom(x.d, "_NET_WM_STATE", False),
		  XA_ATOM, 32, PropModeReplace,
		  (const unsigned char*)"_NET_WM_STATE_FULLSCREEN", 1);
		XSync(x.d, False);
		XMapWindow(x.d, x.w);
	}
	x.gc = XCreateGC(x.d, x.w, 0, NULL);
	do {
		if(clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
			break;
		XGetWindowAttributes(x.d, x.w, &x.a);
		if(animations[i](&x) < 0)
			break;
		xrootgen_restnsleep(ts, 50000000);
	} while(xrootgen_keypressed(x.d) == 0);
	XSync(x.d, False);
	if(root == false)
		XFreeCursor(x.d, cursor);
	XFreeGC(x.d, x.gc);
	for(j = 0; j < x.cleanup.num; j++)
		x.cleanup.func[j](&x, x.cleanup.data[j]);
	XCloseDisplay(x.d);
	return EXIT_SUCCESS;
}
