obj-m := vhdd.o vdisk.o

KDIR  := /work/linux/linux-2.6.21
#KDIR	:= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean