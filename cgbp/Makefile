
# cgbp makefile

CFLAGS = -D_DEFAULT_SOURCE $(DEV_CFLAGS)
LDFLAGS = $(DEV_LDFLAGS)
LDLIBS = -lrt -ldl

OBJECTS = cgbp.o langtonsant.o

all: langtonsant

langtonsant: $(OBJECTS)
	$(LINK)

fbdev.o: fbdev.c
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c -o $@ $<

fbdev.so: fbdev.o
	$(LINK_SHARED)

xlib.o: xlib.c
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c -o $@ $<

xlib.so: xlib.o
	$(LINK_SHARED) -lX11

$(OBJECTS): $(.TARGET:C/\.o/.c/)

clean:
	rm $(OBJECTS) langtonsant fbdev.o fbdev.so xlib.o xlib.so || true

include ../global.mk

.PHONY: all clean
