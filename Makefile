.PHONY:all
all: client server

client:lib
	make -C client

server:lib
	make -C server

.PHONY:lib
lib:
	make -C lib

.PHONY:clean
clean:
	make -C lib clean
	make -C client clean
	make -C server clean

smooth:smooth.c
	cc -o $@ $< `pkg-config --cflags --libs gtk+-3.0`

.PHONY:run

run:smooth
	./smooth
