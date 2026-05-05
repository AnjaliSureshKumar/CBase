CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: udp_server udp_client

udp_server: udp_server.c
	$(CC) $(CFLAGS) -o udp_server udp_server.c

udp_client: udp_client.c
	$(CC) $(CFLAGS) -o udp_client udp_client.c

clean:
	rm -f udp_server udp_client

run_udp_server: udp_server
	./udp_server

run_udp_client: udp_client
	./udp_client

.PHONY: all clean run_udp_server run_udp_client