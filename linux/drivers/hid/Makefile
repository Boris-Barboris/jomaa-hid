KERNEL_VERSION	:= $(shell uname -r)
KERNEL_MODULES	:= /lib/modules/$(KERNEL_VERSION)/build

hid-y := hid-jomaa.o

obj-m += hid-jomaa.o

all:
	$(MAKE) -C $(KERNEL_MODULES) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_MODULES) M=$(PWD) clean
