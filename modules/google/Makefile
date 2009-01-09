CC=gcc
CFLAGS=-fPIC -Wall -pedantic -g
INCLUDE=-I../../include

all: google.o
	$(CC) -shared google.o -o google.so
google.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c google.c
clean:
	rm -f google.so *.o
