# Environnement et conventions

## Chemins
- Racine projet : C:\Users\noahc\Downloads\ROBINTERNAL\robrun-projet-2026-11-22
- $Root explicite a chaque session, sinon le recursif ramasse extras_backup.
- DLL chargee : RobRun_Loader\RobRun_Loader\builds\RobRunInternal.dll
  (dossier double, volontaire).
- Sortie de build : build\RobRunInternal.dll.

## Outillage
- VS C++20 x64, /MT et /GS-, module mappe manuellement.
- HVCI = 0, VulnerableDriverBlocklistEnable = 0, PowerShell admin, Steam
  lance depuis la meme session.
- Pas de dossier tools/. Il2CppDumper refuse la metadata v39 : tout passe par
  rr_il2cpp_dump.c et l API d embedding.

## Interaction
- PowerShell uniquement, jamais d edition manuelle.
- Une commande par message, ou tout dans un seul bloc.
- Ne pas coller la ligne de prompt : elle produit une erreur de parametre
  positionnel.
- -LiteralPath partout, certains chemins ont des crochets.
- %VAR% non expanse : utiliser $env:LOCALAPPDATA.

## Patch
- Sauvegarde <fichier>.modifs-backup-<tag> avant toute modification.
- Remplacement litteral ou regex sur le texte complet, jamais .Replace
  multi-lignes (fins de ligne differentes, echec silencieux).
- Affichage numerote apres chaque patch : applique ne veut pas dire correct.
- Aucun fragment orphelin apres suppression de lignes, recompiler tot.
- Declarations en tete de bloc dans les .c (compilation C).
- Un toggle doit tester le toggle, pas seulement recevoir un callback.

## Fragilites
- web_scrape et questionnaire echouent parfois : commandes courtes.
- Les collages longs sont interrompus : decouper.