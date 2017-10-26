
/* xbp.c
 *
 * Copyright (c) 2016, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include "xbp.h"

#include <stdio.h>
#include <X11/Xatom.h>

int xbp_connect(struct xbp *x, char *display_name) {
	if(x == NULL)
		return -1;
	x->disp = XOpenDisplay(display_name);
	if(x->disp == NULL) {
		fprintf(stderr, "Error: failed to open Display.\n");
		return -1;
	}
	x->scr = DefaultScreen(x->disp);
	if(!XMatchVisualInfo(x->disp, x->scr, 32, TrueColor, &x->vinfo)) {
		fprintf(stderr, "Error: XMatchVisualInfo: no such visual\n");
		return -1;
	}
	x->cmap = XCreateColormap(x->disp, RootWindow(x->disp, x->scr),
	                          x->vinfo.visual, AllocNone);
	return 0;
}

unsigned long xbp_rgb(struct xbp *x, unsigned short red,
                      unsigned short green, unsigned short blue) {
	XColor c = {
		.red = red,
		.green = green,
		.blue = blue,
		.flags = DoRed|DoGreen|DoBlue,
	};
	XAllocColor(x->disp, x->cmap, &c);
	XFreeColors(x->disp, x->cmap, &c.pixel, 1, 0);
	return c.pixel;
}

void xbp_cursorinvisible(struct xbp *x, struct xbp_win *w) {
	XColor d = { .pixel = XBlackPixel(x->disp, x->scr) };
	Pixmap p = XCreatePixmap(x->disp, w->win, 1, 1, 1);
	Cursor c = XCreatePixmapCursor(x->disp, p, p, &d, &d, 0, 0);
	XChangeWindowAttributes(x->disp, w->win, CWCursor,
	  &(XSetWindowAttributes){ .cursor = c });
	XFreeCursor(x->disp, c);
	XFreePixmap(x->disp, p);
}

int xbp_getrootwin(struct xbp *x, struct xbp_win *w) {
	if(x == NULL || w == NULL)
		return -1;
	w->win = RootWindow(x->disp, x->scr);
	XGetWindowAttributes(x->disp, w->win, &w->attr);
	w->gc = XCreateGC(x->disp, w->win, 0, NULL);
	return 0;
}

int xbp_getfullscreenwin(struct xbp *x, struct xbp_win *w) {
	if(x == NULL || w == NULL)
		return -1;
	xbp_getrootwin(x, w);
	XFreeGC(x->disp, w->gc);
	w->win = XCreateWindow(x->disp, w->win, 0, 0, w->attr.width, w->attr.height,
		0, x->vinfo.depth, InputOutput, x->vinfo.visual,
		CWBackPixel | CWColormap | CWBorderPixel | CWOverrideRedirect,
		&(XSetWindowAttributes){
			.background_pixel = BlackPixel(x->disp, x->scr),
			.border_pixel = 0,
			.colormap = XCreateColormap(x->disp, XRootWindow(x->disp, x->scr), x->vinfo.visual, AllocNone),
			.override_redirect = True,
		}
	);
	XGetWindowAttributes(x->disp, w->win, &w->attr);
	w->gc = XCreateGC(x->disp, w->win, 0, NULL);
	XChangeProperty(x->disp, w->win,
	  XInternAtom(x->disp, "_NET_WM_STATE", False), XA_ATOM, 32,
	  PropModeReplace, (const unsigned char*)"_NET_WM_STATE_FULLSCREEN", 1);
	XSync(x->disp, False);
	XMapWindow(x->disp, w->win);
	return 0;
}

void xbp_setpixmap(struct xbp *x, struct xbp_win *w, Pixmap *p) {
	Atom a, b;
	XSetWindowBackgroundPixmap(x->disp, w->win, *p);
	XClearWindow(x->disp, w->win);
	if(w->win != RootWindow(x->disp, x->scr)) {
		XSync(x->disp, False);
		return;
	}
	a = XInternAtom(x->disp, "_XROOTPMAP_ID", False);
	b = XInternAtom(x->disp, "ESETROOT_PMAP_ID", False);
	if(a == None || b == None)
		return;
	XChangeProperty(x->disp, w->win, a, XA_PIXMAP, 32, PropModeReplace,
	  (unsigned char*)p, 1);
	XChangeProperty(x->disp, w->win, b, XA_PIXMAP, 32, PropModeReplace,
	  (unsigned char*)p, 1);
	XSync(x->disp, False);
}

void xbp_destroywin(struct xbp *x, struct xbp_win *w) {
	if(x == NULL || w == NULL || w->win == RootWindow(x->disp, x->scr))
		return;
	XFreeGC(x->disp, w->gc);
	XDestroyWindow(x->disp, w->win);
}

void xbp_disconnect(struct xbp *x) {
	if(x == NULL)
		return;
	XCloseDisplay(x->disp);
}
