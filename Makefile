obj-m += touch_remap.o

# KERNEL_SRC should be provided by the GitHub Action environment.
# Default to current running kernel headers if not provided.
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
