
# fbdev makefile

CFLAGS += -D_DEFAULT_SOURCE $(DEV_CFLAGS)
LDFLAGS += $(DEV_LDFLAGS)
LDLIBS += -lrt

all: langtonsant

langtonsant: langtonsant.o
	$(LINK)

clean:
	rm langtonsant langtonsant.o || true

include ../global.mk

.PHONY: all clean
