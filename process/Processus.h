#ifndef PROCESSUS_H
#define PROCESSUS_H

/**
 * Informations sur un processus système.
 *
 * Contient les identifiants du processus, l'utilisateur associé,
 * la ligne de commande, l'état, l'utilisation CPU, le temps
 * d'exécution et un pointeur vers le processus suivant (liste chaînée).
 */
typedef struct proc{
    int PID;             /**< Identifiant du processus. */
    int PPID;            /**< Identifiant du processus parent. */
    char *user;          /**< Nom de l'utilisateur propriétaire du processus. */
    char *cmdline;       /**< Ligne de commande utilisée pour lancer le processus. */
    char state;          /**< État du processus (par ex. 'R', 'S', 'Z'...). */
    double CPU;          /**< Pourcentage ou part d'utilisation CPU. */
    double time;         /**< Temps CPU consommé ou temps d'exécution. */
    struct proc *next;   /**< Pointeur vers le processus suivant dans la liste. */
} Proc;

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


#endif