#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ui/key_detector.h"
#include "network/network_SSH.h"
#include "process/Processus.h"
#include "tool/tool.h"

/*fonction main */
int main() {
    int error;
    list_serv l = get_serveur_config("", &error);
    ssh_state *state = init_ssh_session(l->serv);

    char *test = get_char_ssh(state, "/proc/95/stat");
    printf("%s\n", test);
    return 0;
}