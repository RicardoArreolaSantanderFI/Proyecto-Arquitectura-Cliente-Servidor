# 01 · Servidor TCP y estructura base

**Rama:** `feature/servidor`
**Commit:** `feat: agregar servidor TCP y estructura base del proyecto`

## Qué se agregó

| Archivo | Para qué sirve |
|---|---|
| `datos/alumno.txt` | La tabla Alumno. Primera línea cabecera, una línea por registro, campos separados por coma. |
| `src/protocolo.h` | Constantes que comparten cliente y servidor: puerto, tamaños de buffer, carácter de fin de respuesta y comando SALIR. |
| `src/util.h`, `src/util.c` | Funciones de apoyo: `recortar_cadena` y `quitar_comillas`. |
| `src/servidor.c` | El servidor. En este commit todavía no ejecuta SQL, solo confirma el comando recibido. |
| `Makefile` | Compila `bin/servidor`. |
| `.gitignore` | Ignora binarios, objetos y la documentación en Markdown. |

## Cómo funciona

### Reglas de comunicación

TCP entrega un flujo de bytes sin separar mensajes, así que cliente y servidor acuerdan dos reglas:

1. El cliente envía un comando por línea, terminado con salto de línea.
2. El servidor responde con texto de cualquier longitud y termina la respuesta con el carácter `0x04`, definido como `FIN_RESPUESTA` en `protocolo.h`.

### Secuencia del servidor

```
socket      crea el socket TCP
setsockopt  permite reutilizar el puerto al reiniciar
bind        asocia el socket al puerto en todas las interfaces
listen      lo pone a escuchar con una cola de 5 conexiones
accept      espera a un cliente y regresa un descriptor nuevo
fork        el hijo atiende al cliente, el padre vuelve a accept
```

Cada cliente se atiende en un proceso hijo, por lo que varios pueden estar conectados al mismo tiempo. `signal(SIGCHLD, SIG_IGN)` hace que el sistema limpie a los hijos que terminan.

### Funciones

- `enviar_todo`: repite `send` hasta enviar todos los bytes, porque una llamada puede enviar menos de los pedidos.
- `enviar_respuesta`: envía el texto y después el carácter de fin.
- `recibir_linea`: lee del socket un byte a la vez hasta el salto de línea. Regresa los bytes leídos, 0 si el cliente cerró o -1 si hubo error.
- `atender_cliente`: ciclo que recibe un comando, lo imprime en la terminal del servidor con la IP del cliente, responde, y termina cuando llega SALIR o el cliente cierra.

El servidor imprime fecha, hora e IP de cada cliente que se conecta.

## Cómo probar

```bash
make
./bin/servidor 4898            # terminal 1

nc 127.0.0.1 4898              # terminal 2
SELECT * FROM Alumno
SALIR
```

En este commit la respuesta es `Comando recibido: ...`. El SQL se agrega en las ramas siguientes.
