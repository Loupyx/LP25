#include <libtelnet.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "./../tool/tool.h"
#include "network_telnet.h"

/* Handler d'événements libtelnet */
void telnet_event_handler(telnet_t *telnet, telnet_event_t *ev, void *user_data) {
    telnet_user_data *ud = (telnet_user_data *)user_data;

    switch (ev->type) {
        case TELNET_EV_DATA:
            size_t new_size = ud->size + ev->data.size;
            char *tmp = realloc(ud->buffer, new_size + 1);
            if (!tmp) {
                write_log("ERROR allocation telnet_event_handler");
                return;
            }
            ud->buffer = tmp;
            memcpy(ud->buffer + ud->size, ev->data.buffer, ev->data.size);
            ud->size = new_size;
            ud->buffer[ud->size] = '\0';
            break;

        case TELNET_EV_SEND:
            send(ud->state->sock, ev->data.buffer, ev->data.size, 0);
            break;

        case TELNET_EV_ERROR:
            ud->state->rc = -1;
            write_log("telnet error");
            break;

        default:
            break;
    }
}

telnet_state *init_telnet_session(server *serv) {
    if (!serv || !serv->adresse || serv->port <= 0) {
        write_log("ERROR init_telnet_session : invalid server");
        return NULL;
    }

    telnet_state *s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;

    // Création et connexion du socket TCP
    s->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock < 0) {
        write_log("ERROR init_telnet_session : socket");
        free(s);
        return NULL;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(serv->port);

    if (inet_pton(AF_INET, serv->adresse, &addr.sin_addr) <= 0) {
        write_log("ERROR init_telnet_session : inet_pton");
        close(s->sock);
        free(s);
        return NULL;
    }

    if (connect(s->sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        write_log("ERROR init_telnet_session : connect");
        close(s->sock);
        free(s);
        return NULL;
    }

    // Initialisation de libtelnet
    s->rc = 0;
    s->telnet = telnet_init(telopts, telnet_event_handler, 0, NULL);
    if (!s->telnet) {
        write_log("ERROR init_telnet_session : telnet_init");
        close(s->sock);
        free(s);
        return NULL;
    }

    return s;
}

void destroy_telnet_state(telnet_state *s) {
    if (!s) return;
    if (s->telnet)
        telnet_free(s->telnet);
    if (s->sock >= 0)
        close(s->sock);
    free(s);
}

char *get_char_telnet(telnet_state *state, char *path) {
    //TODO: implement telnet read file
    write_log("get_char_telnet not implemented yet for path: %s", path);
    return NULL;
}

