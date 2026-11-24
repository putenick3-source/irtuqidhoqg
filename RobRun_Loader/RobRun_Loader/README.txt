ROB RUN - loader
================

start.bat verifie tout avant d'agir et s'arrete avec la correction si un prerequis manque.
Tu n'as rien a controler a la main : lance start.bat en administrateur et suis les messages.

Ce qu'il verifie, dans l'ordre
------------------------------
1. Droits administrateur          -> relance le fichier en administrateur
2. Presence des 4 fichiers        -> re-dezippe, l'antivirus a peut-etre supprime un fichier
3. Version de Windows             -> refuse sous Windows 10 22H2
4. Memoire virtuelle securisee    -> desactive-la et redemarre
5. Exclusion antivirus            -> propose de l'ajouter, accepte
6. Jeu lance                      -> lance ROB RUN et connecte-toi
7. Hook de capture OBS            -> note seulement, ne bloque pas (OBS en admin + capture de jeu conseilles)
8. Mapping du driver, puis injection

Regles
------
- Une seule injection par session de jeu. Ne relance pas start.bat dans un process deja injecte.
- Si une etape echoue, REDEMARRE le PC avant de reessayer. Un driver deja charge laisse un etat
  sale et l'injection suivante fait tomber le jeu.
- Le DLL correspond a la version actuelle du jeu. Apres une mise a jour du jeu, il faut un autre DLL.

Utilisation
-----------
1. Lance OBS en administrateur, source Capture de jeu sur la fenetre de ROB RUN, apercu visible.
2. Lance ROB RUN, connecte-toi.
3. Clic droit sur start.bat -> Executer en tant qu'administrateur.
4. Attends "Pret", retourne en jeu, appuie sur INSERT (HOME si rien ne s'ouvre).

Fichiers
--------
start.bat                     lanceur
loader.ps1                    verifications et injection
builds\orelia-mapper.exe      mappe le driver
builds\orelia-ioctl.sys       driver vulnerable
builds\ioctl-injector.exe     injecteur
builds\RobRunInternal.dll     module
