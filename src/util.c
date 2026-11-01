/* Funciones de apoyo para cadenas */
#include <ctype.h>
#include <string.h>
#include "util.h"

/* Avanza sobre los espacios del inicio, borra los del final y mueve el texto al principio */
void recortar_cadena(char *s)
{
    char *inicio = s;
    size_t n;

    while (*inicio != '\0' && isspace((unsigned char) *inicio))
        inicio++;

    n = strlen(inicio);
    while (n > 0 && isspace((unsigned char) inicio[n - 1]))
        inicio[--n] = '\0';

    memmove(s, inicio, n + 1);
}

/* Si el primer y el ultimo caracter son comillas, se eliminan */
void quitar_comillas(char *s)
{
    size_t n = strlen(s);

    if (n >= 2 && (s[0] == '\'' || s[0] == '"') && s[n - 1] == s[0]) {
        memmove(s, s + 1, n - 2);
        s[n - 2] = '\0';
    }
}
