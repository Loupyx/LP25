#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "ui/key_detector.h"
#include "network/network_SSH.h"
#include "process/Processus.h"
#include "tool/tool.h"

/*fonction main */
int main(int argc, char *argv[]){
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
        {0, 0, 0, 0} // stooooooopppppppppppppppppppp
    };

    // Boucle qui passe en revue toutes les options passées (dans opt ducoup)
    while ((opt = getopt_long(argc, argv, "hc:t:P:l:s:u:p:a", long_options, NULL)) != -1) { // quand ya : après l'option, ça veut dire qu'on attend un argument
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
                exit(0);
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
                fprintf(stderr, "Option inconnue ! Utilise -h ou --help pour voir comment ça fonctionne, nananananèreuh !!.\n");
                exit(1);
        }
    }

    // Si aucune option n'a été donnée, on affiche juste les processus locaux
    if (argc == 1) {
        printf("Pas d'option précisée, on se contente d'afficher les processus locaux.\n");
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
        }
        // Vérifier les permissions rw-------
        if ((st.st_mode & 0777) != 0600) {
            fprintf(stderr, "Alerte : le fichier de config '%s' doit avoir les droits rw------- (600)\n", remote_config);
        }
    } else {
        fprintf(stderr, "Le fichier de config '%s' n'existe pas.\n", remote_config);
    }

    // Si l'utilisateur a donné un remote_server, on vérifie s'il a fourni username/password

    if (remote_server != NULL) {
        if (username == NULL) { // Demande à l'utilisateur de saisir le nom d'utilisateur pour cette machine distante
            printf("Entrez le nom d'utilisateur pour %s : ", remote_server);
            fflush(stdout); // on force l'affichage du prompt avant la saisie
            char buf[128];  // on fait un buffer temporaire pour lire ce qu'on écrit
            if (fgets(buf, sizeof(buf), stdin)) { // lit ce qu'on écrit
                buf[strcspn(buf, "\n")] = 0; // retire le caractère de fin de ligne '\n'
                username = strdup(buf); // copie la chaîne saisie dans username
            }
        }

        if (password == NULL) { // Demande à l'utilisateur de saisir le mot de passe pour cette machine distante
            printf("Entrez le mot de passe pour %s : ", remote_server);
            fflush(stdout); // on force l'affichage du prompt avant la saisie
            char buf[128];  // buffer temporaire pour lire le mot de passe
            if (fgets(buf, sizeof(buf), stdin)) { // lit la saisie utilisateur
                buf[strcspn(buf, "\n")] = 0;      // retire le caractère de fin de ligne '\n'
                password = strdup(buf);           // copie la chaîne saisie dans password
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
		fflush(stdout);
		char buf[128];
		if (fgets(buf, sizeof(buf), stdin)) {
			buf[strcspn(buf, "\n")] = 0; // supprime le retour à la ligne
			password = strdup(buf); // stocke le mot de passe
		}
	}
    /* si besoin pour vérifier ce qui est donné en paramètre
        // Affichage des valeurs pour vérifier ce qui a été passé
        printf("\n===== Résumé des options =====\n");
        printf("dry_run       : %s\n", dry_run ? "oui" : "non");
        if (remote_config) printf("remote_config : %s\n", remote_config);
        if (connexion_type) printf("connexion_type: %s\n", connexion_type);
        if (port != -1) printf("port          : %d\n", port);
        if (login) printf("login         : %s\n", login);
        if (remote_server) printf("remote_server : %s\n", remote_server);
        if (username) printf("username      : %s\n", username);
        if (password) printf("password      : %s\n", password);
        printf("all           : %s\n", all ? "oui" : "non");
        printf("===============================\n");

    */


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