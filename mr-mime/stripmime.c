#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#include "parser_utils.h"
#include "pop3_multi.h"
#include "mime_chars.h"
#include "mime_msg.h"

/*
 * imprime información de debuging sobre un evento.
 *
 * @param p        prefijo (8 caracteres)
 * @param namefnc  obtiene el nombre de un tipo de evento
 * @param e        evento que se quiere imprimir
 */
static void
debug(const char *p,
      const char * (*namefnc)(unsigned),
      const struct parser_event* e) {
    // DEBUG: imprime
    if (e->n == 0) {
        fprintf(stderr, "%-8s: %-14s\n", p, namefnc(e->type));
    } else {
        for (int j = 0; j < e->n; j++) {
            const char* name = (j == 0) ? namefnc(e->type)
                                        : "";
            if (e->data[j] <= ' ') {
                fprintf(stderr, "%-8s: %-14s 0x%02X\n", p, name, e->data[j]);
            } else {
                fprintf(stderr, "%-8s: %-14s %c\n", p, name, e->data[j]);
            }
        }
    }
}

/* mantiene el estado durante el parseo */
struct ctx {
    /* delimitador respuesta multi-línea POP3 */
    struct parser* multi;
    /* delimitador mensaje "tipo-rfc 822" */
    struct parser* msg;
    /* detector de field-name "Content-Type" */
    struct parser* ctype_header;
    /* detector de field-name "Content-Transfer" */
    struct parser* ctransfer_header;
    /* tokenizador de content-type value */
    struct parser* ctype_value;
    /* detector de boundary name ("boundary=") */
    struct parser* boundary_name;
    /* parser de body */
    struct parser* body;
    /* detector de boundary key (la ultima guardada)*/
    struct parser* boundary_key;
    /* detector de boundary border */
    struct parser* boundary_border;
    /* detector del final de boundary border.
     * Diferencia si es un border comun, un 
     * final de boundary o si no es valido
     */
    struct parser* boundary_border_end;

    /* ¿hemos detectado si el field-name que estamos procesando refiere
     * a Content-Type?. Utilizando dentro msg para los field-name.
     */
    bool *msg_content_type_field_detected;
    /* ¿hemos terminado de guardar el value del 
     * content-type?
     */
    bool *msg_content_type_value_stored;
    /* ¿hemos detectado dentro del Content-Type en el que estamos
     * a "boundary="?
     */
    bool *msg_boundary_name_detected;
    /* ¿hemos terminado de guardar el value del 
     * boundary?
     */
    bool *msg_boundary_key_stored;
    /* ¿hemos detectado el boundary border?
     */
    bool *msg_boundary_border_detected;
    /* ¿es un mime a filtrar el valor actual?
     */
    bool *filter_curr_mime;
    /* ¿es un multipart el mime actual?
     */
    bool *multipart_curr_mime;
    /* ¿es un message/rfc822 el mime actual?
     */
    bool *message_curr_mime;
    /* content-type actual */
    char *content_type;
    /* stack de boundaries */
    char **boundaries;
    /* cantidad de boundaries */
    int boundaries_n;
    /* mensaje de reemplazo */
    char *filter_msg;

    char *blocked_type;
};

static bool T = true;
static bool F = false;

bool is_blocked_type(struct ctx *ctx) {
    char *blocked_type = ctx->blocked_type;
    bool eq = true;
    int slash_pos = -1;
    for(int j=0; j < strlen(blocked_type); j++) {
        if(j > 0) {
            if(blocked_type[j] == '/' && slash_pos < 0) {
                slash_pos = j;
            } else if(blocked_type[j] == '*' && slash_pos == j-1 && j == strlen(blocked_type)-1) {
                return true;
            }
        }
        if(tolower(ctx->content_type[j]) != tolower(blocked_type[j])) {
            eq = false;
            break;
        }
    }
    if(eq && strlen(black) == strlen(ctx->content_type)) {
        return true;
    }
    return false;
}

bool is_multipart(char *content_type) {
    char multi[] = "multipart/";
    for(int i=0; i<strlen(multi); i++) {
        if(multi[i] != tolower(content_type[i])) {
            return false;
        }
    }
    return true;
}

bool is_message(char *content_type) {
    return strcmp(tolower(content_type[j]), "message/rfc822") == 0;
}

/* Detecta si un header-field-name equivale a Content-Type.
 * Deja el valor en `ctx->msg_content_type_field_detected'. Tres valores
 * posibles: NULL (no tenemos información suficiente todavia, por ejemplo
 * viene diciendo Conten), true si matchea, false si no matchea.
 */
static void
content_type_header(struct ctx *ctx, const uint8_t c) {
    const struct parser_event* e = parser_feed(ctx->ctype_header, c);
    do {
        debug("2.typehr", parser_utils_strcmpi_event, e);
        switch(e->type) {
            case STRING_CMP_EQ:
                ctx->msg_content_type_field_detected = &T;
                break;
            case STRING_CMP_NEQ:
                ctx->msg_content_type_field_detected = &F;
                break;
        }
        e = e->next;
    } while (e != NULL);
}

/**
 * Guarda el content-type de la data por venir en el mensaje.
 * El valor se guarda en ctx->content_type
 */
