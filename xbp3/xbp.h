
/* xbp.h
 *
 * Copyright (c) 2017, mar77i <mar77i at protonmail dot ch>
 *
 * This software may be modified and distributed under the terms
 * of the ISC license.  See the LICENSE file for details.
 */

#ifndef XBP_H
#define XBP_H

#include <limits.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define XBP_ERRPRINT(msg) do { \
	fprintf(stderr, "Error: "msg"\n"); \
	fprintf(stderr, "%s:%d: %s()\n", __FILE__, __LINE__, __func__); \
} while(0);

#define XBP_ERRPRINTF(msg, ...) do { \
	fprintf(stderr, "Error: "msg"\n", __VA_ARGS__); \
	fprintf(stderr, "%s:%d: %s()\n", __FILE__, __LINE__, __func__); \
} while(0);

#if defined(XBP_CONFIG_KEEP_TIME) || defined(XBP_C)
#include "xbp_time.h"
#endif // defined(XBP_CONFIG_KEEP_TIME) || defined(XBP_C)

#define XBP_WIDTH(x) ((x)->win_rect[2])
#define XBP_HEIGHT(x) ((x)->win_rect[3])

#define XBP_MAX_COLOR UINT16_MAX

#define XBP_POINTER_EVENTS (\
	ButtonPressMask|ButtonReleaseMask|EnterWindowMask|LeaveWindowMask| \
	PointerMotionMask|PointerMotionHintMask|Button1MotionMask| \
	Button2MotionMask|Button3MotionMask|Button4MotionMask|Button5MotionMask| \
	ButtonMotionMask|KeymapStateMask)

struct xbp {
	Display *disp;
	Colormap cmap;
	Window win;
	GC gc;
	Visual visual;
	size_t img_allo;
	XImage *img;
	timer_t timerid;
	int scr, win_rect[4], old_rect[4];
	unsigned int depth;
	void *data;

	struct xbp_config {
		size_t max_fps;
		long event_mask;
		int width, height;
		unsigned char fullscreen: 1, alpha: 1, image: 1,
		              defaultkeys: 1, time: 1;
	} config;
	struct xbp_callbacks {
		int (*update)(struct xbp*);
		int (*resize)(struct xbp*);
		struct xbp_listener {
			int event;
			int (*callback)(struct xbp*, XEvent*);
		} **listeners; // NULL-terminated list
	} callbacks;
	unsigned char running: 1, gc_set: 1, cmap_set: 1, win_set: 1, fullscreen: 1,
		img_set: 1;
#if defined(XBP_CONFIG_KEEP_TIME) || defined(XBP_C)
	struct xbp_time xt;
#endif // defined(XBP_CONFIG_KEEP_TIME) || defined(XBP_C)
};

int xbp_fullscreen(struct xbp *x);
int xbp_fullscreen_leave(struct xbp *x);
int xbp_init_(struct xbp *x, const char *display_name, char keep_time);

#ifndef XBP_CONFIG_KEEP_TIME
#define xbp_init(x, d) xbp_init_(x, d, 0)
#else // XBP_CONFIG_KEEP_TIME
#define xbp_init(x, d) xbp_init_(x, d, 1)
#endif // XBP_CONFIG_KEEP_TIME

unsigned long xbp_rgb(struct xbp *x, unsigned short red,
                      unsigned short green, unsigned short blue);
int xbp_main_(struct xbp *x, char keep_time);

#ifndef XBP_CONFIG_KEEP_TIME
#define xbp_main(x) xbp_main_(x, 0)
#else // XBP_CONFIG_KEEP_TIME
#define xbp_main(x) xbp_main_(x, 1)
#endif // XBP_CONFIG_KEEP_TIME

#define xbp_rgb8(x, r, g, b) \
	(xbp_rgb((x), ((int)(r) << 8), ((int)(g) << 8), ((int)(b) << 8)))
#define xbp_set_data(x, d) do { (x)->data = (d); } while(0)
#define xbp_get_data(x) ((x)->data)
#define xbp_ximage_data(x) ((x)->img->data)
#define xbp_ximage_allo(x) ((x)->img_allo)
#define xbp_ximage(x) ((x)->img)
#define xbp_ximage_bytes_per_pixel(x) (xbp_ximage(x)->bits_per_pixel / CHAR_BIT)

static inline void xbp_set_pixel(struct xbp *x, int cx, int cy,
                                 unsigned long color) {
	XImage *ximage = xbp_ximage(x);
	size_t i, bytes_per_pixel = xbp_ximage_bytes_per_pixel(x), px;
	if(cx < 0 || cx > XBP_WIDTH(x) || cy < 0 || cy > XBP_HEIGHT(x))
		return;
	px = cy * ximage->bytes_per_line + cx * bytes_per_pixel;
	for(i = 0; i < bytes_per_pixel; i++)
		ximage->data[px + i] = color >> (8 * i);
}

void xbp_cleanup(struct xbp *x);

#endif // XBP_H
