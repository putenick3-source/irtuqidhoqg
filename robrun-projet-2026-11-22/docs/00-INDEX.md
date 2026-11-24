# RobRunInternal - point d entree documentation

Projet : cheat interne IL2CPP pour le jeu (RobRun), DLL mappee manuellement,
menu ImGui in-process, hook Present D3D11.

## A LIRE DANS CET ORDRE
1. 00-INDEX.md (ce fichier) - etat et regles
2. 01-ARCHITECTURE.md - carte des modules, flux de donnees, threads
3. 02-PIPELINE-OFFSETS.md - comment les offsets sont resolus apres un patch
4. 03-ETAT-FONCTIONNALITES.md - ce qui marche, ce qui est casse, ce qui est en cours
5. 04-BUILD-INJECTION-LOGS.md - build, injection, emplacement des logs
6. 05-ENVIRONNEMENT-CONVENTIONS.md - chemins, outillage, pieges connus
7. 06-JOURNAL-DECISIONS.md - historique des decisions et des impasses

## ETAT RESUME (2026-11-24)
- Build : OK (build_internal.bat -> BUILD_OK).
- ESP : fonctionnel (boites, squelettes, noms, barres de vie, armes, distances).
- Pipeline d offsets : fonctionnel, 184/197 resolus, 13 UNRESOLVED conserves.
- Visible check aimbot : EN COURS. Le toggle est desormais lu et gate le
  filtre, instrumentation los.log en place, mais non valide en jeu.
- Skip party : bloque, en attente d un collegue pour tester un groupe.
- Casses : healthbar/armor ESP, crosshair, watermark, sauvegarde config.
- robrun_chain.txt absent -> chain symbols = 0.

## REGLES DE TRAVAIL
- Tout est livre en PowerShell a coller. Jamais d edition manuelle.
- Une seule commande par message, ou tout dans un seul bloc.
- Le projet racine est C:\Users\noahc\Downloads\ROBINTERNAL\robrun-projet-2026-11-22
  et $Root doit etre defini explicitement (le selecteur recursif ramasse
  extras_backup\extras_backup sinon).
- Toute modification de fichier source laisse une sauvegarde
  <fichier>.modifs-backup-<tag>.
- Les headers d offsets sont la source de verite unique. On ne les edite pas a
  la main sans passer par le pipeline (02).
- A la fin de chaque session : mettre a jour 03 et 06.