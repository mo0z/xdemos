
# cgbp makefile

# use bmake instead.

CFLAGS = -D_DEFAULT_SOURCE $(DEV_CFLAGS)
LDFLAGS = $(DEV_LDFLAGS)
LDLIBS = -lrt
LDLIBS_xlib = -lX11

DRIVERS = fbdev xlib
TARGETS = langtonsant

RM_FILES = cgbp.o

all: $(DRIVERS:C/\</langtonsant_/)
cgbp.o: cgbp.c

# build drivers
.for drv in $(DRIVERS)
$(drv): $(TARGETS:C/$/_$(drv)/)
drv_obj_$(drv) = $(drv:C/$/.o/)

$(drv_obj_$(drv)): $(drv:C/$/.c/) cgbp.h
	$(CC) $(CFLAGS) $(SHARED_CFLAGS) -c $<

RM_FILES += $(drv_obj_$(drv))
.endfor # drv in $(DRIVERS)

# build targets
.for target in $(TARGETS)

$(target:C/$/.o/): $(target:C/$/.c/) cgbp.h
RM_FILES += $(target:C/$/.o/)

# combine targets with drivers
.for drv in $(DRIVERS)
target_$(drv) = $(target:C/$/_$(drv)/)
$(target_$(drv)): cgbp.o $(target:C/$/.o/) $(drv:C/$/.o/)
	$(LINK) $(LDLIBS_$(drv))
RM_FILES += $(target_$(drv))
.endfor # drv in $(DRIVERS)

.endfor # target in $(TARGETS)

clean:
	rm $(RM_FILES) || true

include ../global.mk

.PHONY: all clean $(DRIVERS)
