#Makefile made by michele 25/5/2017

# sources
SRCS= calcola.c function.c
OBJS=$(SRCS:.c=.o)
EXECUTABLE=calcola.x

# config
CC=gcc
CFLAGS= -c
LD=gcc

# target
all: $(EXECUTABLE)
	@echo Building $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	@echo Building $@
	@ $(LD) -o $@ $^

function.o: function.c function.h
	@echo Building $@
	@ $(CC) $(CFLAGS) -o $@ $< -lpthread

calcola.o: calcola.c function.h
	@echo Building $@
	@ $(CC) $(CFLAGS) -o $@ $< -lpthread

clean:
	rm -f $(EXECUTABLE)
	rm -f $(OBJS)

.phony: all clean
