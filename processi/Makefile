#Makefile made by michele 15/5/2017

# sources
SRCS= calcola.c worker.c function.c
OBJS=$(SRCS:.c=.o)
EXECUTABLE=calcola.x worker.x

# config
CC=gcc
CFLAGS= -c -std=gnu99
LD=gcc

# target
all: $(EXECUTABLE)
	@echo Building $(EXECUTABLE)

calcola.x: calcola.o function.o
	$(LD) -o $@ $^

worker.x: worker.o function.o
	$(LD) -o $@ $^

calcola.o: calcola.c function.h
	$(CC) $(CFLAGS) -o $@ $<

worker.o: worker.c function.h
	$(CC) $(CFLAGS) -o $@ $<

function.o: function.c
	$(CC) $(CFLAGS) -o $@ $<

doc: $(OBJS)
	@echo Generating doxygen
	@doxygen

clean:
	rm -f $(EXECUTABLE)
	rm -f $(OBJS)
	rm -rf html
.phony: all clean
