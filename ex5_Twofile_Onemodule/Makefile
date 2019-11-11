obj-m += startstop.o
startstop-objs := start.o stop.o

#KDIR:=/lib/modules/$(shell uname -r)/build
KDIR:=/work/kernel-mds2450-3.0.22

all:
	make -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean

