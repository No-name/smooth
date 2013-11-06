include ../Makefile.inc

TARGET = libsmooth.a
SOURCES = smooth_lib.c smooth_msg.c

OBJS = $(SOURCES:.c=.o)

$(TARGET):$(OBJS)
	ar cvr $@ $?

#smooth_lib.o:smooth_lib.c

$(OBJS):%.o:%.c
	gcc -c -o $@ $^ -I../include

.PHONY:clean
clean:
	-rm -rf $(TARGET) $(OBJS)
