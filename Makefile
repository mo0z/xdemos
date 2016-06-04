
CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += $(PROD_LDFLAGS)
LDLIBS += -lX11 -lbulk77i

all: xrootgen maze

xrootgen1: xrootgen1.o
xrootgen: xrootgen.o animations.o
maze: maze.o xbp.o

clean: xrootgen1 xrootgen1.o xrootgen xrootgen.o animations.o
	$(RM) $> $^

include global.mk

.PHONY: all clean
