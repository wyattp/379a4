CC=gcc
CFLAGS=-Wall -g

all: a4vmsim mrefgen

a4vmsim: a4vmsim.c
	$(CC) $(CFLAGS) a4vmsim.c -g -o a4vmsim

mrefgen: mrefgen.c
	$(CC) $(CFLAGS) mrefgen.c -o mrefgen -lm

tar:
	tar -cf submit.tar Makefile a4vmsim.c mrefgen.c

backup:
	tar -cf .backup/backup.tar Makefile a4vmsim.c mrefgen.c

clean:
	rm -f a4vmsim
	rm -f mrefgen
