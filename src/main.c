#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/stat.h>
#include "ui/key_detector.h"
#include "network/network_SSH.h"
#include "process/Processus.h"
#include "tool/tool.h"

int opt; // c'est tout ce qu'on met après ./projet, ce qu'on va analyser
int dry_run = 0;      // va servir à tester l'accès à la liste des process sans les afficher (si c'est 1, on teste sans afficher, si non on affichera )
int all = 0;          // va servir à récup en distance et en local si c'est 1, qu'en local sinon
char *remote_config = NULL;   // c'est le chemin vers le fichier de config pour les trucs à distance
char *connexion_type = NULL;  // on choisit ssh ou telnet
int port = -1;                // port de connexion (-1 par défaut si on le donne pas)
char *login = NULL;            // login, genre win
char *remote_server = NULL;    // nom DNS ou IP de la machine distante
char *username = NULL;          // nom d'utilisateur pour la connexion à distance
char *password = NULL;          // mot de passe pour la connexion à distance
int max_y, max_x;   //taille fenetre 

// On définit les options longues, que -h = --help
struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"dry-run", no_argument, &dry_run, 1},
    {"remote-config", required_argument, 0, 'c'},
    {"connexion-type", required_argument, 0, 't'},
    {"port", required_argument, 0, 'P'},
    {"login", required_argument, 0, 'l'},
    {"remote-server", required_argument, 0, 's'},
    {"username", required_argument, 0, 'u'},
    {"password", required_argument, 0, 'p'},
    {"all", no_argument, 0, 'a'},
    {0, 0, 0, 0} // stop
};

