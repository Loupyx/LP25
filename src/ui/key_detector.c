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


// fonction qui permet d'enlever les majuscules pour simplififer les recherches dans F4
int starts_with_case(const char *str, const char *term) {
    if (!str || !term) return 0;
    size_t len_term = strlen(term);
    if (len_term == 0) return 1; // une recherche vide correspond à tout

    for (size_t i = 0; i < len_term; i++) {
        // si la chaîne est plus courte que le terme ou si un caractère diffère
        if (!str[i] || tolower((unsigned char)str[i]) != tolower((unsigned char)term[i])) {
            return 0; 
        }
    }
    return 1;
}

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

// partie qui gere la fenetre recherche 
void draw_search_results(WINDOW *work, list_proc lproc, char *term, int max_y) {
    proc *temp = lproc;
    int lignes_affichees = 0;
    int match_count = 0;

    while (temp && (lignes_affichees + 7 < max_y - 2)) {
        int match = 0;
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "%d", temp->PID);
        
        // comparaison avec un numero de PID qui commence par ...
        if (strlen(term) == 0) {
            match = 1;
        } else if (starts_with_case(pid_str, term)) { 
            match = 1;
        }
        // comparaison avec une chaine de caracteres qui commence par ... (en enlevant les '(' et les majuscules)
        else if (temp->cmdline) {
            // verification parenthese 
            char *nom_nettoye = (temp->cmdline[0] == '(') ? &temp->cmdline[1] : temp->cmdline;
            
            // comparaison avec le debut 
            if (starts_with_case(nom_nettoye, term)) {
                match = 1;
            }
        }

        if (match) {
            mvwprintw(work, lignes_affichees + 7, 0, "%-6d\t%-6d\t%-25s\t%-5.1f\t%c\t%.40s",
                      temp->PID, temp->PPID, temp->user ? temp->user : "?",
                      temp->CPU, temp->state, temp->cmdline ? temp->cmdline : "?");
            lignes_affichees++;
            match_count++;
        }
        temp = temp->next;
    }

    if (match_count == 0 && strlen(term) > 0) {
        mvwprintw(work, 8, 0, " [!] Aucun processus ne correspond a : '%s'", term);
    }
}

void draw_ui(WINDOW *work, programme_state *state, list_proc lproc, proc *selected_proc) {
    int max_y, max_x;
    getmaxyx(work, max_y, max_x);
    werase(work);   //permet d'effacer la fenetre d'avant 
    mvprintw(0, 0, "--- Interface HTOP projet LP25 ---");
    
    if (state->current_server == NULL) {
        attron(A_BOLD); 
        mvprintw(1, 0, "MACHINE : [ LOCAL ]");
        attroff(A_BOLD); // On arrête le gras ici
    } else {
        attron(A_BOLD);
        mvprintw(1, 0, "MACHINE : [ %s (%s) ]", state->current_server->serv->name, state->current_server->serv->adresse);
        attroff(A_BOLD); // On arrête le gras ici aussi
    }
    if (state->is_help_displayed) {
        //affiche le panneau d'aide 
        draw_help(work, max_y, max_x);
    } else if (state->is_search_active) {
        mvprintw(2, 0, "Mode recherche activé (ENTREE pour filtrer | F4 ou 'q' pour quitter)");
        mvprintw(4, 0, "Recherche : %s", state->search_term); // Texte simplifié
        mvwprintw(work, 6, 0, "PID\tPPID\tUSER\t\t\t\tCPU\tSTATE\tCMD");   
        draw_search_results(work, lproc, state->search_term, max_y);
        curs_set(1); //permet de mettre a jour l'affichage et de voir ce que l'on écrit 
        // on place le curseur juste après le mot "recherche"
        move(4, strlen("Recherche : ") + strlen(state->search_term));
    }
    else {
        //affiche l'interface noraml 
        int total_proc = 0;
        proc *tmp = lproc;
        while (tmp != NULL) {
            ++total_proc;
            tmp = tmp->next;
        }
        int window_size = max_y - 2;

        mvprintw(2, 0, "Appuyer sur une touche F (F1 à F8) ou q pour quitter ");
        // partie qui affiche dynamiquement l'état des actions realisées sur les processus 
        if (strstr(state->last_key_pressed, "SUCCES")) {
            attron(A_BOLD);
            mvprintw(4, 0, "STATUT : %-60s", state->last_key_pressed);
            attroff(A_BOLD);
        } else if (strstr(state->last_key_pressed, "ERREUR")) {
            attron(A_STANDOUT | A_BOLD); 
            mvprintw(4, 0, "ALERTE : %-60s", state->last_key_pressed);
            attroff(A_STANDOUT | A_BOLD);
        } else {
            // affichage par défaut si aucune action spéciale n'est en cours
            mvprintw(4, 0, "Derniere action : %-60s", state->last_key_pressed);
        }

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
        // c'est ici qu'on va afficher la liste des processus (SIMONNN)
    }
    // affichage commun des raccourcis
    mvprintw(max_y - 1, 0, "[F1] Aide | [F2] Onglet suivant | [F3] Onglet précédant | [F4] Recherche | [F5-F8] Actions processus (voir types d'action dans l'aide [F1]) | [q] Quitter ");

    if (state->is_search_active && !state->is_help_displayed) {
        curs_set(1); // curseur visible
        // On utilise wmove pour être sûr de cibler la bonne fenêtre
        wmove(work, 4, strlen("Recherche : ") + strlen(state->search_term));
    } else {
        curs_set(0); // curseur invisible en mode normal ou aide
    }

    wrefresh(work); 
}


