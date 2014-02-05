PROG	= kawaii-bar
CC	= gcc
PREFIX ?= /usr/bin

LIBS	= `pkg-config --libs --cflags gtk+-3.0 cairo pango` -lasound
CFLAGS	= -Os -pedantic -Wall

${PROG}: ${PROG}.c
	@${CC} ${CFLAGS} ${LIBS} -o ${PROG} ${PROG}.c

install:
	install ${PROG} ${PREFIX}/${PROG}

uninstall:
	rm -f ${PREFIX}/${PROG}

clean:
	rm -f ${PROG}
