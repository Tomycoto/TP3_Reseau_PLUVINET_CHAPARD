## Fonctionnalités implémentées
1. Connexion|Inscription avec login et mot de passe  
_Enregistrement en dur dans un fichier listant les utilisateurs ; vérification lors de la connexion_  
**Utilisation :** Dans le répertoire Client/, faire `./client ipServeur login/password` (commande d'exécution du client).
2. Discussions en privé (Cardinalité 1:1)  
_Enregistrement des dates et contenus des messages dans un fichier de discussion_  
**Utilisation :** Dans le répertoire Client/ et une fois le client lancé, faire `@recipient messageToSend`.
3. Création de groupe (Cardinalité 1:n)  
_Enregistrement du groupe en dur dans un fichier listant les groupes_  
**Utilisation :** Dans le répertoire Client/ et une fois le client lancé, faire `#C groupName member1,member2,memberX messageToSend`.
4. Discussions de groupe (Cardinalité 1:n)   
_Enregistrement des dates et contenus des messages dans un fichier de discussion de groupe, et partage du message à tous les membres du groupe_  
**Utilisation :** Dans le répertoire Client/ et une fois le client lancé, faire `@groupName messageToSend`. Notons que l'utilisation est la même que pour l'envoi d'un message privé, par soucis de facilité et praticité pour l'utilisateur.
5. Fonctionnalité d'historique
_À la connexion, affichage de l'historique des messages reçus par l'utilisateur lorsqu'il était hors-ligne. Ces messages sont aussi enregistrés en dur dans les conversations du client (privées ou groupes)_  
**Utilisation :** Dans le répertoire Client/ et une fois le client lancé, l'historique des messages reçus pendant la déconnexion est automatiquement affiché.
6. Affichage de la liste des options disponibles  
**Utilisation :** Dans le répertoire Client/ et une fois le client lancé, faire `#O`.
5. Affichage de la liste des clients connectés  
**Utilisation :** Dans le répertoire Client/ et une fois le client lancé, faire `#L`.
6. Affichage de la liste des groupes dont le client fait partie  
**Utilisation :** Dans le répertoire Client/ et une fois le client lancé, faire `#G`.

## Compilation et exécution du programme
Le programme comprend 2 types d'acteurs : les clients et le serveur.
> :warning: **Le server doit être exécuté avant les clients pour que ceux-ci puissent se connecter**  
###__Client__  
* Compilation: Se placer dans le répertoire Client/ et faire `make client`.
* Exécution: Rester dans le répertoire Client/ et faire `./client ipServeur login/password`.
###__Serveur__
* Compilation: Se placer dans le répertoire Serveur/ et faire `make server`.
* Exécution: Rester dans le répertoire Serveur/ et faire `./server`.
