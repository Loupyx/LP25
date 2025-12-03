#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "Processus.h"
#include "./../tool/tool.h"
#include "./../network/network_SSH.h"

#define SIZE_CHAR 300

proc *add_queue_proc(proc *list, proc *p) {
    if (!p) {
        return list;
    }
    p->next = NULL;  // on s'assure qu'on ajoute une "queue" propre

    if (!list) {
        return p;
    }

    proc *temp = list;
    while (temp->next) {
        temp = temp->next;
    }
    temp->next = p;
    return list;
}

void print_proc(proc *p) {
    fprintf(stderr,
        "------------------------------------\nPID : %d\nPPID : %d\nUser : %s\ncmdline : %s\nState : %c\nCPU : %.3f\nTIME : %f\n",
        p->PID, p->PPID, p->user, p->cmdline,p->state, p->CPU, p->time);

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

void print_cpu(proc *p) {
    printf("CPU : %.3f\n", p->CPU);
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
    return new_proc;
}

int get_processus(proc **lproc) { //le vieux n'est plus utiliser
    struct dirent *entry;
    DIR *dir = opendir("/proc");
    if (!dir) {
        return EXIT_FAILURE;
    }
    while ((entry = readdir(dir))) {
        if (is_number(entry->d_name)) {
            char path[SIZE_CHAR],word[SIZE_CHAR], temp[SIZE_CHAR];
            int cpt_word = 0;
            FILE *fichier = NULL;
            long int utime = 0, stime = 0;
            proc *new_proc = create_proc();
            if (!new_proc) {
                return EXIT_FAILURE;
            }

            //récupe la commande du processus
            snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
            fichier = fopen(path, "r");
            if (!fichier) {
                return EXIT_FAILURE;
            }

            if (fscanf(fichier, "%127s", word) == 1) {
                strcpy(new_proc->cmdline, word);
            }
            fclose(fichier);
            //fin

            //récupére PID/PPID/name/state/time
            snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
            fichier = fopen(path, "r");
            while (fscanf(fichier, "%127s", word) == 1) {
                cpt_word++;
                switch (cpt_word) {
                case 1:
                    new_proc->PID = atoi(word);
                    break;

                case 2:
                    if (strcmp(new_proc->cmdline, "") == 0) {
                        strcpy(new_proc->cmdline, word);
                    }
                    break;

                case 3:
                    new_proc->state = word[0];
                    break;

                case 4:
                    new_proc->PPID = atoi(word);
                    break;

                case 14:
                    utime = atol(word);
                    break;

                case 15:
                    stime = atol(word);
                    break;
                
                default:
                    break;
                }
            }
            fclose(fichier);
            long int time = utime+stime;
            long ticks_per_sec = sysconf(_SC_CLK_TCK);
            double sec = (double)time/(double)ticks_per_sec;
            new_proc->time = sec;
            //fin

            //récupére user
            fichier = NULL;
            snprintf(path, sizeof(path), "/proc/%s/status", entry->d_name);
            fichier = fopen(path, "r");
            if (!fichier) {
                return EXIT_FAILURE;
            }

            while (fscanf(fichier, "%127s", word) == 1) {
                if (strcmp(word, "Uid:") == 0) {
                    if (fscanf(fichier, "%127s", temp) != 1) {
                        return EXIT_FAILURE;
                    }
                    struct passwd *pw = getpwuid(atoi(temp));
                    if (!pw) {
                        return EXIT_FAILURE;
                    }
                    strcpy(new_proc->user, pw->pw_name);
                }
            }
            fclose(fichier);
            //fin

            new_proc->CPU = 0;

            if (new_proc->state == 'S') {
                if (*lproc == NULL) {
                    *lproc = new_proc;
                } else {
                    new_proc->next = *lproc;
                    *lproc = new_proc;
                }
            } else {
                free(new_proc->cmdline);
                free(new_proc->user);
                free(new_proc);
            }
        }
    }
    return EXIT_SUCCESS;
}

//implémentation de l'envoie du signal au processus
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

char *get_stat(char *pid, enum acces_type connexion, ssh_state *state) {
    char path[SIZE_CHAR], *text;
    if (connexion == SSH && state == NULL) {
        fprintf(stderr, "SSH_STATE NULL in *get_stat for PID : %s\n", pid);
        return NULL;
    }
    snprintf(path, sizeof(path), "/proc/%s/stat", pid);

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

int get_all_proc(list_proc *lproc, ssh_state *state, char *list_dir[], enum acces_type connexion) {
    int i = 0;
    char **data, *unformated;
    long utime, stime, ticks_per_sec, time;
    double sec;
    proc *list = NULL;

    while (list_dir[i]) {

        unformated = get_stat(list_dir[i], connexion, state);
        if (!unformated) {
            fprintf(stderr, "return NULL to unformated for : %s\n", list_dir[i]);
            i++;
            continue;
        }
        data = split(unformated, ' ');

        if (!data || !data[0]) {
            fprintf(stderr, "split returned empty for '%s'\n", unformated);
            // soit tu skip ce PID, soit tu gères l’erreur
            continue;
        }

        free(unformated);

        proc *new = create_proc();
        if (!new) {
            fprintf(stderr, "Can't alloc new in get_all_proc\n");
            return EXIT_FAILURE;
        }

        if (!data[1] || !data[2] || !data[3] || !data[4] || !data[14] || !data[15]) {
            fprintf(stderr, "ligne /proc/%s/stat pas assez longue\n", list_dir[i]);
            free(new);
        } else {
            new->PID = atoi(data[0]);
            strcpy(new->cmdline, data[1]);
            new->user = "CODE NOM DE DIEU";
            new->state = data[2][0];
            new->PPID = atoi(data[3]);
            utime = atol(data[14]);
            stime = atol(data[15]);
            time = utime+stime;
            ticks_per_sec = sysconf(_SC_CLK_TCK);
            sec = (double)time/(double)ticks_per_sec;
            new->time = sec;
            list = add_queue_proc(list, new);
        }
        
        for (int j = 0; data[j] != NULL; ++j) {
            free(data[j]);
        }
        ++i;
    }
    *lproc = list;
    return EXIT_SUCCESS;
}
