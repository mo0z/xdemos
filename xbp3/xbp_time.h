
// xbp_time.h

#ifndef XBP_TIME_H
#define XBP_TIME_H

#include <time.h>
#include <stdio.h>

#define XBP_BILLION (1000 * 1000 * 1000)

struct xbp_time {
	struct timespec start_time, total_frametime, frame_start;
	size_t num_frames;
};

static inline struct timespec timespec_add(const struct timespec ts1,
                                           const struct timespec ts2) {
	register struct timespec result = ts1;
	register long sec;

	result.tv_nsec += ts2.tv_nsec;
	result.tv_sec += ts2.tv_sec;

	if(result.tv_nsec >= XBP_BILLION) {
		result.tv_sec += result.tv_nsec / XBP_BILLION;
		result.tv_nsec %= XBP_BILLION;
	}
	if(result.tv_nsec < 0) {
		sec = 1 + -(result.tv_nsec / XBP_BILLION);
		result.tv_nsec += XBP_BILLION * sec;
		result.tv_sec -= sec;
	}
	return result;
}

#define timespec_diff(ts1, ts2) \
	(timespec_add(ts1, (struct timespec){ -(ts2).tv_sec, -(ts2).tv_nsec }))

#define timespec_double(ts) \
	((double)(ts).tv_sec + (double)(ts).tv_nsec / XBP_BILLION)

#define XBP_TIME_INITIAL ((struct xbp_time){ { 0, 0 }, { 0, 0 }, 0 })

static inline int xbp_time_init(struct xbp_time *xt) {
	if(clock_gettime(CLOCK_MONOTONIC, &xt->start_time) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	xt->total_frametime = (struct timespec){ 0, 0 };
	xt->num_frames = 0;
	return 0;
}

static inline int xbp_time_frame_start(struct xbp_time *xt) {
	if(clock_gettime(CLOCK_MONOTONIC, &xt->frame_start) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	return 0;
}

static inline int xbp_time_frame_end(struct xbp_time *xt) {
	struct timespec frame_end;
	if(clock_gettime(CLOCK_MONOTONIC, &frame_end) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	xt->total_frametime = timespec_add(
		xt->total_frametime,
		timespec_diff(frame_end, xt->frame_start)
	);
	xt->num_frames++;
	return 0;
}

static inline int xbp_time_print_stats(struct xbp_time *xt) {
	struct timespec ts;
	double runtime;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
		XBP_ERRPRINT("Error: clock_gettime");
		return -1;
	}
	runtime = timespec_double(timespec_diff(ts, xt->start_time));
	fprintf(stderr, "total runtime: %.2f\n", runtime);
	fprintf(stderr, "num frames: %zu\n", xt->num_frames);
	fprintf(stderr, "FPS: %.2f\n", xt->num_frames / runtime);
	fprintf(stderr, "time spent for calulation in total: %f\n",
	        timespec_double(xt->total_frametime));
	return 0;
}

#endif // XBP_TIME_H
