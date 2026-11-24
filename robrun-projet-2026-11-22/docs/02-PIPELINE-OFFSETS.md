# Pipeline d offsets

## Pourquoi
Chaque patch du jeu deplace les champs IL2CPP. Historiquement les offsets
etaient corriges a la main, ce qui cassait tout a chaque mise a jour : le
processus mourait sur le premier tick monde (violation d acces ~5 s apres
l injection). Il n y a pas de dossier tools/ et Il2CppDumper refuse la
metadata v39, donc la resolution se fait dans le processus injecte, via l API
d embedding IL2CPP.

## Principe
Les headers d offsets restent la source de verite unique. Le pipeline les lit,
construit une table, resout chaque entree dans le binaire courant, puis
reecrit les headers avec les valeurs trouvees. On ne devine jamais une valeur.

## Etapes
1. `gen_autoupdate_table.ps1` lit game/offsets/robrun_offsets.h et
   robrun_sdk_offsets.h et produit game/rr_autoupdate_table.h.
   Format de ligne : { "MACRO", "Classe", "Membre", index_occurrence, defaut }.
   Regles de parsing :
   - une parenthese collee au nom = methode (SetRotation) ;
   - le membre est coupe au premier espace (les annotations de type en fin de
     commentaire ne doivent pas entrer dans le nom) ;
   - les commentaires contiennent des noms de propriete, pas des champs.
2. `game/rr_autoupdate.c` enumere les assemblies (il2cpp_domain_get_assemblies
   + il2cpp_assembly_get_image), indexe les classes, puis pour chaque entree
   essaie des candidats de nom dans l ordre : nom exact, `<Nom>k__BackingField`
   (les proprietes sont commentees dans les headers mais les vrais champs
   IL2CPP sont des backing fields), nom court, suffixe `_1` retire, underscore
   initial retire, premiere lettre en majuscule.
   Les exports non critiques sont optionnels ; une porte explicite bloque
   seulement si un export critique manque.
3. Sortie : generated_offsets.h + autoupdate.log dans
   %LOCALAPPDATA%\RobRunInternal.
4. `apply_generated.ps1` relit le log et reecrit les headers, puis verifie par
   un compteur `applied` et un `diff` entre valeurs actuelles et trouvees.

## Declenchement
`game_init` appelle `rr_buildid_check()`. Si le resultat vaut -1 (taille de
GameAssembly.dll differente de EXPECTED_SIZE), le resolver tourne avant toute
lecture de monde. Le resolver s execute de maniere synchrone sur le thread
appelant : une version sur thread dedie mourait avant d etre planifiee.

## Verification
- `applied=197 diff=0` : le pipeline reproduit exactement les headers.
- `buildid.log` doit repondre OK apres injection.
- Les RVA de methodes se verifient par inspection d octets, pas par confiance
  au dump. `SetRotation` = 0xC517E0, prologue `push rbx; sub rsp,0x40;
  movss xmm4,[rip+_eps]`. Le dump annoncait 0x497C140, faux.

## Entrees non resolues
13 entrees restent UNRESOLVED (4 RR_SF_*, 4 BRAIN_*/CAMERA_GET_FOV, 4 CAR_*,
BRAIN_ACTIVE_VCAM). Elles sont conservees telles quelles avec un commentaire,
pas supprimees. Les annoter est optionnel.

## Pieges deja payes
- `PartyModel._entities (HashSet<int>)` etait classe en methode a cause de la
  parenthese du commentaire.
- `il2cpp_domain_assembly_open(domain, "GameAssembly.dll")` echoue : il attend
  un nom d assembly manage.
- `CameraState.Lens` n existe pas comme classe, c est le champ `LensSettings`.
- `_sensitivityModel` est en realite `<SensitivityModel>k__BackingField`.
- Collision de double macro sur GameModel : l index d occurrence #N tranche.
- Les `.Replace` multi-lignes PowerShell echouent silencieusement (fins de
  ligne differentes) : utiliser des remplacements ligne a ligne par index ou
  des regex sur le texte complet.
- `List.InsertRange` refuse un System.Object[] : utiliser `Insert` un par un.
- `$Matches` est ecrase par un `-match` imbrique : capturer avant.
- `IndexOf` echoue si la ligne a des espaces de fin : preferer une regex ou un
  `.Replace` litteral.