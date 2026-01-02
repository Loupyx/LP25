#ifndef NETWORK_TELNET_H
#define NETWORK_TELNET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "libtelnet.h"
#include "network_main.h"

static const telnet_telopt_t telopts[] = {
    { TELNET_TELOPT_ECHO,      TELNET_WONT, TELNET_DO  },
    { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DONT },
    { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DONT },
    { -1, 0, 0 }  /* fin du tableau */
};

// network_telnet_types.h (par exemple)
typedef struct telnet_state {
    int sock;
    int rc;
    telnet_t *telnet;
} telnet_state;

typedef struct telnet_user_data {
    telnet_state *state;
    char *buffer;
    size_t size;
} telnet_user_data;


void telnet_event_handler(telnet_t *telnet, telnet_event_t *ev, void *user_data);

/**
 * Initialise une session Telnet vers un serveur donné.
 *
 * \param serv  Serveur pour lequel établir la session Telnet.
 * \return Un pointeur sur une structure telnet_state initialisée,
 *         ou NULL en cas d'erreur.
 */
telnet_state *init_telnet_session(server *serv);

/**
 * Libère les ressources associées à un état de session Telnet.
 *
 * \param s Pointeur vers la structure telnet_state à détruire.
 */
void destroy_telnet_state(telnet_state *s);

/**
 * Envoie des données au serveur Telnet.
 *
 * \param state  État de session Telnet valide.
 * \param buf    Données à envoyer.
 * \param len    Taille des données à envoyer.
 * \return 0 en cas de succès, !=0 en cas d'erreur.
 */
int telnet_send_data(telnet_state *state, const char *buf, size_t len);

/**
 * Reçoit et traite des données en provenance du serveur Telnet.
 *
 * \param state  État de session Telnet valide.
 * \return 0 en cas de succès, !=0 en cas d'erreur ou de fermeture.
 */
int telnet_recv_data(telnet_state *state);

/* Lit le fichier donné sur la machine distante via Telnet et renvoie son contenu (alloué) */
char *get_char_telnet(telnet_state *state, char *path);

#endif