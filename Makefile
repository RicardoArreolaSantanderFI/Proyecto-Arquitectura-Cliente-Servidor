CC     = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -Isrc

SERVIDOR_SRC = src/servidor.c src/sql.c src/tabla.c src/util.c
CLIENTE_SRC  = src/cliente.c src/util.c

all: bin/servidor bin/cliente

bin:
	mkdir -p bin

bin/servidor: $(SERVIDOR_SRC) src/protocolo.h src/sql.h src/tabla.h src/util.h | bin
	$(CC) $(CFLAGS) -o $@ $(SERVIDOR_SRC)

bin/cliente: $(CLIENTE_SRC) src/protocolo.h src/util.h | bin
	$(CC) $(CFLAGS) -o $@ $(CLIENTE_SRC)

clean:
	rm -rf bin

.PHONY: all clean
