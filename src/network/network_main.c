#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network_main.h"
#include "./../process/Processus.h"
#include "./../tool/tool.h"

list_serv add_queue(list_serv list, server *serv) {     
    maillon *new = (maillon*)malloc(sizeof(maillon));
    if (!new) { 
        return NULL;
    }
    new->next = NULL;
    new->prev = NULL;
    new->serv = serv;
    if (!list) {
        return new;
    }
    if (!list->next) {
        list->next = new;
        new->prev = list;
        return list;
    }

    maillon *temp = list->next;
    while (temp->next) {
        temp = temp->next;
    }
    temp->next = new;
    new->prev = temp;
    return list;
}

list_serv get_serveur_config(char *path, int *error) {
    FILE *file = NULL;
    list_serv list = NULL;
    server *new = NULL;
    char line[CHAR_SIZE];
    int n;

    if (!path || strcmp(path,"") == 0 ) {
        path = ".config";
    }
    file = fopen(path, "r");
    if (!file) {
        *error = CANT_OPEN_FILE;
        return NULL;
    }

    while (fgets(line, CHAR_SIZE, file) != NULL) {
        n = strlen(line);

        // enlever le '\n' éventuel
        if (n > 0 && line[n-1] == '\n') {
            line[n-1] = '\0';
            n--;
        }

        new = (server*)malloc(sizeof(server));
        if (!new) {
            *error = CANT_ALLOC_SERV;
            fclose(file);
            return NULL;
        }

        int start = 0;
        int cpt_word = 0;

        for (int i=0; i<=n; i++) {
            if (line[i] == ':' || line[i] == '\0') {
                int size = i - start;
                char *word = (char*)malloc(size + 1);
                if (!word) {
                    *error = CANT_ALLOC_WORD;
                    destroy_server(new);
                    fclose(file);
                    return NULL;
                }

                strncpy(word, line + start, size);
                word[size] = '\0';

                cpt_word++;
                switch (cpt_word) {
                    case 1:
                        new->name = (char*)malloc(size + 1);
                        strcpy(new->name, word);
                        break;
                    case 2:
                        new->adresse = (char*)malloc(size + 1);
                        strcpy(new->adresse, word);
                        break;
                    case 3:
                        new->port = atoi(word);
                        break;
                    case 4:
                        new->username = (char*)malloc(size + 1);
                        strcpy(new->username, word);
                        break;
                    case 5:
                        new->password = (char*)malloc(size + 1);
                        strcpy(new->password, word);
                        break;
                    case 6:
                        if (strcmp(word, "SSH") == 0) {
                            new->connexion_type = SSH;
                        } else if (strcmp(word, "TELNET") == 0) {
                            new->connexion_type = TELNET;
                        } else {
                            new->connexion_type = LOCAL; // valeur par défaut
                        }
                        break;
                    default:
                        break;
                }

                free(word);
                start = i + 1; // prochain mot commence après le ':'
            }
        }

        if (cpt_word != NB_CHAMP) {
            destroy_server(new);
            write_log("ligne invalide, champs lus = %d\n", cpt_word);
            *error = SERVER_SKIPED;
        } else {
            list = add_queue(list, new);
            if (!list) {
                *error = CANT_ALLOC_SERV;
                fclose(file);
                return NULL;
            }
        }
    }

    fclose(file);
    return list;
}

server *create_server(const char *name, const char *adresse, int port, const char *username, const char *password, const char *connexion_type) {

    if ((!name || !adresse || !username || !password) && strcmp(connexion_type, "local") != 0) {
        write_log("create_server: paramètres invalides\n");
        return NULL;
    }

    server *new = (server*)malloc(sizeof(server));
    if (!new) {
        write_log("create_server: échec de l'allocation mémoire\n");
        return NULL;
    }

    new->name = (char*)malloc(strlen(name) + 1);
    if (!new->name) {
        write_log("create_server: échec de l'allocation mémoire pour le nom\n");
        free(new);
        return NULL;
    }
    strcpy(new->name, name);

    new->adresse = (char*)malloc(strlen(adresse) + 1);
    if (!new->adresse) {
        write_log("create_server: échec de l'allocation mémoire pour l'adresse\n");
        free(new->name);
        free(new);
        return NULL;
    }
    strcpy(new->adresse, adresse);

    new->port = port;

    new->username = (char*)malloc(strlen(username) + 1);
    if (!new->username) {
        write_log("create_server: échec de l'allocation mémoire pour le nom d'utilisateur\n");
        free(new->adresse);
        free(new->name);
        free(new);
        return NULL;
    }
    strcpy(new->username, username);

    new->password = (char*)malloc(strlen(password) + 1);
    if (!new->password) {
        write_log("create_server: échec de l'allocation mémoire pour le mot de passe\n");
        free(new->username);
        free(new->adresse);
        free(new->name);
        free(new);
        return NULL;
    }
    strcpy(new->password, password);

    if (connexion_type != NULL) {
        if (strcmp(connexion_type, "ssh") == 0) {
            new->connexion_type = SSH;
            if (port == -1) {
                new->port = 22; // port par défaut SSH
            }
        } else if (strcmp(connexion_type, "telnet") == 0) {
            new->connexion_type = TELNET;
            if (port == -1) {
                new->port = 23; // port par défaut TELNET
            }
        } else if (strcmp(connexion_type, "local") == 0) {
            new->connexion_type = LOCAL;
            new->port = 0; // pas de port pour local
        } else {
            write_log("Type de connexion inconnu : %s", connexion_type);
            free(new->username);
            free(new->adresse);
            free(new->name);
            free(new);
            return NULL;
        }
    }
    new->ssh = NULL;

    return new;
}

void print_list_serv(list_serv l) {
    list_serv temp = l;
    if (!temp) {
        write_log("Liste de serveurs vide.");
        return;
    }
    int i = 1;
    while (temp != NULL) {
        server *s = temp->serv;
        write_log("Serveur %d:", i);
        write_log("  name           : %s", s->name);
        write_log("  adresse        : %s", s->adresse);
        write_log("  port           : %d", s->port);
        write_log("  username       : %s", s->username);
        write_log("  password       : %s", s->password);
        write_log("  connexion_type : %d", s->connexion_type);

        temp = temp->next;
        i++;
    }
}

void destroy_server(server *serv) {
    if (serv) {
        free(serv->name);
        free(serv->adresse);
        free(serv->username);
        free(serv->password);
    }
    free(serv);
}