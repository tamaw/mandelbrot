# Makefile for mandelbrot
# Date: Tue Apr 16 18:01:59 EST 2013
# Tama Waddell <tama.waddell@griffithuni.edu.au>

PROJECT	= mandelbrot
CC		= gcc
FLAGS	= -std=c99 -Wall -Iinclude -Iinclude/SDL -I/usr/include/openmpi-x86_64 -D_GNU_SOURCE=1 -D_REENTRANT -pthread -m64
LDFLAGS	= -Llib -L./lib/SDL -L/usr/lib64/openmpi/lib -Wl,-rpath,/tmp/gcc/lib 
DEPS	= -lgmp -lSDL -lmpi -ldl -lpthread 

all: build

build: main.o display.o
	$(CC) main.o display.o $(LDFLAGS) $(DEPS) -o bin/$(PROJECT)

main.o: 
	$(CC) -c $(FLAGS) main.c

display.o:
	$(CC) -c $(FLAGS) display.c

clean:
	rm *.o bin/$(PROJECT)


