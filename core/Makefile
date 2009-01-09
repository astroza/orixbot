CC      = gcc
INCLUDE = -I../include/ -I/usr/include/libxml2
CFLAGS  = -Wall -g -DENABLE_DEBUG
LIBS    = -lxml2 -lz -lm -lsqlite3 /usr/lib/mplx2.so /usr/lib/ocorelib.so

all: orix_bot

orix_bot: modules.o parse.o bot.o net.o server.o signal.o irc.o user.o md5.o main.o util.o log.o config.o built-in.o
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) modules.o parse.o bot.o net.o server.o signal.o irc.o user.o md5.o main.o util.o log.o config.o built-in.o -o main $(LIBS) -rdynamic

bot.o: bot.c
	$(CC) -c $(CFLAGS) $(INCLUDE) bot.c

parse.o: parse.c
	$(CC) -c $(CFLAGS) $(INCLUDE) parse.c

modules.o: modules.c
	$(CC) -c $(CFLAGS) $(INCLUDE) modules.c

net.o: net.c
	$(CC) -c $(CFLAGS) $(INCLUDE) net.c

config.o: config.c
	$(CC) -c $(CFLAGS) $(INCLUDE)  config.c 

server.o: server.c
	$(CC) -c $(CFLAGS) $(INCLUDE) server.c

clean:
	rm -f *~ *.o main

signal.o: signal.c
	$(CC) -c $(CFLAGS) $(INCLUDE) signal.c

user.o: user.c
	$(CC) -c $(CFLAGS) $(INCLUDE) user.c

md5.o: md5.c
	$(CC) -c $(CFLAGS) -I../include/orix/ md5.c

util.o: util.c
	$(CC) -c $(CFLAGS) $(INCLUDE) util.c 

log.o: log.c
	$(CC) -c $(CFLAGS) $(INCLUDE) log.c

main.o: main.c
	$(CC) -c $(CFLAGS) $(INCLUDE) main.c

irc.o: irc.c
	$(CC) -c $(CFLAGS) $(INCLUDE) irc.c

built-in.o: built-in.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $?
