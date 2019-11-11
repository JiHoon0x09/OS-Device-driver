obj-m := blocking.o

#KDIR	:= /lib/modules/$(shell uname -r)/build
KDIR  := /work/linux/linux-2.6.21

all:
	make -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean

