#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include "Processus.h"

#define SIZE_CHAR 300

int is_number(const char *s) {
    for (size_t i = 0; s[i]; i++)
        if (!isdigit((unsigned char)s[i]))
            return 0;
    return 1;
}

void print_proc(Proc *p){
    printf(
        "------------------------------------\nPID : %d\nPPID : %d\nUser : %s\nName : %s\nState : %c\nCPU : %.3f\nTIME : %f\n",
        p->PID, p->PPID, p->user, p->cmdline,p->state, p->CPU, p->time);

}

void print_CPU(Proc *p){
    printf("CPU : %.3f\n", p->CPU);

}

Proc* create_proc(){
    //initialisation d'un nouveau processus
    Proc *new_proc = (Proc*)calloc(1, sizeof(Proc));
    if(!new_proc){
        printf("new_proc\n");
        return NULL;
    }
    new_proc->cmdline = (char*)calloc(SIZE_CHAR,sizeof(char));
    if(!new_proc->cmdline){
        printf("cmdline\n");
        return NULL;
    }
    new_proc->user = (char*)malloc(SIZE_CHAR*sizeof(char));
    if(!new_proc->user){
        printf("user\n");
        return NULL;
    }
    new_proc->next = NULL;
    return new_proc;
}

int Get_processus(Proc **lproc){
    struct dirent *entry;
    DIR *dir = opendir("/proc");
    if(!dir){
        return EXIT_FAILURE;
    }
    while ((entry = readdir(dir))) {
        if(is_number(entry->d_name)){
            char path[SIZE_CHAR],word[SIZE_CHAR], temp[SIZE_CHAR];
            int cpt_word = 0;
            FILE *fichier = NULL;
            long int utime = 0, stime = 0;
            Proc *new_proc = create_proc();
            if(!new_proc){
                return EXIT_FAILURE;
            }

            //récupe la commande du processus
            sprintf(path, "/proc/%s/cmdline", entry->d_name);
            fichier = fopen(path, "r");
            if(!fichier){
                return EXIT_FAILURE;
            }

            if (fscanf(fichier, "%127s", word) == 1) {
                strcpy(new_proc->cmdline, word);
            }
            fclose(fichier);
            //fin

            //récupére PID/PPID/name/state/time
            sprintf(path, "/proc/%s/stat", entry->d_name);
            fichier = fopen(path, "r");
            while (fscanf(fichier, "%127s", word) == 1) {
                cpt_word++;
                switch (cpt_word)
                {
                case 1:
                    new_proc->PID = atoi(word);
                    break;

                case 2:
                    if(strcmp(new_proc->cmdline, "") == 0){
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
            sprintf(path, "/proc/%s/status", entry->d_name);
            fichier = fopen(path, "r");
            if(!fichier){
                return EXIT_FAILURE;
            }

            while(fscanf(fichier, "%127s", word) == 1){
                if(strcmp(word, "Uid:") == 0){
                    if(fscanf(fichier, "%127s", temp) != 1){
                        return EXIT_FAILURE;
                    }
                    struct passwd *pw = getpwuid(atoi(temp));
                    if(!pw){ return EXIT_FAILURE;}
                    strcpy(new_proc->user, pw->pw_name);
                }
            }
            fclose(fichier);
            //fin

            new_proc->CPU = 0;

            if(new_proc->state == 'S'){
                if(*lproc == NULL){
                    *lproc = new_proc;
                }else {
                    new_proc->next = *lproc;
                    *lproc = new_proc;
                }
            } else{
                free(new_proc->cmdline);
                free(new_proc->user);
                free(new_proc);
            }
        }
    }
    return EXIT_SUCCESS;
}