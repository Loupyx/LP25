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
#include "./../network/network_main.h"
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
    write_log(
        "------------------------------------\nPID : %d \tPPID : %d\tUser : %s\tcmdline : %s\tState : %c\tCPU : %.3f\tVsize : %ldko\tTIME : %.2f\nupdate : %ld",
        p->PID, p->PPID, p->user, p->cmdline,p->state, p->CPU,p->vsize, p->time, p->update_time
    );
}

void print_l_proc(list_proc l) {
    if (!l) {
        write_log("Liste NULL");
        return;
    }
    list_proc temp = l;
    while (temp) {
        print_proc(temp);
        temp = temp->next;
    }
}

proc *create_proc() {
    //initialisation d'un nouveau processus
    proc *new_proc = (proc*)calloc(1, sizeof(proc));
    if (!new_proc) {
        write_log("ERROR : new_proc");
        return NULL;
    }
    new_proc->cmdline = (char*)calloc(SIZE_CHAR,sizeof(char));
    if (!new_proc->cmdline) {
        write_log("ERROR : cmdline");
        return NULL;
    }
    new_proc->user = (char*)malloc(SIZE_CHAR*sizeof(char));
    if (!new_proc->user) {
        write_log("ERROR : user");
        return NULL;
    }
    new_proc->next = NULL;
    new_proc->prev = NULL;
    return new_proc;
}

char *get_char(char *pid, char *file, server *serv) {
    if (!serv) {
        write_log("get_char : NULL argument");
        return NULL;
    }
    char path[SIZE_CHAR], *text;
    if (serv->connexion_type == SSH && serv->ssh == NULL) {
        write_log("SSH_STATE NULL in *get_stat for PID : %s", pid);
        return NULL;
    }
    snprintf(path, sizeof(path), "/proc/%s/%s", pid, file);

    switch (serv->connexion_type) {
        case SSH:
            text = get_char_ssh(serv->ssh, path);
            break;
        case LOCAL:
            text = get_char_file(path);
            break;
        case TELNET:
            text = get_char_telnet();
            break;
        default:
            write_log("Wrong connexion type");
            return NULL;
            break;
    }
    if (!text) {
        write_log("Cannot get char for : %s", path);

    }
    return text;
}

int get_time(char *pid, proc *p, server *serv) {
    if (!serv) {
        write_log("get_time : NULL argument");
        return 1;
    }
    char *unformated, **data;
    long utime, stime, ticks_per_sec = sysconf(_SC_CLK_TCK), ttime, dtick;
    double sec, cpu;
    double prev_time; // ancienne valeur en secondes
    time_t dt, n_update_time;

    unformated = get_char(pid, "stat", serv);
    if (!unformated) {
        return 1;
    }

    data = split(unformated, ' ');
    free(unformated);
    if (!data || !data[0]) {
        return 1;
    }
    utime = atol(data[14]);
    stime = atol(data[15]);
    ttime = utime + stime;

    ticks_per_sec = sysconf(_SC_CLK_TCK);

    n_update_time = time(NULL);
    dt = n_update_time - p->update_time; // delta temps en s

    // temps CPU total (user+system) en secondes depuis le démarrage du processus
    sec = (double)ttime / (double)ticks_per_sec;

    prev_time = p->time;  // ancienne valeur en secondes
    p->time = sec;        // nouvelle valeur
    destoy_char(data);

    if (dt > 0.1) {
        // différence de ticks CPU entre les deux mesures
        dtick = (long)((sec - prev_time) * (double)ticks_per_sec);

        cpu = (double)dtick / (double)(dt * ticks_per_sec);
        p->CPU = cpu * 100.0;
    }

    p->update_time = n_update_time;
    return 0;
}

