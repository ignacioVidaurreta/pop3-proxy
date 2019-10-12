/**
 * pop3nio.c  - controla el flujo de un proxy POP3 con sockets no bloqueantes
 */
#include<stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // memset
#include <assert.h>  // assert
#include <errno.h>
#include <time.h>
#include <unistd.h>  // close
#include <pthread.h>

#include <arpa/inet.h>

#include "hello.h"
#include "request.h"
#include "buffer.h"

#include "stm.h"
#include "pop3nio.h"
#include"netutils.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))

/** maquina de estados general */
enum pop3_state {
   /**
     * Se conecta con el servidor de origen.
     *
     * Transiciones:
     *   - CAPA     mientras el mensaje no esté completo
     *   - ERROR    ante cualquier error de conexion
     */
    CONNECTING,

    /**
     * Recibe el mensaje de bienvenida del servidor de origen
     *
     * Transiciones:
     *   - CAPA     mientras el mensaje no esté completo
     *   - ERROR    ante cualquier error (IO/parseo)
     */
    EHLO,

    /**
     * Recibe un pedido del cliente y lo transmite al host
     *
     * Transiciones:
     *   - REQUEST       mientras el mensaje no esté completo
     *   - RESPONSE      si el mensajte esta completo y transmitido por completo
     *   - ERROR         ante cualquier error (IO/parseo)
     */
    REQUEST,

    /**
     * Recibe una respuesta del servidor origen y la procesa
     * 
     * Transiciones:
     *     - RESPONSE_RECV        mientras la respuesta no este completa
     *     - FILTER               si se recibe un mail para filtrar
     *     - RESPONSE_SEND        si la respuesta no requiere procesamiento
     *     - ERROR                ante cualquier error (IO/parseo)
     */
    RESPONSE_RECV,

    /**
     * Envia la respuesta del servidor origen al cliente
     * 
     * Transiciones:
     *    - RESPONSE_SEND    mientras el mensaje no se haya enviado por completo
     *    - REQUEST          una vez finalizado el envio del mensaje
     *    - ERROR            ante cualquier error (IO/parseo)
     */
    RESPONSE_SEND,

    /**
     * envia el contenido del mail recivido al filtro para ser procesado
     *
     * Transiciones:
     *   - RESPONSE_SEND  una vez que el mensaje haya sido filtrado
     *   - ERROR        ante error durante la ejecucion/conexion con de filtro.
     */
    FILTER,

    /**
     * Copia bytes entre client_fd y origin_fd.
     *
     * Intereses: (tanto para client_fd y origin_fd)
     *   - OP_READ  si hay espacio para escribir en el buffer de lectura
     *   - OP_WRITE si hay bytes para leer en el buffer de escritura
     *
     * Transicion:
     *   - DONE     cuando no queda nada mas por copiar.
     */
    COPY,

    // estados terminales
    DONE,
    ERROR,
};