# 02 · Cliente TCP

**Rama:** `feature/cliente`
**Commit:** `feat: agregar cliente TCP`

## Qué se agregó

| Archivo | Para qué sirve |
|---|---|
| `src/cliente.c` | El cliente interactivo. |
| `Makefile` | Se agrega el objetivo `bin/cliente`. |

## Cómo funciona

### Secuencia del cliente

```
socket          crea el socket TCP
gethostbyname   obtiene la dirección a partir de una IP o un nombre
connect         se conecta al servidor
send / recv     envía comandos y recibe respuestas
close           cierra la conexión
```

El host y el puerto se reciben por línea de comandos:

```bash
./bin/cliente 192.168.100.102 4898
```

### Ciclo de conversación

El cliente escribe primero y espera la respuesta antes de pedir otro comando:

1. Muestra el indicador `SQL>` y lee una línea del teclado.
2. Le quita espacios sobrantes. Si queda vacía, vuelve a pedir.
3. La envía con un salto de línea al final.
4. Lee la respuesta con `recv` en un ciclo, imprimiendo lo que llega, hasta encontrar el carácter de fin.
5. Si el comando fue SALIR, termina después de recibir la despedida.

`Ctrl-D` se trata como SALIR para no dejar la conexión abierta. Si `recv` regresa 0, el servidor cerró y el cliente termina.

## Cómo probar

```bash
make
./bin/servidor 4898                   # terminal 1

./bin/cliente 127.0.0.1 4898          # terminal 2
SQL> SELECT * FROM Alumno
Comando recibido: SELECT * FROM Alumno
SQL> SALIR
Adios. Conexion cerrada por el servidor.
```
