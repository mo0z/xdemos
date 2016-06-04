
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
		.flags = DoRed|DoGreen|DoBlue,
	};
	XAllocColor(x->d, x->cm, &c);
	XFreeColors(x->d, x->cm, &c.pixel, 1, 0);
	return c.pixel;
}

void xrootgen_setpixmap(struct xconn *x, Pixmap *p) {
	Atom a, b;
	XSetWindowBackgroundPixmap(x->d, x->w, *p);
	XClearWindow(x->d, x->w);
	if(x->w != RootWindow(x->d, x->s)) {
		XSync(x->d, False);
		return;
	}
	a = XInternAtom(x->d, "_XROOTPMAP_ID", False);
	b = XInternAtom(x->d, "ESETROOT_PMAP_ID", False);
	if(a == None || b == None)
		return;
	XChangeProperty(x->d, x->w, a, XA_PIXMAP, 32, PropModeReplace,
	  (unsigned char*)p, 1);
	XChangeProperty(x->d, x->w, b, XA_PIXMAP, 32, PropModeReplace,
	  (unsigned char*)p, 1);
	XSync(x->d, False);
}

void xrootgen_invisible_cursor(struct xconn *x, Cursor *c) {
	Pixmap p;
	XColor d = { .pixel = XBlackPixel(x->d, x->s) };
	p = XCreatePixmap(x->d, x->w, 1, 1, 1);
	*c = XCreatePixmapCursor(x->d, p, p, &d, &d, 0, 0);
	XFreePixmap(x->d, p);
}

int main(int argc, char *argv[]) {
	Cursor cursor;
	struct animations_data data;
	struct timespec ts;
	struct xconn x;
	size_t j;
	int i = 0;
	bool root = false;
	x.cleanup.num = 0;
	data.dir = -1;
	while(argc > 2) {
		if(strcmp(argv[1], "-r") == 0) {
			root = true;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "-up") == 0) {
			data.dir = 0;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "-down") == 0) {
			data.dir = 1;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "-left") == 0) {
			data.dir = 2;
			argc--;
			argv++;
		} else if(strcmp(argv[1], "-right") == 0) {
			data.dir = 3;
			argc--;
			argv++;
		} else
			break;
	}
	srand(time(NULL));
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
	} else
		i = rand() % animations_count(animations);
	x.d = XOpenDisplay(NULL);
	if(x.d == NULL) {
		fprintf(stderr, "Error: failed to open Display.\n");
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
		if(animations[i](&x, (void*)&data) < 0)
			break;
		xrootgen_restnsleep(ts, 50000000);
	} while(xrootgen_keypressed(x.d) == 0);
	XSync(x.d, False);
	for(j = 0; j < x.cleanup.num; j++)
		x.cleanup.func[j](&x, x.cleanup.data[j]);
	XFreeGC(x.d, x.gc);
	if(root == false) {
		XFreeCursor(x.d, cursor);
		XDestroyWindow(x.d, x.w);
	}
	XCloseDisplay(x.d);
	return EXIT_SUCCESS;
}
