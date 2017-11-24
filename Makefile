CC=gcc
CFLAGS=-Wall -g

a4vsim: a4vsim.c
	$(CC) $(CFLAGS) a4vsim.c -g -o a4vsim

mrefgen: mrefgen.c
	$(CC) $(CFLAGS) mrefgen.c -o mrefgen -lm

tar:

backup:

clean:
