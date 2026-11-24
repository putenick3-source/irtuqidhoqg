# Reprise rapide apres changement de machine

Le chemin du projet change : ne jamais le supposer, le localiser.

## 0. Prerequis machine
- Visual Studio avec charge de travail C++ x64 et Windows SDK.
- HVCI = 0 et VulnerableDriverBlocklistEnable = 0.
- PowerShell en administrateur, Steam lance depuis cette session.

## 1. Localiser le projet
$bat = Get-ChildItem -LiteralPath 'C:\Users' -Recurse -Filter build_internal.bat -File -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch 'modifs-backup|extras_backup' } | Select-Object -First 1
$Root = Split-Path -Parent $bat.FullName
$Root

## 2. Lire l etat avant de toucher au code
Get-ChildItem -LiteralPath (Join-Path $Root 'docs') | Select-Object Name,Length
Puis lire 00-INDEX, 03-ETAT-FONCTIONNALITES et 06-JOURNAL-DECISIONS.
Ne rien redecouvrir : tout est trace.

## 3. Build
Push-Location $Root
cmd /c build_internal.bat 2>&1 | Select-String -Pattern ' error |BUILD_OK|COPIED|BUILD_FAIL' | ForEach-Object { $_.Line }
Pop-Location
Attendu : BUILD_OK puis COPIED. LNK4099 freetype n est pas un echec.

## 4. Verifier le deploiement (obligatoire, jamais sauter)
Le loader charge sa propre copie, pas build\. Localiser son dossier builds :
$l = (Get-ChildItem -LiteralPath 'C:\Users' -Recurse -Filter RobRunInternal.dll -File -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\builds\\' } | Select-Object -First 1).FullName
$l
Comparer avec build\RobRunInternal.dll : meme taille, meme date, chaine
los.log presente dans la copie chargee.
$b = Join-Path $Root 'build\RobRunInternal.dll'
foreach ($p in @($b,$l)) { $f = Get-Item -LiteralPath $p; $s = [System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($p)); "{0} taille={1} ecrit={2} los={3}" -f $p,$f.Length,$f.LastWriteTime,$s.Contains('los.log') }
Copie plus ancienne ou los=False : test invalide, recopier et relancer le jeu.

## 5. Tester le visible check
1. Supprimer los.log dans %LOCALAPPDATA%\RobRunInternal.
2. Fermer le jeu, le relancer, injecter, verifier buildid.log OK.
3. Aimbot + visible check actives, 10 s face a un adversaire derriere un mur.
4. Lire los.log.

## 6. Arbre de decision
- Log ABSENT : deploiement faux (retour a 4), ou rr_aim_pick non atteint.
- pick avec los=0 : le toggle n arrive pas du menu, corriger le bridge.
- los=1 et res=-1 permanent : pipeline LOS en UNKNOWN, remplacer la
  reconstruction BVH par Physics.Linecast resolu via IL2CPP.
- res=0 et res=1 melanges : le filtre fonctionne, valider puis retirer
  l instrumentation.