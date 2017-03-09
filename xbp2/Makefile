
CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += $(PROD_LDFLAGS)
LDLIBS += -lX11 -lbulk77i -lm

all: parallax metaballs

parallax: parallax.o demo.o xbp.o

metaballs: metaballs.o demo.o xbp.o

clean: parallax parallax.o demo.o xbp.o metaballs metaballs.o
	$(RM) $> $^

include ../global.mk

.PHONY: all clean
