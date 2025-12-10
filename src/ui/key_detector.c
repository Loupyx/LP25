#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> // Pour les constantes de signaux (SIGSTOP, SIGKILL, etc.)
#include "./../process/Processus.h"
#include "./../tool/tool.h"
#include "key_detector.h"

extern int max_y, max_x; 

/*initilisation de ncurses
WINDOW* est un pointeur vers la fenetre principale (stdscr)*/

WINDOW *initialize_ncurses(){
    WINDOW *work = initscr();
    if (work == NULL){
        write_log("erreur lors de l'initialisation de ncurses");
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
void draw_ui(WINDOW *work, programme_state *state, list_proc lproc, proc *selected_proc){
    int window_size = max_y - 2;
    int total_proc = 0;
    proc *tmp = lproc;
    while (tmp != NULL) {
        ++total_proc;
        tmp = tmp->next;
    }
    werase(work); //permet d'effacer la fenetre d'avant utile pour afficher dynamiquement les processus 

    mvprintw(0, 0, "--- HTOP MAISON LP25 ---");
    if (state->is_help_displayed) {
        //affiche le panneau d'aide 
        draw_help(work, max_y, max_x);
    } else if (state->is_search_active) {
        mvprintw(2, 0, "Mode recherche activé (F4 ou entrée pour quitter la fenetre)");
        mvprintw(4, 0, "Rechercher avec le nom ou le PID : %s", state->search_term);
        //on place le curseur à la fin du terme recherché 
        move(4, strlen("Rechercher avec le nom ou le PID :") + strlen(state->search_term));
    }
    else {
        //affiche l'interface noraml 
        mvprintw(2, 0, "appuyer sur une touche F (F1 à F8) ou q pour quitter ");
        mvprintw(4, 0, "voici la derniere touche detectee : %s", state->last_key_pressed);
        mvwprintw(work, 5, 0, "Nombre processus : %d ", total_proc);
        mvwprintw(work, 6, 0, "PID\tPPID\tUSER\t\t\t\tCPU\tSTATE\tCMD");

        proc *temp_aff = selected_proc;
        for (int i=0; temp_aff && i+7<window_size; i++) {
            if (i == 0) {
                wattron(work, A_REVERSE);    // surligne la ligne du selected_proc
            }
            mvwprintw(work, i+7, 0,
                    "%-6d\t%-6d\t%-25s\t%-5.1f\t%c\t%.40s  ",
                    temp_aff->PID,
                    temp_aff->PPID,
                    temp_aff->user ? temp_aff->user : "?",
                    temp_aff->CPU,
                    temp_aff->state,
                    temp_aff->cmdline ? temp_aff->cmdline : "?"); 
            if (i == 0) {
                wattroff(work, A_REVERSE);    // surligne la ligne du selected_proc
                state->selected_pid = temp_aff->PID;
            }
            if (temp_aff->next != NULL) {
                temp_aff = temp_aff->next;
            } else {
                break;
            }  
    }
    }
    // affichage commun des raccourcis (commun à l'interface help et normale)
    mvprintw(max_y - 1, 0, "[F1] aide | [F2/F3] onglets | [F4] recherche | [F5-F8] actions processus | q quitter ");
    wrefresh(work);
}

/*dessine le contenu du panneau d'aide*/
void draw_help(WINDOW *work, int max_y, int max_x) {
    mvprintw(2, 5, "--- Aide : Raccourcis Clavier ---");
    //contenu de l'aide
    mvprintw(4, 5, "[F1] : Afficher de l'aide / cacher l'aide");
    mvprintw(5, 5, "[F2] : Onglet suivant (navigaion)");
    mvprintw(6, 5, "[F3] : Onglet précédent (navigaion)");
    mvprintw(7, 5, "[F4] : Recherche / Filetrage des processus");
    mvprintw(9, 5, "[F5] : Mettre en Pause le processus sélectionné (SIGSTOP)");
    mvprintw(10, 5, "[F6] : Arreter le processus sélectionné (SIGTERM)");
    mvprintw(11, 5, "[F7] : Tuer (Kill) le processus sélectionné (SIGKILL)");
    mvprintw(12, 5, "[F8] : Reprendre le processus sélectionné (SIGCONT)");
    mvprintw(13, 5, "[q] : Quitter l'application");
    //message indiquer en bas 
    mvprintw(max_y -2, (max_x /2 ) - 25, "Appuyer sur F1 ou Q pour revenir à la liste des processus");
}

/*gere les entrees du clavier et met a jour l'etat avec le parametre state (etat actuel du prog a modif)*/
void handle_input(programme_state *state, int key){
    const char *key_name = NULL;
    pid_t target_pid = state->selected_pid;
    int erreur;

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
            if (state->is_search_active) {
                int len = strlen(state->search_term);
                if (key == '\n' || key == KEY_ENTER || key == KEY_F(4)) {
                    //sortie du mode recherche
                    state->is_search_active = 0;
                    key_name = "Fin de recherche (ENTREE/F4)";
                    //C'EST ICI QU'ON VA LANCER LE FILTRAGE DES PROCESSUS EN UTILISANT LA CHAINE STOCKEE DANS STATE-<SEARCH_TERM
                } else if (key == KEY_BACKSPACE || key == 127) {    //127 correspond au code clavier 
                    //supp du dernier caractere 
                    if (len > 0 ) {
                        state->search_term[len - 1] = '\0';
                    }
                    key_name = "Saisie : BACKSPACE";
                } else if (isprint(key) && (len < sizeof(state->search_term) - 1)) {
                    state->search_term[len] = (char)key;
                    state->search_term[len + 1] = '\0';
                    key_name = "Saisie : Caractere";
                }
                if (key_name) {
                    strncpy(state->last_key_pressed, key_name, sizeof(state->last_key_pressed) - 1);
                    state->last_key_pressed[sizeof(state->last_key_pressed) - 1] = '\0';
                }
                return;
            }
            
            // Traitement des autres touches si l'aide n'est PAS affichée
            switch (key) {
                case KEY_F(4):
                    state->is_search_active =1;
                    key_name = "F4 (Recherche)";
                    state->search_term[0] = '\0';
                    break;
                case KEY_F(2):
                    key_name = "F2 (onglet suivant)";
                    break;
                case KEY_F(3):
                    key_name = "F3 (onglet precedent)";
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