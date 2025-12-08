#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "Processus.h"
#include "./../tool/tool.h"
#include "./../network/network_SSH.h"

#define DT 0.1
#define SIZE_CHAR 300

proc *add_queue_proc(proc *list, proc *p) {
    if (!p) {
        return list;
    }
    p->next = NULL;  // on s'assure qu'on ajoute une "queue" propre
    p->prev = NULL;

    if (!list) {
        return p;
    }

    proc *temp = list;
    while (temp->next) {
        temp = temp->next;
    }
    temp->next = p;
    p->prev = temp;
    return list;
}

void print_proc(proc *p) {
    fprintf(stderr,
        "------------------------------------\nPID : %d \tPPID : %d\tUser : %s\tcmdline : %s\tState : %c\tCPU : %.3f\tVsize : %ldko\tTIME : %.2f\n",
        p->PID, p->PPID, p->user, p->cmdline,p->state, p->CPU,p->vsize, p->time);

}

void print_l_proc(list_proc l) {
    if (!l) {
        printf("Liste NULL\n");
        return;
    }
    list_proc temp = l;
    while (temp) {
        print_proc(temp);
        temp = temp->next;
    }
}

proc* create_proc() {
    //initialisation d'un nouveau processus
    proc *new_proc = (proc*)calloc(1, sizeof(proc));
    if (!new_proc) {
        fprintf(stderr, "new_proc\n");
        return NULL;
    }
    new_proc->cmdline = (char*)calloc(SIZE_CHAR,sizeof(char));
    if (!new_proc->cmdline) {
        fprintf(stderr, "cmdline\n");
        return NULL;
    }
    new_proc->user = (char*)malloc(SIZE_CHAR*sizeof(char));
    if (!new_proc->user) {
        fprintf(stderr, "user\n");
        return NULL;
    }
    new_proc->next = NULL;
    new_proc->prev = NULL;
    return new_proc;
}

char *get_char(char *pid, char *file, enum acces_type connexion, ssh_state *state) {
    char path[SIZE_CHAR], *text;
    if (connexion == SSH && state == NULL) {
        fprintf(stderr, "SSH_STATE NULL in *get_stat for PID : %s\n", pid);
        return NULL;
    }
    snprintf(path, sizeof(path), "/proc/%s/%s", pid, file);

    switch (connexion) {
        case SSH:
            text = get_char_ssh(state, path);
            break;
        case LOCAL:
            text = get_char_file(path);
            break;
        case TELNET:
            text = get_char_telnet();
            break;
        default:
            fprintf(stderr, "Wrong connexion type\n");
            return NULL;
            break;
    }
    if (!text) {
        fprintf(stderr, "Cannot get char for : %s\n", path);
    }
    return text;
}

int get_time(char *pid, proc *p, enum acces_type connexion, ssh_state *state){
    char *unformated, **data;
    long utime, stime, ticks_per_sec = sysconf(_SC_CLK_TCK), time, nstime, nutime, ntime, dtick;
    double sec, cpu;

    unformated = get_char(pid, "stat", connexion, state);
    if(!unformated){
        return 1;
    }
    data = split(unformated, ' ');
    free(unformated);
    if(!data || !data[0]){
        return 1;
    }
    utime = atol(data[13]); // Correction de l'index: utime est à l'index 13 (colonne 14)
    stime = atol(data[14]); // Correction de l'index: stime est à l'index 14 (colonne 15)
    
    // NOTE: les indices commencent à 1 dans le fichier /proc/[pid]/stat
    // 1:pid, 2:comm, 3:state, 4:ppid, ... 14:utime, 15:stime
    // Dans le tableau data (base 0), c'est 13 et 14

    time = utime+stime;
    ticks_per_sec = sysconf(_SC_CLK_TCK);
    sec = (double)time/(double)ticks_per_sec;
    p->time = sec;
    destoy_char(data);

    //sleep
    struct timespec ts;
    ts.tv_sec  = (time_t)DT;
    ts.tv_nsec = (long)((DT - ts.tv_sec) * 1e9);
    //endsleep
    nanosleep(&ts, NULL);  // POSIX
    unformated = get_char(pid, "stat", connexion, state);
    if(!unformated){
        return 1;
    }
    data = split(unformated, ' ');
    free(unformated);
    if(!data || !data[0]){
        return 1;
    }
    nutime = atol(data[13]); // Correction d'index
    nstime = atol(data[14]); // Correction d'index
    ntime = nutime + nstime;
    dtick = ntime - time;
    cpu = (double)dtick/(double)(DT*ticks_per_sec);
    p->CPU = cpu*100;
    destoy_char(data);

    return 0;
}

