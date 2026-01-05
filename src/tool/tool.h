#ifndef TOOL_H
#define TOOL_H

#include "./../network/network_SSH.h"

/**
 * Lit le contenu d'un fichier et le retourne sous forme de chaîne.
 *
 * Ouvre le fichier indiqué, lit son contenu en mémoire et renvoie
 * un buffer terminé par '\0'. La mémoire doit être libérée par l'appelant.
 *
 * \param path Chemin vers le fichier à lire.
 * \return Pointeur vers le buffer contenant le fichier, ou NULL en cas d'erreur.
 */
char *get_char_file(char *path);

/**
 * Récupère la liste des sous-répertoires d'un chemin donné.
 *
 * \param path Chemin du répertoire à explorer.
 * \return Tableau de chaînes (terminé par NULL) contenant les noms des
 *         sous-répertoires, ou NULL en cas d'erreur.
 */
char **get_list_dirs(const char *path);

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
 * Libère la mémoire associée à une liste de chaînes allouées par get_ssh_dir.
 *
 * Parcourt le tableau, libère chaque chaîne puis le tableau lui‑même.
 *
 * \param list Tableau de chaînes à libérer (peut être NULL).
 */
void free_ssh_dir(char **list);

char *get_char_telnet();

/**
 * Découpe une chaîne en tokens selon des délimiteurs donnés.
 *
 * La fonction sépare la chaîne fournie en sous-chaînes, en utilisant
 * les caractères de delim comme séparateurs, et renvoie un tableau
 * de chaînes terminé par un pointeur NULL.
 *
 * \param line  Chaîne à découper (peut être modifiée).
 * \param delim Le caractère délimiteur.
 * \return Tableau de chaînes (char **), terminé par NULL, ou NULL en cas d'erreur.
 */
char **split(char *line, char delim);

/**
 * Vérifie si une chaîne représente un nombre entier valide.
 *
 * \param s Chaîne à analyser.
 * \return 1 si la chaîne représente un nombre, 0 sinon.
 */
int is_number(const char *s);

/**
 * Affiche un tableau de chaînes de caractères.
 *
 * Parcourt le tableau de chaînes terminé par un pointeur NULL
 * et affiche chaque chaîne sur une ligne séparée.
 *
 * \param tab Tableau de chaînes de caractères à afficher (terminé par NULL).
 */
void print_str_array(char **tab);

/**
 * Libère un tableau de chaînes de caractères.
 *
 * Parcourt le tableau de chaînes terminé par un pointeur NULL
 * et libère chaque élément, puis le tableau lui‑même si nécessaire.
 *
 * \param line Tableau de chaînes de caractères à détruire.
 */
void destoy_char(char *line[]);

/**
 * Écrit un message formaté dans le journal d'exécution.
 *
 * Utilise une interface de type printf avec arguments variadiques pour
 * enregistrer des informations (erreurs, actions, traces) dans un log.
 *
 * \param text Format de la chaîne (comme pour printf), suivi des paramètres variadiques.
 * \return 0 en cas de succès, une valeur non nulle en cas d'erreur d'écriture.
 */
int write_log(const char *text, ...);

#endif