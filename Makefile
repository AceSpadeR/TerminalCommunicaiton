# Makefile for homework MMIO #
#

all:  c p

p: p.c messages.h
	gcc -o p p.c -lrt

c: c.c messages.h
	gcc -o c c.c -lrt

clean:
	rm c p
