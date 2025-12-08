#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network_SSH.h"
#include "network_main.h"

//SSH
ssh_state *init_ssh_session(server *serv) {
    ssh_state *state = (ssh_state*)malloc(sizeof(ssh_state));
    if (!state) {
        fprintf(stderr, "Erreur allocation ssh_state");
        return NULL;
    }
    state->session = NULL;
    state->sftp = NULL;
    state->dir = NULL;
    state->attr = NULL;

    state->session = ssh_new(); //création nouvelle session
    if (!state->session) {
        fprintf(stderr, "Erreur: ssh_new\n");
        return NULL;
    }

    ssh_options_set(state->session, SSH_OPTIONS_HOST, serv->adresse);
    ssh_options_set(state->session, SSH_OPTIONS_USER, serv->username);
    ssh_options_set(state->session, SSH_OPTIONS_PORT, &serv->port);
   
    state->rc = ssh_connect(state->session); //connexion
    if (state->rc != SSH_OK) {
        fprintf(stderr, "Erreur de connexion: %s\n", ssh_get_error(state->session));
        destroy_ssh_state(state);
        return NULL;
    }

    state->rc = ssh_userauth_password(state->session, NULL, serv->password); //identification
    if (state->rc != SSH_AUTH_SUCCESS) {
        fprintf(stderr, "Erreur d'authentification: %s\n", ssh_get_error(state->session));
        ssh_disconnect(state->session);
        destroy_ssh_state(state);
        return NULL;
    }

    state->sftp = sftp_new(state->session); //session sftp
    if (state->sftp == NULL) {
        fprintf(stderr, "Erreur sftp_new: %s\n", ssh_get_error(state->session));
        destroy_ssh_state(state);
        return NULL;
    }

    state->rc = sftp_init(state->sftp);
    if (state->rc != SSH_OK) {
        fprintf(stderr, "Erreur sftp_init: %d\n", sftp_get_error(state->sftp));
        destroy_ssh_state(state);
        return NULL;
    }

    printf("Connexion réussi\n");
    return state;
}

/**
 * @brief Exécute une commande shell simple sur la session SSH distante.
 * * N'effectue aucune capture de sortie, uniquement la vérification du statut d'exécution.
 * * \param state   État de session SSH valide.
 * \param command Chaîne de commande à exécuter (ex: "kill -9 1234").
 * \return Le code de sortie de la commande distante (0 pour succès), ou -1 en cas d'erreur SSH.
 */
int ssh_exec_command(ssh_state *state, const char *command) {
    ssh_channel chan = NULL;
    int exit_status = -1;

    if (!state || !state->session || !command) {
        fprintf(stderr, "ssh_exec_command: Etat SSH ou commande invalide.\n");
        return -1;
    }

    chan = ssh_channel_new(state->session);
    if (!chan) {
        fprintf(stderr, "ssh_exec_command: Erreur ssh_channel_new.\n");
        return -1;
    }

    if (ssh_channel_open_session(chan) != SSH_OK) {
        fprintf(stderr, "ssh_exec_command: Erreur ssh_channel_open_session: %s\n", ssh_get_error(state->session));
        ssh_channel_free(chan);
        return -1;
    }

    if (ssh_channel_request_exec(chan, command) != SSH_OK) {
        fprintf(stderr, "ssh_exec_command: Erreur ssh_channel_request_exec (commande: %s): %s\n", command, ssh_get_error(state->session));
        ssh_channel_close(chan);
        ssh_channel_free(chan);
        return -1;
    }

    // Attendre la fin de l'exécution et fermer le canal
    ssh_channel_send_eof(chan);
    ssh_channel_wait_closed(chan);

    // Récupérer le statut de sortie de la commande distante
    if (ssh_channel_get_exit_status(chan, &exit_status) != SSH_OK) {
        fprintf(stderr, "ssh_exec_command: Erreur lors de la recuperation du statut de sortie.\n");
        exit_status = -1;
    }
    
    ssh_channel_close(chan);
    ssh_channel_free(chan);
    
    return exit_status;
}


int open_dir_ssh(ssh_state *state) {
    char *path = "/proc";

    state->dir = sftp_opendir(state->sftp, path);
    if (state->dir == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le dossier %s: %s\n", path, ssh_get_error(state->session));
        destroy_ssh_state(state);
        return 1;
    }
    while ((state->attr = sftp_readdir(state->sftp, state->dir)) != NULL) {//Lire les entrées du dossier
        printf("- %s\n", state->attr->name);
        sftp_attributes_free(state->attr);
    }
    return 0;

}

int close_dir_ssh(ssh_state *state) {
    state->rc = sftp_closedir(state->dir);
    if (state->rc != SSH_OK) {
        fprintf(stderr, "Erreur sftp_closedir: %s\n", ssh_get_error(state->sftp));
        return 1;
    }

    return 0;

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
//fin SSH