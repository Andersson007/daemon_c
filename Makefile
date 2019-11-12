CC=gcc
OPTIMIZATION=-O3
DEBUG=
STANDARD=-std=c11
WARN_LEVEL=-Wall
CFLAGS=-c $(WARN_LEVEL) $(DEBUG) $(STANDARD) $(OPTIMIZATION)
LDLIBS=
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
