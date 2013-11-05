smooth:smooth.c
	cc -o $@ $< `pkg-config --cflags --libs gtk+-3.0`

.PHONY:client
client:lib
	cd client && make client

.PHONY:server
server:lib
	cd server && make server

.PHONY:lib
lib:
	cd lib && make libsmooth.a

.PHONY:run

run:smooth
	./smooth
