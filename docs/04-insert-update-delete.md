# 04 · Comandos INSERT, UPDATE y DELETE

**Rama:** `feature/insert-update-delete`
**Commit:** `feat: agregar comandos INSERT, UPDATE y DELETE`

## Qué se agregó

| Archivo | Cambio |
|---|---|
| `src/sql.c` | Funciones de apoyo para analizar comandos y las funciones `sql_insert`, `sql_update` y `sql_delete`. |

Con esto quedan completos los cuatro comandos del enunciado más SALIR.

## Funciones de apoyo

- `buscar_palabra`: localiza VALUES, SET o WHERE sin distinguir mayúsculas y como palabra completa.
- `copiar_entre`: copia el texto entre dos posiciones del comando, por ejemplo lo que hay entre SET y WHERE.
- `separar_asignacion`: parte `columna=valor` en sus dos partes y quita comillas al valor.
- `cumple_condicion`: compara el campo de un registro con el valor del WHERE.

## INSERT

```
INSERT INTO Alumno VALUES (Jose,Sanchez,9,Telecom)
```

1. Localiza VALUES y toma el texto entre paréntesis.
2. Lo separa por comas y quita comillas opcionales.
3. Carga la tabla con bloqueo exclusivo.
4. Verifica que lleguen `num_columnas - 1` valores, porque el Id no se envía, y que ninguno esté vacío.
5. Asigna el Id con `tabla_siguiente_id`, agrega el registro al final y guarda.

Respuesta: `OK: registro insertado en Alumno con Id=11`.

## UPDATE

```
UPDATE Alumno SET Apellido=Gomez WHERE Id=2
```

1. Localiza SET y WHERE. Los dos son obligatorios, para no modificar toda la tabla por accidente.
2. Separa las asignaciones, que pueden ser varias con coma, y la condición.
3. Carga la tabla y verifica que las columnas existan.
4. Rechaza el comando si alguna asignación es sobre la primera columna, el Id.
5. Modifica los registros que cumplen la condición. Si ninguno cumple, responde error y no toca el archivo.

Respuestas: `OK: 1 registro(s) actualizado(s) en Alumno`, `ERROR: la columna Id es la llave primaria y no puede actualizarse`.

## DELETE

```
DELETE FROM Alumno WHERE Id=2
```

1. Localiza WHERE, que es obligatorio.
2. Carga la tabla y recorre los registros con dos índices: los que no cumplen se copian hacia adelante, los que cumplen se dejan fuera.
3. Si ninguno cumple, responde error y no toca el archivo. Los Id restantes no se renumeran.

Respuesta: `OK: 1 registro(s) eliminado(s) de Alumno`.

## Cómo probar

```bash
./bin/cliente 127.0.0.1 4898
SQL> INSERT INTO Alumno VALUES (Jose,Sanchez,9,Telecom)
OK: registro insertado en Alumno con Id=11
SQL> UPDATE Alumno SET Apellido=Gomez WHERE Id=2
OK: 1 registro(s) actualizado(s) en Alumno
SQL> UPDATE Alumno SET Id=50 WHERE Id=2
ERROR: la columna Id es la llave primaria y no puede actualizarse
SQL> DELETE FROM Alumno WHERE Id=2
OK: 1 registro(s) eliminado(s) de Alumno
SQL> DELETE FROM Alumno WHERE Id=2
ERROR: ningun registro de Alumno cumple la condicion Id=2
```

Para dejar la tabla como estaba después de probar:

```bash
git checkout datos/alumno.txt
```
