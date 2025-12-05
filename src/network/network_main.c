#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network_main.h"

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
                        new->connexion_type = (char*)malloc(size + 1);
                        strcpy(new->connexion_type, word);
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

void print_error(char mess[]) {
    printf("%s\n", mess);
    fflush(stdout);
}

void print_list_serv(list_serv l) {
    list_serv temp = l;
    int i = 1;
    while (temp != NULL) {
        server *s = temp->serv;
        printf("Serveur %d:\n", i);
        printf("  name           : %s\n", s->name);
        printf("  adresse        : %s\n", s->adresse);
        printf("  port           : %d\n", s->port);
        printf("  username       : %s\n", s->username);
        printf("  password       : %s\n", s->password);
        printf("  connexion_type : %s\n", s->connexion_type);
        printf("\n");

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
        free(serv->connexion_type);
    }
    free(serv);
}