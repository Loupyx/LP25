#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> // Pour les constantes de signaux (SIGSTOP, SIGKILL, etc.)
#include "./../process/Processus.h"
#include "key_detector.h"


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

/*dessine le contenu du panneau d'aide*/
void draw_help(WINDOW *work, int max_y, int max_x) {
    mvprintw(2, 5, "--- Aide : Raccourcis Clavier ---");
    //contenu de l'aide
    mvprintw(4, 5, "[F1] : Afficher de l'aide / caccher l'aide");
    mvprintw(5, 5, "[F2] : Onglet suivant (navigaion)");
    mvprintw(6, 5, "[F3] : Onglet précédent (navigaion)");
    mvprintw(7, 5, "[F4] : Recherche / Filetrage des processus");
    mvprintw(9, 5, "[F5] : Mettre en Pause le processus sélectionné (SIGSTOP)");
    mvprintw(10, 5, "[F6] : Arreter le processus sélectionné (SIGTERM)");
    mvprintw(11, 5, "[F7] : Tuer (Kill) le processus sélectionné (SIGKILL)");
    mvprintw(12, 5, "[F8] : Reprendre le processus sélectionné (SIGCONT");
    mvprintw(13, 5, "[q] : Quitter l'application");
    //message indiquer en bas 
    mvprintw(max_y -2, (max_x /2 ) - 25, "Appuyer sur F1 ou Q pour revenir à la liste des processus");
}

void draw_ui(WINDOW *work, programme_state *state) {
    int max_y, max_x;
    getmaxyx(work, max_y, max_x);
    werase(work);   //permet d'effacer la fenetre d'avant 
    mvprintw(0, 0, "--- testeur de raccourcis clavier F-keys (ncurses) ---");
    if (state->is_help_displayed) {
        //affiche le panneau d'aide 
        draw_help(work, max_y, max_x);
    } else {
        //affiche l'interface noraml 
        mvprintw(2, 0, "appuyer sur une touche F (F1 à F8) ou q pour quitter ");
        mvprintw(4, 0, "voici la derniere touche detectee : %s", state->last_key_pressed);
        // c'est ici qu'on va afficher la liste des processus (SIMONNN)
    }
    // affichage commun des raccourcis (commun à l'interface help et normale)
    mvprintw(max_y - 1, 0, "[F1] aide | [F2/F3] onglets | [F4] recherche | [F5-F8] actions processus | q quitter ");
    wrefresh(work); 
}
/*gere les entrees du clavier et met a jour l'etat avec le parametre state (etat actuel du prog a modif)*/
void handle_input(programme_state *state){
    int key = getch();
    if (key == ERR){
        return;     //aucune touche pressee
    }
    const char *key_name = NULL;
    pid_t target_pid = state->selected_pid;

    switch (key){
        case KEY_F(1):
            state->is_help_displayed =! state->is_help_displayed;
            key_name = "F1 (aide)";
            break;

        case 'q':
            if (state->is_help_displayed) {
                // Si on est dans l'aide, 'q' ferme l'aide
                state->is_help_displayed = 0;
                key_name = "'q' (ferme l'aide)";
            } else {
                // Sinon, 'q' quitte l'application
                state->is_running = 0;
                key_name = "'q' (quitter)";
            }
            break;

        default: 
            // Si l'aide est affichée, ignorer toutes les autres touches (F2-F8, etc.)
            if (state->is_help_displayed) {
                 return;
            }
            
            // Traitement des autres touches si l'aide n'est PAS affichée
            switch (key) {
                case KEY_F(2):
                    key_name = "F2 (onglet suivant)";
                    break;
                case KEY_F(3):
                    key_name = "F3 (onglet precedent)";
                    break;
                case KEY_F(4):
                    key_name = "F4 (recherche)";
                    break;
                case KEY_F(5):
                    key_name = "F5 (pause processus)";
                    send_process_action(target_pid, SIGSTOP, "Pause");     
                    break;
                case KEY_F(6):
                    key_name = "F6 (arret processus)";
                    send_process_action(target_pid, SIGTERM, "Arret");     
                    break;
                case KEY_F(7):
                    key_name = "F7 (tuer/kill le processus)";
                    send_process_action(target_pid, SIGKILL, "Kill");  
                    break;
                case KEY_F(8):
                    key_name = "F8 (redemarrer/reprendre le processus)";
                    send_process_action(target_pid,SIGCONT, "Reprise");     
                    break;
                case KEY_RESIZE:        //permet le redimmensionnement du terminal
                    key_name = "terminal redimensionne (KEY_RESIZE)";
                    break;
                default:
                    // Pour toutes les autres touches
                    snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "autre touche : code = %d", key);
                    return; // on affiche le code
            }
    }

    /*met a jour l'etat du programme avec le nom de la touche (pour l'affichage)*/
    if  (key_name){
        strncpy(state->last_key_pressed, key_name,sizeof(state->last_key_pressed) - 1);
        state->last_key_pressed[sizeof(state->last_key_pressed) - 1] = '\0';
    }
}