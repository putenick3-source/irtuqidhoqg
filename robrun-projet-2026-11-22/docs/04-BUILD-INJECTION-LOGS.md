# Build, injection, logs

## Build
Push-Location 'C:\Users\noahc\Downloads\ROBINTERNAL\robrun-projet-2026-11-22'
cmd /c build_internal.bat 2>&1 | Select-String -Pattern ' error |BUILD_OK|BUILD_FAIL' | ForEach-Object { $_.Line }
Pop-Location

Sortie attendue : BUILD_OK -> build\RobRunInternal.dll.
Les avertissements LNK4099 sur freetype.pdb sont normaux. Un vrai echec
affiche error C.... puis BUILD_FAIL.

## Injection
1. La copie vers RobRun_Loader\RobRun_Loader\builds\RobRunInternal.dll est
   automatique en fin de build (ligne COPIED). Si elle manque, le DLL injecte
   est l ancien : comparer la date des deux fichiers avant de tester.
2. Lancer le loader, injecter dans le processus du jeu.
3. Verifier buildid.log avant de conclure quoi que ce soit.

Un retour buildid: OK est la precondition de tout test. Si le build ID ne
correspond pas, le resolver d offsets tourne avant la premiere lecture monde.

## Logs
Dans %LOCALAPPDATA%\RobRunInternal\ : buildid.log, autoupdate.log,
rr_internal.log, party.log, los.log, accent.txt.
Dans la racine du projet : inject.log, probe.log, preload.log, map.log,
map_trace.log, selftest.log.

Lecture sans se tromper sur les variables :
$log = Join-Path $env:LOCALAPPDATA 'RobRunInternal\los.log'
if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log -Tail 25 } else { 'ABSENT' }

type/Get-Content n expanse pas %LOCALAPPDATA%.

## Procedure de test d une feature
1. Supprimer le log concerne pour que l absence soit significative.
2. Injecter, verifier buildid OK.
3. Activer la feature, reproduire la situation, 10 s minimum.
4. Lire le log. Ne jamais conclure sur un log absent sans avoir confirme 3.