#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "ui/key_detector.h"
#include "network/network_SSH.h"
#include "process/Processus.h"
#include "tool/tool.h"

int main(void){
    WINDOW *main_work;
    programme_state state = {.is_running = 1};
    int selected_index = 0; 
    int window_start_index = 0;

    // initialisation
    strcpy(state.last_key_pressed, "aucune");
    state.selected_pid = 0;
    main_work = initialize_ncurses();
    if (main_work == NULL){
        return 1;
    }

    draw_ui(main_work, &state);


    // on récupère les processus
    list_proc lproc = NULL;
    char **dirs = get_list_dirs("/proc");
    if (dirs != NULL){
        int err = get_all_proc(&lproc, NULL, dirs, LOCAL);
        destoy_char(dirs);
        if (err != 0){
            endwin();
            printf("ERREUR: get_all_proc = %d\n", err);
            return 1;
        }
    }

    // on compte le nb total de processus
    int total_proc = 0;
    proc *tmp = lproc;
    while (tmp != NULL){
        total_proc++;
        tmp = tmp->next;
    }

    int window_size = 25; // nombre de processus affichés à l'écran

    // C'est partiiiiiii !!!

    while (state.is_running){

        // on lit les touches
        char old_key[64];
        strcpy(old_key, state.last_key_pressed);
        handle_input(&state);
        char *lkp = state.last_key_pressed;

        // si on touche une touche, on rafraichit (pour pas rafraichir souvent)
        if (strcmp(old_key, lkp) != 0) {
            draw_ui(main_work, &state);
        }

        // flèche haut
        if (strstr(lkp, "259") != NULL && selected_index > 0){
            selected_index--;
            strcpy(state.last_key_pressed, ""); 
        }
        // flèche bas
        else if (strstr(lkp, "258") != NULL && selected_index < total_proc - 1){
            selected_index++;
            strcpy(state.last_key_pressed, "");
        }

        // ajuste la fenêtre pour suivre la sélection
        if (selected_index < window_start_index){
            window_start_index = selected_index;
        } else if (selected_index >= window_start_index + window_size){
            window_start_index = selected_index - window_size + 1;
        }

        mvwprintw(main_work, 5, 0, "Processus : %d/%d ", selected_index, total_proc);
        mvwprintw(main_work, 6, 0, "Idx  PID    PPID   USER                      CPU   STATE CMD");

        tmp = lproc;
        int count = 0;
        int line = 7;
        while (tmp != NULL){
            if (count >= window_start_index && count < window_start_index + window_size){
                if (count == selected_index){
                    wattron(main_work, A_REVERSE);
                }

                mvwprintw(main_work, line, 0,
                        "%-4d %-6d %-6d %-25s %-5.1f %c %.40s  ",
                        count,
                        tmp->PID,
                        tmp->PPID,
                        tmp->user ? tmp->user : "?",
                        tmp->CPU,
                        tmp->state,
                        tmp->cmdline ? tmp->cmdline : "?");

                if (count == selected_index){
                    wattroff(main_work, A_REVERSE);
                    state.selected_pid = tmp->PID;
                }
                line++;
            }
            tmp = tmp->next;
            count++;
        }

    }

    // on nettoie !
    endwin();
    while (lproc != NULL){
        proc *tmp = lproc;
        lproc = lproc->next;
        if (tmp->user) free(tmp->user);
        if (tmp->cmdline) free(tmp->cmdline);
        free(tmp);
    }

    printf("test clavier termine\n");
    return 0;
}