#ifndef KEY_DETECTOR_H
#define KEY_DETECTOR_H
#include <sys/types.h>

/**
 * Représente l'état global du programme pour l'interface ncurses.
 *
 * is_running indique si la boucle principale doit continuer à s'exécuter,
 * last_key_pressed garde en mémoire la dernière touche saisie par l'utilisateur.
 */
typedef struct {
    int  is_running;              /**< Indique si le programme est en cours d'exécution (booléen). */
    char last_key_pressed[128];   /**< Dernière touche (ou séquence) pressée par l'utilisateur. */
    pid_t selected_pid;
} programme_state;

/**
 * Initialise la bibliothèque ncurses et crée la fenêtre principale.
 *
 * Configure le mode ncurses (mode raw, désactivation de l'écho, etc.)
 * et retourne une fenêtre prête à l'emploi pour l'affichage.
 *
 * \return Pointeur vers la fenêtre principale ncurses, ou NULL en cas d'erreur.
 */
WINDOW *initialize_ncurses();

/**
 * Dessine l'interface utilisateur dans la fenêtre fournie.
 *
 * \param work  Fenêtre ncurses dans laquelle dessiner l'interface.
 * \param state État courant du programme à refléter dans l'affichage.
 */
void draw_ui(WINDOW *work, programme_state *state);

/**
 * Gère les entrées clavier de l'utilisateur et met à jour l'état du programme.
 *
 * \param state État du programme à modifier en fonction des touches pressées.
 */
void handle_input(programme_state *state);


#endif