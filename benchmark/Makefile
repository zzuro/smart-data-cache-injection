# Minimal Makefile to compile um

CC = gcc
CFLAGS = -march=native
LDFLAGS = ../go-pci-lib/libgopci.so

all: um

um: um.c
	$(CC) $(CFLAGS) um.c -o um $(LDFLAGS)

clean:
	rm -f um

.PHONY: all clean
