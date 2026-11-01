CC     = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -Isrc

all: bin/servidor

bin:
	mkdir -p bin

bin/servidor: src/servidor.c src/util.c src/protocolo.h src/util.h | bin
	$(CC) $(CFLAGS) -o $@ src/servidor.c src/util.c

clean:
	rm -rf bin

.PHONY: all clean
