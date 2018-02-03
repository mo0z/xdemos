
# xbp3 makefile

CFLAGS += -D_DEFAULT_SOURCE $(PROD_CFLAGS)
LDFLAGS += $(PROD_LDFLAGS)
LDLIBS += -lX11 -lm -lrt

TARGETS_BIN = draw metaballs xor voronoi
TARGETS_OBJ = $(TARGETS_BIN:C/$/.o/)

HACKS_BIN = hacks
HACKS_OBJ = hacks.o hacks_collection.o

XBP_OBJ = xbp.o

all: $(TARGETS_BIN) $(HACKS_BIN)

$(TARGETS_BIN): $(.TARGET:C/$/.o/) $(XBP_OBJ)
	$(LINK)

$(HACKS_BIN): $(HACKS_OBJ) $(XBP_OBJ)
	$(LINK)

include ../global.mk

TARGETS_ALL = $(TARGETS_BIN) $(TARGETS_OBJ) $(HACKS_BIN) $(HACKS_OBJ) $(XBP_OBJ)

clean:
	for file in $(TARGETS_ALL); do \
		[[ -e "$$file" ]] && rm "$$file"; \
	done || true

.PHONY: all clean
