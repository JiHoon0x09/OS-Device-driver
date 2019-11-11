###############################################
#
# NAME: Makefile - Linux test program Makefile
# Brief history
# -------------
#             
#	           
# 2009,02,04  Jurngyu : Created
#
####################################################

CC=arm-generic-linux-gnu-gcc

TARGET		= touch_app

INCLUDES	= -I. 
SUBLIBS		= 
CFLAGS		= -Wall
LDFLAGS		= 

SRCS    = mep2440_key_touch_app.c
OBJS	= $(SRCS:.c=.o)

#CFLAGS      += -D__MAIN__

.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(SUBLIBS)
#	cp -a ./$(TARGET) /work/rootfs_nfs/root
	cp -a ./$(TARGET) /opt/4.2.4-arm/arm-generic-linux-gnu/sys-root/root

all:	$(TARGET)

clean:
	rm -f $(TARGET) *.o *.a *~

distclean:
	rm -f $(TARGET) *.o *.a *.bak *~ .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend


#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
