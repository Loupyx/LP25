#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>

#include "./../network/network_SSH.h"
#include "tool.h"

// LOCAL (Fonctions d'accès aux fichiers locaux)
char *get_char_file(char *path) {
    if (!path) {
        return NULL;
    }
    FILE *file;
    char *text = NULL, c;
    int size = 0, ic;
    file = fopen(path, "r");
    if (!file) {
        printf("Erreur d'ouverture du fichier (fopen)\n");
        return NULL;
    }
    // Lit le fichier caractère par caractère, ignorant les retours à la ligne
    while ((ic = fgetc(file)) != EOF) {
        c = (char)ic;
        if (c != '\n') {
            char *temp = (char*)realloc(text, (size + 2) * sizeof(char));
            if (!temp) {
                free(text);
                fclose(file);
                return NULL;
            }
            text = temp;
            text[size++] = c;
        }
    }

    if (!text) {
        text = malloc(1);
        fclose(file);
        if (!text) return NULL;
        text[0] = '\0';
        return text;
    }

    fclose(file);

    text[size] = '\0';
    return text;
}

char **get_list_dirs(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char **names = NULL;
    int size, nb_dir = 0;
    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "erreur opendir\n");
        return NULL;
    }
    while ((entry = readdir(dir)) != NULL) {
        // Ignore les entrées '.' et '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Filtre pour les répertoires dont le nom est composé uniquement de chiffres (PID)
        if (entry->d_type == DT_DIR && is_number(entry->d_name) == 1) {
            size = strlen(entry->d_name);
            char *word = malloc(size + 1);
            if (!word) {
                return NULL;
            }
            strcpy(word, entry->d_name);
            // Réalloue l'espace pour le nouveau nom de répertoire + un pointeur NULL final
            char **tmp = realloc(names, (nb_dir + 2) * sizeof *names);
            if (!tmp) {
                free(word);
                return NULL;
            }
            names = tmp;
            names[nb_dir++] = word;
            names[nb_dir] = NULL;
        }
    }
    closedir(dir);
    return names;

}

// SSH (Fonctions d'accès aux fichiers distants via SSH)
char *get_char_ssh(ssh_state *state, char *path) {
    char command[256];
    if (!path) {
        fprintf(stderr, "get_char_ssh : chemin invalide\n");
        return NULL;
    }

    if (!state) {
        fprintf(stderr, "get_char_ssh : état de session SSH invalide\n");
        return NULL;
    }

    ssh_channel chan = ssh_channel_new(state->session);
    if (!chan) {
        fprintf(stderr, "get_char_ssh : échec de création du canal\n");
        return NULL;
    }

    if (ssh_channel_open_session(chan) != SSH_OK) {
        ssh_channel_free(chan);
        fprintf(stderr, "get_char_ssh : échec de l'ouverture de session du canal\n");
        return NULL;
    }

    // Exécute la commande 'cat' pour lire le contenu du fichier
    snprintf(command, sizeof(command), "cat %s", path);
    if (ssh_channel_request_exec(chan, command) != SSH_OK) {
        ssh_channel_close(chan);
        ssh_channel_free(chan);
        fprintf(stderr, "get_char_ssh : échec de l'exécution de 'cat'\n");
        return NULL;
    }

    char *text = NULL, c;
    int size = 0;
    int n;
    // Lit la sortie de la commande, ignorant les retours à la ligne
    while ((n = ssh_channel_read(chan, &c, 1, 0)) > 0) {
        if (c != '\n') {
            char *temp = (char*)realloc(text, (size + 2) * sizeof(char));
            if (!temp) {
                free(text);
                ssh_channel_send_eof(chan);
                ssh_channel_close(chan);
                ssh_channel_free(chan);
                return NULL;
            }
            text = temp;
            text[size++] = c;
        }
    }

    // Nettoyage et fermeture du canal
    if (text) {
        text[size] = '\0';
    } else {
        text = malloc(1);
        if (text) text[0] = '\0';
    }

    ssh_channel_send_eof(chan);
    ssh_channel_close(chan);
    ssh_channel_free(chan);
    return text;
}

