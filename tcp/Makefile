CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client

run_server: server
	./server

run_client: client
	./client

.PHONY: all clean run_server run_client

.PHONY: all clean run_server run_client
