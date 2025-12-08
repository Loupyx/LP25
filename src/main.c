#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "ui/key_detector.h"
#include "network/network_main.h"
#include "network/network_SSH.h"
#include "process/Processus.h"
#include "tool/tool.h"

// Définition de la fréquence de rafraîchissement
#define UI_REFRESH_DELAY_US 50000 // Délai de 50ms pour le rafraîchissement de l'UI
#define PROCESS_UPDATE_INTERVAL_TICKS 20 // 20 * 50ms = 1000ms (Mise à jour des processus toutes les 1 seconde)

/* Fonction principale du programme */
int main() {
    int error;
    WINDOW *main_window = NULL;
    
    // Initialisation de l'état du programme
    programme_state state = {0}; 
    state.is_running = 1;
    state.selected_pid = 0; // Aucun PID sélectionné initialement

    // 1. Initialisation SSH et configuration (Connexion LOCAL par défaut pour l'exemple)
    list_serv l = get_serveur_config(NULL, &error);
    if (!l) {
        fprintf(stderr, "Erreur lors de la récupération de la configuration serveur.\n");
        return 1;
    }
    ssh_state *serv = init_ssh_session(l->serv); // Utilise la première configuration
    enum acces_type current_connexion = LOCAL; 

    // 2. Initialisation et chargement initial des processus
    char **l_dir = get_list_dirs("/proc");
    if (!l_dir) {
        fprintf(stderr, "Erreur lors de la récupération des répertoires /proc.\n");
        // Ajouter ici les fonctions de libération si elles existent (ex: free_list_serv(l))
        return 1;
    }

    list_proc l_proc = NULL;
    error = get_all_proc(&l_proc, serv, l_dir, current_connexion);
    if (error != 0 || !l_proc) {
        fprintf(stderr, "Erreur lors du chargement initial des processus.\n");
        // Ajouter ici les fonctions de libération si elles existent
        return 1;
    }
    // print_l_proc(l_proc); // Ligne de débogage commentée
    
    // 3. Initialisation Ncurses pour l'interface utilisateur
    main_window = initialize_ncurses();
    if (main_window == NULL) {
        fprintf(stderr, "Échec de l'initialisation de l'interface Ncurses.\n");
        // Libération des ressources (processus, SSH, config)
        return 1;
    }
    
    int update_tick = 0;

    // 4. Boucle principale de l'application (Run-Loop)
    while (state.is_running) {
        // A. Gestion des entrées utilisateur (non-bloquante grâce à nodelay(work, TRUE))
        handle_input(&state, current_connexion, serv);

        // B. Mise à jour de la liste des processus (périodique)
        if (update_tick % PROCESS_UPDATE_INTERVAL_TICKS == 0) {
            fprintf(stderr, "Mise à jour des processus...\n");
            // L'état 'l_proc' est mis à jour en temps réel
            error = update_l_proc(&l_proc, serv, l_dir, current_connexion);
            // Il faudrait ici implémenter la logique pour mettre à jour l'affichage de la liste des processus
        }
        update_tick++;

        // C. Affichage de l'interface utilisateur
        draw_ui(main_window, &state);

        // D. Délai pour gérer la vitesse de rafraîchissement et le CPU
        usleep(UI_REFRESH_DELAY_US);
    }

    // 5. Nettoyage et arrêt
    endwin(); // Termine Ncurses

    // Ajouter ici les fonctions de libération de mémoire
    // ex: destoy_char(l_dir); 
    // ex: free_l_proc(l_proc); 
    // ex: free_list_serv(l); 
    // ex: ssh_close_session(serv); 

    return 0;
}