+++
id = "LOT-100"
titre = "Planification par versions"
version = "0.0.1"
filiere = "standard"
statut = "livre"
taille = "M"
resume = "Le dossier `Planning/`, son lint et son site remplacent la feuille de route unique : le référentiel `0.1.0` devient l'Empire central, la `0.0.1` une démo basique."
reprend = ["feuille de route 0.1.0"]
livrables = [
  "`Planning/` : vision, versions, fiches de lots, référentiels, standards.",
  "`Planning/outils/lint_planning.py` dans le job `lint-exigences`, et ses tests dans `pytest`.",
  "Le site de planification publié sous `planning/` du site du projet, relié à la documentation.",
  "`Planning/vision/archives/feuille-de-route-jeu.md` figée, avec un renvoi vers `Planning/`.",
]
criteres = [
  "`python Planning/outils/lint_planning.py` est vert, en local et en CI.",
  "Le site s'engendre sans réseau et sans dépendance, et s'ouvre depuis la page d'accueil de la documentation.",
  "Tout lot non livré de l'ancienne feuille de route figure dans la table de correspondance.",
]
+++

## Pourquoi

L'ancienne feuille de route visait d'un bloc le monde entier — treize régions, seize classes, le plan
pénombral — sous le nom de `0.1.0`. En commençant le travail, l'auteur a constaté qu'elle était
**titanesque** : trop de zones, pas assez de conception par zone. Ce lot la remplace par une
trajectoire en versions courtes, où seule la prochaine est détaillée à fond.

## Périmètre

Dedans : le dossier, l'outillage, le site, le gel de l'ancienne page. **Pas dedans** : le retrait du
lint `scripts/lint_lots.py`, qui continue de garder la page figée tant qu'elle existe ; la
suppression des assets (LOT-102) ; la réécriture des spécifications (LOT-101, LOT-103).

## Questions ouvertes

- Le lint de l'ancienne feuille de route se retire-t-il avec elle, ou la page reste-t-elle comme
  archive gardée ? Proposé : elle reste jusqu'à la `0.0.1`, puis part avec son lint.

## Livraison

PR #94.
