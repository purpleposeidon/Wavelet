

CC = gcc
FLAGS = -std=gnu99 -Wall
LIBS = -lSDL -lSDL_gfx -lreadline


all: wavelet

wavelet: wavelet.c
	$(CC) $(FLAGS) $(LIBS) -o $@ wavelet.c

wavelet.c: .PHONY

.PHONY:

clean:
	rm -f *.o *.out wavelet

nuke: clean

c: clean
n: nuke
a: all
ca: c a
na: n a


