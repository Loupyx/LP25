#ifndef PROCESSUS_H
#define PROCESSUS_H
#include <sys/types.h>

#include "./../network/network_SSH.h"

enum acces_type{
    SSH,
    LOCAL,
    TELNET,
};

/**
 * Informations sur un processus système.
 *
 * Contient les identifiants du processus, l'utilisateur associé,
 * la ligne de commande, l'état, l'utilisation CPU, le temps
 * d'exécution et un pointeur vers le processus suivant (liste chaînée).
 */
typedef struct proc_{
    int PID;             /**< Identifiant du processus. */
    int PPID;            /**< Identifiant du processus parent. */
    char *user;          /**< Nom de l'utilisateur propriétaire du processus. */
    char *cmdline;       /**< Ligne de commande utilisée pour lancer le processus. */
    char state;          /**< État du processus (par ex. 'R', 'S', 'Z'...). */
    double CPU;          /**< Pourcentage ou part d'utilisation CPU. */
    double time;         /**< Temps CPU consommé ou temps d'exécution. */
    struct proc_ *next;   /**< Pointeur vers le processus suivant dans la liste. */
} Proc;

typedef Proc *list_proc;

/**
 * Ajoute un processus à la fin d'une file (liste chaînée de processus).
 *
 * \param list Tête de la liste de processus (peut être NULL si vide).
 * \param p    Processus à ajouter à la file.
 * \return Nouvelle tête de liste (identique ou mise à jour).
 */
Proc *add_queue_proc(Proc *list, Proc *p);

int get_all_proc(Proc **lproc, ssh_state *state, char *list_dir[], enum acces_type connexion);

/**
 * Récupère la liste des processus et remplit une liste chaînée.
 *
 * \param lproc Pointeur vers la tête de liste de processus à initialiser.
 * \return 0 en cas de succès, une valeur non nulle en cas d'erreur.
 */
int get_processus(Proc **lproc);

/**
 * Affiche les informations d'un processus.
 *
 * \param p Pointeur vers la structure Proc à afficher.
 */
void print_proc(Proc *p);

/**
 * Affiche l'utilisation CPU de tous les processus de la liste.
 *
 * \param p Pointeur vers le premier élément de la liste de processus.
 */
void print_cpu(Proc *p);

int send_process_action(pid_t pid, int action_signal, const char *action_name);


#endif