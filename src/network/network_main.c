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
                        if (strcmp(word, "SSH")) {
                            new->connexion_type = SSH;
                        } else if (strcmp(word, "TELNET")) {
                            new->connexion_type = TELNET;
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
            printf("ligne invalide, champs lus = %d\n", cpt_word);
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

server *create_serve_arg(int port, int type, char *serv, char *user, char *password) {
    server *new = (server*)malloc(sizeof(server));
    if (!new) {
        return NULL;
    }

    if (type == LOCAL) {
        new->adresse = "localhost";
        new->connexion_type = LOCAL;
        new->name = "MonPC";
        new->password = NULL;
        new->port = -1;
        new->username = NULL;
        return new;
    }

    new->port = port;
    new->connexion_type = type;

    if (port == -1) {
        switch (type) {
            case SSH:
                new->port = 22;
                break;

            case TELNET:
                new->port = 23;
                break;
            
            default:
                free(new);
                return NULL;
        }
    }

    new->adresse = (char*)malloc(strlen(serv)+1);
    strcpy(new->adresse, serv);
    new->name = (char*)malloc(strlen(serv)+1);
    strcpy(new->name, serv);
    new->username = (char*)malloc(strlen(user)+1);
    strcpy(new->username, user);
    new->password = (char*)malloc(strlen(password)+1);
    strcpy(new->password, password);
    return new;
}

void print_list_serv(list_serv l) {
    list_serv temp = l;
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
        free(serv);
    }
}