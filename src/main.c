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
    int selected_index = 0;  // index du processus sélectionné
    int window_start_index = 0; // début de la fenêtre visible

    // initialisation
    strcpy(state.last_key_pressed, "aucune");
    state.selected_pid = 0; // PID du processus sélectionné
    main_work = initialize_ncurses();
    if (main_work == NULL){
        return 1;
    }

    // récupération des processus
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

    // compter le nombre total de processus
    int total_proc = 0;
    proc *tmp = lproc;
    while (tmp != NULL){
        total_proc++;
        tmp = tmp->next;
    }

    int window_size = 10; // nombre de processus affichés à l'écran

    // boucle principale
    while (state.is_running){
        int key = getch(); // lecture des touches
        handle_input(&state);

        // déplacement de la sélection avec les flèches
        if (key == KEY_UP && selected_index > 0){
            selected_index--;
        } else if (key == KEY_DOWN && selected_index < total_proc - 1){
            selected_index++;
        }

        // ajuster la fenêtre pour suivre la sélection
        if (selected_index < window_start_index){
            window_start_index = selected_index;
        } else if (selected_index >= window_start_index + window_size){
            window_start_index = selected_index - window_size + 1;
        }

        draw_ui(main_work, &state); // interface statique

        // affichage des processus
        mvwprintw(main_work, 5, 0, "Total processus : %d", total_proc);
        mvwprintw(main_work, 6, 0, "Idx  PID    PPID   USER         CPU   STATE   CMD");

        tmp = lproc;
        int count = 0;
        int line = 7;
        while (tmp != NULL){
            if (count >= window_start_index && count < window_start_index + window_size){
                if (count == selected_index){
                    wattron(main_work, A_REVERSE); // inversion de couleurs
                }

                mvwprintw(main_work, line, 0,
                          "%-3d %-6d %-6d %-12s %-5.1f %c %.40s",
                          count,
                          tmp->PID,
                          tmp->PPID,
                          tmp->user ? tmp->user : "?",
                          tmp->CPU,
                          tmp->state,
                          tmp->cmdline ? tmp->cmdline : "?");

                if (count == selected_index){
                    wattroff(main_work, A_REVERSE);
                    state.selected_pid = tmp->PID; // PID du processus sélectionné
                }
                line++;
            }
            tmp = tmp->next;
            count++;
        }

        mvwprintw(main_work, line + 1, 0, "Sélection: index=%d PID=%d", selected_index, state.selected_pid);
        wrefresh(main_work);
        usleep(5000);
    }

    // nettoyage ncurses et libération de la liste
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
