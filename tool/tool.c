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

//LOCAL
char *get_char_file(char *path) {
    if(!path){
        return NULL;
    }
    FILE *file;
    char *text = NULL, c;
    int size = 0,ic;
    file = fopen(path, "r");
    if (!file) {
        printf("Error fopen\n");
        return NULL;
    }
    while ((ic = fgetc(file)) != EOF) {
        c = (char)ic;
        if (c != '\n') {
            char *temp = (char*)realloc(text, (size+1)*sizeof(char));
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
        if (!text) return NULL;
        text[0] = '\0';
        return text;
    }

    text[size] = '\0';
    return text;
}

char **get_list_dirs(const char *path){
    DIR *dir;
    struct dirent *entry;
    char **names = NULL;
    int size, nb_dir = 0;
    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "opendir");
        return NULL;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (entry->d_type == DT_DIR && is_number(entry->d_name) == 1) {
            size = strlen(entry->d_name);
            char *word = malloc(size + 1);
            if (!word) {
                return NULL;
            }
            strcpy(word, entry->d_name);
            char **tmp = realloc(names, (nb_dir + 2) * sizeof *names);/* +2 : un pour le nouveau nom, un pour le pointeur NULL final */
            if (!tmp) {
                free(word);
                return NULL;
            }
            names = tmp;
            names[nb_dir++] = word;
            names[nb_dir] = NULL;
        }
    }
    printf("oui\n");
    return names;

}

//SSH
char *get_char_ssh(ssh_state *state, char *path){
    if (!path) {
        fprintf(stderr, "get_char_ssh : path\n");
        return "NULL";
    }

    if (!state) {
        fprintf(stderr, "get_char_ssh : state\n");
        return "NULL";
    }

    ssh_channel chan = ssh_channel_new(state->session);
    if (!chan) {
        fprintf(stderr, "get_char_ssh : channel\n");
        return "NULL";
    }

    if (ssh_channel_open_session(chan) != SSH_OK) {
        ssh_channel_free(chan);
        fprintf(stderr, "get_char_ssh : channel session\n");
        return "NULL";
    }

    if (ssh_channel_request_exec(chan, "cat /proc/self/stat") != SSH_OK) {
        ssh_channel_close(chan);
        ssh_channel_free(chan);
        fprintf(stderr, "get_char_ssh : channel cat\n");
        return "NULL";
    }

    char *text = NULL, c;
    int size = 0;
    int n;
    while ((n = ssh_channel_read(chan, &c, 1, 0)) > 0) {
        if (c != '\n') {
            char *temp = (char*)realloc(text, (size+1)*sizeof(char));
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

    ssh_channel_send_eof(chan);
    ssh_channel_close(chan);
    ssh_channel_free(chan);
    return text;

}

//TELNET
char *get_char_telnet(){
    return "à faire";
}

//AUTRE
char **split(char *line, char delim) {
    if (!line) {
        return NULL;
    }
    int n = strlen(line);
    char **res = NULL;
    int nb_word = 0;
    int start = 0;
    
    for (int i = 0; i <= n; ++i) {
        if (line[i] == delim || line[i] == '\n' || line[i] == '\0') {
            int size = i - start;
            if (size > 0) {
                char *word = malloc(size + 1);
                if (!word) {
                    return NULL;
                }
                memcpy(word, line + start, size);
                word[size] = '\0';
                char **tmp = realloc(res, (nb_word + 2) * sizeof *res);/* +2 : un pour le nouveau mot, un pour le pointeur NULL final */
                if (!tmp) {
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

int is_number(const char *s) {
    for (size_t i = 0; s[i]; i++){
        if (!isdigit((unsigned char)s[i])){
            return 0;
        }
    }
    return 1;
}

void print_str_array(char **tab) {
    if (!tab)
        return;
    int i = 0;
    while(tab[i]){
        printf("|%d|%s\n",i, tab[i++]);
    }
}
