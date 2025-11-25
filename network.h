#ifndef NETWORK_H
#define NETWORK_H

enum error_network {
    CONTINUE,
    CANT_OPEN_FILE,
    CANT_ALLOC_SERV,
    CANT_ALLOC_WORD,
    SERVER_SKIPED, //tourne toujours mais avertie le user
};

typedef struct {
    char *name;
    char *adresse;
    char *port;
    char *username;
    char *password;
    char *connexion_type;
}server;

typedef struct maillon_s {
    server *serv;
    struct maillon_s *next;
}maillon;

typedef maillon *list_serv;

/**
 * Lit un fichier de configuration et construit la liste de serveurs.
 *
 * \param path Chemin vers le fichier de configuration.
 * \param error Code d'erreur retourné (0 si succès, autre valeur si erreur).
 * \return La liste de serveurs lue dans le fichier.
 */
list_serv get_serveur_Config(char *path, int *error);

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

#endif