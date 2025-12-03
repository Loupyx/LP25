# LP25
Ce projet consiste à concevoir un programme de gestion de processus pour systèmes Linux, capable de fonctionner aussi bien en local que sur des hôtes distants. L’objectif est de fournir une interface interactive, inspirée de l’outil ```htop```, permettant :

- d’afficher dynamiquement la liste des processus actifs sur une ou plusieurs machines,
- de visualiser des informations détaillées (PID, utilisateur, consommation CPU/mémoire, temps d’exécution, etc.),
- et d’interagir directement avec les processus (mise en pause, arrêt, reprise, redémarrage). L’outil reposera sur des mécanismes standards de communication et d’administration à distance (SSH, etc.), compatibilité avec les principales distributions Linux.

Ce projet prend place dans l'UE LP25, enseignée à l'UTMB lors du semestre A25.


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

## Créer un serveur SSH
Pour créer un serveur SSH, il faut installer "openssh-server" avec la commande\
```sudo apt install openssh-server```\
Ensuite vous pouvez démarrer le serveur :\
```sudo systemctl start ssh```\
Ou en exécutant le fichier ```sshstart```\
(Vous pouvez faire : ```sudo systemctl enable ssh``` pour activer le démarrage automatique du serveur à l'allumage de la machine 
et ```sudo systemctl disable ssh``` pour désactiver cette option)\
Vérifier ensuite que le serveur est bien démarrer :\
```sudo systemctl status ssh```\
Regarder bien que la ligne "Active" est la valeur : "active (running)". Le fichier de configuration du serveur est dans ```/etc/ssh/sshd_config```. Si vous voulez relancer le serveur (après avoir modifier la configuration du serveur) faites :\
```sudo systemctl restart ssh```\
Pensez à arreter votre serveur SSH avec :\
```sudo systemctl stop ssh``` \
Et arreter le démarrage à la demande :\
```sudo systemctl stop ssh.socket ssh && sudo systemctl disable ssh.socket ssh```
