#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "key_detector.h"




/*fonction main */
int main(){
    WINDOW *main_work;
    programme_state state = {.is_running = 1};

    //initialisation
    strcpy(state.last_key_pressed, "aucune");
    main_work = initialize_ncurses();
    if (main_work ==NULL){
        return 1;
    }

    //boucle principale 
    while (state.is_running){
        handle_input(&state);   //lit et traite l'entrée 
        draw_ui(main_work, &state);     //dessine l'inteface 
        usleep(50000);      //50 millisecondes pour limiter l'utilisation du CPU
    }
    //nettoyage et restauration du terminal 
    endwin();
    printf("test clavier termine\n");
    return 0;
}