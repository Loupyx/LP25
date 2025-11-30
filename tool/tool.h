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

void destoy_char(char *line[]);

#endif