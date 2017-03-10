
CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += $(PROD_LDFLAGS)
LDLIBS += -lX11 -lbulk77i

all: xrootgen life2d life1d

xrootgen: xrootgen.o animations.o
life2d: life2d.o time_stat.o xbp.o
life1d: life1d.o time_stat.o xbp.o

clean:
	for file in xrootgen xrootgen.o animations.o life1d life1d.o life2d \
			life2d.o xbp.o; do \
		[[ -e "$$file" ]] && rm "$$file"; \
	done || true

include ../global.mk

.PHONY: all clean
