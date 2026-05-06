CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: file_server file_client

file_server: server.c
	$(CC) $(CFLAGS) -o file_server server.c

file_client: client.c
	$(CC) $(CFLAGS) -o file_client client.c

clean:
	rm -f file_server file_client

run_server: file_server
	./file_server

run_client: file_client
	./file_client

.PHONY: all clean run_server run_client
