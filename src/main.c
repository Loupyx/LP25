#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ui/key_detector.h"
#include "network/network_SSH.h"
#include "process/Processus.h"
#include "network/network_telnet.h"




/*fonction main */
int main(){
    char *path_to_config = "";
    int error = CONTINUE;
    list_serv l = get_serveur_config(path_to_config, &error);

    printf("OK : %d\n", error);
    
    return error;
}