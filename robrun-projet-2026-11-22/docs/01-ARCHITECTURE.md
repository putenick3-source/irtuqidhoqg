# Architecture

## Deux frontieres, un coeur partage
Le projet a deux consommateurs des memes modules de features :

- `internal/` : module mappe manuellement, menu ImGui in-process, hook Present
  D3D11. C est la cible construite et utilisee aujourd hui.
- `app/` : frontiere alternative (rr_esp.c, rr_spawn.c, rr_startup.c) avec son
  propre hook. Compilee dans l ancien chemin, pas dans build_internal.bat.

`features/`, `game/`, `memory/` sont partages. Une modif de signature dans
`features/` casse les deux frontieres : verifier les deux sites d appel.

## Modules

| Fichier | Role | Etat |
|---|---|---|
| memory/rr_mem.c | primitives de lecture/ecriture (PL, rr_u64, rr_i32, u_is) | stable |
| memory/rr_chain.c | resolution par chaine depuis robrun_chain.txt | inactif, fichier absent |
| game/rr_unity.c | helpers IL2CPP (nom de classe, test de type) | stable |
| game/rr_game.c | decouverte du monde, construction du snapshot, party_refresh, pdiag | stable, instrumentation a retirer |
| game/rr_buildid.c | garde d ID de build (taille attendue) | stable |
| game/rr_autoupdate.c | resolution des offsets dans le processus | stable, voir 02 |
| game/rr_il2cpp_dump.c | dump de classes/champs pour diagnostic | outil |
| game/offsets/*.h | macros d offsets, source de verite | genere/verifie |
| features/esp/rr_draw.c | rendu ESP (boites, squelette, textes, barres) | fonctionnel |
| features/aim/rr_aim.c | selection de cible, conversion en pas souris | en cours |
| features/visibility/rr_los*.c | ligne de vue par reconstruction BVH | fragile |
| chams/rr_native_chams.c | chams natifs | fonctionnel |
| ui/amphetaminehaxx/* | menu ImGui, onglets, widgets, bridge BR_Config | fonctionnel |
| internal/rr_int_main.cpp | worker de jeu, hook Present, tick d aim, snapshot | fonctionnel |
| diagnostics/* | scan de classes et de noeuds | outil |
| extras/silent_aim.c | silent aim | experimental |

## Flux de donnees
`game_worker` (thread dedie) -> game_init -> game_snapshot() sous SRW lock ->
`draw_frame` copie le snapshot -> features de rendu, et `aim_tick` consomme le
meme snapshot pour la selection de cible.

Le rendu ne touche jamais la memoire du jeu directement : il lit le snapshot.
L aim non plus pour la selection ; seul l envoi de pas souris sort.

## Threads
- Thread principal du jeu : hooks Present/Present1, rendu ImGui, draw_frame.
- Thread worker : boucle de snapshot, cadence ~16 ms.
- Thread LOS (`rr_los_start`) : capture geometrique en fond.
Les requetes physiques Unity doivent partir d un thread attache IL2CPP.

## Configuration
`BR_Config` (ui/menu_bridge.h) est la structure partagee. Le menu ecrit via
`eternal_ui::settings()` puis `push()` persiste vers BR_Config. Ne jamais
ecrire BR_Config directement depuis un onglet : `push()` ecraserait la valeur
dans la meme frame.

`telem_los_state` (0 off, 1 chargement/perime, 2 pret) est expose au menu,
onglet debug.

## Pieges
- `rr_aim_pick` recoit un callback `clear` optionnel ; en interne il est NULL,
  d ou le repli sur `rr_los_check` (voir 03).
- `rr_aim_point` n a pas de parametre `clear` (6 args), `rr_aim_visible_point`
  en a un (7 args). Confondre les deux casse le build.
- `_partiesByEntity` est indexe par id de party, pas par id de personnage.