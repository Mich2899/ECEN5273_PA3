CC=gcc
CFLAGS =

all:	webserver

webserver:	httpechosrv.c
	$(CC) $(CFLAGS) -o webserver httpechosrv.c

clean:
	rm -rf webserver