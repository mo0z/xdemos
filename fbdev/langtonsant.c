
// langtonsant.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "graphics.h"

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

void langtonsant_step(struct graphics *g, struct langtonsant *l) {
	if(graphics_get_pixel(g, l->x, l->y) == 0) {
		l->direction++;
		graphics_set_pixel(g, l->x, l->y, 0xffffff);
	} else {
		l->direction--;
		graphics_set_pixel(g, l->x, l->y, 0);
	}
	switch(nonneg_mod(l->direction, 4)) {
	case 0:
		if(l->y == 0)
			l->y = graphics_height(g) - 1;
		else
			l->y--;
		break;
	case 1:
		if(l->x == graphics_width(g) - 1)
			l->x = 0;
		else
			l->x++;
		break;
	case 2:
		if(l->y == graphics_height(g) - 1)
			l->y = 0;
		else
			l->y++;
		break;
	case 3:
		if(l->x == 0)
			l->x = graphics_width(g) - 1;
		else
			l->x--;
		break;
	}
}


int langtonsant_update(struct graphics *g, void *data) {
	size_t i;
	char c;
	for(i = 0; i < 1000; i++)
		langtonsant_step(g, data);
	if(read(STDIN_FILENO, &c, 1) > 0 && (c == 'q' || c == 'Q'))
		g->running = 0;
	return 0;
}

int main(void) {
	struct graphics g = EMPTY_GRAPHICS;
	struct langtonsant l;
	int ret = EXIT_FAILURE;

	if(graphics_init(&g) < 0)
		goto error;

	langtonsant_setup(&l, g.vinfo.xres, g.vinfo.yres);
	if(graphics_main(&g, &l, langtonsant_update) == 0)
		ret = EXIT_SUCCESS;
error:
	graphics_cleanup(&g);
	return ret;
}
