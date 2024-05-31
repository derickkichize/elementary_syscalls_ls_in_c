PROG 	= ls
CC 		= cc
CFLAGS  = -std=c99 -Wall -Wextra -Wvisibility -I${.CURDIR} -g
CFLAGS += -Wno-knr-promoted-parameter

SRCS = main.c 
OBJS = ${SRCS:S/.c/.o/g}

.SUFFIXES: .c.o

.c.o:
	${CC} ${CFLAGS} -c $<

${PROG}: ${OBJS}
	${CC} ${CFLAGS} -o $@ ${OBJS}

clean:
	rm -rf ${PROG} *.o *.core

.PHONY: clean
