#include <stdio.h>
#include "Processus.h"

int main(){
    Proc *lproc = NULL;
    int res = Get_processus(&lproc);
    Proc *temp = lproc;
    while(temp != NULL){
        print_proc(temp);
        temp = temp->next;
    }
    return res;
}