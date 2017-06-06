#Makefile made by michele 25/5/2017

# sources
SRCS= calcola.c function.c
OBJS=$(SRCS:.c=.o)
EXECUTABLE=calcola.x

# config
CC=gcc
CFLAGS= -c -w -std=c99
LD=gcc
LDFLAGS= -Llib

# target
all: $(EXECUTABLE)
	@echo Building $(EXECUTABLE)
	@echo Generating doxygen
	@doxygen

$(EXECUTABLE): $(OBJS)
	@echo Building $@
	@ $(LD) $(LDFLAGS) -o $@ $^ -lpthread

function.o: function.c function.h
	@echo Building $@
	@ $(CC) $(CFLAGS) -o $@ $<

calcola.o: calcola.c function.h
	@echo Building $@
	@ $(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(EXECUTABLE)
	rm -f $(OBJS)
	rm -rf html 

.phony: all clean
