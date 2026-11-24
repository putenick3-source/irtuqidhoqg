# Reprise RobRunInternal

Lire docs/00-INDEX.md en premier, puis dans l ordre indique.

Etat : build OK, ESP fonctionnel, pipeline d offsets operationnel (applied=197
diff=0), visible check en cours de diagnostic, skip party bloque faute de
joueur dans le groupe, healthbar/armor/crosshair/watermark/config casses,
robrun_chain.txt absent.

En cours : instrumentation los.log dans rr_aim.c (los_default + ligne
inconditionnelle dans rr_aim_pick). But : savoir si los=0 (synchro menu) ou
res=-1 permanent (pipeline LOS en UNKNOWN, a remplacer par Physics.Linecast).
Details et plan dans docs/03.

Regles : PowerShell uniquement, une commande par message ou tout en un bloc,
jamais d edition manuelle, sauvegarde .modifs-backup-* avant tout patch.