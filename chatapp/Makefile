CC = gcc
CFLAGS = -Wall -Wextra -std=c99
PKG_CONFIG := /opt/homebrew/bin/pkg-config
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0)

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o client client.c $(GTK_LIBS) -pthread

clean:
	rm -f server client

run_server: server
	./server

run_client: client
	./client

.PHONY: all clean run_server run_client
