#include <stdlib.h>
#include <stdio.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <fcntl.h>

#include "./../network/network_SSH.h"
#include "tool.h"

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
