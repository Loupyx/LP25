#ifndef PROCESSUS_H
#define PROCESSUS_H
#include <sys/types.h>
#include <time.h>

#include "./../network/network_main.h"

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

/**
 * Met à jour la liste des processus associée à un serveur donné.
 *
 * Récupère à nouveau les processus (localement ou à distance selon la
 * configuration du serveur) et remplace le contenu de la liste pointée
 * par lproc par les informations actualisées.
 *
 * \param lproc Pointeur vers la tête de liste des processus à mettre à jour.
 * \param serv  Serveur pour lequel actualiser la liste des processus.
 * \return 0 en cas de succès, une valeur non nulle en cas d'erreur.
 */
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
 * Crée et initialise une structure proc vide.
 *
 * Alloue dynamiquement une structure proc, initialise ses champs
 * (PID, PPID, utilisateur, commande, etc.) avec des valeurs par défaut
 * et renvoie un pointeur vers cette structure.
 *
 * \return Pointeur vers la structure proc créée, ou NULL en cas d'erreur.
 */
proc *create_proc();

/**
 * Lit le contenu d'un fichier d'information de processus.
 *
 * Construit le chemin vers le fichier (par exemple dans /proc/<pid>/file
 * localement ou via SSH) en utilisant le PID, le nom de fichier et le serveur,
 * puis renvoie son contenu dans un buffer terminé par '\0'.
 * La mémoire doit être libérée par l'appelant.
 *
 * \param pid   PID du processus sous forme de chaîne.
 * \param file  Nom du fichier d'information à lire (par exemple "stat", "status").
 * \param serv  Serveur sur lequel le processus s'exécute.
 * \return Pointeur vers le buffer contenant le texte lu, ou NULL en cas d'erreur.
 */
char *get_char(char *pid, char *file, server *serv);

/**
 * Récupère et calcule le temps CPU d'un processus.
 *
 * Lit les informations nécessaires (par exemple dans /proc/<pid>/stat)
 * via get_char, extrait le temps d'exécution et le stocke dans la structure proc.
 *
 * \param pid  PID du processus sous forme de chaîne.
 * \param p    Pointeur vers la structure proc à mettre à jour.
 * \param serv Serveur sur lequel le processus s'exécute.
 * \return 0 en cas de succès, une valeur non nulle en cas d'erreur.
 */
int get_time(char *pid, proc *p, server *serv);

/**
 * Récupère toutes les informations sur un processus donné.
 *
 * À partir du PID et du serveur, lit les différents fichiers d'information
 * (commande, utilisateur, état, temps CPU, etc.) et remplit une nouvelle
 * structure proc avec ces données.
 *
 * \param pid  PID du processus sous forme de chaîne.
 * \param serv Serveur sur lequel le processus s'exécute.
 * \return Pointeur vers la structure proc remplie, ou NULL en cas d'erreur.
 */
proc *get_info(char *pid, server *serv);

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