#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "key_detector.h"




/*fonction main */
int main(int nb_arg, char *liste_arg[]){
    WINDOW *main_work;
    programme_state state = {.is_running = 1};
/*
    //initialisation
    strcpy(state.last_key_pressed, "aucune");
    main_work = initialize_ncurses();
    if (main_work ==NULL){
        return 1;
    }
*/

    for (int i = 1; i < nb_arg; i++) {

    if (strcmp(liste_arg[i], "-h") == 0 || strcmp(liste_arg[i], "--help") == 0) {
        printf("HELP : helpppp !!! SOS je comprends pas le programme !!\n");
    }

    if (strcmp(liste_arg[i], "--dry-run") == 0) {
        printf("DRY-RUN : ici on teste l'accès à la liste\n");
    }

    if (strcmp(liste_arg[i], "-c") == 0 || strcmp(liste_arg[i], "--remote-config") == 0) {
        if (i + 1 < nb_arg && liste_arg[i + 1][0] != '-') {
            printf("C ou REMOTE-CONFIG : %s\n", liste_arg[i + 1]);
            i++;
        } else {
            printf("ERREUR : option %s attend un chemin de fichier\n", liste_arg[i]);
        }
    }

    if (strcmp(liste_arg[i], "-t") == 0 || strcmp(liste_arg[i], "--connexion-type") == 0) {
        if (i + 1 < nb_arg && liste_arg[i + 1][0] != '-') {
            printf("connexion-type : %s\n", liste_arg[i + 1]);
            i++;
        } else {
            printf("ERREUR : option %s attend une valeur\n", liste_arg[i]);
        }
    }

    if (strcmp(liste_arg[i], "-P") == 0 || strcmp(liste_arg[i], "--port") == 0) {
        if (i + 1 < nb_arg && liste_arg[i + 1][0] != '-') {
            printf("port : %s\n", liste_arg[i + 1]);
            i++;
        } else {
            printf("ERREUR : option %s attend un numéro de port\n", liste_arg[i]);
        }
    }

    if (strcmp(liste_arg[i], "-l") == 0 || strcmp(liste_arg[i], "--login") == 0) {
        if (i + 1 < nb_arg && liste_arg[i + 1][0] != '-') {
            printf("login : %s\n", liste_arg[i + 1]);
            i++;
        } else {
            printf("ERREUR : option %s attend un login\n", liste_arg[i]);
        }
    }

    if (strcmp(liste_arg[i], "-s") == 0 || strcmp(liste_arg[i], "--remote-server") == 0) {
        if (i + 1 < nb_arg && liste_arg[i + 1][0] != '-') {
            printf("remote-server : %s\n", liste_arg[i + 1]);
            i++;
        } else {
            printf("ERREUR : option %s attend un DNS ou une IP\n", liste_arg[i]);
        }
    }

    if (strcmp(liste_arg[i], "-u") == 0 || strcmp(liste_arg[i], "--username") == 0) {
        if (i + 1 < nb_arg && liste_arg[i + 1][0] != '-') {
            printf("username : %s\n", liste_arg[i + 1]);
            i++;
        } else {
            printf("ERREUR : option %s attend un nom d'utilisateur\n", liste_arg[i]);
        }
    }

    if (strcmp(liste_arg[i], "-p") == 0 || strcmp(liste_arg[i], "--password") == 0) {
        if (i + 1 < nb_arg && liste_arg[i + 1][0] != '-') {
            printf("password : %s\n", liste_arg[i + 1]);
            i++;
        } else {
            printf("ERREUR : option %s attend un mot de passe\n", liste_arg[i]);
        }
    }

    if (strcmp(liste_arg[i], "-a") == 0 || strcmp(liste_arg[i], "--all") == 0) {
        printf("all\n");
    }

}

/*

    //boucle principale 
    while (state.is_running){
        handle_input(&state);   //lit et traite l'entrée 
        draw_ui(main_work, &state);     //dessine l'inteface 
        usleep(50000);      //50 millisecondes pour limiter l'utilisation du CPU
    }
    //nettoyage et restauration du terminal 
    endwin();
    printf("test clavier termine\n");
    */
    return 0;
}