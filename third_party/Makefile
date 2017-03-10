
CFLAGS += -D_DEFAULT_SOURCE
LDFLAGS += -lGL -lGLU -lX11

all: opengl betteros_dri betteros_x11 sofb

opengl: opengl.o

betteros_dri: betteros_dri.o
betteros_x11: betteros_x11.o
sofb: sofb.o

include ../global.mk

clean:
	for file in opengl opengl.o betteros_dri betteros_dri.o betteros_x11 \
			betteros_x11.o; do \
		[[ -e "$$file" ]] && rm "$$file"; \
	done || true

.PHONY: all clean
