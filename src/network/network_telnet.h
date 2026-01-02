#ifndef NETWORK_TELNET_H
#define NETWORK_TELNET_H

#include "libtelnet.h"
#include "network_main.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/**
 * Représente l'état d'une session Telnet.
 *
 * Contient le socket TCP sous-jacent et l'état du protocole Telnet
 * géré par libtelnet.
 */
typedef struct {
    int       sock;      /**< Descripteur de socket connecté au serveur Telnet. */
    telnet_t *telnet;    /**< Contexte libtelnet pour cette connexion. */
    int       rc;        /**< Dernier code de retour / erreur. */
} telnet_state;

typedef struct {
    telnet_state *state;  /**< Pointeur vers l'état Telnet courant. */
    char         *buffer; /**< Buffer pour accumuler les données reçues. */
    size_t        size;   /**< Taille actuelle des données dans buffer. */
} telnet_user_data;

static const telnet_telopt_t telopts[] = {
    { TELNET_TELOPT_ECHO,      TELNET_WONT, TELNET_DO  },
    { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2, TELNET_WONT, TELNET_DONT },
    { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DONT },
    { -1, 0, 0 }  /* fin du tableau */
};

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

#endif