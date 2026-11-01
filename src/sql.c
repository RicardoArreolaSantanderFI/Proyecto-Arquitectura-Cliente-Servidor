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

/* Busca una palabra clave dentro del comando sin distinguir mayusculas.
   Solo cuenta si esta separada del resto por espacios o parentesis */
static const char *buscar_palabra(const char *texto, const char *palabra)
{
    size_t len = strlen(palabra);
    const char *p;

    for (p = texto; *p != '\0'; p++) {
        if (strncasecmp(p, palabra, len) != 0)
            continue;
        if (p != texto && !isspace((unsigned char) p[-1]) && p[-1] != ')')
            continue;
        if (p[len] != '\0' && !isspace((unsigned char) p[len]) && p[len] != '(')
            continue;
        return p;
    }
    return NULL;
}

/* Copia el texto que hay entre dos posiciones y le quita los espacios */
static void copiar_entre(const char *inicio, const char *fin, char *destino, size_t tam)
{
    size_t len = (fin > inicio) ? (size_t) (fin - inicio) : 0;

    if (len >= tam)
        len = tam - 1;
    memcpy(destino, inicio, len);
    destino[len] = '\0';
    recortar_cadena(destino);
}

/* Separa una expresion columna=valor en sus dos partes */
static int separar_asignacion(char *expresion, char *columna, char *valor)
{
    char *igual = strchr(expresion, '=');

    if (igual == NULL)
        return -1;
    *igual = '\0';

    strncpy(columna, expresion, MAX_VALOR - 1);
    columna[MAX_VALOR - 1] = '\0';
    recortar_cadena(columna);

    strncpy(valor, igual + 1, MAX_VALOR - 1);
    valor[MAX_VALOR - 1] = '\0';
    recortar_cadena(valor);
    quitar_comillas(valor);

    return (columna[0] == '\0') ? -1 : 0;
}

