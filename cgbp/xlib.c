
// xlib.c

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "cgbp.h"

#define BITDEPTH 32

struct xlib {
	Display *disp;
	int scr;
	XVisualInfo vinfo;
	Colormap cmap;
	Window win;
	GC gc;
	XImage *img;
	XIM xim;
	XIC xic;
	size_t img_allo;
	uint8_t cmap_set: 1, win_set: 1, gc_set: 1;
};

void xlib_cleanup(void *data);

static inline int setup_input(struct xlib *x) {
	x->xim = XOpenIM(x->disp, NULL, NULL, NULL);
	if(x->xim != NULL)
		goto done;
	XSetLocaleModifiers("@im=local");
	x->xim = XOpenIM(x->disp, NULL, NULL, NULL);
	if(x->xim != NULL)
		goto done;
	XSetLocaleModifiers("@im=");
	x->xim = XOpenIM(x->disp, NULL, NULL, NULL);
	if(x->xim == NULL) {
		fprintf(stderr, "Error: XOpenIM: could not open input devive.\n");
		return -1;
	}
done:
	x->xic = XCreateIC(x->xim, XNInputStyle, XIMPreeditNothing
					   | XIMStatusNothing, XNClientWindow, x->win,
					   XNFocusWindow, x->win, NULL);
	if(x->xic == NULL)
		return -1;
	return 0;
}

void *xlib_init(void) {
	struct xlib *x = malloc(sizeof *x);
	Window root;
	XWindowAttributes attr;
	size_t bytesize, i;
	if(x == NULL) {
		perror("malloc");
		return NULL;
	}
	x->cmap_set = 0;
	x->win_set = 0;
	x->gc_set = 0;
	x->xic_set = 0;
	x->img = NULL;
	x->disp = XOpenDisplay(NULL);
	if(x->disp == NULL) {
		fprintf(stderr, "Error: failed to open Display.\n");
		xlib_cleanup(x);
		return NULL;
	}
	x->scr = DefaultScreen(x->disp);
	if(XMatchVisualInfo(x->disp, x->scr, BITDEPTH, TrueColor, &x->vinfo) == 0) {
		fprintf(stderr, "Error: XMatchVisualInfo: no such visual.\n");
		goto error;
	}
	root = RootWindow(x->disp, x->scr);
	x->cmap = XCreateColormap(x->disp, root, x->vinfo.visual, AllocNone);
	x->cmap_set = 1;

	if(XGetWindowAttributes(x->disp, root, &attr) == 0) {
		fprintf(stderr, "Error: XGetWindowAttributes failed.\n");
		goto error;
	}
	x->win = XCreateWindow(x->disp, root,
		0, 0, attr.width, attr.height,
		0, x->vinfo.depth, InputOutput, x->vinfo.visual,
		CWBackPixel|CWColormap|CWBorderPixel|CWEventMask|CWOverrideRedirect,
		&(XSetWindowAttributes){
			.background_pixel = BlackPixel(x->disp, x->scr),
			.border_pixel = 0,
			.colormap = x->cmap,
			.event_mask = StructureNotifyMask,
			.override_redirect = True,
		}
	);
	x->win_set = 1;
	x->gc = XCreateGC(x->disp, x->win, 0, NULL);
	x->gc_set = 1;

	XMapWindow(x->disp, x->win);
	XGrabKeyboard(x->disp, x->win, 0, GrabModeAsync, GrabModeAsync,
	              CurrentTime);
	XMoveResizeWindow(x->disp, x->win, 0, 0, attr.width, attr.height);
	XRaiseWindow(x->disp, x->win);

	x->img = XCreateImage(
		x->disp, x->vinfo.visual, x->vinfo.depth, ZPixmap, 0, NULL,
		attr.width, attr.height, 8, 0
	);
	if(x->img == NULL) {
		fprintf(stderr, "Error: XCreateImage failed.\n");
		goto error;
	}
	bytesize = x->img->depth / CHAR_BIT * x->img->width * x->img->height;
	x->img->data = malloc(bytesize);
	if(x->img->data == NULL) {
		perror("malloc");
		goto error;
	}
	for(i = 0; i < bytesize; i++)
		x->img->data[i] = i % (x->img->depth / CHAR_BIT) > 2 ? 255 : 0;
	if(setup_input(x) < 0)
		goto error;
	return x;
error:
	xlib_cleanup(x);
	return NULL;
}

