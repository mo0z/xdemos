
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

int langtonsant_setup(struct langtonsant *l, size_t x, size_t y) {
	l->x = x / 2;
	l->y = y / 2;
	l->direction = 0;
	return 0;
}

void langtonsant_step(struct cgbp *c, struct langtonsant *l) {
	if(cgbp_get_pixel(c, l->x, l->y) == 0) {
		l->direction++;
		cgbp_set_pixel(c, l->x, l->y, 0xffffff);
	} else {
		l->direction--;
		cgbp_set_pixel(c, l->x, l->y, 0);
	}
	switch(nonneg_mod(l->direction, 4)) {
	case 0:
		if(l->y == 0)
			l->y = cgbp_height(c) - 1;
		else
			l->y--;
		break;
	case 1:
		if(l->x == cgbp_width(c) - 1)
			l->x = 0;
		else
			l->x++;
		break;
	case 2:
		if(l->y == cgbp_height(c) - 1)
			l->y = 0;
		else
			l->y++;
		break;
	case 3:
		if(l->x == 0)
			l->x = cgbp_width(c) - 1;
		else
			l->x--;
		break;
	}
	return;
}


int langtonsant_update(struct cgbp *c, void *data) {
	size_t i;
	char x;
	for(i = 0; i < 1000; i++)
		langtonsant_step(c, data);
	if(read(STDIN_FILENO, &x, 1) > 0 && (x == 'q' || x == 'Q'))
		c->running = 0;
	return 0;
}

int main(void) {
	struct cgbp c = EMPTY_CGBP;
	struct langtonsant l;
	int ret = EXIT_FAILURE;

	if(cgbp_init(&c) < 0)
		goto error;

	langtonsant_setup(&l, cgbp_width(&c), cgbp_height(&c));
	if(cgbp_main(&c, &l, langtonsant_update) == 0)
		ret = EXIT_SUCCESS;
error:
	cgbp_cleanup(&c);
	return ret;
}
