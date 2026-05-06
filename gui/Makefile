CC = gcc
CFLAGS = -Wall -Wextra -std=c99
PKG_CONFIG := /opt/homebrew/bin/pkg-config
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0)

all: odd_even_gui

odd_even_gui: odd_even_gui.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o odd_even_gui odd_even_gui.c $(GTK_LIBS)

clean:
	rm -f odd_even_gui

run: odd_even_gui
	./odd_even_gui

.PHONY: all clean run