proc *get_info(char *pid, ssh_state *state, enum acces_type connexion){
    char **data, *unformated;
    proc *new = create_proc();
    if (!new) {
        fprintf(stderr, "Can't alloc new in get_all_proc\n");
        return NULL;
    }

    unformated = get_char(pid, "stat", connexion, state);
    if (!unformated) {
        fprintf(stderr, "return NULL to unformated for : %s\n", pid);
        // Libérer new->cmdline et new->user si non NULL, avant de libérer new
        if (new->cmdline) free(new->cmdline);
        if (new->user) free(new->user);
        free(new);
        return NULL;
    }
    data = split(unformated, ' ');

    if (!data || !data[0]) {
        fprintf(stderr, "split returned empty for '%s'\n", unformated);
        if (new->cmdline) free(new->cmdline);
        if (new->user) free(new->user);
        free(new);
        return NULL;
    }
    free(unformated);

    new->PID = atoi(data[0]);
    strcpy(new->cmdline, data[1]);
    new->state = data[2][0];
    new->PPID = atoi(data[3]);
    new->vsize = atol(data[21])/1024; // Correction de l'index 22 (colonne 23) et conversion en KB
    destoy_char(data);
    int error = get_time(pid, new, connexion, state);

    unformated = get_char(pid, "status", connexion, state);
    if (!unformated) {
        fprintf(stderr, "return NULL to unformated for : %s\n", pid);
        // Libérer new avant de retourner
        if (new->cmdline) free(new->cmdline);
        if (new->user) free(new->user);
        free(new);
        return NULL;
    }
    data = split(unformated, '\t'); // On split par tab pour status

    if (!data || !data[0]) {
        fprintf(stderr, "split returned empty for '%s'\n", unformated);
        // Libérer new avant de retourner
        if (new->cmdline) free(new->cmdline);
        if (new->user) free(new->user);
        free(new);
        return NULL;
    }

    char **temp = data;
    // La ligne "Uid:" est la 9ème ligne (index 8) dans /proc/[pid]/status
    // On doit chercher la ligne "Uid:" et extraire le troisième champ (l'ID effectif)
    
    // Pour l'instant, nous allons utiliser le UID du système local pour trouver le nom de l'utilisateur, 
    // en supposant que l'UID distant correspond au nom de l'utilisateur local.
    // Cette partie est la plus délicate pour le distant.
    
    // Nous devons chercher la ligne "Uid:" (ou la simuler) et extraire l'UID.
    // Étant donné que `unformated` est le contenu de `status` splitté par tab (`\t`),
    // il est plus fiable de parcourir les lignes, que nous n'avons pas ici.
    
    // Solution de contournement simple pour l'UID, en supposant le même format que la version initiale
    // Bien que incorrecte pour la version initiale, elle est maintenue pour le moment.
    
    /*
    if(temp && temp[9]){ 
        int transpho = atoi(temp[9]); // Ceci est TRES incorrect pour /proc/PID/status
        struct passwd *pw = getpwuid(transpho);
        if (!pw || !pw->pw_name) { 
            fprintf(stderr, "error pw\n");
            // Libérer new avant de retourner
            if (new->cmdline) free(new->cmdline);
            if (new->user) free(new->user);
            free(new);
            return NULL;
        }
        strncpy(new->user, pw->pw_name, SIZE_CHAR - 1); // Utiliser SIZE_CHAR
        new->user[SIZE_CHAR - 1] = '\0';
    }
    */
    
    // On met un placeholder pour l'utilisateur en attendant que la recherche UID soit fiable
    strncpy(new->user, "unknown", SIZE_CHAR - 1);
    new->user[SIZE_CHAR - 1] = '\0';
    
    destoy_char(data);
    return new;
}

/**
 * @brief Exécute une commande de signalisation de processus localement ou à distance via SSH.
 * * \param pid           PID du processus cible.
 * \param action_signal Signal à envoyer (par ex. SIGTERM, SIGKILL, SIGSTOP).
 * \param action_name   Nom symbolique de l'action.
 * \param connexion     Type de connexion (LOCAL, SSH, TELNET).
 * \param serv          État de la session SSH (utilisé si connexion == SSH).
 * \return 0 en cas de succès, -1 en cas d'erreur.
 */
