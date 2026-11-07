CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g
CPPFLAGS = -Iinclude
LDLIBS = -pthread -lrt

.PHONY: all test clean

all: analista controlador

analista: src/analista.c src/logger.c include/logger.h
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		src/analista.c src/logger.c \
		-o analista $(LDLIBS)

controlador: tests/test_controlador.c src/logger.c include/logger.h analista
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		tests/test_controlador.c src/logger.c \
		-o controlador $(LDLIBS)

test: controlador
	rm -f lng.txt lng.txt.tmp
	./controlador

clean:
	rm -f analista controlador lng.txt lng.txt.tmp