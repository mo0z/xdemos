
CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += $(PROD_LDFLAGS)
LDLIBS += -lX11 -lbulk77i

all: xrootgen life2d

xrootgen: xrootgen.o animations.o
life2d: life2d.o xbp.o

clean: xrootgen xrootgen.o animations.o life2d life2d.o xbp.o
	$(RM) $> $^

include global.mk

.PHONY: all clean
