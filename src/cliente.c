/*
 * Cliente del manejador de bases de datos.
 * Se conecta al servidor por TCP, envia los comandos SQL que escribe
 * el usuario y muestra la respuesta. Termina con el comando SALIR.
 *
 * Uso: ./bin/cliente <ip_servidor> [puerto]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "protocolo.h"
#include "util.h"

/* Envia el buffer completo repitiendo send hasta terminar */
static int enviar_todo(int sd, const char *datos, size_t cantidad)
{
    size_t enviados = 0;
    ssize_t r;

    while (enviados < cantidad) {
        r = send(sd, datos + enviados, cantidad - enviados, 0);
        if (r <= 0)
            return -1;
        enviados += (size_t) r;
    }
    return 0;
}

/* Lee la respuesta del servidor y la imprime conforme llega,
   hasta encontrar el caracter de fin de respuesta */
static int recibir_respuesta(int sd)
{
    char buf[TAM_RESPUESTA];
    ssize_t n;
    ssize_t i;

    while (1) {
        n = recv(sd, buf, sizeof(buf), 0);
        if (n <= 0)
            return -1;

        for (i = 0; i < n; i++) {
            if (buf[i] == FIN_RESPUESTA)
                break;
        }

        if (i > 0)
            write(1, buf, (size_t) i);

        if (i < n)
            return 0;
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server;
    struct hostent *sp;
    int sd;
    int puerto = PUERTO_DEFAULT;
    char linea[TAM_PETICION];
    char peticion[TAM_PETICION];
    int es_salir;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <ip_servidor> [puerto]\n", argv[0]);
        exit(1);
    }
    if (argc >= 3)
        puerto = atoi(argv[2]);
    if (puerto <= 0 || puerto > 65535) {
        fprintf(stderr, "Puerto invalido: %s\n", argv[2]);
        exit(1);
    }

    /* Se crea el socket y se obtiene la direccion del servidor */
    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd < 0) {
        perror("socket");
        exit(1);
    }

    sp = gethostbyname(argv[1]);
    if (sp == NULL) {
        fprintf(stderr, "No se pudo resolver el host: %s\n", argv[1]);
        exit(1);
    }

    memset((char *) &server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons((u_short) puerto);
    memcpy(&server.sin_addr, sp->h_addr, sp->h_length);

    /* Conexion con el servidor */
    if (connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Conectado al servidor %s:%d\n", inet_ntoa(server.sin_addr), puerto);
    printf("Escribe un comando SQL por linea. SALIR para terminar.\n");

    /* Se lee un comando del teclado, se envia y se espera la respuesta */
    while (1) {
        printf("SQL> ");
        fflush(stdout);

        if (fgets(linea, sizeof(linea), stdin) == NULL) {
            printf("\n");
            strcpy(linea, CMD_SALIR);
        }

        recortar_cadena(linea);
        if (linea[0] == '\0')
            continue;

        es_salir = (strcasecmp(linea, CMD_SALIR) == 0);

        snprintf(peticion, sizeof(peticion), "%s\n", linea);
        if (enviar_todo(sd, peticion, strlen(peticion)) != 0) {
            printf("No se pudo enviar el comando. El servidor cerro la conexion.\n");
            break;
        }

        if (recibir_respuesta(sd) != 0) {
            printf("El servidor cerro la conexion.\n");
            break;
        }

        if (es_salir)
            break;
    }

    close(sd);
    return 0;
}
