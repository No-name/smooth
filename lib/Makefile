include ../Makefile.inc

libsmooth.a:smooth_lib.o
	ar cvr $@ $<

smooth_lib.o:smooth_lib.c
	gcc -c -o $@ $< -I../include
