CC=gcc
CFLAGS=-c
DFLAGS=-c -DDEBUG

DEP=src/*.h
SRC=src/*.c
#OBJ=$(SRC:src/*.c=obj/*.o)
OBJ=obj/*.o

LIBS=-pthread -ldl

all: obj
	$(CC) $(OBJ) $(LIBS) -o aCBudget

obj: $(SRC) $(DEPS)
	$(CC) $(SRC) $(CFLAGS)
	mkdir -p obj
	mv -f *.o obj/

sql: SRC=src/*.c sql/sqlite3.c
sql: all

debug: CFLAGS = $(DFLAGS)
debug: all

redo:
	rm aCBudget obj/a*
redo: all

clean:
	rm -r obj aCBudget