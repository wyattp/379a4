CC=gcc
CFLAGS=-Wall -g

a4vmsim: a4vmsim.c
	$(CC) $(CFLAGS) a4vmsim.c -g -o a4vmsim

mrefgen: mrefgen.c
	$(CC) $(CFLAGS) mrefgen.c -o mrefgen -lm

tar:

backup:

clean:
