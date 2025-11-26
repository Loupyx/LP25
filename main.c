#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ui/key_detector.h"
#include "network/network_SSH.h"
#include "process/Processus.h"




/*fonction main */
int main(){
    char *path_to_config = "";
    int error = CONTINUE;
    list_serv l = get_serveur_config(path_to_config, &error);
    ssh_state *state = NULL;
    state = init_ssh_session(l->serv);
    if(!state){
        printf("Erreur main : init_ssh_session (line 18)\n");
        return EXIT_FAILURE;
    }

    if ((error != CONTINUE && error != SERVER_SKIPED) || l == NULL) {
        fprintf(stderr, "Erreur get_serveur_config: %d\n", error);
        return 1;
    }

    open_dir_ssh(state);
    close_dir_ssh(state);

    print_list_serv(l);
    destroy_ssh_state(state);

    return error;
}