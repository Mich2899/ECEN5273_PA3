CC=gcc
CFLAGS =

all:	proxy

proxy:	httpechosrv.c
	$(CC) $(CFLAGS) -o proxy httpechosrv.c

clean:
	rm -rf proxy