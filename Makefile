TARGET_MODULE:=ma120x0

ifneq ($(KERNELRELEASE),)
# kbuild part of makefile

obj-m += $(TARGET_MODULE).o

else
# normal makefile

KDIR ?= /lib/modules/$(shell uname -r)/build
# Allows building with sudo
# See https://sysprog21.github.io/lkmpg/#the-simplest-module
PWD := $(CURDIR)

default:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

load:
	insmod ./$(TARGET_MODULE).ko

unload:
	rmmod ./$(TARGET_MODULE).ko

dts: ma120x0-example-overlay.dts
	dtc -@ -I dts -O dtb -o /boot/overlays/ma120x0-example.dtbo ma120x0-example-overlay.dts

endif