/* Revisa si el campo del registro es igual al valor del WHERE */
static int cumple_condicion(const Tabla *t, int fila, int idx, const char *valor)
{
    return strcasecmp(t->registros[fila].campos[idx], valor) == 0;
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

/* INSERT INTO tabla VALUES valor1,valor2,...
   Toma los valores entre parentesis, les asigna el siguiente Id
   y agrega el registro al final de la tabla */
static void sql_insert(const char *cmd, char *resp, size_t tam)
{
    char k1[16], k2[16], tabla[MAX_VALOR];
    char valores[MAX_LINEA];
    char campos[MAX_COLUMNAS][MAX_VALOR];
    char error[TAM_ERROR];
    const char *values, *abre, *cierra, *p;
    size_t len;
    Tabla t;
    Registro nuevo;
    int n, j, nuevo_id;

    if (sscanf(cmd, "%15s %15s %63s", k1, k2, tabla) != 3 || strcasecmp(k2, "INTO") != 0) {
        agregar(resp, tam, "ERROR de sintaxis. Uso: INSERT INTO <tabla> VALUES (v1,v2,...)\n");
        return;
    }

    values = buscar_palabra(cmd, "VALUES");
    abre = (values != NULL) ? strchr(values, '(') : NULL;
    cierra = (values != NULL) ? strrchr(values, ')') : NULL;
    if (abre == NULL || cierra == NULL || cierra < abre) {
        agregar(resp, tam, "ERROR de sintaxis. Uso: INSERT INTO <tabla> VALUES (v1,v2,...)\n");
        return;
    }
    for (p = cierra + 1; *p != '\0'; p++) {
        if (!isspace((unsigned char) *p)) {
            agregar(resp, tam, "ERROR de sintaxis: texto inesperado despues de ')'\n");
            return;
        }
    }

    len = (size_t) (cierra - abre - 1);
    if (len >= sizeof(valores)) {
        agregar(resp, tam, "ERROR: la lista de valores es demasiado larga\n");
        return;
    }
    memcpy(valores, abre + 1, len);
    valores[len] = '\0';

    n = tabla_dividir_campos(valores, campos);
    for (j = 0; j < n; j++)
        quitar_comillas(campos[j]);

    if (tabla_cargar(tabla, 1, &t, error) != 0) {
        agregar(resp, tam, "%s\n", error);
        return;
    }

    if (n != t.num_columnas - 1) {
        agregar(resp, tam, "ERROR: se esperaban %d valores (", t.num_columnas - 1);
        for (j = 1; j < t.num_columnas; j++)
            agregar(resp, tam, "%s%s", t.columnas[j], (j < t.num_columnas - 1) ? "," : "");
        agregar(resp, tam, ") y se recibieron %d\n", n);
        tabla_liberar(&t);
        return;
    }
    for (j = 0; j < n; j++) {
        if (campos[j][0] == '\0') {
            agregar(resp, tam, "ERROR: el valor de la columna %s esta vacio\n", t.columnas[j + 1]);
            tabla_liberar(&t);
            return;
        }
    }
    if (t.num_registros >= MAX_REGISTROS) {
        agregar(resp, tam, "ERROR: la tabla esta llena (%d registros)\n", MAX_REGISTROS);
        tabla_liberar(&t);
        return;
    }

    memset(&nuevo, 0, sizeof(nuevo));
    nuevo_id = tabla_siguiente_id(&t);
    snprintf(nuevo.campos[0], MAX_VALOR, "%d", nuevo_id);
    for (j = 0; j < n; j++)
        strcpy(nuevo.campos[j + 1], campos[j]);

    t.registros[t.num_registros] = nuevo;
    t.num_registros++;

    if (tabla_guardar(&t, error) != 0) {
        agregar(resp, tam, "%s\n", error);
        tabla_liberar(&t);
        return;
    }

    agregar(resp, tam, "OK: registro insertado en %s con Id=%d\n", t.nombre, nuevo_id);
    tabla_liberar(&t);
}

/* UPDATE tabla SET columna=valor WHERE columna=valor
   Cambia los valores indicados en los registros que cumplen la condicion.
   La columna Id no se puede modificar */
static void sql_update(const char *cmd, char *resp, size_t tam)
{
    char k1[16], tabla[MAX_VALOR];
    char asignaciones[MAX_LINEA], condicion[MAX_LINEA];
    char col_where[MAX_VALOR], val_where[MAX_VALOR];
    char cols[MAX_COLUMNAS][MAX_VALOR], vals[MAX_COLUMNAS][MAX_VALOR];
    int idx[MAX_COLUMNAS];
    char error[TAM_ERROR];
    const char *set, *where;
    char *token, *coma;
    Tabla t;
    int num_asig = 0, idx_where, afectados = 0, i, j;

    if (sscanf(cmd, "%15s %63s", k1, tabla) != 2) {
        agregar(resp, tam, "ERROR de sintaxis. Uso: UPDATE <tabla> SET <columna>=<valor> WHERE <columna>=<valor>\n");
        return;
    }

    set = buscar_palabra(cmd, "SET");
    where = buscar_palabra(cmd, "WHERE");
    if (set == NULL || where == NULL || where < set) {
        agregar(resp, tam, "ERROR de sintaxis. Uso: UPDATE <tabla> SET <columna>=<valor> WHERE <columna>=<valor>\n");
        return;
    }

    copiar_entre(set + strlen("SET"), where, asignaciones, sizeof(asignaciones));
    copiar_entre(where + strlen("WHERE"), cmd + strlen(cmd), condicion, sizeof(condicion));

    if (separar_asignacion(condicion, col_where, val_where) != 0) {
        agregar(resp, tam, "ERROR de sintaxis en WHERE. Uso: WHERE <columna>=<valor>\n");
        return;
    }

    token = asignaciones;
    while (token != NULL) {
        if (num_asig >= MAX_COLUMNAS) {
            agregar(resp, tam, "ERROR: demasiadas asignaciones en SET\n");
            return;
        }
        coma = strchr(token, ',');
        if (coma != NULL)
            *coma = '\0';
        if (separar_asignacion(token, cols[num_asig], vals[num_asig]) != 0) {
            agregar(resp, tam, "ERROR de sintaxis en SET. Uso: SET <columna>=<valor>[, <columna>=<valor>]\n");
            return;
        }
        num_asig++;
        token = (coma != NULL) ? coma + 1 : NULL;
    }

    if (tabla_cargar(tabla, 1, &t, error) != 0) {
        agregar(resp, tam, "%s\n", error);
        return;
    }

    idx_where = tabla_indice_columna(&t, col_where);
    if (idx_where < 0) {
        agregar(resp, tam, "ERROR: la columna '%s' no existe en %s\n", col_where, t.nombre);
        tabla_liberar(&t);
        return;
    }
    for (j = 0; j < num_asig; j++) {
        idx[j] = tabla_indice_columna(&t, cols[j]);
        if (idx[j] < 0) {
            agregar(resp, tam, "ERROR: la columna '%s' no existe en %s\n", cols[j], t.nombre);
            tabla_liberar(&t);
            return;
        }
        if (idx[j] == 0) {
            agregar(resp, tam, "ERROR: la columna %s es la llave primaria y no puede actualizarse\n",
                    t.columnas[0]);
            tabla_liberar(&t);
            return;
        }
        if (vals[j][0] == '\0') {
            agregar(resp, tam, "ERROR: el valor para la columna %s esta vacio\n", t.columnas[idx[j]]);
            tabla_liberar(&t);
            return;
        }
    }

    for (i = 0; i < t.num_registros; i++) {
        if (!cumple_condicion(&t, i, idx_where, val_where))
            continue;
        for (j = 0; j < num_asig; j++)
            strcpy(t.registros[i].campos[idx[j]], vals[j]);
        afectados++;
    }

    if (afectados == 0) {
        agregar(resp, tam, "ERROR: ningun registro de %s cumple la condicion %s=%s\n",
                t.nombre, col_where, val_where);
        tabla_liberar(&t);
        return;
    }

    if (tabla_guardar(&t, error) != 0) {
        agregar(resp, tam, "%s\n", error);
        tabla_liberar(&t);
        return;
    }

    agregar(resp, tam, "OK: %d registro(s) actualizado(s) en %s\n", afectados, t.nombre);
    tabla_liberar(&t);
}

/* DELETE FROM tabla WHERE columna=valor
   Quita de la tabla los registros que cumplen la condicion */
static void sql_delete(const char *cmd, char *resp, size_t tam)
{
    char k1[16], k2[16], tabla[MAX_VALOR];
    char condicion[MAX_LINEA];
    char col_where[MAX_VALOR], val_where[MAX_VALOR];
    char error[TAM_ERROR];
    const char *where;
    Tabla t;
    int idx_where, eliminados = 0, i, destino;

    if (sscanf(cmd, "%15s %15s %63s", k1, k2, tabla) != 3 || strcasecmp(k2, "FROM") != 0) {
        agregar(resp, tam, "ERROR de sintaxis. Uso: DELETE FROM <tabla> WHERE <columna>=<valor>\n");
        return;
    }

    where = buscar_palabra(cmd, "WHERE");
    if (where == NULL) {
        agregar(resp, tam, "ERROR de sintaxis. Uso: DELETE FROM <tabla> WHERE <columna>=<valor>\n");
        return;
    }
    copiar_entre(where + strlen("WHERE"), cmd + strlen(cmd), condicion, sizeof(condicion));
    if (separar_asignacion(condicion, col_where, val_where) != 0) {
        agregar(resp, tam, "ERROR de sintaxis en WHERE. Uso: WHERE <columna>=<valor>\n");
        return;
    }

    if (tabla_cargar(tabla, 1, &t, error) != 0) {
        agregar(resp, tam, "%s\n", error);
        return;
    }

    idx_where = tabla_indice_columna(&t, col_where);
    if (idx_where < 0) {
        agregar(resp, tam, "ERROR: la columna '%s' no existe en %s\n", col_where, t.nombre);
        tabla_liberar(&t);
        return;
    }

    /* Los registros que no cumplen se van recorriendo hacia adelante
       y los que cumplen se dejan fuera */
    destino = 0;
    for (i = 0; i < t.num_registros; i++) {
        if (cumple_condicion(&t, i, idx_where, val_where)) {
            eliminados++;
            continue;
        }
        if (destino != i)
            t.registros[destino] = t.registros[i];
        destino++;
    }
    t.num_registros = destino;

    if (eliminados == 0) {
        agregar(resp, tam, "ERROR: ningun registro de %s cumple la condicion %s=%s\n",
                t.nombre, col_where, val_where);
        tabla_liberar(&t);
        return;
    }

    if (tabla_guardar(&t, error) != 0) {
        agregar(resp, tam, "%s\n", error);
        tabla_liberar(&t);
        return;
    }

    agregar(resp, tam, "OK: %d registro(s) eliminado(s) de %s\n", eliminados, t.nombre);
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
    else if (strcasecmp(verbo, "INSERT") == 0)
        sql_insert(cmd, respuesta, tam);
    else if (strcasecmp(verbo, "UPDATE") == 0)
        sql_update(cmd, respuesta, tam);
    else if (strcasecmp(verbo, "DELETE") == 0)
        sql_delete(cmd, respuesta, tam);
    else
        agregar(respuesta, tam,
                "ERROR: comando no reconocido '%s'. Comandos validos: SELECT, INSERT, UPDATE, DELETE, SALIR\n",
                verbo);
}
