CROSS_COMPILE ?= arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc

CFLAGS := -O0 -g3 -Wall -Wextra

all: hello_dbg

hello_dbg: main.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f hello_dbg

.PHONY: all clean
