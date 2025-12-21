#ifndef KEY_DETECTOR_H
#define KEY_DETECTOR_H
#include <sys/types.h>
#include "./../process/Processus.h"

/**
 * Représente l'état global du programme pour l'interface ncurses.
 *
 * is_running indique si la boucle principale doit continuer à s'exécuter,
 * last_key_pressed garde en mémoire la dernière touche saisie par l'utilisateur.
 */
typedef struct {
    int  is_running;              /** Indique si le programme est en cours d'exécution (booléen). */
    char last_key_pressed[128];   /** Dernière touche (ou séquence) pressée par l'utilisateur. */
    pid_t selected_pid;           /**doonne le PID selectionné */
    int is_help_displayed;      /**Indicateur pour afficher le panneau d'aide (1=Oui, 0=Non)*/
    int is_search_active;       /**indique si le mode recherche est actif (1=oui et 0=non) */
    char search_term[128];      /** terme de recherche saisie par l'utilisateur*/ 
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
 * Dessine l'interface utilisateur pour la partie recherche des processus
 *
 * \param work  Fenêtre ncurses dans laquelle dessiner l'interface.
 * \param term   Permet de donner une chaine de caracteres pour trouver un processus.
 * \param lproc        Liste chaînée des processus à afficher.
 * \param max_y Taille maximal de la fenetre en ordonnée.
 */
void draw_search_results(WINDOW *work, list_proc lproc, char *term, int max_y);

/**
 * Dessine l'interface utilisateur dans la fenêtre fournie.
 *
 * \param work  Fenêtre ncurses dans laquelle dessiner l'interface.
 * \param state État courant du programme à refléter dans l'affichage.
 * \param lproc        Liste chaînée des processus à afficher.
 * \param selected_proc Processus actuellement sélectionné par l'utilisateur.
 */
void draw_ui(WINDOW *work, programme_state *state, list_proc lproc, proc *selected_proc);

/**
 * Gère les entrées clavier de l'utilisateur et met à jour l'état du programme.
 *
 * \param state État du programme à modifier en fonction des touches pressées.
 */
void handle_input(programme_state *state, int key);


#endif