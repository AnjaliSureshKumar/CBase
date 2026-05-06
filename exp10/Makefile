CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: raw_capture

raw_capture: raw_capture.c
	$(CC) $(CFLAGS) -o raw_capture raw_capture.c

clean:
	rm -f raw_capture

run: raw_capture
	sudo ./raw_capture

.PHONY: all clean run