proc *get_info(char *pid, server *serv) {
    if (!serv) {
        write_log("get_info : NULL argument");
        return NULL;
    }
    char **data, *unformated;
    proc *new = create_proc();
    if (!new) {
        write_log("Can't alloc new in get_all_proc");
        return NULL;
    }

    unformated = get_char(pid, "stat", serv);       //j'ai modif ici pour masquer les erreurs et donc pas tout casser l'affichage 
    if (!unformated) {
        write_log("return NULL to unformated for : %s", pid);
        return NULL;
    }
    data = split(unformated, ' ');

    if (!data || !data[0]) {
        write_log("split returned empty for '%s'", unformated);
        return NULL;
    }
    free(unformated);

    new->PID = atoi(data[0]);
    strcpy(new->cmdline, data[1]);
    new->state = data[2][0];
    new->PPID = atoi(data[3]);
    new->vsize = atol(data[22])/8000;
    destoy_char(data);
    int error = get_time(pid, new, serv);

    if (error == 1) {
        write_log("erreur get_time for %d", new->PID);
        return NULL;
    }

    unformated = get_char(pid, "status", serv);     //j'ai modif ici pour masquer les erreurs et donc pas tout casser l'affichage 
    if (!unformated) {
        write_log("return NULL to unformated for : %s", pid);
        free(new);
        return NULL;
    }
    data = split(unformated, '\t');

    if (!data || !data[0]) {
        write_log("split returned empty for '%s'", unformated);
        free(new);
        return NULL;
    }

    char **temp = data;
    if (temp && temp[9]) {
        int transpho = atoi(temp[9]);
        struct passwd *pw = getpwuid(transpho);
        if (!pw || !pw->pw_name) { 
            write_log("error pw");
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
        write_log("erreur action : PID invalide (%d)", pid);
        return -1;
    }
    if (kill(pid, action_signal) == 0) {
        write_log("Succes : action '%s' envoyée au PID %d", action_name, pid );
        return 0;
    } else {
        write_log("erreur lors de l'envoie du signal %s au PID %d: %s",action_name, pid, strerror(errno));
        return -1;
    }
}

int get_all_proc(list_proc *lproc, server *serv) {
    if (!serv) {
        write_log("get_all_proc : NULL argument");
        return -1;
    }
    int i = 0;
    proc *list = NULL;
    char **dir;

    switch (serv->connexion_type) {
        case SSH:
            if (serv->ssh == NULL) {
                write_log("SSH_STATE NULL in get_all_proc");
                return -1;
            }
            dir = get_ssh_dir(serv->ssh, "/proc");
            if (!dir) {
                write_log("get_ssh_dir returned NULL in get_all_proc");
                return -1;
            }
            break;
        case LOCAL:
            dir = get_list_dirs("/proc");
            if (!dir) {
                write_log("get_list_dirs returned NULL in get_all_proc");
                return -1;
            }
            break;
        case TELNET:
            break;
        default:
            write_log("Wrong connexion type");
            return -1;
            break;
    }

    while (dir[i]) {
        proc *new = get_info(dir[i], serv);
        if (new != NULL) {
            new->CPU = 0;
            list = add_queue_proc(list, new);
        }
        ++i;
    }

    if (lproc) {
        *lproc = list;
    } else {}
    
    return EXIT_SUCCESS;
}

int update_l_proc(list_proc *lproc, server *serv) {
    if (!serv) {
        write_log("update_l_proc : NULL argument");
        return -1;
    }
    if (lproc == NULL || *lproc == NULL) {
        get_all_proc(lproc, serv);
        return 0;
    }

    char **list_dir;
    switch (serv->connexion_type) {
        case SSH:
            if (serv->ssh == NULL) {
                serv->ssh = init_ssh_session(serv);
                if (serv->ssh == NULL) {
                    write_log("Failed to initialize SSH session for server: %s", serv->name);
                    return -1;
                }
            }
            list_dir = get_ssh_dir(serv->ssh, "/proc");
            if (!list_dir) {
                write_log("get_ssh_dir returned NULL in get_all_proc");
                return -1;
            }
            break;
        case LOCAL:
            list_dir = get_list_dirs("/proc");
            if (!list_dir) {
                write_log("get_list_dirs returned NULL in get_all_proc");
                return -1;
            }
            break;
        case TELNET:
            break;
        default:
            write_log("Wrong connexion type");
            return -1;
            break;
    }

    proc *temp = *lproc;
    while (temp) {
        int find = 0;
        for (int i = 0; list_dir[i]; i++) {
            char pid[32];
            snprintf(pid, sizeof(pid), "%d", temp->PID);
            if (strcmp(list_dir[i], pid) == 0) {
                find = 1;
                if (get_time(list_dir[i], temp, serv) != 0) {
                    write_log("ERROR : get_time for %s", pid);
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
        while (temp) {
            char pid[32];
            snprintf(pid, sizeof(pid), "%d", temp->PID);
            if (strcmp(list_dir[i], pid) == 0) {
                find = 1;
                break;
            }
            temp=temp->next;
        }

        if (find == 0) {
            char *pid =  list_dir[i];
            proc *new = get_info(pid, serv);
            if ( new != NULL) {
                *lproc = add_queue_proc(*lproc, new);
            }
        }
    }
    return 0;
}