int get_arg(int argc, char *argv[]){

    while ((opt = getopt_long(argc, argv, "hc:t:P:l:s:u:p:a", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                printf("\nMode d'emploi du programme :\n");
                printf(" -h, --help               : affiche ce message\n");
                printf(" --dry-run                : teste l'accès aux processus sans rien afficher\n");
                printf(" -c, --remote-config FILE : chemin vers le fichier de config des machines distantes\n");
                printf(" -t, --connexion-type TYPE: type de connexion distante (ssh ou telnet)\n");
                printf(" -P, --port PORT           : port à utiliser pour la connexion à distance\n");
                printf(" -l, --login LOGIN         : win@machine_distante pour la connexion à distance\n");
                printf(" -s, --remote-server HOST  : nom DNS ou IP de la machine distante\n");
                printf(" -u, --username USER       : nom d'utilisateur pour la connexion à distance\n");
                printf(" -p, --password PASS       : mot de passe pour la connexion à distance\n");
                printf(" -a, --all                 : récupere les processus locaux et distants\n\n");
                return -1;
                break;

            case 'c':
                remote_config = optarg;
                break;

            case 't':
                connexion_type = optarg;
                break;

            case 'P':
                port = atoi(optarg);
                break;

            case 'l':
                login = optarg;
                break;

            case 's':
                remote_server = optarg;
                break;

            case 'u':
                username = optarg;
                break;

            case 'p':
                password = optarg;
                break;

            case 'a':
                all = 1;
                break;

            case 0: // pour les options qui mettent directement à jour leur variable
                break;

            default:
                fprintf(stderr, "Option inconnue ! Utilise -h ou --help pour voir comment ça fonctionne !!.\n");
                return 1;
        }
    }

    // Gestion du fichier de configuration distant
    if (remote_config == NULL) {
        remote_config = ".config"; // nom par défaut
    }
    // Vérification que le fichier existe
    struct stat st;
    if (stat(remote_config, &st) == 0) {
        // Vérifier que le fichier est caché (nom commence par .)
        if (remote_config[0] != '.') {
            fprintf(stderr, "Alerte : le fichier de config '%s' doit être caché (commencer par '.')\n", remote_config);
            return 1;
        }
        // Vérifier les permissions rw-------
        if ((st.st_mode & 0777) != 0600) {
            fprintf(stderr, "Alerte : le fichier de config '%s' doit avoir les droits rw------- (600)\n", remote_config);
            return 1;
        }
    } else {
        fprintf(stderr, "Le fichier de config '%s' n'existe pas.\n", remote_config);
        return 1;
    }

    // Si aucune option n'a été donnée, on affiche juste les processus locaux
    if (argc == 1) {
        printf("Pas d'option précisée, on se contente d'afficher les processus locaux.\n");
        return 0;
    }

    

    // Si l'utilisateur a donné un remote_server, on vérifie s'il a fourni username/password

    if (remote_server != NULL) {
        if (username == NULL) { // Demande à l'utilisateur de saisir le nom d'utilisateur pour cette machine distante
            printf("Entrez le nom d'utilisateur pour %s : ", remote_server);
            char buf[128];
            if (fgets(buf, sizeof(buf), stdin)) {
                buf[strcspn(buf, "\n")] = 0; // retire le caractère de fin de ligne '\n'
                username = strdup(buf); // copie la chaîne saisie dans username
            }
        }

        if (password == NULL) { // Demande à l'utilisateur de saisir le mot de passe pour cette machine distante
            printf("Entrez le mot de passe pour %s : ", remote_server);
            char buf[128];
            if (fgets(buf, sizeof(buf), stdin)) {
                buf[strcspn(buf, "\n")] = 0;
                password = strdup(buf);
            }
        }
    }

    // Si l'utilisateur a donné login user@server et pas de password
    if (login != NULL && password == NULL) {
        char *at = strchr(login, '@'); // cherche le caractère '@' dans login
        if (at && username == NULL) {
            *at = 0; // coupe la chaîne à '@'
            username = strdup(login); // username = partie avant '@'
            login = at + 1;        // login = partie après '@'
        }
        printf("Entrez le mot de passe pour %s@%s : ", username, login);
        char buf[128];
        if (fgets(buf, sizeof(buf), stdin)) {
            buf[strcspn(buf, "\n")] = 0; // supprime le retour à la ligne
            password = strdup(buf); // stocke le mot de passe
        }
    }
    return 0;
}

int main(int argc, char *argv[]){
    int argument = get_arg(argc, argv);

    if (argument == 1) { //erreur dans les arguments
        return 1;
    }

    if (argument == -1) { //pas de probleme mais on execute rien rien
        return 0;
    }

    WINDOW *main_work;
    programme_state state = {.is_running = 1};
    int tout = 200; //dt du refesh

    // initialisation
    strcpy(state.last_key_pressed, "aucune");
    state.selected_pid = 0;
    main_work = initialize_ncurses();
    if (main_work == NULL){
        return 1;
    }
    wtimeout(main_work, tout); //definition du refresh 
    // on récupère les processus
    list_proc lproc = NULL;
    char **dirs = get_list_dirs("/proc");
    if (dirs != NULL) {
        int err = get_all_proc(&lproc, NULL, dirs, LOCAL);
        destoy_char(dirs);
        if (err != 0) {
            endwin();
            printf("ERREUR: get_all_proc = %d\n", err);
            return 1;
        }
    }
    proc *selected_proc = lproc;
    proc *temp = NULL;

    while (state.is_running) {
        int ch = wgetch(main_work);
        
        // on compte le nb total de processus
        getmaxyx(main_work, max_y, max_x);

        if (ch != ERR) {
            // on lit les touches
            char old_key[64];
            strcpy(old_key, state.last_key_pressed);
            handle_input(&state, ch);
        }
        char *lkp = state.last_key_pressed;
        // flèche haut
        if (strstr(lkp, "Flèche/pavier haut") != NULL){ //fleche haut
            if (selected_proc->prev != NULL) {
                selected_proc = selected_proc->prev;
            }
            strcpy(state.last_key_pressed, ""); 
        }

        // flèche bas
        else if (strstr(lkp, "Flèche/pavier bas") != NULL) { //fleche bas
            if (selected_proc->next != NULL) {
                selected_proc = selected_proc->next;
            }
            strcpy(state.last_key_pressed, "");
        }

        draw_ui(main_work, &state, lproc, selected_proc);
        dirs = get_list_dirs("/proc");
        update_l_proc(&lproc, NULL, dirs, LOCAL);
        if (!lproc) {
            state.is_running = -1;
        }

        temp = lproc;
        while (temp->next && (temp->PID < selected_proc->PID)){
            temp = temp->next;
        }

        if (!temp) {
            state.is_running = -1;
        } else {
            selected_proc = temp;
        }

        wrefresh(main_work);
    }

    // on nettoie !
    endwin();
    while (lproc != NULL) {
        proc *tmp = lproc;
        lproc = lproc->next;
        if (tmp->user) free(tmp->user);
        if (tmp->cmdline) free(tmp->cmdline);
        free(tmp);
    }

    if (state.is_running == -1) {
        FILE *log = fopen(".log", "a");
        if (!log) {
            // éventuellement fallback sur stderr
            fprintf(stderr, "Can't open log file\n");
            return 1;
        }
        fprintf(log, "Erreur app\n");
        fclose(log);
    }

    printf("LP25 Fini\n");
    return 0;
}