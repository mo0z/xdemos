
// cgbp.h

#ifndef CGBP_H
#define CGBP_H

#include <stdint.h>
#include <time.h>

struct cgbp;

struct cgbp_size {
	size_t w, h;
};

struct cgbp_callbacks {
	int (*update)(struct cgbp*, void*);
	int (*action)(struct cgbp*, void*, char);
};

extern struct cgbp_driver {
	void *(*init)(void);
	int (*update)(struct cgbp*, void*, struct cgbp_callbacks);
	void (*cleanup)(void*);
	uint32_t (*get_pixel)(void*, size_t, size_t);
	void (*set_pixel)(void*, size_t, size_t, uint32_t);
	struct cgbp_size (*size)(void*);
} driver;

struct cgbp {
	struct timespec start_time, total_frametime;
	void *driver_data;
	timer_t timerid;
	size_t num_frames;
	uint8_t running: 1, timer_set: 1;
};

int cgbp_init(struct cgbp *c);
int cgbp_main(struct cgbp *c, void *data, struct cgbp_callbacks cb);
void cgbp_cleanup(struct cgbp *c);

#endif // CGBP_H
