# 03 · Manejo de la tabla y comando SELECT

**Rama:** `feature/select`
**Commit:** `feat: agregar manejo de la tabla en archivo y comando SELECT`

## Qué se agregó

| Archivo | Para qué sirve |
|---|---|
| `src/tabla.h`, `src/tabla.c` | Carga la tabla desde el archivo, la guarda y da utilidades. |
| `src/sql.h`, `src/sql.c` | Intérprete SQL. En este commit solo SELECT. |
| `src/servidor.c` | Ahora llama a `sql_procesar` en lugar de confirmar el comando. |
| `Makefile` | Se agregan `tabla.c` y `sql.c` al servidor. |

## Cómo funciona

### La tabla en memoria

```c
typedef struct { char campos[MAX_COLUMNAS][MAX_VALOR]; } Registro;

typedef struct {
    FILE *archivo;        /* abierto y bloqueado mientras está cargada */
    int   num_columnas;
    char  columnas[..];   /* Id, Nombre, Apellido, Semestre, Carrera */
    int   num_registros;
    Registro *registros;
} Tabla;
```

Límites definidos en `tabla.h`: 8 columnas, 1000 registros, 64 caracteres por campo.

### Funciones de `tabla.c`

- `tabla_cargar(nombre, escritura, &t, error)`: valida el nombre, forma la ruta `datos/<nombre en minúsculas>.txt`, abre el archivo, lo bloquea con `flock` y lee la cabecera y los registros. Con `escritura = 1` el bloqueo es exclusivo, con `0` es compartido.
- `tabla_guardar`: vacía el archivo y reescribe cabecera y registros.
- `tabla_liberar`: quita el bloqueo, cierra y libera memoria.
- `tabla_indice_columna`: posición de una columna por nombre, sin distinguir mayúsculas.
- `tabla_siguiente_id`: Id mayor más uno.
- `tabla_dividir_campos`: separa una línea por comas.

El bloqueo del archivo evita que dos procesos hijo lo escriban al mismo tiempo. El servidor debe ejecutarse desde la raíz del repositorio porque `datos/` es una ruta relativa.

### `sql_procesar`

Copia el comando, le quita espacios y el `;` final si lo hay, lee la primera palabra y llama a la función del comando. Las palabras clave se comparan sin distinguir mayúsculas.

### `sql_select`

Espera exactamente `SELECT * FROM <tabla>`. Carga la tabla en modo lectura y copia a la respuesta la cabecera y todos los registros con el mismo formato del archivo. `agregar` escribe al final del buffer de respuesta sin pasarse de su tamaño.

## Cómo probar

```bash
./bin/cliente 127.0.0.1 4898
SQL> SELECT * FROM Alumno
Id,Nombre,Apellido,Semestre,Carrera
1,Juan,Perez,5,Computacion
...
SQL> SELECT * FROM Profesor
ERROR: la tabla 'Profesor' no existe, no se encontro el archivo datos/profesor.txt
```