char **get_ssh_dir(ssh_state *state, char *path) {
    if (!state || !state->session || !path) {
        fprintf(stderr, "get_ssh_dir: arguments invalides\n");
        return NULL;
    }

    sftp_session sftp = sftp_new(state->session);
    if (!sftp) {
        fprintf(stderr, "get_ssh_dir: sftp_new a échoué\n");
        return NULL;
    }

    if (sftp_init(sftp) != SSH_OK) {
        fprintf(stderr, "get_ssh_dir: sftp_init a échoué: %s\n", ssh_get_error(state->session));
        sftp_free(sftp);
        return NULL;
    }

    sftp_dir dir = sftp_opendir(sftp, path);
    if (!dir) {
        fprintf(stderr, "get_ssh_dir: sftp_opendir('%s') a échoué: %s\n", path, ssh_get_error(state->session));
        sftp_free(sftp);
        return NULL;
    }

    char **res = NULL;
    int size, nb_dir = 0;

    // Lecture des entrées du répertoire
    while (!sftp_dir_eof(dir)) {
        sftp_attributes attrs = sftp_readdir(sftp, dir);
        if (!attrs) {
            continue;
        }
        // Vérifie si le nom de l'entrée est un nombre (potentiellement un PID)
        if (is_number(attrs->name)) {
            char **temp = (char**)realloc(res, (nb_dir + 1) * sizeof(char*));
            if (!temp) {
                sftp_attributes_free(attrs);
                free_ssh_dir(res);
                sftp_closedir(dir);
                sftp_free(sftp);
                return NULL;
            }
            res = temp;

            size = strlen(attrs->name);
            res[nb_dir] = (char*)malloc(sizeof(char) * (size + 1));
            if (!res[nb_dir]) {
                fprintf(stderr, "get_ssh_dir: échec de l'allocation mémoire\n");
                sftp_attributes_free(attrs);
                free_ssh_dir(res);
                sftp_closedir(dir);
                sftp_free(sftp);
                return NULL;
            }

            memcpy(res[nb_dir], attrs->name, size + 1);
            nb_dir++;
        }
        sftp_attributes_free(attrs); // Libère les attributs après utilisation
    }

    // Réalloue une dernière fois pour ajouter le pointeur NULL de fin de tableau
    char **tmp = realloc(res, (nb_dir + 1) * sizeof(char *));
    if (!tmp && nb_dir > 0) {
        // Cas très rare, mais on gère proprement
        fprintf(stderr, "get_ssh_dir: réallocation finale a échoué\n");
        free_ssh_dir(res);
        sftp_closedir(dir);
        sftp_free(sftp);
        return NULL;
    }
    res = tmp;
    res[nb_dir] = NULL;
    sftp_closedir(dir);
    sftp_free(sftp);
    return res;
}

// Libère la mémoire allouée pour le tableau de chaînes de caractères retourné par get_ssh_dir/get_list_dirs
void free_ssh_dir(char **list) {
    if (!list) return;
    for (int i=0; list[i]!=NULL; ++i) {
        free(list[i]);
    }
    free(list);
}

// TELNET (Fonctions d'accès aux fichiers distants via Telnet - À implémenter)
char *get_char_telnet(){
    return "à faire"; // Indique que l'implémentation est en attente
}

// AUTRE (Fonctions utilitaires diverses)
// Sépare une chaîne de caractères en fonction d'un délimiteur, en ignorant les délimiteurs entre parenthèses
char **split(char *line, char delim) {
    if (!line) {
        fprintf(stderr, "DEBUG: split a reçu une ligne NULL\n");
        return NULL;
    }
    int n = strlen(line);
    char **res = NULL;
    int nb_word = 0;
    int start = 0;
    int parenthese = 0;

    for (int i = 0; i <= n; ++i) {
        if (line[i] == '(') {
            parenthese = 1;
        } else if (line[i] == ')') {
            parenthese = 0;
        }
        // Séparation si délimiteur, fin de ligne, ou fin de chaîne, et pas dans des parenthèses
        if ((line[i] == delim || line[i] == '\n' || line[i] == '\0') && parenthese == 0) {
            int size = i - start;
            if (size > 0) {
                char *word = malloc(size + 1);
                if (!word) {
                    destoy_char(res);
                    return NULL;
                }
                memcpy(word, line + start, size);
                word[size] = '\0';
                // Réalloue l'espace pour le nouveau mot + un pointeur NULL final
                char **tmp = realloc(res, (nb_word + 2) * sizeof *res);
                if (!tmp) {
                    destoy_char(tmp);
                    destoy_char(res);
                    free(word);
                    return NULL;
                }
                res = tmp;
                res[nb_word] = word;
                nb_word++;
                res[nb_word] = NULL;
            }
            start = i + 1;
        }
    }
    return res;
}

// Libère la mémoire d'un tableau de chaînes de caractères (char**)
void destoy_char(char *line[]){
    if (!line) {
        return;
    }
    for (int i=0; line[i]; i++) {
        free(line[i]);
    }
    free(line);
}

// Vérifie si une chaîne de caractères est composée uniquement de chiffres
int is_number(const char *s) {
    for (size_t i = 0; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) {
            return 0; // Contient un caractère non numérique
        }
    }
    return 1; // Contient uniquement des chiffres
}

// Affiche un tableau de chaînes de caractères pour le débogage
void print_str_array(char **tab) {
    if (!tab){
        return;
    }
    for (int i=0; tab[i]; i++) {
        printf("|%d|%s\n",i, tab[i]);
    }
}