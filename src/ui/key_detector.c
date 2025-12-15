#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> // Pour les constantes de signaux (SIGSTOP, SIGKILL, etc.)
#include "./../process/Processus.h"
#include "key_detector.h"

extern int max_y, max_x; 

/*initilisation de ncurses
WINDOW* est un pointeur vers la fenetre principale (stdscr)*/

WINDOW *initialize_ncurses(){
    WINDOW *work = initscr();
    if (work == NULL){
        fprintf(stderr, "erreur lors de l'initialisation de ncurses\n");
        return NULL;
    }
    /*configuration des gestions des touches entrées*/
    cbreak();    //permet  la lecture immediate 
    noecho();   //affiche pas les caracteres tapés
    keypad(work, TRUE);     //permet de traduire les F1,F2...
    nodelay(work, TRUE);    //ne bloque pas getch()
    return work;
}

/*creation de l'interface avec comme parametre work (fonction ncurses) et programm_state (etat du programme)*/
void draw_ui(WINDOW *work, programme_state *state){
    werase(work); //permet d'effacer la fenetre d'avant utile pour afficher dynamiquement les processus 
    mvprintw(0, 0, "--- testeur de raccourcis clavier F-keys (ncurses)---\n");
    mvprintw(2, 0, "appuyer sur une touche F (F1 à F8) ou q pour quitter\n");
    mvprintw(4, 0, "voici la derniere touche detectee : %s", state->last_key_pressed);
    mvprintw(max_y - 1, 0, "[F1-F8] detectee | [q] quitter");
    wrefresh(work);     //rafraichit la page 
}

/*gere les entrees du clavier et met a jour l'etat avec le parametre state (etat actuel du prog a modif)*/
void handle_input(programme_state *state){
    int key = getch();
    if (key == ERR){
        return;     //aucune touche pressee
    }
    const char *key_name = NULL;
    pid_t target_pid = state->selected_pid;
    int erreur;

    switch (key){       //creation des cas en fonction des touches pressees
        case KEY_F(1):
            key_name = "F1 (aide)\n";
            break;
        case KEY_F(2):
            key_name = "F2 (onglet suivant)\n";
            break;
        case KEY_F(3):
            key_name = "F3 (onglet precedent)\n";
            break;
        case KEY_F(4):
            key_name = "F4 (recherche)\n";
            break;
        case KEY_F(5):
            key_name = "F5 (pause processus)\n";
            erreur = send_process_action(target_pid, SIGSTOP, "Pause");     
            break;
        case KEY_F(6):
            key_name = "F6 (arret processus)\n";
            erreur = send_process_action(target_pid, SIGTERM, "Arret");     
            break;
        case KEY_F(7):
            key_name = "F7 (tuer/kill le processus )\n";
            erreur = send_process_action(target_pid, SIGKILL, "Kill");  
            break;
        case KEY_F(8):
            key_name = "F8 (redemarrer/reprendre le processus)\n";
            erreur = send_process_action(target_pid,SIGCONT, "Reprise");     
            break;
        case 'q':
            state->is_running =0;   //permet de quitter la boucle 
            key_name = "'q' (quitter)";
            break;
        case KEY_RESIZE:        //permet le redimmensionnement du terminal
            key_name = "terminal redimensionne (KEY_RESIZE)\n";
            break;
        case 258:
            key_name = "Flèche/pavier bas";
            break;
        case 259:
            key_name = "Flèche/pavier haut";
            break;
        default: //pour toutes les autres touches on affiche le code numérique
            snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "autre touche : code = %d", key);
            return; //on affiche le code 
    }

    if (erreur == -1){
        mvprintw(max_y - 2, 0, "Erreur envoie signal à %d pour %s", target_pid, key_name);
    }

    if (erreur == 0){
        mvprintw(max_y - 2, 0, "Envoie signal à %d pour %s réussi", target_pid, key_name);
    }

    /*met a jour l'etat du programme avec le nom de la touche*/
    if  (key_name){
        strncpy(state->last_key_pressed, key_name,sizeof(state->last_key_pressed) - 1);
        state->last_key_pressed[sizeof(state->last_key_pressed) - 1] = '\0';
    }
}