obj-m := keysample.o
keysample-objs	:= keyint_bottom_kscan_platform_input.o

EXTRA_CFLAGS += -I./include

#KDIR  := /work/linux/linux-2.6.21


default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	cp -ra keysample.ko /opt/4.2.4-arm/arm-generic-linux-gnu/sys-root/root/
#	cp -ra keysample.ko /work/rootfs_nfs/root
	
clean:
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
	rm -rf *.bak
	rm -rf Module.symvers
	


