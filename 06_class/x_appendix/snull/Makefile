obj-m += snull.o

KDIR	:= /work/REBIS_BSP/linux-2.6.17.13-rebis
#KDIR	:= /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean
