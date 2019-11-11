#CC=arm-generic-linux-gnueabi-gcc
CC=arm-generic-linux-gnu-gcc

TARGET		= 01oss

INCLUDES	= -I. 
SUBLIBS		= 
CFLAGS		= -Wall
LDFLAGS		= 

SRCS    = oss_cap.c
OBJS	= $(SRCS:.c=.o)

#CFLAGS      += -D__MAIN__

.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(SUBLIBS) -lm
#	cp -a $(TARGET) /opt/arm-generic/arm-generic-linux-gnueabi/sys-root/root
	cp -a $(TARGET) /opt/4.2.4-arm/arm-generic-linux-gnu/sys-root/root


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
