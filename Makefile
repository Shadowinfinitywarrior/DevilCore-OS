# DevilCore Master Makefile

SUBDIRS = kernel bootloader shell gui apps tools privacy utils

all:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean 2>/dev/null || true; \
	done

.PHONY: all clean