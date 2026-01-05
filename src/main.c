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
#include "network/network_main.h"
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
int test_mode = 0; //mode test pour les devs

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
    {"test-mode", no_argument, &test_mode, 1},
    {0, 0, 0, 0} // stop
};

int get_arg(int argc, char *argv[]) {

    while ((opt = getopt_long(argc, argv, "hc::t:P:l:s:u:p:a", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                printf("\nMode d'emploi du programme :\n");
                printf(" -h, --help                : affiche ce message\n");
                printf(" --dry-run                 : teste l'accès aux processus sans rien afficher\n");
                printf(" -c, --remote-config FILE  : chemin vers le fichier de config des machines distantes\n");
                printf(" -t, --connexion-type TYPE : type de connexion distante (ssh ou telnet)\n");
                printf(" -P, --port PORT           : port à utiliser pour la connexion à distance\n");
                printf(" -l, --login LOGIN         : win@machine_distante pour la connexion à distance\n");
                printf(" -s, --remote-server HOST  : nom DNS ou IP de la machine distante\n");
                printf(" -u, --username USER       : nom d'utilisateur pour la connexion à distance\n");
                printf(" -p, --password PASS       : mot de passe pour la connexion à distance\n");
                printf(" -a, --all                 : récupere les processus locaux et distants\n\n");
                return -1;
                break;

            case 'c':
                // Gestion du fichier de configuration distant
                if (optarg == NULL) {
                    remote_config = ".config"; // nom par défaut
                } else {
                    remote_config = optarg;
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

    // Si aucune option n'a été donnée, on affiche juste les processus locaux
    if (argc == 1) {
        write_log("Pas d'option précisée, on se contente d'afficher les processus locaux.");
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
    }

    if (login) {
        char *at = strchr(login, '@'); // cherche le caractère '@' dans login
        if (at && username == NULL) {
            *at = 0; // coupe la chaîne à '@'
            username = strdup(login); // username = partie avant '@'
            remote_server = at + 1;        // remote_server = partie après '@'
        }

    }

    // Si l'utilisateur a donné login user@server et pas de password
    if (remote_server != NULL && password == NULL) {
        printf("Entrez le mot de passe pour %s@%s : ", username, remote_server);
        char buf[128];
        if (fgets(buf, sizeof(buf), stdin)) {
            buf[strcspn(buf, "\n")] = 0; // supprime le retour à la ligne
            password = strdup(buf); // stocke le mot de passe
        }
    }

    if (remote_server != NULL && connexion_type == NULL) {
        printf("Entrez le mode de connexion (ssh/telnet) : ");
        char buf[128];
        if (fgets(buf, sizeof(buf), stdin)) {
            buf[strcspn(buf, "\n")] = 0; // supprime le retour à la ligne
            connexion_type = strdup(buf); // stocke le type de connexion
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    write_log("--------------------Init new session----------------------\nNb args : %d", argc);
    int argument = get_arg(argc, argv);
    int err = 0;

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
    
    //partie pour la liste des serveurs 
    int error_serv = 0;

    if (remote_config != NULL) { //création de la liste des serveurs distants avec le fichier de config
        write_log("Fichier de configuration distant : %s", remote_config);
        list_serv list = get_serveur_config(remote_config, &error_serv);
        
        if (error_serv != 0) {
            write_log("Erreur lors du chargement de la configuration distante : %d", error_serv);
            return 1;
        }
        state.server_list = list;
    }

    if (password && remote_server && connexion_type && username) { //création d'un serveur avec les infos passées en ligne de commande
        server *serv = create_server("Distant_from_args", remote_server, port, username, password, connexion_type);
        if (!serv) {
            write_log("Échec de la création du serveur distant.");
        } else {
            state.server_list = add_queue(state.server_list, serv);
            write_log("Ajout du serveur distant depuis les arguments de la ligne de commande.");
        }
    } else {
        if (!password) {
            write_log("Pas de mot de passe fourni pour la connexion distante.");
        } else {
            write_log("Mot de passe : %s", password);
        }
        if (!remote_server) {
            write_log("Pas de serveur distant fourni pour la connexion distante.");
        } else {
            write_log("Serveur distant : %s", remote_server);
        }
        if (!connexion_type) {
            write_log("Pas de type de connexion fourni pour la connexion distante.");
        } else {
            write_log("Type de connexion : %s", connexion_type);
        }
        if (!username) {
            write_log("Pas de nom d'utilisateur fourni pour la connexion distante.");
        } else {
            write_log("Nom d'utilisateur : %s", username);
        }

    }

    if (all == 1 || argc <= 2) { //on ajoute le local si on a demandé tout ou rien
        server *local = create_server("Localhost", "localhost", 0, "", "", "local");
        state.server_list = add_queue(state.server_list, local);
    }
    state.current_server = state.server_list;

    if (dry_run == 1) {
        // on teste juste l'accès aux processus locaux
        server *local = create_server("Localhost", "localhost", 0, "", "", "local");
        update_l_proc(NULL, local);
        write_log("Dry-run : accès aux processus locaux réussi.");
        printf("Dry-run : accès aux processus locaux réussi.\n");
        return 0;
    }

    if (state.current_server == NULL) {
        write_log("Aucun serveur disponible pour la connexion.");
        return 1;
    }

    // pre-charge la liste initiale selon la machine choisie
    list_proc lproc = NULL;
    print_list_serv(state.server_list);
    proc *selected_proc = lproc;
    main_work = initialize_ncurses();
    if (main_work == NULL) {
        return 1;
    }
    wtimeout(main_work, tout); //definition du refresh 
    
    write_log("Début de la boucle principale");
    while (state.is_running) {
        int ch = wgetch(main_work); 
        getmaxyx(main_work, max_y, max_x);

        // GESTION DES TOUCHES
        if (ch != ERR) {
            handle_input(&state, ch, &lproc);

            if (ch == '\n' || ch == KEY_ENTER) {
                proc *scan = lproc;
                while (scan) {
                    if (scan->PID == state.selected_pid) {
                        selected_proc = scan; 
                        break;
                    }
                    scan = scan->next;
                }
            }
            else if (ch == KEY_UP && selected_proc && selected_proc->prev) {
                selected_proc = selected_proc->prev;
            } else if (ch == KEY_DOWN && selected_proc && selected_proc->next) {
                selected_proc = selected_proc->next;
            } else if (ch == KEY_F(2)) {
                if (state.current_server->next != NULL) {
                    state.current_server = state.current_server->next;
                }
            } else if (ch == KEY_F(3)) {
                if (state.current_server->prev != NULL) {
                    state.current_server = state.current_server->prev;
                }
            }
        }
        if (test_mode == 0) {
            err = update_l_proc(&lproc, state.current_server->serv);
        }

        // SECURITE DU CURSEUR (Empeche le crash si un processus disparait)
        int found = 0;
        proc *check = lproc;
        while (check) {
            if (check == selected_proc) {
                found = 1;
                break;
            }
            check = check->next;
        }
        if (!found) {
            selected_proc = lproc; 
        }

        draw_ui(main_work, &state, lproc, selected_proc);
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

    if (state.is_running != 0) {
        write_log("Erreur app : %d", state.is_running);
    }

    write_log("LP25 Fini\n");
    return err;
}