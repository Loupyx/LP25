#ifndef PROCESSUS_H
#define PROCESSUS_H

typedef struct proc{
    int PID;
    int PPID;
    char *user;
    char *cmdline;
    char state;
    float CPU;
    long int time;
    struct proc *next;
}Proc;

int Get_processus(Proc **lproc);

void print_proc(Proc *p);

#endif