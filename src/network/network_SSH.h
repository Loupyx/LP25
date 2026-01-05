#ifndef NETWORK_SSH_H
#define NETWORK_SSH_H
#include "network_main.h"

/**
 * Initialise une session SSH/SFTP pour un serveur donné.
 *
 * \param serv  Serveur pour lequel établir la session SSH/SFTP.
 * \return Un tableau de pointeurs sur des structures ssh_state initialisées,
 *         ou NULL en cas d'erreur.
 */
ssh_state *init_ssh_session(server *serv);

/**
 * Lit le contenu d'un fichier distant via une session SSH/SFTP.
 *
 * Utilise l'état SSH fourni pour accéder au fichier distant et renvoie
 * son contenu dans un buffer terminé par '\0'. La mémoire doit être libérée
 * par l'appelant.
 *
 * \param state État de la session SSH/SFTP à utiliser.
 * \param path  Chemin du fichier distant à lire.
 * \return Pointeur vers le buffer contenant le fichier, ou NULL en cas d'erreur.
 */
char *get_char_ssh(ssh_state *state, char *path);

/**
 * Récupère la liste des fichiers et répertoires d'un dossier distant via SSH/SFTP.
 *
 * Ouvre le répertoire indiqué sur le serveur distant, lit les entrées
 * et renvoie un tableau de chaînes terminées par NULL contenant les noms.
 * La mémoire du tableau et des chaînes doit être libérée par l'appelant,
 * par exemple avec free_ssh_dir.
 *
 * \param state État de la session SSH/SFTP à utiliser.
 * \param path  Chemin du répertoire distant à lister.
 * \return Tableau de chaînes (char **), terminé par NULL, ou NULL en cas d'erreur.
 */
char **get_ssh_dir(ssh_state *state, char *path);

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

/**
 * Libère les ressources associées à un état de session SSH/SFTP.
 *
 * \param s Pointeur vers la structure ssh_state à détruire.
 */
void destroy_ssh_state(ssh_state *s);

/**
 * Libère la mémoire associée à une liste de chaînes allouées par get_ssh_dir.
 *
 * Parcourt le tableau, libère chaque chaîne puis le tableau lui‑même.
 *
 * \param list Tableau de chaînes à libérer (peut être NULL).
 */
void free_ssh_dir(char **list);

#endif