/*gere les entrees du clavier et met a jour l'etat avec le parametre state (etat actuel du prog a modif)*/
void handle_input(programme_state *state, int key, list_proc *lproc){
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
                state->is_help_displayed = 0;
            } else if (state->is_search_active) {
                state->is_search_active = 0; //  ferme juste la recherche
                state->search_term[0] = '\0'; // réinitialise la recherche
            } else {
                state->is_running = 0; // quitte le programme seulement si ni aide ni recherche ne sont actives
            }
            key_name = "'q'";
            break;

        default: 
            // si l'aide est affichée, ignorer toutes les autres touches (F2-F8, etc.)
            if (state->is_help_displayed) {
                 return;
            }

            if (state->is_search_active) {
                int len = strlen(state->search_term);
                
                if (key == '\n' || key == KEY_ENTER) {
                    proc *temp = *lproc;
                    int found_pid = -1;

                    while (temp) {
                        char pid_str[16];
                        snprintf(pid_str, sizeof(pid_str), "%d", temp->PID);
                        char *nom_nettoye = (temp->cmdline && temp->cmdline[0] == '(') ? &temp->cmdline[1] : temp->cmdline;

                        if (starts_with_case(pid_str, state->search_term) || 
                           (nom_nettoye && starts_with_case(nom_nettoye, state->search_term))) {
                            found_pid = temp->PID;
                            break; 
                        }
                        temp = temp->next;
                    }

                    if (found_pid != -1) {
                        state->selected_pid = found_pid; 
                        state->is_search_active = 0;     
                        key_name = "Recherche validée";
                    } else {
                        key_name = "ERREUR : Aucun match";
                    }
                } 
                else if (key == KEY_F(4)) {
                    state->is_search_active = 0;
                    key_name = "Fin de recherche (F4)";
                } 
                else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
                    if (len > 0) state->search_term[len - 1] = '\0';
                } 
                else if (isprint(key) && (len < (int)sizeof(state->search_term) - 1)) {
                    state->search_term[len] = (char)key;
                    state->search_term[len + 1] = '\0';
                }
                
                if (key_name) {
                    strncpy(state->last_key_pressed, key_name, sizeof(state->last_key_pressed) - 1);
                }
                return;
            }
            
            // Traitement des autres touches si l'aide n'est PAS affichée
            switch (key) { 
                case KEY_F(2): // Onglet Suivant
                    if (state->server_list != NULL) {
                    // Si on est en local (NULL), on va au premier serveur
                        if (state->current_server == NULL) {
                            state->current_server = state->server_list;
                        } 
                        // Sinon on avance si possible
                        else if (state->current_server->next != NULL) {
                            state->current_server = state->current_server->next;
                        }
                        if (state->current_server) {
                            snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "Onglet : %s", state->current_server->serv->name);
                        }
                    }
                    break;
                case KEY_F(3): // Onglet Précédent
                    if (state->current_server != NULL) {
                        // Si on est sur le premier maillon, on revient en local
                        if (state->current_server->prev == NULL) {
                            state->current_server = NULL;
                            strcpy(state->last_key_pressed, "Onglet : LOCAL");
                        } else {
                            state->current_server = state->current_server->prev;
                            snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "Onglet : %s", state->current_server->serv->name);
                        }   
                    }
                    break;
                case KEY_F(4):
                    state->is_search_active =1;
                    key_name = "F4 (Recherche)";
                    state->search_term[0] = '\0';
                    break;
                case KEY_F(5):
                    key_name = "F5 (Pause)";
                    erreur = send_process_action(target_pid, SIGSTOP, "Pause");
                    if (erreur == 0) {
                    // utilise SIGSTOP pour mettre en pause un processus (T)
                    snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "SUCCES : PID %d mis en pause", target_pid);
                    } else {
                        snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "ERREUR : Echec sur PID %d", target_pid);
                    }
                    break;
                case KEY_F(6):
                    key_name = "F6 (Arrêt)";
                    // SIGTERM demande au processus de se terminer proprement
                    erreur = send_process_action(target_pid, SIGTERM, "Arrêt");
                    if (erreur == 0) {
                        snprintf(state->last_key_pressed, sizeof(state->last_key_pressed),"SUCCES : Signal Terminaison envoyé au PID %d", target_pid);
                    } else {
                        snprintf(state->last_key_pressed, sizeof(state->last_key_pressed),"ERREUR : Echec signal Arrêt sur PID %d", target_pid);
                    }
                    break;
                case KEY_F(7):
                    key_name = "F7 (Kill)";
                    erreur = send_process_action(target_pid, SIGKILL, "Kill");
                    if (erreur == 0) {
                        snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "SUCCES : PID %d tue par SIGKILL", target_pid);
                    } else {
                        snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "ERREUR : Impossible de tuer le PID %d", target_pid);
                    }
                    break;
                case KEY_F(8):
                    key_name = "F8 (Reprise)";
                    // utilise SIGCONT pour relancer un processus stoppé (S ou R)
                    erreur = send_process_action(target_pid, SIGCONT, "Reprise");
                    if (erreur == 0) {
                        snprintf(state->last_key_pressed, sizeof(state->last_key_pressed),"SUCCES : PID %d relance", target_pid);
                    } else {
                        snprintf(state->last_key_pressed, sizeof(state->last_key_pressed),"ERREUR : Echec reprise sur PID %d", target_pid);
                    }
                    break;
                case KEY_RESIZE:        //permet le redimmensionnement du terminal
                    key_name = "terminal redimensionne (KEY_RESIZE)";
                    break;
                case 258:
                    key_name = "Flèche/pavier bas";
                    break;
                case 259:
                    key_name = "Flèche/pavier haut";
                    break;
                default:
                    // on ne met pas de return ici pour laisser le code du bas s'exécuter
                    snprintf(state->last_key_pressed, sizeof(state->last_key_pressed), "Code touche : %d", key);
                    key_name = NULL; //pour ne pas écraser le snprintf ci-dessus
                    break;
            }
    } // fin du switch 

    // MISE À JOUR FINALE DU MESSAGE (seulement si on n'a pas déjà un message SUCCES/ERREUR)
    if (key_name != NULL) {
        if (!strstr(state->last_key_pressed, "SUCCES") && !strstr(state->last_key_pressed, "ERREUR")) {
             strncpy(state->last_key_pressed, key_name, sizeof(state->last_key_pressed) - 1);
        }
    }
}