static inline int handle_events(struct cgbp *c, XEvent *ev) {
	struct xlib *x = c->driver_data;
	KeySym keysym;
	Status st;
	int len, i;
	char buf[32];
	switch(ev->type) {
	case KeyPress:
		len = XmbLookupString(x->xic, &ev->xkey, buf, sizeof buf, &keysym, &st);

		if(st != XLookupChars && st != XLookupBoth)
			goto keysym;
		if(len == 1 && (buf[0] == 'q' || buf[0] == '\x1b'))
			c->running = 0;
		for(i = 0; i < len; i++)
			printf("%02X%c", buf[i] & 0xff, i + 1 == len ? '\n' : ' ');
		if(st == XLookupChars)
			break;
keysym:
		if(st != XLookupKeySym && st != XLookupBoth)
			break;
		if(keysym == XK_q || keysym == XK_Escape)
			c->running = 0;
		break;
	case KeyRelease:
	case MapNotify:
		break;
	default:
		printf("event: %d\n", ev->type);
		break;
	}
	return 0;
}

int xlib_update(struct cgbp *c, void *cb_data, cgbp_updatecb *cb) {
	struct xlib *x = c->driver_data;
	XEvent ev;
	while(XPending(x->disp) > 0) {
		XNextEvent(x->disp, &ev);
		if(handle_events(c, &ev) < 0)
			return -1;
	}
	if(cb != NULL && cb(c, cb_data) < 0)
		return -1;
	XPutImage(x->disp, x->win, x->gc, x->img,
	          0, 0, 0, 0, x->img->width, x->img->height);
	return 0;
}

void xlib_cleanup(void *data) {
	struct xlib *x = data;
	if(x->xic != NULL)
		XDestroyIC(x->xic);
	if(x->xim != NULL)
		XCloseIM(x->xim);
	if(x->img != NULL)
		XDestroyImage(x->img);
	if(x->cmap_set == 1)
		XFreeColormap(x->disp, x->cmap);
	if(x->gc_set)
		XFreeGC(x->disp, x->gc);
	if(x->win_set)
		XDestroyWindow(x->disp, x->win);
	if(x->disp != NULL)
		XCloseDisplay(x->disp);
	free(x);
}

uint32_t xlib_get_pixel(void *data, size_t cx, size_t cy) {
	struct xlib *x = data;
	size_t i, bytes_per_pixel = x->img->bits_per_pixel / CHAR_BIT, px;
	uint32_t value;
	px = cy * x->img->bytes_per_line + cx * bytes_per_pixel;
	for(value = x->img->data[px], i = 1; i < bytes_per_pixel; i++)
		value |= (x->img->data[px + i] & 0xff) << (8 * i);
	return value & 0xffffff;
}

void xlib_set_pixel(void *data, size_t cx, size_t cy, uint32_t color) {
	struct xlib *x = data;
	size_t i, bytes_per_pixel = x->img->bits_per_pixel / CHAR_BIT, px;
	px = cy * x->img->bytes_per_line + cx * bytes_per_pixel;
	color = 0xff000000 | (color & 0xffffff);
	for(i = 0; i < bytes_per_pixel; i++)
		x->img->data[px + i] = color >> (8 * i);
}

struct cgbp_size xlib_size(void *data) {
	struct xlib *x = data;
	return (struct cgbp_size){
		x->img->width,
		x->img->height,
	};
}

struct cgbp_driver driver = {
	xlib_init,
	xlib_update,
	xlib_cleanup,
	xlib_get_pixel,
	xlib_set_pixel,
	xlib_size,
};
