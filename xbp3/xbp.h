
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
#include <stdbool.h>
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

#define XBP_BILLION (1000 * 1000 * 1000)

struct xbp {
	Display *disp;
	Colormap cmap;
	Window win;
	GC gc;
	Visual visual;
	union {
		struct {
			XImage *img;
			size_t img_allo;
			void *data;
		} i;
		Pixmap pixmap;
	} u;
	timer_t timerid;
	int scr, win_rect[4], old_rect[4];
	unsigned int depth;

	struct xbp_config {
		size_t max_fps;
		long event_mask;
		int width, height;
		unsigned char fullscreen: 1, alpha: 1, defaultkeys: 1;
		enum {
			XBP_IMAGE,
			XBP_PIXMAP,
		} mode;
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
};

int xbp_fullscreen(struct xbp *x);
int xbp_fullscreen_leave(struct xbp *x);
int xbp_init(struct xbp *x, const char *display_name);
int xbp_main(struct xbp *x);

static inline void xbp_set_pixel(XImage *img, int x, int y,
                                 unsigned long color) {
	int i, px;
	if(x < 0 || x > img->width || y < 0 || y > img->height)
		return;
	px = img->bytes_per_line * y + x * img->bits_per_pixel / CHAR_BIT;
	switch(img->bits_per_pixel) {
	case sizeof(uint8_t):
		img->data[px] = color;
		break;
	case sizeof(uint16_t):
		*(uint16_t*)&img->data[px] = color;
		break;
	case sizeof(uint32_t):
		*(uint32_t*)&img->data[px] = color;
		break;
	case sizeof(uint64_t):
		*(uint64_t*)&img->data[px] = color;
		break;
	default:
		for(i = 0; i < img->bits_per_pixel / CHAR_BIT; i++)
			img->data[px + i] = ((unsigned char*)&color)[i];
		break;
	}
}

#define xbp_set_data(x, d) do { (x)->u.i.data = (d); } while(0)
#define xbp_get_data(x) ((x)->u.i.data)
#define xbp_ximage(x) ((x)->u.i.img)

void xbp_cleanup(struct xbp *x);

#endif // XBP_H
