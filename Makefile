CC=gcc
CFLAGS= -std=gnu99 -Wall
LDLIBS = -lpthread -lm

.PHONY: clean all

all: mole.c index.c update_struct.h globals.c file.h priority_queue.h priority_queue.c
	${CC} -o mole ${CFLAGS} ${LDLIBS} mole.c priority_queue.c globals.c index.c stack.c

debug:
	${CC} -o mole_debug ${CFLAGS} -ggdb -O0 ${LDLIBS} mole.c priority_queue.c globals.c index.c stack.c

# use valgrind only without the perioding indexing: no -t n
grind: debug
	valgrind --tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all ./mole_debug -d .

clean:
	-rm mole mole_debug
