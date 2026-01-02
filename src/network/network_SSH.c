#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network_SSH.h"
#include "network_main.h"
#include "./../tool/tool.h"

ssh_state *init_ssh_session(server *serv) {
    ssh_state *state = (ssh_state*)malloc(sizeof(ssh_state));
    if (!state) {
        write_log("Erreur allocation ssh_state");
        return NULL;
    }
    state->session = NULL;
    state->sftp = NULL;
    state->dir = NULL;
    state->attr = NULL;

    state->session = ssh_new(); //création nouvelle session
    if (!state->session) {
        write_log("Erreur: ssh_new");
        return NULL;
    }

    ssh_options_set(state->session, SSH_OPTIONS_HOST, serv->adresse);
    ssh_options_set(state->session, SSH_OPTIONS_USER, serv->username);
    ssh_options_set(state->session, SSH_OPTIONS_PORT, &serv->port);

    state->rc = ssh_connect(state->session); //connexion
    if (state->rc != SSH_OK) {
        write_log("Erreur de connexion: %s", ssh_get_error(state->session));
        destroy_ssh_state(state);
        return NULL;
    }

    state->rc = ssh_userauth_password(state->session, NULL, serv->password); //identification
    if (state->rc != SSH_AUTH_SUCCESS) {
        write_log("Erreur d'authentification: %s", ssh_get_error(state->session));
        ssh_disconnect(state->session);
        destroy_ssh_state(state);
        return NULL;
    }

    state->sftp = sftp_new(state->session); //session sftp
    if (state->sftp == NULL) {
        write_log("Erreur sftp_new: %s", ssh_get_error(state->session));
        destroy_ssh_state(state);
        return NULL;
    }

    state->rc = sftp_init(state->sftp);
    if (state->rc != SSH_OK) {
        write_log("Erreur sftp_init: %d", sftp_get_error(state->sftp));
        destroy_ssh_state(state);
        return NULL;
    }

    write_log("Connexion réussi\n");
    return state;
}

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

    char **tmp = realloc(res, (nb_dir + 1)*sizeof(char *));
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

void destroy_ssh_state(ssh_state *s) {
    if (!s) return;

    if (s->attr) {
        sftp_attributes_free(s->attr);
        s->attr = NULL;
    }

    if (s->dir) {
        s->dir = NULL;
    }

    if (s->sftp) {
        sftp_free(s->sftp);
        s->sftp = NULL;
    }

    if (s->session) {
        ssh_disconnect(s->session);
        ssh_free(s->session);
        s->session = NULL;
    }
    s->rc = 0;
}

void free_ssh_dir(char **list) {
    if (!list) return;
    for (int i=0; list[i]!=NULL; ++i) {
        free(list[i]);
    }
    free(list);
}