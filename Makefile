ifneq ($(KERNELRELEASE),)
	obj-m += ultrason.o


	EXTRA_CFLAGS := -I /usr/xenomai/include/
else

	XENOCONFIG=/usr/xenomai/bin/xeno-config
	CC=$(shell      $(XENOCONFIG) --cc)
	CFLAGS=$(shell  $(XENOCONFIG) --skin=posix --cflags)
	LDFLAGS=$(shell $(XENOCONFIG) --skin=posix --ldflags)
	LIBDIR=$(shell  $(XENOCONFIG) --skin=posix --libdir)

	CROSS_COMPILE ?=
	KERNEL_DIR ?= /usr/src/linux
	MODULE_DIR := $(shell pwd)

.PHONY: all
all:: modules executable

.PHONY: modules
modules:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(MODULE_DIR) CROSS_COMPILE=$(CROSS_COMPILE) modules

.PHONY: executable
executable: demo_dev_linux

XENOCONFIG=/usr/xenomai/bin/xeno-config

demo_dev_linux: demo_dev_linux.c
	$(CC) -c -o demo_dev_linux.o demo_dev_linux.c $(CFLAGS) $(LDFLAGS)
	/usr/xenomai/bin/wrap-link.sh -v $(CC) -o demo_dev_linux demo_dev_linux.o $(LDFLAGS)


.PHONY: clean
clean::
	rm -f  *.o  .*.o  .*.o.* *.ko  .*.ko  *.mod.* .*.mod.* .*.cmd *~
	rm -f Module.symvers Module.markers modules.order
	rm -rf .tmp_versions
	rm -f rtdm-user-read rtdm-user-write
endif
