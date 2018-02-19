
# cgbp makefile

CFLAGS = -D_DEFAULT_SOURCE $(DEV_CFLAGS)
LDFLAGS = $(DEV_LDFLAGS)
LDLIBS = -lrt -ldl

OBJECTS = cgbp.o langtonsant.o
DRIVERS = fbdev xlib

all: $(DRIVERS:C/\</langtonsant_/)

.for drv in $(DRIVERS)
langtonsant_$(drv): $(OBJECTS) $(drv).o
	$(LINK) $(drv:C/xlib/-lX11/:C/fbdev//)

$(drv).o: $(drv).c
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c $<
.endfor

$(OBJECTS): $(.TARGET:C/\.o/.c/)

clean:
	rm $(OBJECTS) langtonsant_fbdev langtonsant_xlib \
		fbdev.o fbdev.so xlib.o xlib.so || true

include ../global.mk

.PHONY: all clean
