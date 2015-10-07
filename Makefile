CC=gcc
CFLAGS=-c
DFLAGS=-c -g -DDEBUG

DEP=src/*.h
SRC=src/*.c
SQLSRC=src/*.c sql/sqlite3.c
OBJ=obj/*.o
SQLOBJ=$(OBJ:sql/%.c=obj/%.o)

LIBS=-pthread -ldl

all: obj
	$(CC) $(OBJ) $(LIBS) -o aCBudget

obj: $(SRC) $(DEP)
	$(CC) $(SRC) $(CFLAGS)
	mkdir -p obj
	mv -f *.o obj/

dsql: SRC=$(SQLSRC)
dsql: debug

sql: SRC=$(SQLSRC)
sql: all

debug: CFLAGS = $(DFLAGS)
debug: all

remove:
	rm aCBudget obj/a*
redo: remove all

clean:
	rm -r obj
	rm aCBudget
