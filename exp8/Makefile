CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: time_server time_client

time_server: server.c
	$(CC) $(CFLAGS) -o time_server server.c

time_client: client.c
	$(CC) $(CFLAGS) -o time_client client.c

clean:
	rm -f time_server time_client

run_server: time_server
	./time_server

run_client: time_client
	./time_client

.PHONY: all clean run_server run_client
