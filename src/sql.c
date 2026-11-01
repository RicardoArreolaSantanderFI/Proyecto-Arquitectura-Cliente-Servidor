/*
 * Interprete de los comandos SQL que recibe el servidor.
 * Cada comando se revisa con funciones de cadenas, se ejecuta sobre la
 * tabla y se arma el texto de respuesta para el cliente.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <ctype.h>

#include "protocolo.h"
#include "sql.h"
#include "tabla.h"
#include "util.h"

/* Agrega texto con formato al final de la respuesta sin pasarse del tamanio */
static void agregar(char *respuesta, size_t tam, const char *formato, ...)
{
    size_t usado = strlen(respuesta);
    va_list args;

    if (usado >= tam - 1)
        return;
    va_start(args, formato);
    vsnprintf(respuesta + usado, tam - usado, formato, args);
    va_end(args);
}

/* SELECT * FROM tabla
   Carga la tabla y copia la cabecera y todos los registros a la respuesta */
static void sql_select(const char *cmd, char *resp, size_t tam)
{
    char k1[16], k2[16], k3[16], tabla[MAX_VALOR], extra[16];
    char error[TAM_ERROR];
    Tabla t;
    int i, j, n;

    n = sscanf(cmd, "%15s %15s %15s %63s %15s", k1, k2, k3, tabla, extra);
    if (n != 4 || strcmp(k2, "*") != 0 || strcasecmp(k3, "FROM") != 0) {
        agregar(resp, tam, "ERROR de sintaxis. Uso: SELECT * FROM <tabla>\n");
        return;
    }

    if (tabla_cargar(tabla, 0, &t, error) != 0) {
        agregar(resp, tam, "%s\n", error);
        return;
    }

    for (j = 0; j < t.num_columnas; j++)
        agregar(resp, tam, "%s%c", t.columnas[j],
                (j < t.num_columnas - 1) ? SEPARADOR : '\n');

    for (i = 0; i < t.num_registros; i++) {
        for (j = 0; j < t.num_columnas; j++)
            agregar(resp, tam, "%s%c", t.registros[i].campos[j],
                    (j < t.num_columnas - 1) ? SEPARADOR : '\n');
    }

    tabla_liberar(&t);
}

/* Limpia el comando, identifica la primera palabra y llama a la funcion
   que le corresponde */
void sql_procesar(const char *comando, char *respuesta, size_t tam)
{
    char cmd[TAM_PETICION];
    char verbo[16];
    size_t n;

    respuesta[0] = '\0';

    strncpy(cmd, comando, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    recortar_cadena(cmd);
    n = strlen(cmd);
    if (n > 0 && cmd[n - 1] == ';') {
        cmd[n - 1] = '\0';
        recortar_cadena(cmd);
    }

    if (cmd[0] == '\0' || sscanf(cmd, "%15s", verbo) != 1) {
        agregar(respuesta, tam, "ERROR: comando vacio\n");
        return;
    }

    if (strcasecmp(verbo, "SELECT") == 0)
        sql_select(cmd, respuesta, tam);
    else
        agregar(respuesta, tam,
                "ERROR: comando no reconocido '%s'. Comandos validos: SELECT, SALIR\n",
                verbo);
}
