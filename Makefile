
OBJ_NAME := it7260_ts

ifneq ($(KERNELRELEASE),)
obj-m := $(OBJ_NAME).o
else
KDIR := /home/cjok/work/s5pc110-kernel/s5pc110-kernel

.PHONY: all clean install

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=/usr/local/arm-2009q3/bin/arm-none-linux-gnueabi-
clean:
	rm -fr *.ko *.o *.mod.o *.mod.c *.symvers *.order
endif
