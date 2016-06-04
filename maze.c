
// maze.c

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xbp.h"

#define ATTR_SIZE(w) ((size_t)((w)->attr.width * (w)->attr.height))

int keypressed(struct xbp *x) {
	char kr[32];
	XQueryKeymap(x->disp, kr);
	// exit on LSHIFT|LCONTROL
	return (kr[4] & 0x20) && (kr[6] & 0x04);
}

static char toroidal_sum(const char *c, size_t i, int w, int h) {
#define X (i % w)
#define Y (i / w)
#define FIRSTROW (Y == 0)
#define FIRSTCOL (X == 0)
#define TOP    (FIRSTROW * h + Y - 1)
#define LEFT   (FIRSTCOL * w + X - 1)
#define BOTTOM ((Y + 1) % h)
#define RIGHT  ((X + 1) % w)
	return c[LEFT + TOP * w] + c[X + TOP * w] + c[RIGHT + TOP * w] +
	  c[LEFT + Y * w] + c[RIGHT + Y * w] +
	  c[LEFT + BOTTOM * w] + c[X + BOTTOM * w] + c[RIGHT + BOTTOM * w];
}

void update(struct xbp *x, struct xbp_win *w, char *c, char *c_new, Pixmap *p) {
	size_t i;
	char nsum;
	for(i = 0; i < ATTR_SIZE(w); i++) {
		nsum = toroidal_sum(c, i, w->attr.width,w->attr.height);
		c_new[i] = c[i] != 0 ? !(nsum > 5) : (nsum == 3);
	}
	for(i = 0; i < ATTR_SIZE(w); i++) {
		if(i == 0 || c_new[i] ^ c_new[i - 1])
			XSetForeground(x->disp, w->gc, c_new[i] == 0 ?
			  XBlackPixel(x->disp, x->scr) : XWhitePixel(x->disp, x->scr));
		XDrawPoint(x->disp, *p, w->gc, i % w->attr.width, i / w->attr.width);
	}
}

int main(int argc, char *argv[]) {
	Pixmap p;
	struct xbp x;
	struct xbp_win w;
	char *c;
	size_t i, size;
	bool fb = false;
	if(xbp_connect(&x, NULL) < 0)
		return EXIT_FAILURE;
	xbp_getfullscreenwin(&x, &w);
	xbp_cursorinvisible(&x, &w);
	p = XCreatePixmap(x.disp, w.win, w.attr.width, w.attr.height, w.attr.depth);
	size = ATTR_SIZE(&w);
	/* TODO: instead of a flipflop buffer, I should have a 3 * width
	 * nsum buffer. the first 2 will stack down vertically over the screen and
	 * the third buffer holds the counts of the top row...
	 */
	c = malloc(size * 2);
	if(c == NULL) {
		perror("malloc");
		goto fail_c;
	}
	memset(c, 0, size);
	srand(time(NULL));
	for(i = 0; i < 100; i++) {
		c[w.attr.width * (w.attr.height / 2 - 5) + w.attr.width * (i / 10) +
		  w.attr.width / 2 - 5 + i % 10] = rand() & 1;
	}
	xbp_setpixmap(&x, &w, &p);
	do {
		update(&x, &w, c + fb * size, c + (fb ^ true) * size, &p);
		fb ^= true;
		xbp_setpixmap(&x, &w, &p);
	} while(keypressed(&x) == 0);
	free(c);
fail_c:
	XFreePixmap(x.disp, p);
	xbp_destroywin(&x, &w);
	xbp_disconnect(&x);
	return EXIT_SUCCESS;
	(void)argc;
	(void)argv;
}
