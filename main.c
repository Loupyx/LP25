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
    list_serv l = get_serveur_config(NULL, &error);
    if(!l){
        printf("l main");
        return 1;
    }

    char **l_dir = get_list_dirs("/proc");
    if(!l_dir){
        printf("l_dir main\n");
        return 1;
    }
    list_proc l_proc = NULL;
    error = get_all_proc(&l_proc, NULL, l_dir, LOCAL);
    fprintf(stderr, "get_all_proc OK\n");
    if(!l_proc){
        printf("oeuruzerioze\n");
        return 1;
    }
    print_proc(l_proc);
    return 0;
}