#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> 
#include "./../process/Processus.h"
#include "./../network/network_SSH.h"
#include "key_detector.h"


/*
 * Initialisation de ncurses
 * WINDOW* work est un pointeur vers la fenêtre principale (stdscr)
 */
WINDOW *initialize_ncurses(){
    WINDOW *work = initscr();
    if (work == NULL){
        fprintf(stderr, "Erreur lors de l'initialisation de ncurses\n");
        return NULL;
    }
    /* Configuration de la gestion des entrées clavier */
    cbreak();    // Permet la lecture immédiate
    noecho();   // N'affiche pas les caractères tapés
    keypad(work, TRUE);     // Permet de traduire les touches spéciales (F1, F2, flèches, etc.)
    nodelay(work, TRUE);    // N'attend pas indéfiniment getch()
    return work;
}

/* * Création de l'interface avec comme paramètre work (fenêtre ncurses) et programme_state (état du programme) 
 */
void draw_ui(WINDOW *work, programme_state *state){
    int max_y, max_x;   // Taille de la fenêtre 
    getmaxyx(work, max_y, max_x);
    werase(work); // Efface la fenêtre précédente, utile pour un affichage dynamique 
    
    mvprintw(0, 0, "--- Testeur de raccourcis clavier F-keys (ncurses) ---\n");
    mvprintw(2, 0, "Appuyez sur une touche F (F1 à F8) ou 'q' pour quitter\n");
    mvprintw(4, 0, "Voici la dernière touche détectée : %s", state->last_key_pressed);
    
    // Affichage des raccourcis en bas de l'écran
    mvprintw(max_y - 1, 0, "[F1-F8] Actions | [q] Quitter");
    wrefresh(work);     // Rafraîchit l'affichage de la page 
}

/* * Gère les entrées du clavier et met à jour l'état (state) avec la touche pressée.
 * Envoie un signal au processus sélectionné si une touche d'action (F5-F8) est pressée.
 */
void handle_input(programme_state *state, enum acces_type connexion, ssh_state *serv){
    int key = getch();
    if (key == ERR){
        return;     // Aucune touche pressée
    }
    const char *key_name = NULL;
    pid_t target_pid = state->selected_pid;

    // Détermination si l'action de signal est possible
    if (target_pid == 0) {
        // Aucune action de signal n'est possible sans PID sélectionné.
        // On laisse les touches F s'afficher, mais l'action (send_process_action) ne sera pas effectuée.
    }

    switch (key){       // Création des cas en fonction des touches pressées
        case KEY_F(1):
            key_name = "F1 (Aide)\n";
            break;
        case KEY_F(2):
            key_name = "F2 (Onglet suivant)\n";
            break;
        case KEY_F(3):
            key_name = "F3 (Onglet précédent)\n";
            break;
        case KEY_F(4):
            key_name = "F4 (Recherche)\n";
            break;
        case KEY_F(5):
            key_name = "F5 (Pause processus)\n";
            send_process_action(target_pid, SIGSTOP, "Pause", connexion, serv);     
            break;
        case KEY_F(6):
            key_name = "F6 (Arrêt processus - SIGTERM)\n";
            send_process_action(target_pid, SIGTERM, "Arrêt", connexion, serv);     
            break;
        case KEY_F(7):
            key_name = "F7 (Tuer le processus - SIGKILL)\n";
            send_process_action(target_pid, SIGKILL, "Kill", connexion, serv);  
            break;
        case KEY_F(8):
            key_name = "F8 (Reprendre le processus - SIGCONT)\n";
            send_process_action(target_pid, SIGCONT, "Reprise", connexion, serv);     
            break;
        case 'q':
            state->is_running = 0;   // Permet de quitter la boucle principale 
            key_name = "'q' (Quitter)";
            break;
        case KEY_RESIZE:        // Permet le redimensionnement du terminal
            key_name = "Terminal redimensionné (KEY_RESIZE)\n";
            break;
        default: // Pour toutes les autres touches, on affiche le code numérique
            snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "Autre touche : code = %d", key);
            return; // On affiche le code
    }

    /* Met à jour l'état du programme avec le nom de la touche */
    if  (key_name){
        strncpy(state->last_key_pressed, key_name,sizeof(state->last_key_pressed) - 1);
        state->last_key_pressed[sizeof(state->last_key_pressed) - 1] = '\0';
    }
}