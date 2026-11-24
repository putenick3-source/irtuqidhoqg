# Journal des decisions et impasses

## 2026-11-24
- Jeu ferme 5 s apres injection : offsets perimes, violation d acces au premier
  tick monde. Decision : resolver automatique plutot que patch manuel.
- Generateur : parenthese dans un commentaire classait un champ en methode.
  Exiger la parenthese collee au nom.
- Quatre lignes de table gardaient leur annotation de type : membre coupe au
  premier espace.
- Abandon sur export manquant : exports optionnels + porte critique.
- il2cpp_domain_assembly_open attend un nom d assembly manage, pas
  GameAssembly.dll : passage a il2cpp_domain_get_assemblies.
- Resolver sur thread dedie : mort avant planification, passe en synchrone.
- 0 hit sur 216 assemblies : les headers commentent des proprietes, les champs
  reels sont des backing fields. Candidats ajoutes, 67 -> 184.
- CameraState.Lens n existe pas : reecrit en LensSettings.FieldOfView.
- _sensitivityModel n existe pas : c est <SensitivityModel>k__BackingField.
- Dump annoncait SetRotation a 0x497C140 : octets inspectes, vrai prologue
  0xC517E0. Le dump n est pas fiable pour une RVA.
- applied=197 diff=0, build OK, ESP verifie en jeu.
- Party : _partiesByEntity est indexe par id de party. Reecrit sur
  _actualPartyMemberModels. Non validable sans collegue.
- Visible check : clear applique sans lire aa_los_on, toggle inerte ; en
  interne le callback valait NULL. Corrige, gate par le toggle, repli
  rr_los_check ajoute.
- Recherche web : les autres appellent Physics.Raycast/Linecast via le pointeur
  IL2CPP, layer mask limite a la geometrie, depuis un thread de jeu. Personne
  ne reconstruit le BVH. Plan de remplacement dans 03.
- Premier test : los.log ABSENT, instrumentation rendue inconditionnelle pour
  distinguer un toggle a 0 d un pipeline en UNKNOWN.