
// langtonsant.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cgbp.h"

struct langtonsant {
	size_t x, y, bytesize;
	char direction;
};

int nonneg_mod(int num, int div) {
	num = num % div;
	if(num < 0)
		num += div;
	return num;
}

int langtonsant_setup(struct langtonsant *l, struct cgbp_size size) {
	l->x = size.w / 2;
	l->y = size.h / 2;
	l->direction = 0;
	return 0;
}

void langtonsant_step(struct cgbp *c, struct langtonsant *l) {
	struct cgbp_size size = c->driver->size(c->driver_data);
	if(c->driver->get_pixel(c->driver_data, l->x, l->y) == 0) {
		l->direction++;
		c->driver->set_pixel(c->driver_data, l->x, l->y, 0xffffff);
	} else {
		l->direction--;
		c->driver->set_pixel(c->driver_data, l->x, l->y, 0);
	}
	switch(nonneg_mod(l->direction, 4)) {
	case 0:
		if(l->y == 0)
			l->y = size.h - 1;
		else
			l->y--;
		break;
	case 1:
		if(l->x == size.w - 1)
			l->x = 0;
		else
			l->x++;
		break;
	case 2:
		if(l->y == size.h - 1)
			l->y = 0;
		else
			l->y++;
		break;
	case 3:
		if(l->x == 0)
			l->x = size.w - 1;
		else
			l->x--;
		break;
	}
	return;
}

int langtonsant_update(struct cgbp *c, void *data) {
	size_t i;
	char x;
	for(i = 0; i < 100000; i++)
		langtonsant_step(c, data);
	if(read(STDIN_FILENO, &x, 1) > 0 && (x == 'q' || x == 'Q'))
		c->running = 0;
	return 0;
}

int main(void) {
	struct cgbp c;
	struct langtonsant l;
	int ret = EXIT_FAILURE;
	char *driver = getenv("CGBP_DRIVER");
	if(driver == NULL)
		driver = "fbdev";

	if(cgbp_init(&c, driver) < 0)
		goto error;

	langtonsant_setup(&l, c.driver->size(c.driver_data));
	if(cgbp_main(&c, &l, langtonsant_update) == 0)
		ret = EXIT_SUCCESS;
error:
	cgbp_cleanup(&c);
	return ret;
}
