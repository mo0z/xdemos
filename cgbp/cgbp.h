
// cgbp.h

#ifndef CGBP_H
#define CGBP_H

#include <stdint.h>
#include <time.h>

struct cgbp;

typedef int cgbp_updatecb(struct cgbp*, void*);

struct cgbp_size {
	size_t w, h;
};

extern struct cgbp_driver {
	void *(*init)(void);
	int (*update)(struct cgbp*, void*, cgbp_updatecb*);
	void (*cleanup)(void*);
	uint32_t (*get_pixel)(void*, size_t, size_t);
	void (*set_pixel)(void*, size_t, size_t, uint32_t);
	struct cgbp_size (*size)(void*);
} driver;

struct cgbp {
	void *driver_data;
	timer_t timerid;
	uint8_t running: 1, timer_set: 1;
};

int cgbp_init(struct cgbp *c);
int cgbp_main(struct cgbp *c, void *data, cgbp_updatecb *update);
void cgbp_cleanup(struct cgbp *c);

#endif // CGBP_H
