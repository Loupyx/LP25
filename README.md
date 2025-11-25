# LP25
le projet consiste à concevoir un programme de gestion de processus pour systèmes Linux, capable de fonctionner aussi bien en local que sur des hôtes distants. L’objectif est de fournir une interface interactive, inspirée de l’outil ```htop```, permettant :

- d’afficher dynamiquement la liste des processus actifs sur une ou plusieurs machines,
- de visualiser des informations détaillées (PID, utilisateur, consommation CPU/mémoire, temps d’exécution, etc.),
- et d’interagir directement avec les processus (mise en pause, arrêt, reprise, redémarrage). L’outil reposera sur des mécanismes standards de communication et d’administration à distance (SSH, etc.), compatibilité avec les principales distributions Linux.


## Dépendance du projet
Ce projet dépend de plusieurs bibliothèques :
- Ncurses
- libssh

Pour les installer sur Ubuntu faites :\
```sudo apt install libncurses5-dev libncursesw5-dev libssh-dev```

## Configuration
Une fois toutes les dépendances installez faites\
```cp .config.example .config```\
Puis remplissez le fichier comme indiqué.
