
// cgbp.c

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cgbp.h"

#define CGBP_BACKEND_PATH_LEN (sizeof CGBP_BACKEND_PATH - 1)
#define CGBP_FPS 30

int cgbp_init(struct cgbp *c) {
	c->driver_data = NULL;
	c->timer_set = 0;
	c->running = 1;
	if(driver.init != NULL)
		c->driver_data = driver.init();
	if(c->driver_data == NULL) {
		cgbp_cleanup(c);
		return -1;
	}

	if(timer_create(CLOCK_MONOTONIC, NULL, &c->timerid) < 0) {
		perror("timer_create");
		cgbp_cleanup(c);
		return -1;
	}
	c->timer_set = 1;
	return 0;
}

static char cgbp_ticked = 0;
void cgbp_alarm(int sig) {
	if(sig == SIGALRM)
		cgbp_ticked = 1;
}

int cgbp_main(struct cgbp *c, void *data, cgbp_updatecb *cb) {
	struct sigaction sa = {
		.sa_flags = 0,
		.sa_handler = cgbp_alarm,
	};
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);
	timer_settime(c->timerid, 0, &(struct itimerspec){
		.it_value = { 0, 1e9 / CGBP_FPS },
		.it_interval = { 0, 1e9 / CGBP_FPS },
	}, NULL);

	do {
		if(cgbp_ticked == 0)
			sleep(1);
		cgbp_ticked = 0;
		if(driver.update != NULL) {
			if(driver.update(c, data, cb) < 0)
				return -1;
		} else if(cb(c, data) < 0)
			return -1;
	} while(c->running);

	timer_settime(c->timerid, 0, &(struct itimerspec){
		.it_value = { 0, 0 },
		.it_interval = { 0, 0 },
	}, NULL);
	return 0;
}

void cgbp_cleanup(struct cgbp *c) {
	if(c->timer_set)
		timer_delete(c->timerid);
	if(c->driver_data != NULL)
		driver.cleanup(c->driver_data);
}
