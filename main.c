#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "key_detector.h"
#include "network.h"




/*fonction main */
int main(){
    char *path_to_config = "";
    int error = CONTINUE;
    list_serv l = get_serveur_Config(path_to_config, &error);
    if ((error != CONTINUE && error != SERVER_SKIPED) || l == NULL) {
        fprintf(stderr, "Erreur Get_serveur_Config: %d\n", error);
        return 1;
    }

    print_list_serv(l);
    return error;
}