
DIRS = xbp1 xbp2 third_party

all: $(DIRS)

$(DIRS):
	$(MAKE) $(MAKEFLAGS) -C $@/

clean:
	for dir in $(DIRS); do \
		$(MAKE) $(MAKEFLAGS) $@ -C "$$dir"; \
	done

.PHONY: $(DIRS) clean
