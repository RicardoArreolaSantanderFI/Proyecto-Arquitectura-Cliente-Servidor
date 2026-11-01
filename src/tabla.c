/*
 * Manejo de una tabla guardada en un archivo de texto.
 * La tabla se carga completa en memoria, se modifica ahi y al guardar
 * se reescribe el archivo. Mientras esta cargada, el archivo queda
 * bloqueado para que dos procesos no lo escriban al mismo tiempo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/file.h>

#include "tabla.h"
#include "util.h"

/* Recorre la linea buscando comas y copia cada pedazo como un campo */
int tabla_dividir_campos(char *linea, char campos[][MAX_VALOR])
{
    int n = 0;
    char *token = linea;
    char *coma;

    while (n < MAX_COLUMNAS) {
        coma = strchr(token, SEPARADOR);
        if (coma != NULL)
            *coma = '\0';
        strncpy(campos[n], token, MAX_VALOR - 1);
        campos[n][MAX_VALOR - 1] = '\0';
        recortar_cadena(campos[n]);
        n++;
        if (coma == NULL)
            break;
        token = coma + 1;
    }
    return n;
}

/* El nombre de la tabla solo puede tener letras, digitos y guion bajo */
static int nombre_valido(const char *nombre)
{
    size_t i;

    if (nombre[0] == '\0')
        return 0;
    for (i = 0; nombre[i] != '\0'; i++) {
        if (!isalnum((unsigned char) nombre[i]) && nombre[i] != '_')
            return 0;
    }
    return 1;
}

/* Forma la ruta del archivo con el nombre en minusculas, lo abre, lo bloquea
   y lee la cabecera y despues cada registro */
int tabla_cargar(const char *nombre, int escritura, Tabla *t, char *error)
{
    char linea[MAX_LINEA];
    char nombre_min[MAX_VALOR];
    size_t i;

    memset(t, 0, sizeof(Tabla));
    strncpy(t->nombre, nombre, MAX_VALOR - 1);
    t->escritura = escritura;

    if (!nombre_valido(nombre)) {
        snprintf(error, TAM_ERROR, "ERROR: nombre de tabla invalido '%s'", nombre);
        return -1;
    }

    for (i = 0; nombre[i] != '\0' && i < MAX_VALOR - 1; i++)
        nombre_min[i] = (char) tolower((unsigned char) nombre[i]);
    nombre_min[i] = '\0';
    snprintf(t->ruta, sizeof(t->ruta), "%s/%s%s", DIRECTORIO_DATOS, nombre_min, EXTENSION_TABLA);

    t->archivo = fopen(t->ruta, escritura ? "r+" : "r");
    if (t->archivo == NULL) {
        snprintf(error, TAM_ERROR, "ERROR: la tabla '%s' no existe, no se encontro el archivo %s",
                 nombre, t->ruta);
        return -1;
    }

    if (flock(fileno(t->archivo), escritura ? LOCK_EX : LOCK_SH) != 0) {
        snprintf(error, TAM_ERROR, "ERROR: no se pudo bloquear el archivo %s", t->ruta);
        tabla_liberar(t);
        return -1;
    }

    t->registros = calloc(MAX_REGISTROS, sizeof(Registro));
    if (t->registros == NULL) {
        snprintf(error, TAM_ERROR, "ERROR: sin memoria para cargar la tabla");
        tabla_liberar(t);
        return -1;
    }

    if (fgets(linea, sizeof(linea), t->archivo) == NULL) {
        snprintf(error, TAM_ERROR, "ERROR: el archivo %s esta vacio, no tiene cabecera", t->ruta);
        tabla_liberar(t);
        return -1;
    }
    t->num_columnas = tabla_dividir_campos(linea, t->columnas);

    while (fgets(linea, sizeof(linea), t->archivo) != NULL) {
        recortar_cadena(linea);
        if (linea[0] == '\0')
            continue;
        if (t->num_registros >= MAX_REGISTROS) {
            snprintf(error, TAM_ERROR, "ERROR: la tabla excede el maximo de %d registros", MAX_REGISTROS);
            tabla_liberar(t);
            return -1;
        }
        tabla_dividir_campos(linea, t->registros[t->num_registros].campos);
        t->num_registros++;
    }

    return 0;
}

/* Vacia el archivo y vuelve a escribir la cabecera y todos los registros */
int tabla_guardar(Tabla *t, char *error)
{
    int i, j;

    if (!t->escritura || t->archivo == NULL) {
        snprintf(error, TAM_ERROR, "ERROR: la tabla no se abrio para escritura");
        return -1;
    }

    rewind(t->archivo);
    if (ftruncate(fileno(t->archivo), 0) != 0) {
        snprintf(error, TAM_ERROR, "ERROR: no se pudo escribir el archivo %s", t->ruta);
        return -1;
    }

    for (j = 0; j < t->num_columnas; j++)
        fprintf(t->archivo, "%s%c", t->columnas[j],
                (j < t->num_columnas - 1) ? SEPARADOR : '\n');

    for (i = 0; i < t->num_registros; i++) {
        for (j = 0; j < t->num_columnas; j++)
            fprintf(t->archivo, "%s%c", t->registros[i].campos[j],
                    (j < t->num_columnas - 1) ? SEPARADOR : '\n');
    }

    if (fflush(t->archivo) != 0) {
        snprintf(error, TAM_ERROR, "ERROR: no se pudo escribir el archivo %s", t->ruta);
        return -1;
    }
    return 0;
}

/* Quita el bloqueo, cierra el archivo y libera los registros */
void tabla_liberar(Tabla *t)
{
    if (t->archivo != NULL) {
        fflush(t->archivo);
        flock(fileno(t->archivo), LOCK_UN);
        fclose(t->archivo);
        t->archivo = NULL;
    }
    free(t->registros);
    t->registros = NULL;
}

/* Compara el nombre con cada columna sin distinguir mayusculas */
int tabla_indice_columna(const Tabla *t, const char *columna)
{
    int j;

    for (j = 0; j < t->num_columnas; j++) {
        if (strcasecmp(t->columnas[j], columna) == 0)
            return j;
    }
    return -1;
}

/* Busca el Id mas grande y le suma uno */
int tabla_siguiente_id(const Tabla *t)
{
    int i, id, mayor = 0;

    for (i = 0; i < t->num_registros; i++) {
        id = atoi(t->registros[i].campos[0]);
        if (id > mayor)
            mayor = id;
    }
    return mayor + 1;
}
