smooth:smooth.c
	cc -o $@ $< `pkg-config --cflags --libs gtk+-3.0`
