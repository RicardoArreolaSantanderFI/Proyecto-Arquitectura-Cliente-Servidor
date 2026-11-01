/* Interprete de los comandos SQL que recibe el servidor */
#ifndef SQL_H
#define SQL_H

#include <stddef.h>

/* Ejecuta el comando y deja en respuesta el resultado o un mensaje de error */
void sql_procesar(const char *comando, char *respuesta, size_t tam);

#endif
