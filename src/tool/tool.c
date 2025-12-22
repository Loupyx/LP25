#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <stdarg.h>

#include "./../network/network_SSH.h"
#include "tool.h"

int write_log(const char *text, ...) {
    FILE *log = fopen(".log", "a");
    FILE *f;
    if (!log) {
        // éventuellement fallback sur stderr
        printf("Can't open log file for : ");
        f = stdout;
    } else {
        f = log;
    }
    fprintf(f, "[INFO] ");
    va_list args;
    va_start(args, text);
    vfprintf(f, text, args);   // applique fmt + variables
    va_end(args);
    fputc('\n', f); 
    if (log){
        fclose(log);
        return 0;
    }
    return 1;
}

//LOCAL
char *get_char_file(char *path) {
    if (!path) {
        return NULL;
    }
    FILE *file;
    char *text = NULL, c;
    int size = 0,ic;
    file = fopen(path, "r");
    if (!file) {
        write_log("Error fopen for : %s", path);
        return NULL;
    }
    while ((ic = fgetc(file)) != EOF) {
        c = (char)ic;
        if (c != '\n') {
            char *temp = (char*)realloc(text, (size+2)*sizeof(char));
            if (!temp) {
                free(text);
                fclose(file);
                return NULL;
            }
            text = temp;
            text[size++] = c;
        }
    }

    fclose(file);

    if (!text) {
        text = malloc(1);
        if (!text) return NULL;
        text[0] = '\0';
        return text;
    }
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
        write_log("ERROR : Canno't opendir for %s", path);
        return NULL;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (entry->d_type == DT_DIR && is_number(entry->d_name) == 1) {
            size = strlen(entry->d_name);
            char *word = malloc(size + 1);
            if (!word) {
                write_log("ERROR : allocation word");
                for (int i = 0; i < nb_dir; i++) {
                    free(names[i]);
                }
                free(names);
                closedir(dir);
                return NULL;
            }
            strcpy(word, entry->d_name);
            char **tmp = realloc(names, (nb_dir + 2) * sizeof *names);/* +2 : un pour le nouveau nom, un pour le pointeur NULL final */
            if (!tmp) {
                free(word);
                write_log("ERROR : reallocation tmp");
                for (int i = 0; i < nb_dir; i++) {
                    free(names[i]);
                }
                free(names);
                closedir(dir);
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

//SSH
char *get_char_ssh(ssh_state *state, char *path) {
    char command[256];
    if (!path) {
        write_log("get_char_ssh : path");
        return NULL;
    }

    if (!state) {
        write_log("get_char_ssh : state");
        return NULL;
    }

    ssh_channel chan = ssh_channel_new(state->session);
    if (!chan) {
        write_log("get_char_ssh : channel");
        return NULL;
    }

    if (ssh_channel_open_session(chan) != SSH_OK) {
        ssh_channel_free(chan);
        write_log("get_char_ssh : channel session");
        return NULL;
    }

    snprintf(command, sizeof(command), "cat %s", path);
    if (ssh_channel_request_exec(chan, command) != SSH_OK) {
        ssh_channel_close(chan);
        ssh_channel_free(chan);
        write_log("get_char_ssh : channel cat");
        return NULL;
    }

    char *text = NULL, c;
    int size = 0;
    int n;
    while ((n = ssh_channel_read(chan, &c, 1, 0)) > 0) {
        if (c != '\n') {
            char *temp = (char*)realloc(text, (size+2)*sizeof(char));
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

char **get_ssh_dir(ssh_state *state, char *path) {
    if (!state || !state->session || !path) {
        write_log("get_ssh_dir: bad arguments");
        return NULL;
    }

    sftp_session sftp = sftp_new(state->session);
    if (!sftp) {
        write_log("get_ssh_dir: sftp_new failed");
        return NULL;
    }

    if (sftp_init(sftp) != SSH_OK) {
        write_log("get_ssh_dir: sftp_init failed: %s", ssh_get_error(state->session));
        sftp_free(sftp);
        return NULL;
    }

    sftp_dir dir = sftp_opendir(sftp, path);
    if (!dir) {
        write_log("get_ssh_dir: sftp_opendir('%s') failed: %s", path, ssh_get_error(state->session));
        sftp_free(sftp);
        return NULL;
    }

    char **res = NULL;
    int size, nb_dir = 0;

    while (!sftp_dir_eof(dir)) {
        sftp_attributes attrs = sftp_readdir(sftp, dir);
        if (!attrs) {
            continue;
        }
        if (is_number(attrs->name)) {
            char **temp = (char**)realloc(res, (nb_dir+1)*sizeof(char*));
            if (!temp) {
                sftp_attributes_free(attrs);
                free_ssh_dir(res);
                sftp_closedir(dir);
                sftp_free(sftp);
                return NULL;
            }
            res = temp;
            
            size = strlen(attrs->name);
            res[nb_dir] = (char*)malloc(sizeof(char)*(size+1));
            if (!res[nb_dir]) {
                write_log("get_ssh_dir: malloc failed");
                sftp_attributes_free(attrs);
                free_ssh_dir(res);
                sftp_closedir(dir);
                sftp_free(sftp);
                return NULL;
            }

            memcpy(res[nb_dir], attrs->name, size + 1);
            nb_dir++;
        }
    }

    char **tmp = realloc(res, (nb_dir + 1) * sizeof(char *));
    if (!tmp && nb_dir > 0) {
        // cas très rare, mais on gère proprement
        write_log("get_ssh_dir: final realloc failed");
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

void free_ssh_dir(char **list) {
    if (!list) return;
    for (int i=0; list[i]!=NULL; ++i) {
        free(list[i]);
    }
    free(list);
}
//TELNET
char *get_char_telnet(){
    return "à faire";
}

//AUTRE
char **split(char *line, char delim) {
    if (!line) {
        write_log("DEBUG: split got NULL line");
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
                char **tmp = realloc(res, (nb_word + 2) * sizeof *res);/* +2 : un pour le nouveau mot, un pour le pointeur NULL final */
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

void destoy_char(char *line[]){
    if (!line) {
        return;
    }
    for (int i=0; line[i]; i++) {
        free(line[i]);
    }
    return;
}

int is_number(const char *s) {
    for (size_t i = 0; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    return 1;
}

void print_str_array(char **tab) {
    if (!tab){
        return;
    }
    for (int i=0; tab[i]; i++) {
        printf("|%d|%s\n",i, tab[i]);
    }
}
