+++
id = "LOT-77"
titre = "La moitié RPG de la spécification"
version = "0.0.0"
filiere = "standard"
statut = "livre"
taille = "M"
resume = "Les cinq familles d'exigences que vingt lots citaient sans qu'elles existent sont écrites, et le lint d'exigences repasse au vert sans exception."
prerequis = []
livrables = [
  "`Specification/regles-d20.md` (`EX-REG-*`, 15 exigences).",
  "`Specification/rpg.md` (`EX-RPG-*`, 17 exigences).",
  "`Specification/combat.md` (`EX-CBT-*`, 14 exigences).",
  "`Specification/inventaire.md` (`EX-INV-*`, 8 exigences).",
  "`Specification/contenu.md` (`EX-CNT-*`, 15 exigences).",
  "La rubrique « Exigences couvertes » posée sur les 25 lots qui les implémentent, et les cinq documents insérés dans l'ordre de lecture de `specifications.md`.",
]
criteres = [
  "`python scripts/checks/lint_exigences.py` passe au vert **sans entrée ajoutée** à `UNREFERENCED_ALLOWED` — 339 exigences déclarées, 339 référencées.",
  "Chaque exigence déclarée est citée par **au moins un lot** ou par une autre spécification.",
  "Les cinq documents figurent dans l'ordre de lecture de `specifications.md`.",
  "Aucune exigence nouvelle ne décrit un mécanisme que la feuille de route déclare hors périmètre.",
  "Doxygen reste vert (`WARN_AS_ERROR`), sans ancre en double.",
]
+++

## Pourquoi

Écrire les cinq documents de spécification que vingt lots citaient sans qu'ils existent, et éteindre
l'échec volontaire du lint d'exigences.

Le dépôt comptait **13 familles réelles pour 269 exigences**, et **cinq familles fantômes** —
`EX-CNT-*`, `EX-REG-*`, `EX-RPG-*`, `EX-CBT-*`, `EX-INV-*` — référencées par la feuille de route
sans qu'aucun document ne les porte. Le garde-fou avait été réparé avant ce lot (`FAMILY_REF_RE`
dans `lint_exigences.py`), ce qui rendait la CI **rouge à dessein** jusqu'à ce que les documents
existent. Ce lot est ce qui la remet au vert — et c'est la seule façon légitime de le faire.

## Périmètre

Cinq documents, **69 exigences**, insérés dans l'ordre de lecture de `specifications.md` :

| Document | Famille | Exigences | Ce qu'il fixe |
|---|---|---|---|
| `Specification/regles-d20.md` | `EX-REG-*` | 15 | Le jet d20, les caractéristiques, la maîtrise, le temps et le repos, les conditions, les échelles |
| `Specification/rpg.md` | `EX-RPG-*` | 17 | La fiche comme agrégat dérivé, espèces, classes et ressources, progression, options, sorts |
| `Specification/combat.md` | `EX-CBT-*` | 14 | Bascule, tour, espace, attaque et dégâts, agonie et mort, l'adversaire |
| `Specification/inventaire.md` | `EX-INV-*` | 8 | Emplacements d'équipement, encombrement, monnaie, commerce, butin |
| `Specification/contenu.md` | `EX-CNT-*` | 15 | Provenance, contrats, extraction, ce qu'une donnée promet, assets, contrôle |

Et la **rubrique « Exigences couvertes »** posée sur les 25 lots qui les implémentent — quinze lots
de la filière et dix lots absorbés de la section 11. Elle manquait à toutes les sections de lot,
faute de familles à citer.

## Décisions de réalisation

**Le moteur porte des mécanismes, la donnée porte des valeurs.** C'est la règle qui traverse les
cinq documents, et elle a une conséquence qu'il fallait écrire noir sur blanc : le catalogue sera
complet **longtemps avant** le moteur. D'où `EX-CNT-030` et `EX-CNT-031` — une donnée déclare les
mécanismes qu'elle exige, le moteur **refuse en le disant** ce qu'il ne sait pas honorer. Un
catalogue complet qui ment sur ce qui est jouable coûte plus cher qu'un catalogue incomplet qui le
dit.

**Trois généricités décident de la faisabilité du programme**, et elles sont écrites comme telles :

- `EX-RPG-021` — une ressource de classe est *une quantité, une cadence, ce qu'elle alimente*. Rage,
  ki, points de sorcellerie et second souffle sont la même structure. Sans cela, chacun des quinze
  lots de classes modifierait le code du repos.
- `EX-RPG-023` — ajouter une classe ne touche **aucun** fichier C++ existant hors sa mécanique
  propre. C'est le critère qui rend « une classe par lot » tenable ; s'il tombe, le programme
  s'arrête à la cinquième classe.
- `EX-RPG-052` — **plusieurs systèmes d'emplacements de sorts coexistent**. Ce n'est pas une
  hypothèse : la magie de pacte en fournit un second, récupéré au repos court. Coder « les
  emplacements » au singulier obligerait à tout reprendre.

**Deux exigences héritées sont contredites explicitement**, plutôt que contournées : `EX-CBT-042`
(la mort hors combat n'est pas « redémarrer le niveau ») et `EX-RPG-032` (aucune progression liée au
franchissement d'un tableau). Leur retrait effectif appartient au `LOT-67` ; ce lot pose la
définition qui les remplacera, sans quoi le retrait n'aurait rien à mettre à la place.

**Ce que ce lot n'a pas fait**, et qui n'était pas dans son périmètre : retirer les exigences de
plateforme (`LOT-67`), désambiguïser les 208 renvois de numéros hérités (`LOT-78`), écrire la brique
de chargement (`LOT-79`).

## Bilan

Statut : **fait**. Vérification automatisée : `scripts/checks/lint_exigences.py` vert sans exception ajoutée, `scripts/lint_lots.py` vert, Doxygen vert ; tous les critères d'acceptation sont cochés dans l'epic d'origine. Ce lot est prérequis de toute la filière contenu, à commencer par le `LOT-30`.
