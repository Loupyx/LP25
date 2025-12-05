#ifndef NETWORK_H
#define NETWORK_H

#define CHAR_SIZE 256
#define NB_CHAMP 6

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
 * Lit un fichier de configuration et construit la liste de serveurs.
 *
 * \param path c'est le chemin vers le fichier de configuration.
 * \param error Code d'erreur retourné (0 si succès, autre valeur si erreur).
 * \return La liste de serveurs lu dans le fichier.
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