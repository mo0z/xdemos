
// xrootgen.c

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

int main(int argc, char *argv[]) {
	struct timespec ts;
	struct xconn x;
	size_t j;
	int i;
	x.cleanup.num = 0;
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
	x.w = RootWindow(x.d, x.s);
	x.cm = XDefaultColormap(x.d, x.s);
	x.gc = XCreateGC(x.d, x.w, 0, NULL);
	do {
		if(clock_gettime(CLOCK_MONOTONIC, &ts) < 0 ||
		  animations[i](&x) < 0)
			break;
		xrootgen_restnsleep(ts, 50000000);
	} while(xrootgen_keypressed(x.d) == 0);
	XSync(x.d, False);
	XFreeGC(x.d, x.gc);
	for(j = 0; j < x.cleanup.num; j++)
		x.cleanup.func[j](&x, x.cleanup.data[j]);
	XCloseDisplay(x.d);
	return EXIT_SUCCESS;
}
