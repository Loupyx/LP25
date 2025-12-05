#ifndef NETWORK_SSH_H
#define NETWORK_SSH_H
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include "network_main.h"

/**
 * Représente l'état d'une session SSH/SFTP.
 *
 * Contient les objets nécessaires pour gérer une connexion SSH et SFTP
 * associée mais aussi le code de retour des opérations.
 */
typedef struct {
    ssh_session     session;  /**< Session SSH associée au serveur. */
    sftp_session    sftp;     /**< Session SFTP associée à la session SSH. */
    sftp_dir        dir;      /**< Répertoire SFTP courant ouvert. */
    sftp_attributes attr;     /**< Attributs du fichier ou répertoire courant. */
    int             rc;       /**< Code de retour des dernières opérations. */
} ssh_state;

/**
 * Initialise une session SSH/SFTP pour un serveur donné.
 *
 * \param serv  Serveur pour lequel établir la session SSH/SFTP.
 * \return Un tableau de pointeurs sur des structures ssh_state initialisées,
 *         ou NULL en cas d'erreur.
 */
ssh_state *init_ssh_session(server *serv);

/**
 * Libère les ressources associées à un état de session SSH/SFTP.
 *
 * \param s Pointeur vers la structure ssh_state à détruire.
 */
void destroy_ssh_state(ssh_state *s);

/**
 * Ouvre un répertoire SFTP à partir d'un état de session SSH.
 * Le dossier ouvert sera /proc
 *
 * \param state État de session SSH/SFTP contenant une session valide.
 * \return 0 en cas de succès et 1 si impossible d'ouvrir le fichier.
 */
int open_dir_ssh(ssh_state *state);

/**
 * Ferme le répertoire SFTP actuellement ouvert dans l'état SSH donné.
 *
 * \param state État de session SSH/SFTP contenant le répertoire à fermer.
 * \return 0 en cas de succès, 1 si echec à fermer le répertoire
 */
int close_dir_ssh(ssh_state *state);

#endif