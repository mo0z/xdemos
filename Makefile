
DIRS = xbp1 xbp2

all: $(DIRS)

$(DIRS):
	$(MAKE) $(MAKEFLAGS) -C $@/

clean:
	for dir in $(DIRS); do \
		$(MAKE) $(MAKEFLAGS) $@ -C "$$dir"; \
	done

.PHONY: $(DIRS) clean
