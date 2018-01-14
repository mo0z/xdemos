
# xbp3 makefile

CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += $(PROD_LDFLAGS)
LDLIBS += -lX11 -lm -lrt

all: xrootgen metaballs xor voronoi

xrootgen: xrootgen.o animations.o xbp.o
metaballs: metaballs.o xbp.o
xor: xor.o xbp.o
voronoi: voronoi.o xbp.o

include ../global.mk

clean:
	for file in xbp.o metaballs metaballs.o; do \
		[[ -e "$$file" ]] && rm "$$file"; \
	done || true

.PHONY: all clean
