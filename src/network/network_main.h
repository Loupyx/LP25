#ifndef NETWORK_H
#define NETWORK_H

#define CHAR_SIZE 256
#define NB_CHAMP 6

#include <libssh/libssh.h>
#include <libssh/sftp.h>

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
 * Codes d'erreurs possibles lors du parsing de la configuration serveur.
 */
enum error_parsing {
    CONTINUE,        /**< Aucune erreur bloquante, le parsing peut continuer. */
    CANT_OPEN_FILE,  /**< Impossible d'ouvrir le fichier de configuration. */
    CANT_ALLOC_SERV, /**< Échec d'allocation mémoire pour un serveur. */
    CANT_ALLOC_WORD, /**< Échec d'allocation mémoire pour un mot/chaîne lu. */
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
    int connexion_type;  /**< Type de connexion (par ex. TELNET SSH...). */
    ssh_state *ssh;      /**< État de connexion SSH, si applicable. */

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
    struct maillon_s *prev; /**< Pointeur vers le maillon précédent */
} maillon;

/**
 * Liste chaînée de serveurs.
 *
 * Représentée par un pointeur vers le premier maillon de la liste,
 * ou NULL si la liste est vide.
 */
typedef maillon *list_serv;

/**
 * Ajoute un serveur à la fin de la liste chaînée de serveurs.
 *
 * Insère le serveur donné en queue de liste et renvoie la tête
 * (éventuellement mise à jour si la liste était vide).
 *
 * \param list Tête actuelle de la liste de serveurs (peut être NULL).
 * \param serv Pointeur vers le serveur à ajouter.
 * \return Nouvelle tête de la liste de serveurs.
 */
list_serv add_queue(list_serv list, server *serv);

/**
 * Crée et initialise une structure server avec les paramètres fournis.
 *
 * Alloue dynamiquement un serveur et renseigne son nom, son adresse,
 * son port, ses identifiants et le type de connexion.
 * La structure retournée doit être libérée par l'appelant.
 *
 * \param name            Nom logique du serveur.
 * \param adresse         Adresse du serveur (IP ou nom de domaine).
 * \param port            Port utilisé pour la connexion.
 * \param username        Nom d'utilisateur pour l'authentification.
 * \param password        Mot de passe pour l'authentification.
 * \param connexion_type  Type de connexion (par exemple "SSH", "LOCAL", "TELNET").
 * \return Pointeur vers la structure server créée, ou NULL en cas d'erreur.
 */
server *create_server(const char *name, const char *adresse, int port, const char *username, const char *password, const char *connexion_type);

/**
 * Lit un fichier de configuration et construit la liste de serveurs.
 *
 * \param path c'est le chemin vers le fichier de configuration.
 * \param error Code d'erreur retourné (0 si succès, autre valeur si erreur).
 * \return La liste de serveurs lu dans le fichier.
 */
list_serv get_serveur_config(char *path, int *error);

/**
 * Affiche le contenu d'une liste de serveurs.
 *
 * \param l Liste des serveurs à afficher.
 */
void print_list_serv(list_serv l);

/**
 * Libère toute la mémoire associée à un serveur.
 *
 * \param serv Pointeur vers la structure Server à supprimer.
 */
void destroy_server(server *serv);

#endif