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
    Proc *lproc = NULL;
    int res = Get_processus(&lproc);
    Proc *temp = lproc;
    while(temp != NULL){
        print_CPU(temp);
        temp = temp->next;
    }
    return res;
}