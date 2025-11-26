#ifndef NETWORK_SSH_H
#define NETWORK_SSH_H
#include <libssh/libssh.h>
#include <libssh/sftp.h>

/**
 * Codes d'erreur possibles lors du parsing de la configuration serveur.
 */
enum error_parsing {
    CONTINUE,        /**< Aucune erreur bloquante, le parsing peut continuer. */
    CANT_OPEN_FILE,  /**< Impossible d'ouvrir le fichier de configuration. */
    CANT_ALLOC_SERV, /**< Échec d'allocation mémoire pour un serveur. */
    CANT_ALLOC_WORD, /**< Échec d'allocation mémoire pour un mot/chaîne lue. */
    SERVER_SKIPED,   /**< Serveur ignoré, le programme continue mais avertit l'utilisateur. */
};

/**
 * Représente un serveur distant et ses paramètres de connexion.
 */
typedef struct {
    char *name;            /**< Nom du serveur. */
    char *adresse;         /**< Adresse du serveur (IP ou nom de domaine). */
    int   port;            /**< Port utilisé pour la connexion. */
    char *username;        /**< Nom d'utilisateur pour l'authentification. */
    char *password;        /**< Mot de passe pour l'authentification. */
    char *connexion_type;  /**< Type de connexion (par ex. TCP, UDP, SSH...). */
} server;

/**
 * Élément d'une liste chaînée de serveurs.
 *
 * Chaque maillon contient un pointeur vers un serveur et un pointeur
 * vers le maillon suivant de la liste.
 */
typedef struct maillon_s {
    server *serv;            /**< Serveur stocké dans ce maillon. */
    struct maillon_s *next;  /**< Pointeur vers le maillon suivant. */
    struct maillon_s *prev; /**< Pointuer vers le maillon précedent */
} maillon;

/**
 * Liste chaînée de serveurs.
 *
 * Représentée par un pointeur vers le premier maillon de la liste,
 * ou NULL si la liste est vide.
 */
typedef maillon *list_serv;

/**
 * Lit un fichier de configuration et construit la liste de serveurs.
 *
 * \param path Chemin vers le fichier de configuration.
 * \param error Code d'erreur retourné (0 si succès, autre valeur si erreur).
 * \return La liste de serveurs lue dans le fichier.
 */
list_serv get_serveur_config(char *path, int *error);

/**
 * Affiche un message d'erreur formaté sur la sortie appropriée.
 *
 * \param mess Message d'erreur à afficher.
 */
void print_error(char mess[]);

/**
 * Affiche le contenu d'une liste de serveurs.
 *
 * \param l Liste de serveurs à afficher.
 */
void print_list_serv(list_serv l);

/**
 * Libère toute la mémoire associée à un serveur.
 *
 * \param serv Pointeur vers la structure Server à détruire.
 */
void destroy_server(server *serv);

/**
 * Représente l'état d'une session SSH/SFTP.
 *
 * Contient les objets nécessaires pour gérer une connexion SSH et SFTP
 * associée, ainsi que le code de retour des opérations.
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