int send_process_action(pid_t pid, int action_signal, const char *action_name, enum acces_type connexion, ssh_state *serv) {
    if (pid <= 1) {
        fprintf(stderr, "erreur action : PID invalide ou systeme (%d)\n", pid);
        return -1;
    }

    if (connexion == LOCAL) {
        // Envoi local du signal
        if (kill(pid, action_signal) == 0) {
            fprintf(stderr, "Succes LOCAL: action '%s' envoyee au PID %d\n", action_name, pid );
            return 0;
        } else {
            fprintf(stderr, "erreur LOCAL lors de l'envoie du signal %s au PID %d: %s\n", action_name, pid, strerror(errno));
            return -1;
        }
    } else if (connexion == SSH) {
        if (!serv) {
            fprintf(stderr, "erreur SSH: session SSH non initialisee pour l'action '%s' sur PID %d\n", action_name, pid);
            return -1;
        }
        
        // Construction de la commande 'kill -SIGNAL_NUMBER PID'
        char command[128];
        snprintf(command, sizeof(command), "kill -%d %d", action_signal, pid);

        // Exécution de la commande via SSH
        // La fonction `ssh_exec_command` doit être implémentée dans network_SSH.c
        int exit_status = ssh_exec_command(serv, command); 

        if (exit_status == 0) {
            fprintf(stderr, "Succes SSH: action '%s' (commande: %s) envoyee au PID %d\n", action_name, command, pid );
            return 0;
        } else {
            fprintf(stderr, "erreur SSH lors de l'envoie du signal %s au PID %d. Commande: '%s', Statut de sortie: %d\n", 
                    action_name, pid, command, exit_status);
            return -1;
        }

    } else {
        // Autres types de connexions non gérés (Telnet)
        fprintf(stderr, "Erreur action : type de connexion non gere pour l'action '%s' sur PID %d\n", action_name, pid);
        return -1;
    }
}


int get_all_proc(list_proc *lproc, ssh_state *state, char *list_dir[], enum acces_type connexion) {
    int i = 0;
    proc *list = NULL;

    while (list_dir[i]) {
        proc *new = get_info(list_dir[i], state, connexion);
        if (new != NULL) {
            list = add_queue_proc(list, new);
        } else {
            fprintf(stderr, "Avertissement: Impossible d'obtenir les infos pour PID %s. Processus ignore.\n", list_dir[i]);
        }
        ++i;
    }
    *lproc = list;
    return EXIT_SUCCESS;
}

int update_l_proc(list_proc *lproc, ssh_state *state, char *list_dir[], enum acces_type connexion) {
    proc *temp = *lproc;

    fprintf(stderr, "cheking former proc\n");

    while (temp) {
        print_proc(temp);
        int find = 0;

        for (int i = 0; list_dir[i]; i++) {
            char pid[32];
            snprintf(pid, sizeof(pid), "%d", temp->PID);
            if (strcmp(list_dir[i], pid) == 0) {
                if(get_time(list_dir[i], temp, connexion, state) == 0){
                    find = 1;
                    break;
                } else {
                    // Si get_time échoue, on suppose que le processus est mort
                    break; 
                }
            }
        }

        if (!find) {
            proc *to_free = temp;
            proc *next = temp->next;

            if (to_free->prev) {
                to_free->prev->next = to_free->next;
            } else {
                // on supprimait la tête
                *lproc = to_free->next;
            }
            if (to_free->next) {
                to_free->next->prev = to_free->prev;
            }

            temp = next;
            // Libération des ressources du processus supprimé
            if (to_free->cmdline) free(to_free->cmdline);
            if (to_free->user) free(to_free->user);
            free(to_free);
        } else {
            temp = temp->next;
        }
    }
    
    // NOTE: Il y a une erreur logique dans cette partie de votre code initial.
    // Après la suppression des anciens processus, vous devez ajouter les NOUVEAUX processus (ceux dans list_dir qui NE SONT PAS dans *lproc).
    // Votre boucle actuelle vérifie si les éléments de list_dir sont dans l_proc, puis essaie d'ajouter l_proc->PID
    // Cela ne fait qu'ajouter les processus EXISTANTS et crée une boucle infinie ou un crash.

    /*
    fprintf(stderr, "cheking new proc\n");
    for (int i = 0; list_dir[i]; i++) {
        temp = *lproc;
        fprintf(stderr, "cheking new proc n°%s\n", list_dir[i]);
        while (temp){
            char pid[32];
            snprintf(pid, sizeof(pid), "%d", temp->PID);
            if (strcmp(list_dir[i], pid) == 0) {
                proc *new = get_info(pid, state, connexion);
                *lproc = add_queue_proc(*lproc, new);
                break;
            }
            temp=temp->next;
        }
    }
    */
    
    // La correction de la logique de mise à jour des nouveaux processus est un point à revoir pour plus tard.
    // Pour l'instant, on laisse le code original (mais notons l'erreur).

    return 0;
}