
/* xbp.c
 *
 * Copyright (c) 2017, mar77i <mar77 at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

#include "xbp.h"

#define FRAME_SIGNAL SIGALRM

static char xbp_timer = 0;

static inline int xbp_initimage(struct xbp *x) {
	size_t current_size = XBP_WIDTH(x) * XBP_HEIGHT(x);
	if(!x->config.image)
		return 0;
	if(x->img_set) {
		if(current_size <= x->img_allo) {
			x->img->width = XBP_WIDTH(x);
			x->img->height = XBP_HEIGHT(x);
			return 0;
		}
		XDestroyImage(x->img);
	}
	x->img = XCreateImage(x->disp, &x->visual, x->depth, ZPixmap, 0, NULL,
	                      XBP_WIDTH(x), XBP_HEIGHT(x), 8, 0);
	x->img->data = malloc(xbp_ximage_bytes_per_pixel(x) * current_size);
	if(x->img->data == NULL) {
		perror("malloc");
		XDestroyImage(x->img);
		return -1;
	}
	x->img_allo = current_size;
	x->img_set = 1;
	return 0;
}

static inline long xbp_event_mask(struct xbp *x) {
	long event_mask = x->config.event_mask;
	if(x->config.defaultkeys)
		event_mask |= KeyPressMask;
	if(!x->fullscreen)
		event_mask |= StructureNotifyMask;
	return event_mask;
}

int xbp_fullscreen(struct xbp *x) {
	XWindowAttributes attr;
	XUnmapWindow(x->disp, x->win);
	XChangeWindowAttributes(x->disp, x->win, CWOverrideRedirect,
		&(XSetWindowAttributes){.override_redirect = True});
	XMapWindow(x->disp, x->win);
	XGrabPointer(x->disp, x->win, 0, xbp_event_mask(x) & XBP_POINTER_EVENTS, GrabModeAsync,
	             GrabModeAsync, 0, 0, CurrentTime);
	XGrabKeyboard(x->disp, x->win, 0, GrabModeAsync, GrabModeAsync,
	              CurrentTime);
	assert(sizeof x->win_rect == 4 * sizeof x->win_rect[0]);
	memcpy(x->old_rect, x->win_rect, sizeof x->win_rect);
	if(XGetWindowAttributes(x->disp, RootWindow(x->disp, x->scr), &attr) != 1)
		return -1;
	XMoveResizeWindow(x->disp, x->win, 0, 0, attr.width, attr.height);
	XRaiseWindow(x->disp, x->win);
	x->fullscreen = 1;
	return 0;
}

int xbp_fullscreen_leave(struct xbp *x) {
	XUngrabPointer(x->disp, CurrentTime);
	XUngrabKeyboard(x->disp, CurrentTime);
	XUnmapWindow(x->disp, x->win);
	x->fullscreen = 0;
	XChangeWindowAttributes(x->disp, x->win, CWOverrideRedirect|CWEventMask,
			&(XSetWindowAttributes){
		.override_redirect = False,
		.event_mask = xbp_event_mask(x),
	});
	XMapWindow(x->disp, x->win);
	assert(sizeof x->win_rect == 4 * sizeof x->win_rect[0]);
	memcpy(x->win_rect, x->old_rect, sizeof x->win_rect);
	XMoveResizeWindow(x->disp, x->win, x->win_rect[0], x->win_rect[1],
	                  x->win_rect[2], x->win_rect[3]);
	XRaiseWindow(x->disp, x->win);
	return 0;
}

static inline int xbp_initwindow(struct xbp *x, Window *root) {
	XWindowAttributes root_attr;
	x->win_rect[0] = x->win_rect[1] = x->win_rect[2] = x->win_rect[3] = 0;
	if(XGetWindowAttributes(x->disp, *root, &root_attr) == 0) {
		XBP_ERRPRINT("XGetWindowAttributes failed");
		return -1;
	}
	if(!x->config.fullscreen) {
		x->win_rect[2] = x->config.width;
		x->win_rect[3] = x->config.height;
	}
	if(x->win_rect[2] <= 0 || x->win_rect[2] > root_attr.width)
		x->win_rect[2] = root_attr.width;
	if(x->win_rect[3] <= 0 || x->win_rect[3] > root_attr.height)
		x->win_rect[3] = root_attr.height;
	x->win = XCreateWindow(x->disp, *root,
		x->win_rect[0], x->win_rect[1], x->win_rect[2], x->win_rect[3],
		0, x->depth, InputOutput, &x->visual,
		CWBackPixel | CWColormap | CWBorderPixel | CWEventMask,
		&(XSetWindowAttributes){
			.background_pixel = BlackPixel(x->disp, x->scr),
			.border_pixel = 0,
			.colormap = x->cmap,
			.event_mask = xbp_event_mask(x),
		}
	);
	x->win_set = 1;
	x->gc = XCreateGC(x->disp, x->win, 0, NULL);
	x->gc_set = 1;
	XMapWindow(x->disp, x->win);
	if(x->config.fullscreen)
		return xbp_fullscreen(x);
	return 0;
}

void xbp_main_signal(int sig) {
	xbp_timer |= sig == FRAME_SIGNAL;
}

static inline int xbp_inittimer(struct xbp *x) {
	size_t max_fps = 60;
	struct sigaction sa = {
		.sa_flags = 0,
		.sa_handler = xbp_main_signal,
	};
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = 0,
	};
	if(x->config.max_fps > 0 && x->config.max_fps <= 128)
		max_fps = x->config.max_fps;
	if(max_fps == 1)
		ts.tv_sec = 1;
	else
		ts.tv_nsec = 1e9 / max_fps;
	sigemptyset(&sa.sa_mask);
	sigaction(FRAME_SIGNAL, &sa, NULL);
	if(timer_create(CLOCK_MONOTONIC, NULL, &x->timerid) < 0)
		return -1;
	timer_settime(x->timerid, 0, &(struct itimerspec){
		.it_value = ts,
		.it_interval = ts,
	}, NULL);
	return 0;
}

int xbp_init(struct xbp *x, const char *display_name) {
	Window root;
	XVisualInfo vinfo;
	int depth = 24;
	if(x == NULL)
		return -1;
	x->img_allo = 0;
	x->img = NULL;
	x->timerid = 0;
	x->win_set = 0;
	x->gc_set = 0;
	x->cmap_set = 0;
	x->fullscreen = 0;
	x->img_set = 0;
	x->disp = XOpenDisplay(display_name);
	if(x->disp == NULL) {
		XBP_ERRPRINT("failed to open Display");
		return -1;
	}
	x->scr = DefaultScreen(x->disp);
	if(x->config.alpha)
		depth = 32;
	if(XMatchVisualInfo(x->disp, x->scr, depth, TrueColor, &vinfo) == 0) {
		XBP_ERRPRINT("XMatchVisualInfo: no such visual");
		goto error;
	}
	memcpy(&x->visual, vinfo.visual, sizeof *vinfo.visual);
	x->depth = vinfo.depth;
	root = RootWindow(x->disp, x->scr);
	x->cmap = XCreateColormap(x->disp, root, vinfo.visual, AllocNone);
	x->cmap_set = 1;
	if(xbp_initwindow(x, &root) < 0 || xbp_inittimer(x) < 0 ||
	  xbp_initimage(x) < 0)
		goto error;
	return 0;
error:
	xbp_cleanup(x);
	return -1;
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

static inline int xbp_resize(struct xbp *x, XConfigureEvent xce) {
	x->win_rect[0] = xce.x;
	x->win_rect[1] = xce.y;
	x->win_rect[2] = xce.width;
	x->win_rect[3] = xce.height;
	if(xbp_initimage(x) < 0)
		return -1;
	if(x->callbacks.resize != NULL)
		return x->callbacks.resize(x);
	return 0;
}

static inline int xbp_call_event_callbacks(struct xbp *x, XEvent *ev,
                                           struct xbp_listener **listeners) {
	struct xbp_listener *listener;
	if(listeners == NULL)
		return 0;
	listener = *listeners;
	while(listener != NULL) {
		if(ev->type == listener->event && listener->callback(x, ev) < 0)
			return -1;
		listener = *(++listeners);
	}
	return 0;
}

static inline void xbp_keypress(struct xbp *x, XEvent *ev) {
	KeySym keysym = XkbKeycodeToKeysym(x->disp, ev->xkey.keycode, 0, 0);
	if(keysym == XK_q || keysym == XK_Escape)
		x->running = 0;
}

static inline int xbp_handle(struct xbp *x) {
	XEvent ev;
	while(XPending(x->disp) > 0) {
		XNextEvent(x->disp, &ev);
		if(ev.type == ConfigureNotify && xbp_resize(x, ev.xconfigure) < 0)
			return -1;
		if(x->config.defaultkeys && ev.type == KeyPress)
			xbp_keypress(x, &ev);
		if(xbp_call_event_callbacks(x, &ev, x->callbacks.listeners) < 0)
			return -1;
	}
	return 0;
}

int xbp_main(struct xbp *x) {
	int ret = -1;
	xbp_timer = 0;
	x->running = 1;
	do {
		if(!xbp_timer)
			sleep(1);
		if(!xbp_timer)
			continue;
		xbp_timer = 0;
		if(x->callbacks.update != NULL && x->callbacks.update(x) < 0)
			goto error;
		if(x->img_set)
			XPutImage(x->disp, x->win, x->gc, x->img,
					  0, 0, 0, 0, XBP_WIDTH(x), XBP_HEIGHT(x));
		if(xbp_handle(x) < 0)
			goto error;
	} while(x->running);
	ret = 0;
error:
	if(x->fullscreen)
		xbp_fullscreen_leave(x);
	return ret;
}

void xbp_cleanup(struct xbp *x) {
	if(x->img_set)
		XDestroyImage(x->img);
	if(x->timerid != 0)
		timer_delete(x->timerid);
	if(x->gc_set)
		XFreeGC(x->disp, x->gc);
	if(x->win_set)
		XDestroyWindow(x->disp, x->win);
	if(x->cmap_set)
		XFreeColormap(x->disp, x->cmap);
	if(x->disp != NULL)
		XCloseDisplay(x->disp);
}
