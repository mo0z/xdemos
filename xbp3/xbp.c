
/* xbp.c
 *
 * Copyright (c) 2017, mar77i <mar77 at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "xbp.h"

#define FRAME_SIGNAL SIGALRM

static bool xbp_timer = false;

static inline int xbp_initimage(struct xbp *x, XVisualInfo *vinfo) {
	XWindowAttributes attr;
	if(XGetWindowAttributes(x->disp, x->win, &attr) == 0) {
		XBP_ERRPRINT("Error: XGetWindowAttributes failed");
		return -1;
	}
	x->img = XCreateImage(x->disp, vinfo->visual, vinfo->depth,
	         ZPixmap, 0, NULL, attr.width, attr.height, 8, 0);
	x->img->data = malloc(x->img->bits_per_pixel / CHAR_BIT *
	                      x->img->width * x->img->height);
	if(x->img->data == NULL) {
		perror("malloc");
		XDestroyImage(x->img);
		return -1;
	}
	x->init++;
	return 0;
}

static inline int xbp_initwindow(struct xbp *x, Window *root,
                                 XVisualInfo *vinfo) {
	XWindowAttributes root_attr;
	int width = 0, height = 0;
	Bool override_redirect = True;
	if(XGetWindowAttributes(x->disp, *root, &root_attr) == 0) {
		XBP_ERRPRINT("XGetWindowAttributes failed");
		return -1;
	}
	if(x->sizehint != NULL) {
		width = x->sizehint->width;
		height = x->sizehint->height;
	}
	if(width <= 0 || width > root_attr.width)
		width = root_attr.width;
	if(height <= 0 || height > root_attr.height)
		height = root_attr.height;
	override_redirect = width == root_attr.width && height == root_attr.height;
	x->win = XCreateWindow(x->disp, *root,
		0, 0, width, height,
		0, vinfo->depth, InputOutput, vinfo->visual,
		CWBackPixel | CWColormap | CWBorderPixel | CWOverrideRedirect,
		&(XSetWindowAttributes){
			.background_pixel = BlackPixel(x->disp, x->scr),
			.border_pixel = 0,
			.colormap = x->cmap,
			.override_redirect = override_redirect,
		}
	);
	x->init++;
	x->gc = XCreateGC(x->disp, x->win, 0, NULL);
	x->init++;
	if(override_redirect == True) {
		XChangeProperty(x->disp, x->win,
		  XInternAtom(x->disp, "_NET_WM_STATE", False), XA_ATOM, 32,
		  PropModeReplace, (const unsigned char*)"_NET_WM_STATE_FULLSCREEN", 1);
		XSync(x->disp, False);
	} else
		XSelectInput(x->disp, x->win, StructureNotifyMask);
	XMapWindow(x->disp, x->win);
	return 0;
}

void xbp_main_signal(int sig) {
	xbp_timer |= sig == FRAME_SIGNAL;
}

int xbp_inittimer(struct xbp *x) {
	size_t max_fps = 60;
	struct sigaction sa = {
		.sa_flags = 0,
		.sa_handler = xbp_main_signal,
	};
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = XBP_BILLION / max_fps,
	};
	sigemptyset(&sa.sa_mask);
	sigaction(FRAME_SIGNAL, &sa, NULL);
	if(timer_create(CLOCK_MONOTONIC, NULL, &x->timerid) < 0)
		return -1;
	x->init++;
	timer_settime(x->timerid, 0, &(struct itimerspec){
		.it_value = ts,
		.it_interval = ts,
	}, NULL);
	return 0;
}

int xbp_init(struct xbp *x, const char *display_name) {
	Window root;
	XVisualInfo vinfo;
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
	if(XMatchVisualInfo(x->disp, x->scr, 32, TrueColor, &vinfo) == 0) {
		XBP_ERRPRINT("XMatchVisualInfo: no such visual");
		goto error;
	}
	root = RootWindow(x->disp, x->scr);
	x->cmap = XCreateColormap(x->disp, root, vinfo.visual, AllocNone);
	x->init++;
	if(xbp_initwindow(x, &root, &vinfo) < 0 || xbp_inittimer(x) < 0 ||
	  xbp_initimage(x, &vinfo) < 0)
		goto error;
	return 0;
error:
	xbp_cleanup(x);
	return -1;
}

static inline int xbp_keypress(struct xbp *x, XEvent *ev,
                               void (*action)(void*), void *data) {
	KeySym keysym = XkbKeycodeToKeysym(x->disp, ev->xkey.keycode, 0, 0);
	if(keysym == XK_q || keysym == XK_Escape)
		x->running = false;
	if(keysym == XK_space && action != NULL)
		action(data);
	return 0;
}

static inline int xbp_resize(struct xbp *x, XConfigureEvent xce,
                                  int (*resize)(struct xbp*, void*),
                                  void *data) {
	XVisualInfo vinfo;
	int depth = 24;
	if(x->init <= 4 || xce.window != x->win ||
	  (xce.width == x->img->width && xce.height == x->img->height))
		return 0;
	XDestroyImage(x->img);
	x->img = NULL;
	if(XMatchVisualInfo(x->disp, x->scr, depth, TrueColor, &vinfo) == 0) {
		XBP_ERRPRINT("XMatchVisualInfo: no such visual");
		return -1;
	}
	x->init--;
	xbp_initimage(x, &vinfo);
	if(resize != NULL && resize(x, data) < 0)
		return -1;
	 return 0;
}

static inline int xbp_handle(struct xbp *x, void (*action)(void*),
                             int (*resize)(struct xbp*, void*), void *data) {
	XEvent ev;
	while(XPending(x->disp) > 0) {
		XNextEvent(x->disp, &ev);
		if(ev.type == KeyPress && xbp_keypress(x, &ev, action, data) < 0)
			return -1;
		else if(ev.type == ConfigureNotify &&
		  xbp_resize(x, ev.xconfigure, resize, data) < 0)
			return -1;
	}
	return 0;
}

int xbp_main(struct xbp *x, int (*cb)(struct xbp*, void*),
             void (*action)(void*), int (*resize)(struct xbp*, void*),
             void *data) {
	XGrabKeyboard(x->disp, x->win, 0, GrabModeAsync, GrabModeAsync,
	              CurrentTime);
	xbp_timer = false;
	x->running = true;
	do {
		if(xbp_timer == false)
			sleep(1);
		if(xbp_timer == false)
			continue;
		xbp_timer = false;
		if(cb != NULL && cb(x, data) < 0)
			break;
		XPutImage(x->disp, x->win, x->gc, x->img,
		          0, 0, 0, 0, x->img->width, x->img->height);
		if(xbp_handle(x, action, resize, data) < 0)
			return -1;
	} while(x->running == true);
	XUngrabKeyboard(x->disp, CurrentTime);
	return 0;
}

void xbp_cleanup(struct xbp *x) {
	if(x == NULL || x->disp == NULL)
		return;
	if(x->init > 5)
		XDestroyImage(x->img);
	if(x->init > 4)
		timer_delete(x->timerid);
	if(x->init > 3)
		XFreeGC(x->disp, x->gc);
	if(x->init > 2)
		XDestroyWindow(x->disp, x->win);
	if(x->init > 1)
		XFreeColormap(x->disp, x->cmap);
	if(x->init > 0)
		XCloseDisplay(x->disp);
}
