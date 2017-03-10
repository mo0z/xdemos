
CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += $(PROD_LDFLAGS)
LDLIBS += -lX11 -lbulk77i -lm

all: parallax metaballs

parallax: parallax.o demo.o xbp.o

metaballs: metaballs.o demo.o xbp.o

clean:
	for file in parallax parallax.o demo.o xbp.o metaballs metaballs.o; do \
		[[ -e "$$file" ]] && rm "$$file"; \
	done || true

include ../global.mk

.PHONY: all clean
