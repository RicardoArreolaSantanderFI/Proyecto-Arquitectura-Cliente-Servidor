/*
 * Servidor del manejador de bases de datos.
 * Espera conexiones por TCP, recibe comandos SQL de cada cliente,
 * los ejecuta sobre el archivo de la tabla y regresa la respuesta.
 * Cada cliente se atiende en un proceso hijo.
 *
 * Uso: ./bin/servidor [puerto]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocolo.h"
#include "util.h"

/* Envia el buffer completo repitiendo send hasta terminar */
static int enviar_todo(int fd, const char *datos, size_t cantidad)
{
    size_t enviados = 0;
    ssize_t r;

    while (enviados < cantidad) {
        r = send(fd, datos + enviados, cantidad - enviados, 0);
        if (r <= 0)
            return -1;
        enviados += (size_t) r;
    }
    return 0;
}

/* Envia el texto y despues el caracter que marca el fin de la respuesta */
static int enviar_respuesta(int fd, const char *texto)
{
    char fin = FIN_RESPUESTA;

    if (enviar_todo(fd, texto, strlen(texto)) != 0)
        return -1;
    return enviar_todo(fd, &fin, 1);
}

/* Lee del socket caracter por caracter hasta el salto de linea.
   Regresa cuantos bytes se leyeron, 0 si el cliente cerro o -1 si hubo error */
static int recibir_linea(int fd, char *buf, size_t tam)
{
    size_t n = 0;
    int leidos = 0;
    char c;
    ssize_t r;

    while (n < tam - 1) {
        r = recv(fd, &c, 1, 0);
        if (r < 0)
            return -1;
        if (r == 0)
            break;
        leidos++;
        if (c == '\n')
            break;
        buf[n++] = c;
    }
    buf[n] = '\0';
    return leidos;
}

/* Ciclo de atencion de un cliente: recibe un comando, lo ejecuta y responde,
   hasta que llega SALIR o el cliente cierra la conexion */
static void atender_cliente(int fd_c, const char *ip_cliente, int num_cliente)
{
    char peticion[TAM_PETICION];
    char *respuesta = malloc(TAM_RESPUESTA);
    int n;

    if (respuesta == NULL) {
        perror("malloc");
        return;
    }

    while (1) {
        n = recibir_linea(fd_c, peticion, sizeof(peticion));
        if (n <= 0) {
            printf("[cliente %d - %s] cerro la conexion\n", num_cliente, ip_cliente);
            break;
        }

        recortar_cadena(peticion);
        printf("[cliente %d - %s] comando: %s\n", num_cliente, ip_cliente, peticion);

        if (strcasecmp(peticion, CMD_SALIR) == 0) {
            enviar_respuesta(fd_c, "Adios. Conexion cerrada por el servidor.\n");
            break;
        }

        snprintf(respuesta, TAM_RESPUESTA, "Comando recibido: %s\n", peticion);

        if (enviar_respuesta(fd_c, respuesta) != 0) {
            printf("[cliente %d - %s] error al enviar la respuesta\n", num_cliente, ip_cliente);
            break;
        }
    }

    free(respuesta);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in servidor;
    struct sockaddr_in cliente;
    socklen_t long_cliente;
    int fd_s, fd_c;
    int puerto = PUERTO_DEFAULT;
    int num_cliente = 0;
    int opcion = 1;

    if (argc >= 2)
        puerto = atoi(argv[1]);
    if (puerto <= 0 || puerto > 65535) {
        fprintf(stderr, "Uso: %s [puerto]\n", argv[0]);
        exit(1);
    }

    /* Se crea el socket TCP y se permite reutilizar el puerto al reiniciar */
    fd_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_s < 0) {
        perror("socket");
        exit(1);
    }
    if (setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    /* Se asocia el socket al puerto en todas las interfaces y se pone a escuchar */
    memset((char *) &servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;
    servidor.sin_port = htons((u_short) puerto);

    if (bind(fd_s, (struct sockaddr *) &servidor, sizeof(servidor)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(fd_s, QLEN) < 0) {
        perror("listen");
        exit(1);
    }

    /* Los hijos que terminan se limpian solos */
    signal(SIGCHLD, SIG_IGN);

    printf("Servidor escuchando en el puerto %d. Ctrl-C para terminar.\n", puerto);

    /* Por cada cliente aceptado se crea un hijo que lo atiende */
    while (1) {
        long_cliente = sizeof(cliente);
        fd_c = accept(fd_s, (struct sockaddr *) &cliente, &long_cliente);
        if (fd_c < 0) {
            perror("accept");
            continue;
        }
        num_cliente++;

        time_t T = time(NULL);
        struct tm tm = *localtime(&T);
        printf("%02d/%02d/%04d %02d:%02d:%02d - Cliente %d conectado desde %s:%d\n",
               tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
               tm.tm_hour, tm.tm_min, tm.tm_sec,
               num_cliente, inet_ntoa(cliente.sin_addr), ntohs(cliente.sin_port));

        if (fork() == 0) {
            close(fd_s);
            atender_cliente(fd_c, inet_ntoa(cliente.sin_addr), num_cliente);
            close(fd_c);
            printf("Cliente %d desconectado.\n", num_cliente);
            exit(0);
        } else {
            close(fd_c);
        }
    }

    close(fd_s);
    return 0;
}
