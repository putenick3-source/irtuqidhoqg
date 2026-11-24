# Etat des fonctionnalites

| Fonctionnalite | Emplacement | Etat |
|---|---|---|
| ESP boites / squelette / noms / armes / distances | features/esp/rr_draw.c | OK, verifie en jeu |
| ESP barre de vie | rr_draw.c | toggle present, rien dessine |
| ESP armure | rr_draw.c | toggle present, rien dessine |
| Crosshair | absent du menu | a implementer |
| Watermark | absent du menu | a implementer |
| Sauvegarde / chargement config | onglet config | toggles presents, rien n est persiste |
| Aimbot selection / pas souris | features/aim/rr_aim.c | OK hors filtre de visibilite |
| Visible check (aa_los_on) | rr_aim.c + features/visibility | EN COURS, voir plus bas |
| Skip party (aa_team_check) | rr_aim.c:33 contre RR_REL_PARTY | filtre OK, detection de party cassee |
| Chams | chams/rr_native_chams.c | OK |

## Visible check - etat exact
Le filtre etait applique inconditionnellement : le callback `clear` etait
consulte sans jamais lire `aa_los_on`, donc le toggle etait inerte. En plus,
le chemin interne passait `NULL` pour ce callback, donc aucune verification
du tout. Corrige ainsi dans rr_aim.c :

- `#include "features/visibility/rr_los.h"` (chemin racine, comme rr_game.h) ;
- `static int los_default(RR_V3,RR_V3)` : repli qui appelle `rr_los_check` et
  considere `RR_LOS_UNKNOWN` comme visible (permissif) ;
- dans `rr_aim_pick` : `los = config->aa_los_on != 0` puis
  `vis = los ? (clear ? clear : los_default) : NULL` ;
- les trois usages passent par `vis` (`!vis || vis(...)`, `if (los && !vis(...))`,
  `rr_aim_visible_point(..., vis)`) ;
- `rr_aim_point` est appele sans `vis` : sa signature n a pas de parametre
  `clear` (6 args), la verification se fait apres, sur le point retenu, avec
  re-recherche d un joint visible.

Quand `los` est faux, `vis` vaut NULL et tout le filtre disparait : le
comportement d origine est restaure.

### Instrumentation en place
- `los_default` ecrit une ligne par seconde dans
  %LOCALAPPDATA%\RobRunInternal\los.log : `res=<code> <etat pipeline>`.
- `rr_aim_pick` ecrit une ligne par seconde inconditionnellement :
  `pick on=<aa_on> los=<aa_los_on> mode=<aa_mode> n=<count> cb=<clear!=NULL>`.

### Prochain diagnostic
Premier essai : `los.log` ABSENT. Deux causes possibles, a trancher avec
l instrumentation inconditionnelle :
- `pick ... los=0` -> le toggle n arrive pas depuis le menu (probleme de
  synchro bridge, pas de LOS). Corriger le bridge avant tout le reste.
- `los=1` et `res=-1` en permanence -> le pipeline LOS renvoie UNKNOWN :
  `source()` echoue (offsets en dur `battle+0x60`, `scene+0x10`, `data+0x18`,
  `tree+0x18`) ou les captures ne sont jamais pretes. C est le cas probable :
  remplacer la reconstruction BVH par un `Physics.Linecast` resolu par IL2CPP
  (recherche web faite, voir 06).

## Skip party - etat exact
`aa_team_check` existe et est correct : il teste `RR_REL_PARTY`. Le probleme
est en amont, dans la detection. `PartyModel._partiesByEntity` est indexe par
id de party, pas par id de personnage ; les membres se lisent dans
`_actualPartyMemberModels` (RR_OFF_PARTY_ENTITIES, 0x38), ce que fait
desormais `party_refresh`.

`pdiag()` trace un code : 1 = pas de model, 6 = dictionnaire vide, 8 = OK avec
n membres, 9 et 10 = gardes separees. Dernier essai : `code=1 n=0`, puis plus
rien faute de joueurs dans le snapshot. A refaire avec un collegue connecte,
attendu `code=8 n>=1`. Retirer `pdiag` une fois confirme.