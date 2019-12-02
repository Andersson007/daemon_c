CC=gcc
OPTIMIZATION=-O3
DEBUG=-g
STANDARD=-std=c11
WARN_LEVEL=-Wall
CFLAGS=-I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -c $(WARN_LEVEL) $(DEBUG) $(STANDARD) $(OPTIMIZATION)
LDLIBS=-L/usr/lib64/glib-2.0 -lgthread-2.0 -lglib-2.0 -pthread -lconfig
LDFLAGS=

VPATH=
SOURCES=$(wildcard *.c)
HEADERS=headers/$(wildcard *.h)
OBJECTS=$(patsubst %.c,%.o, $(SOURCES))
EXECUTABLE=main

CLEAR=rm -rf

.c.o:
	$(CC) $(CFLAGS) $< -o $@

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDLIBS) $(OBJECTS) -o $@

$(OBJECTS): $(HEADERS)

.PHONY: clean scp

clean:
	$(CLEAR) $(OBJECTS) $(EXECUTABLE)