static void
content_type_value(struct ctx *ctx, const uint8_t c) {
    const struct parser_event* e = parser_feed(ctx->ctype_value, c);
    do {
        debug("2.    ctypeval", mime_value_event, e);
        switch(e->type) {
            case VALUE:
                nappend(ctx->content_type, c, MAX_STRING_LENGTH);
                ctx->msg_content_type_value_stored = NULL;
                break;
            case VALUE_END:
                fprintf(stderr, "ctype: %s\n",ctx->content_type);
                ctx->msg_content_type_value_stored = &T;
                if(to_is_blocked_type(ctx)) {
                    printf(": text/plain");
                    ctx->filter_curr_mime = &T;
                } else {
                    printf(": %s", ctx->content_type);
                    ctx->filter_curr_mime = &F;
                }
                ctx->multipart_curr_mime = is_multipart(ctx->content_type)? &T : &F;
                ctx->message_curr_mime = is_message(ctx->content_type)? &T : &F;
                break;
            case WAIT:
                break;
            case UNEXPECTED:
                ctx->msg_content_type_value_stored = &F;
                break;
        }
        e = e->next;
    } while (e != NULL);
}

/**
 * Procesa un mensaje `tipo-rfc822'.
 * Si reconoce un al field-header-name Content-Type lo interpreta.
 *
 */
static void
mime_msg(struct ctx *ctx, const uint8_t c) {
    const struct parser_event* e = parser_feed(ctx->msg, c);

    do {
        debug("1.   msg", mime_msg_event, e);
        switch(e->type) {
            case MIME_MSG_NAME:
                if( ctx->msg_content_type_field_detected == 0
                || *ctx->msg_content_type_field_detected) {
                    for(int i = 0; i < e->n; i++) {
                        content_type_header(ctx, e->data[i]);
                    }
                }
                if(c == '\n') {
                    parser_reset(ctx->ctype_header);
                    ctx->msg_content_type_field_detected     = NULL;
                }
                break;
            case MIME_MSG_NAME_END:
                // lo dejamos listo para el próximo header
                parser_reset(ctx->ctype_header);
                //ctx->msg_content_type_field_detected = NULL;
                // if (ctx->msg_content_type_field_detected != NULL
                //  && *ctx->msg_content_type_field_detected){
                //     ctx->print_curr_char = &F;
                //  }
                break;
            case MIME_MSG_VALUE:
                if(ctx->msg_content_type_field_detected != 0
                && ctx->msg_content_type_field_detected) {
                    if( ctx->msg_content_type_value_stored == NULL) {
                        for(int i = 0; i < e->n; i++) {
                            content_type_value(ctx, e->data[i]);
                        }
                    } else if(*ctx->multipart_curr_mime){
                        if(ctx->msg_boundary_name_detected == NULL) {
                            for(int i = 0; i < e->n; i++) {
                               boundary_name(ctx, e->data[i]);
                            }    
                        } else if(*ctx->msg_boundary_name_detected && 
                                   ctx->msg_boundary_key_stored == NULL){
                            for(int i = 0; i < e->n; i++) {
                                boundary_key(ctx, e->data[i]);
                            }
                        }
                    }
                }
                break;
            case MIME_MSG_VALUE_END:
                // si parseabamos Content-Type ya terminamos
                ctx->msg_content_type_field_detected = 0;
                break;
        }
        e = e->next;
    } while (e != NULL);
}

/* Delimita una respuesta multi-línea POP3. Se encarga del "byte-stuffing" */
static void
pop3_multi(struct ctx *ctx, const uint8_t c) {
    const struct parser_event* e = parser_feed(ctx->multi, c);
    do {
        debug("0. multi", pop3_multi_event,  e);
        switch (e->type) {
            case POP3_MULTI_BYTE:
                for(int i = 0; i < e->n; i++) {
                    mime_msg(ctx, e->data[i]);
                }
                break;
            case POP3_MULTI_WAIT:
                // nada para hacer mas que esperar
                break;
            case POP3_MULTI_FIN:
                // arrancamos de vuelta
                parser_reset(ctx->msg);
                ctx->msg_content_type_field_detected = NULL;
                break;
        }
        e = e->next;
    } while (e != NULL);
}

int
main(const int argc, const char **argv) {
    int fd = STDIN_FILENO;
    if(argc > 1) {
        fd = open(argv[1], 0);
        if(fd == -1) {
            perror("opening file");
            return 1;
        }
    }

    const unsigned int* no_class = parser_no_classes();
    struct parser_definition media_header_def =
            parser_utils_strcmpi("content-type");

    struct parser_definition boundary_name_def =
            parser_utils_strcmpi_ignore_lwsp("boundary=\"");

    struct ctx ctx = {
        .multi        = parser_init(no_class, pop3_multi_parser()),
        .msg          = parser_init(init_char_class(), mime_message_parser()),
        .ctype_header = parser_init(no_class, &media_header_def),
        .boundary_name     = parser_init(init_char_class(), &boundary_name_def),

    };

    uint8_t data[4096];
    int n;
    do {
        n = read(fd, data, sizeof(data));
        for(ssize_t i = 0; i < n ; i++) {
            pop3_multi(&ctx, data[i]);
        }
    } while(n > 0);

    parser_destroy(ctx.multi);
    parser_destroy(ctx.msg);
    parser_destroy(ctx.ctype_header);
    parser_utils_strcmpi_destroy(&media_header_def);
}
