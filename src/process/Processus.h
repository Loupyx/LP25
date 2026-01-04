#ifndef PROCESSUS_H
#define PROCESSUS_H
#include <sys/types.h>
#include <time.h>

#include "./../network/network_main.h"

/**
 * Types d'accès possibles à une machine distante ou locale.
 */
enum acces_type{
    SSH,    /**< Accès via le protocole SSH. */
    LOCAL,  /**< Accès local (sur la machine courante). */
    TELNET, /**< Accès via le protocole Telnet. */
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
    long vsize;          /**< Taille mémoire utilisé.  */
    time_t update_time;  /**< Temps de la dernière mise à jour du processus */
    struct proc_ *next;  /**< Pointeur vers le processus suivant dans la liste. */
    struct proc_ *prev;  /**< Pointeur vers le processus précédent dans la liste. */
} proc;

/**
 * Liste chaînée de processus.
 *
 * Représentée par un pointeur vers le premier élément proc de la liste,
 * ou NULL si la liste est vide.
 */
typedef proc *list_proc;

/**
 * Ajoute un processus à la fin d'une file (liste chaînée de processus).
 *
 * \param list Tête de la liste de processus (peut être NULL si vide).
 * \param p    Processus à ajouter à la file.
 * \return Nouvelle tête de liste (identique ou mise à jour).
 */
proc *add_queue_proc(proc *list, proc *p);

/**
 * Récupère la liste des processus selon le mode d'accès demandé.
 *
 * En fonction du type de connexion (LOCAL, SSH, TELNET), remplit une liste
 * chaînée de processus à partir des informations trouvées localement ou à
 * distance (par exemple via /proc ou des commandes exécutées sur la machine cible).
 *
 * \param lproc     Pointeur vers la tête de liste de processus à remplir.
 * \param state     État de la session SSH à utiliser si la connexion est SSH (peut être NULL sinon).
 * \param connexion Type d’accès utilisé (LOCAL, SSH ou TELNET).
 * \return 0 en cas de succès, une valeur non nulle en cas d’erreur.
 */
int get_all_proc(proc **lproc, server *serv);

int update_l_proc(list_proc *lproc, server *serv);

/**
 * Affiche les informations d'un processus.
 *
 * \param p Pointeur vers la structure proc à afficher.
 */
void print_proc(proc *p);

/**
 * Affiche la liste des processus système.
 *
 * Parcourt la liste chaînée de processus et affiche les informations
 * de chaque processus (PID, PPID, utilisateur, commande, état, CPU, etc.)
 * dans un format lisible.
 *
 * \param l Liste de processus à afficher (tête de liste).
 */
void print_l_proc(list_proc l);

/**
 * Affiche l'utilisation CPU de tous les processus de la liste.
 *
 * \param p Pointeur vers le premier élément de la liste de processus.
 */
void print_cpu(proc *p);

/**
 * Envoie une action à un processus via un signal.
 *
 * Utilise le PID fourni pour envoyer le signal donné au processus ciblé,
 * en associant ce signal à un nom d'action pour la journalisation ou l'affichage.
 *
 * \param pid           PID du processus cible.
 * \param action_signal Signal à envoyer (par ex. SIGTERM, SIGKILL, SIGSTOP).
 * \param action_name   Nom symbolique de l'action (par ex. "kill", "stop"), utilisé pour les messages.
 * \return 0 en cas de succès, une valeur non nulle en cas d'erreur (par ex. si le signal n'a pas pu être envoyé).
 */
int send_process_action(pid_t pid, int action_signal, const char *action_name);

#endif