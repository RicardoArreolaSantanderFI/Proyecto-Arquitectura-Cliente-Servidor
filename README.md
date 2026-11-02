# Proyecto-Arquitectura-Cliente-Servidor

Sistema Manejador de Bases de Datos (DBMS) en una arquitectura cliente-servidor, con sockets TCP/IP y conexión remota. Programado en C para Linux.

El servidor guarda la tabla `Alumno` en un archivo de texto y ejecuta los comandos SQL que le envía el cliente desde otro host.

## Comandos

| Comando | Ejemplo | Respuesta |
|---|---|---|
| SELECT | `SELECT * FROM Alumno` | Contenido del archivo, cabecera y registros |
| INSERT | `INSERT INTO Alumno VALUES (Jose,Sanchez,9,Telecom)` | `OK: registro insertado en Alumno con Id=11` |
| UPDATE | `UPDATE Alumno SET Apellido=Gomez WHERE Id=2` | `OK: 1 registro(s) actualizado(s) en Alumno` |
| DELETE | `DELETE FROM Alumno WHERE Id=2` | `OK: 1 registro(s) eliminado(s) de Alumno` |
| SALIR | `SALIR` | Cierra la conexión |

- El `;` final es opcional y las palabras clave no distinguen mayúsculas.
- Los valores no llevan comillas, aunque se aceptan.
- `Id` es autoincremental, no se envía en el INSERT y no se puede modificar con UPDATE.
- UPDATE y DELETE necesitan `WHERE <columna>=<valor>`.

## Compilar

```bash
make          # genera bin/servidor y bin/cliente
make clean
```

Requiere `gcc` y `make` en Linux.

## Ejecutar

Servidor, desde la raíz del repositorio porque busca las tablas en `datos/`:

```bash
./bin/servidor 4898
```

Cliente, en la misma máquina o en otro host:

```bash
./bin/cliente 127.0.0.1 4898
./bin/cliente 192.168.100.102 4898
```

Para probar entre dos máquinas, en el servidor obtén la IP con `hostname -I` y úsala en el cliente.

## Estructura

```
Makefile
datos/alumno.txt          tabla Alumno
src/protocolo.h           constantes compartidas
src/servidor.c            servidor TCP, un proceso hijo por cliente
src/cliente.c             cliente interactivo
src/sql.c  src/sql.h      intérprete de comandos SQL
src/tabla.c src/tabla.h   lectura y escritura de la tabla en el archivo
src/util.c src/util.h     funciones de apoyo para cadenas
docs/                     documentación de cada rama y reporte
```

## Cómo funciona

El cliente envía un comando por línea. El servidor responde con texto terminado en el carácter `0x04`, que marca el fin de la respuesta. Cada cliente se atiende en un proceso hijo creado con `fork`. La tabla se carga completa en memoria, se modifica y se reescribe el archivo; mientras está cargada el archivo se bloquea con `flock` para que dos procesos no lo escriban a la vez.

## Ramas

| Rama | Contenido | Documento |
|---|---|---|
| `feature/servidor` | estructura base y servidor TCP | [docs/01-servidor.md](docs/01-servidor.md) |
| `feature/cliente` | cliente TCP | [docs/02-cliente.md](docs/02-cliente.md) |
| `feature/select` | manejo de la tabla y SELECT | [docs/03-select.md](docs/03-select.md) |
| `feature/insert-update-delete` | INSERT, UPDATE y DELETE | [docs/04-insert-update-delete.md](docs/04-insert-update-delete.md) |
| `fix/cliente-aviso-compilacion` | corrección de un aviso de gcc en el cliente | |

## Referencias

- Material del curso, Tema 3: http://profesores.fi-b.unam.mx/carlos/acs/Tema-03-Sockets-INTERNET-C/index.html
- Funciones POSIX: https://pubs.opengroup.org/onlinepubs/009696799/idx/index.html
