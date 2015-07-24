CC = gcc
CFLAGS = -c
DFLAGS = -c -g -DDEBUG
SOURCES = src/*.c sql/*.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = aCBudget

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

debug: CFLAGS = $(DFLAGS)
debug: all

.PHONY: clean
clean:
	rm -rf *o aCBudget
