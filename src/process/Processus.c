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
        "------------------------------------\nPID : %d \tPPID : %d\tUser : %s\tcmdline : %s\tState : %c\tCPU : %.3f\tVsize : %ldko\tTIME : %.2f\nupdate : %d\n",
        p->PID, p->PPID, p->user, p->cmdline,p->state, p->CPU,p->vsize, p->time, p->update_time);

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
    long utime, stime, ticks_per_sec = sysconf(_SC_CLK_TCK), ttime, pstime, putime, ptime, dtick;
    double sec, cpu;
    time_t dt, n_update_time;

    unformated = get_char(pid, "stat", connexion, state);
    if(!unformated){
        return 1;
    }
    data = split(unformated, ' ');
    free(unformated);
    if(!data || !data[0]){
        return 1;
    }
    utime = atol(data[14]);
    stime = atol(data[15]);
    n_update_time = time(NULL);
    dt = n_update_time - p->update_time; //delta du temps en s
    ttime = utime+stime;
    ticks_per_sec = sysconf(_SC_CLK_TCK);
    sec = (double)ttime/(double)ticks_per_sec;
    ptime = p->time;
    p->time = sec;
    destoy_char(data);

    ptime = p->time;

    dtick = ttime - ptime*(double)ticks_per_sec;
    cpu = (double)dtick/(double)(dt*ticks_per_sec);
    p->CPU = cpu*100;
    p->update_time = n_update_time;

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
        return NULL;
    }
    data = split(unformated, ' ');

    if (!data || !data[0]) {
        fprintf(stderr, "split returned empty for '%s'\n", unformated);
        return NULL;
    }
    free(unformated);

    new->PID = atoi(data[0]);
    strcpy(new->cmdline, data[1]);
    new->state = data[2][0];
    new->PPID = atoi(data[3]);
    new->vsize = atol(data[22])/8000;
    destoy_char(data);
    int error = get_time(pid, new, connexion, state);

    unformated = get_char(pid, "status", connexion, state);
    if (!unformated) {
        fprintf(stderr, "return NULL to unformated for : %s\n", pid);
        free(new);
        return NULL;
    }
    data = split(unformated, '\t');

    if (!data || !data[0]) {
        fprintf(stderr, "split returned empty for '%s'\n", unformated);
        free(new);
        return NULL;
    }

    char **temp = data;
    if(temp && temp[9]){
        int transpho = atoi(temp[9]);
        struct passwd *pw = getpwuid(transpho);
        if (!pw || !pw->pw_name) { 
            fprintf(stderr, "error pw\n");
            free(new);
            return NULL;
        }
        strncpy(new->user, pw->pw_name, 30);
    }

    new->update_time = time(NULL);

    return new;
}

//implémentation de l'envoi du signal au processus
int send_process_action(pid_t pid, int action_signal, const char *action_name) {
    if (pid <= 1) {
        fprintf(stderr, "erreur action : PID invalide (%d)\n", pid);
        return -1;
    }
    if (kill(pid, action_signal) == 0) {
        fprintf(stderr, "Succes : action '%s' envoyée au PID %d\n", action_name, pid );
        return 0;
    } else {
        fprintf(stderr, "erreur lors de l'envoie du signal %s au PID %d: %s\n",action_name, pid, strerror(errno));
        return -1;
    }
}

int get_all_proc(list_proc *lproc, ssh_state *state, char *list_dir[], enum acces_type connexion) {
    int i = 0;
    proc *list = NULL;

    while (list_dir[i]) {
        proc *new = get_info(list_dir[i], state, connexion);
        new->CPU = 0;
        list = add_queue_proc(list, new);
        ++i;
    }
    *lproc = list;
    return EXIT_SUCCESS;
}

int update_l_proc(list_proc *lproc, ssh_state *state, char *list_dir[], enum acces_type connexion) {
    proc *temp = *lproc;
    while (temp) {
        int find = 0;
        for (int i = 0; list_dir[i]; i++) {
            char pid[32];
            snprintf(pid, sizeof(pid), "%d", temp->PID);
            if (strcmp(list_dir[i], pid) == 0) {
                if(get_time(list_dir[i], temp, connexion, state) == 0){
                    find = 1;
                    break;
                } else {
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
            free(to_free);
        } else {
            temp = temp->next;
        }
    }
    for (int i = 0; list_dir[i]; i++) {
        int find = 0;
        temp = *lproc;
        while (temp){
            char pid[32];
            snprintf(pid, sizeof(pid), "%d", temp->PID);
            if (strcmp(list_dir[i], pid) == 0) {
                find = 1;
            }
            temp=temp->next;
        }

        if (find == 0) {
            char pid[32];
            snprintf(pid, sizeof(pid), "%d", list_dir[i]);
            proc *new = get_info(pid, state, connexion);
            *lproc = add_queue_proc(*lproc, new);
        }
    }

    return 0;
}
