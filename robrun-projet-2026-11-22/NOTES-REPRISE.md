# Reprise du chantier menu RobRun

## Etat au 2026-11-22
BUILD_OK. Le DLL est dans build\RobRunInternal.dll.

## Cause racine corrigee
Les onglets ecrivaient dans BR_Config (*br_get()) alors que draw() fait
pull() -> Render() -> push() : push() reecrivait BR_Config depuis la struct
Settings, donc chaque toggle etait annule dans la meme frame.
Desormais les onglets ecrivent dans eternal::Settings via l'accesseur
eternal_ui::settings() (declare dans ui/eternal/eternal_state.hpp), et c'est
push() qui persiste vers BR_Config.

## Architecture des onglets
Tab enum = Aim, Visual, Loot, World, Misc, Config.
draw.cpp dispatche les 6 via Canvas ; components.cpp inclut les .cpp.
eternal_menu.cpp est compile mais mort.
Canvas::Check prend bool&, Slider prend int&, Dropdown prend
(label, int&, std::vector<const char*>, bool, unsigned int*).

## Reste a faire
- Verifier en jeu : armor, health bar, clan tag, police tag, watermark,
  rayon des fleches (pilote aa_fov_px), remplissage Loot/World.
- Verifier que le popup d'ID duplique ne revient pas.
- Restaurer le dossier EasyAntiCheat et les services pour jouer sans le cheat.

## Build
$r = (Get-Item -LiteralPath "C:\Users\mick la sauterelle\Downloads\robrunnnnn\extras_[unknowncheats.me]_").FullName
Set-Location -LiteralPath $r
cmd /c "`"$r\build_internal.bat`""

## Environnement machine
- VS 18 Community, C++20 x64, /MT /GS-, module mappe manuellement.
- HVCI = 0 et VulnerableDriverBlocklistEnable = 0.
- PowerShell admin, Steam lance depuis cette session.
- Chemin avec crochets : utiliser Get-Item -LiteralPath.
