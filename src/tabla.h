/*
 * Manejo de una tabla guardada en un archivo de texto.
 * La primera linea del archivo es la cabecera y cada linea siguiente es
 * un registro. Los campos van separados por coma. La primera columna es
 * el Id y se genera de forma automatica.
 */
#ifndef TABLA_H
#define TABLA_H

#include <stdio.h>

#define DIRECTORIO_DATOS  "datos"
#define EXTENSION_TABLA   ".txt"
#define SEPARADOR         ','
#define MAX_COLUMNAS      8
#define MAX_REGISTROS     1000
#define MAX_VALOR         64
#define MAX_LINEA         1024
#define MAX_RUTA          128
#define TAM_ERROR         256

typedef struct {
    char campos[MAX_COLUMNAS][MAX_VALOR];
} Registro;

typedef struct {
    char nombre[MAX_VALOR];
    char ruta[MAX_RUTA];
    FILE *archivo;
    int escritura;
    int num_columnas;
    char columnas[MAX_COLUMNAS][MAX_VALOR];
    int num_registros;
    Registro *registros;
} Tabla;

/* Abre el archivo de la tabla y carga la cabecera y los registros en memoria */
int tabla_cargar(const char *nombre, int escritura, Tabla *t, char *error);

/* Escribe la tabla completa de regreso al archivo */
int tabla_guardar(Tabla *t, char *error);

/* Cierra el archivo y libera la memoria */
void tabla_liberar(Tabla *t);

/* Regresa la posicion de una columna por su nombre, o -1 si no existe */
int tabla_indice_columna(const Tabla *t, const char *columna);

/* Regresa el Id que le toca al siguiente registro */
int tabla_siguiente_id(const Tabla *t);

/* Separa una linea por comas y guarda cada parte en campos */
int tabla_dividir_campos(char *linea, char campos[][MAX_VALOR]);

#endif
