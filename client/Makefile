include ../Makefile.inc

TARGET = client

SOURCES = client.c

OBJS = $(SOURCES:.c=.o)

$(TARGET):$(OBJS)
	gcc -o $@ $< -L../lib -lsmooth

$(OBJS):%.o:%.c
	gcc -c -o $@ $^ -I../include 

.PHONY:clean
clean:
	rm -rf $(TARGET) $(OBJS)
