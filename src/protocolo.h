/* Constantes que comparten el cliente y el servidor */
#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#define PUERTO_DEFAULT   4898      /* puerto si no se indica otro */
#define TAM_PETICION     1024      /* tamanio maximo de un comando */
#define TAM_RESPUESTA    65536     /* tamanio maximo de una respuesta */
#define FIN_RESPUESTA    '\x04'    /* caracter que marca el fin de una respuesta */
#define CMD_SALIR        "SALIR"   /* comando para cerrar la conexion */
#define QLEN             5         /* conexiones en espera */

#endif
