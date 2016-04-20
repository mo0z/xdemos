
CFLAGS += -D_DEFAULT_SOURCE
LDLIBS += -lX11

all: xrootgen

xrootgen1: xrootgen1.o
xrootgen: xrootgen.o animations.o

include global.mk

.PHONY: all
