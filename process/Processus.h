#ifndef PROCESSUS_H
#define PROCESSUS_H

typedef struct proc{
    int PID;
    int PPID;
    char *user;
    char *cmdline;
    char state;
    double CPU;
    double time;
    struct proc *next;
}Proc;

int Get_processus(Proc **lproc);

void print_proc(Proc *p);
void print_CPU(Proc *p);

#endif