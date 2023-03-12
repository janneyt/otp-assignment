CC=gcc
CFLAGS= -Wall -Wextra -Werror -Wpedantic -Wextra -g -std=c99

all: dec-client dec-server enc-server enc-client keygen

dec-client: dec-client.c
	$(CC) $(CFLAGS) -o dec-client dec-client.c one-time/one-time.c

dec-server: dec-server.c
	$(CC) $(CFLAGS) -o dec-server dec-server.c one-time/one-time.c

enc-server: enc-server.c
	$(CC) $(CFLAGS) -o enc-server enc-server.c one-time/one-time.c reading_funcs.c

enc-client: enc-client.c
	$(CC) $(CFLAGS) -o enc-client enc-client.c reading_funcs.c

keygen: keygen.c
	$(CC) $(CFLAGS) -o keygen keygen.c

clean:
	rm -f dec-client dec-server enc-server enc-client keygen *.o

