
/* xbp.c
 *
 * Copyright (c) 2017, mar77i <mysatyre at gmail dot com>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "xbp.h"

#define BITS_PER_PIXEL 32

static inline int xbp_initimage(struct xbp *x) {
	x->data = malloc(BITS_PER_PIXEL / CHAR_BIT
	                 * x->attr.width * x->attr.height);
	if(x->data == NULL) {
		perror("malloc");
		return -1;
	}
	x->img = XCreateImage(x->disp, x->vinfo.visual, x->vinfo.depth, ZPixmap,
	         0, x->data, x->attr.width, x->attr.height, 4 * CHAR_BIT,
	         4 * x->attr.width);
	x->init++;
	return 0;
}

static inline int xbp_initwindow(struct xbp *x, Window *root) {
	XWindowAttributes root_attr;
	if(XGetWindowAttributes(x->disp, *root, &root_attr) == 0) {
		XBP_ERRPRINT("XGetWindowAttributes failed");
		return -1;
	}
	x->win = XCreateWindow(x->disp, *root,
		0, 0, root_attr.width, root_attr.height,
		0, x->vinfo.depth, InputOutput, x->vinfo.visual,
		CWBackPixel | CWColormap | CWBorderPixel | CWOverrideRedirect,
		&(XSetWindowAttributes){
			.background_pixel = BlackPixel(x->disp, x->scr),
			.border_pixel = 0,
			.colormap = x->cmap,
			.override_redirect = True,
		}
	);
	x->init++;
	if(XGetWindowAttributes(x->disp, x->win, &x->attr) == 0) {
		XBP_ERRPRINT("Error: XGetWindowAttributes failed");
		xbp_cleanup(x);
		return -1;
	}
	x->gc = XCreateGC(x->disp, x->win, 0, NULL);
	x->init++;
	XChangeProperty(x->disp, x->win,
	  XInternAtom(x->disp, "_NET_WM_STATE", False), XA_ATOM, 32,
	  PropModeReplace, (const unsigned char*)"_NET_WM_STATE_FULLSCREEN", 1);
	XSync(x->disp, False);
	XMapWindow(x->disp, x->win);
	return 0;
}

int xbp_init(struct xbp *x, const char *display_name) {
	Window root;
	if(x == NULL)
		return -1;
	x->init = 0;
	x->disp = XOpenDisplay(display_name);
	if(x->disp == NULL) {
		XBP_ERRPRINT("failed to open Display");
		return -1;
	}
	x->init++;
	x->scr = DefaultScreen(x->disp);
	if(XMatchVisualInfo(x->disp, x->scr, 32, TrueColor, &x->vinfo) == 0) {
		XBP_ERRPRINT("XMatchVisualInfo: no such visual");
		goto error;
	}
	root = RootWindow(x->disp, x->scr);
	x->cmap = XCreateColormap(x->disp, root, x->vinfo.visual, AllocNone);
	x->init++;
	if(xbp_initwindow(x, &root) < 0 || xbp_initimage(x) < 0)
		goto error;
	return 0;
error:
	xbp_cleanup(x);
	return -1;
}

static inline int xbp_keypress(struct xbp *x, XEvent *ev) {
	KeySym keysym = XkbKeycodeToKeysym(x->disp, ev->xkey.keycode, 0, 0);
	if(keysym == XK_q || keysym == XK_Escape)
		x->running = false;
	return 0;
}

static inline int xbp_handle(struct xbp *x) {
	XEvent ev;
	while(XPending(x->disp) > 0) {
		XNextEvent(x->disp, &ev);
		if(ev.type == KeyPress && xbp_keypress(x, &ev))
			return -1;
	}
	return 0;
}

int xbp_main(struct xbp *x, int (*cb)(struct xbp*, void*), void *data) {
	XGrabKeyboard(x->disp, x->win, 0, GrabModeAsync, GrabModeAsync,
	              CurrentTime);
	x->running = True;
	while(x->running) {
		cb(x, data);
		XPutImage(x->disp, x->win, x->gc, x->img,
		          0, 0, 0, 0, x->attr.width, x->attr.height);
		XSync(x->disp, False);
		if(xbp_handle(x) < 0)
			return -1;
	}
	XUngrabKeyboard(x->disp, CurrentTime);
	return 0;
}

void xbp_cleanup(struct xbp *x) {
	if(x == NULL || x->disp == NULL)
		return;
	if(x->init > 4)
		XDestroyImage(x->img);
	if(x->init > 3)
		XFreeGC(x->disp, x->gc);
	if(x->init > 2)
		XDestroyWindow(x->disp, x->win);
	if(x->init > 1)
		XFreeColormap(x->disp, x->cmap);
	if(x->init > 0)
		XCloseDisplay(x->disp);
}
