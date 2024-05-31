CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic
.PHONY:clean

all: generator prog

generator: generator.c functions.c makefile
	$(CC) $(CFLAGS) generator.c functions.c -o generator

prog: main.c header.h makefile
	$(CC) $(CFLAGS) main.c header.h -o prog

clean:
	rm -f generator prog