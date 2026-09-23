+++
id = "LOT-105"
titre = "Assets HD — le kit commun de la Capitale"
version = "0.0.1"
filiere = "assets"
statut = "en-cours"
taille = "L"
resume = "Ce que tous les quartiers partagent est produit une fois : pavages, pierre calcaire, mobilier urbain, végétal, bannières de l'Empire."
prerequis = ["LOT-104"]
livrables = [
  "`Regions/central-empire/Common/Scene/` : bannières au lion (redessinées, jamais extraites du livre), emblèmes, matières impériales.",
  "`Regions/central-empire/capital/Common/Scene/` : six sols pavés et leurs bordures, murs et angles de pierre calcaire, lampadaire, banc, vasque, tonneau, caisse, étal nu, cyprès, haie droite et d'angle, massif fleuri.",
  "Les manifestes, et la page de galerie du kit.",
]
criteres = [
  "Une rue de douze cases se compose **avec le seul kit**, sans pièce propre à un quartier.",
  "Chaque pièce respecte la palette de l'Empire et la lumière du standard (relecture de l'auteur sur la galerie).",
  "Le kit pèse moins de 40 Mio.",
]
sources = [
  "Tanares Sourcebook, p. 96-99 : propreté méticuleuse, rues pavées, lanternes",
]
+++

## Pourquoi

C'est la première application de la règle [du commun vers le propre](../../../../standards/arborescence-assets.md).
Produit en premier, le kit évite que Martpart, Arenarea et l'arène refassent chacun leur pavé.

## Conception

Inventaire par famille du standard : **01 Sols** (6), **02 Façades** (mur plein, mur à fenêtre,
angle rentrant, angle sortant), **05 Balustrades** (droite, pilier), **07 Végétal** (5),
**08 Mobilier** (7), bannières (3). Une trentaine de pièces.

## Production — 23 septembre 2026

28 sources générées ; 35 pièces installées (31 dans le commun de la Capitale, 4 dans celui de
l'Empire), avec les deux manifestes à jour. Contrôles HD et vérification des sources réussis ;
environ 2,6 Mio installés. Galerie locale sous
`Tools/AssetsHD/Regions/central-empire/capital/Common/apercus/index.html`, reconstruite par
`scripts/build_capital_gallery.py`, avec une composition de douze cases de long.

Le lot reste **en cours**, en attente de relecture artistique et de validation de la composition
dans le moteur. Le détail des reprises et des points à relire figure dans
`Tools/AssetsHD/Regions/central-empire/capital/Common/generation-suivi.md`.

### Révision 2

Bannières reprises depuis le lampadaire v1, murs régénérés sans piliers de terminaison et 35 pièces recalées sur la grille 256 × 159 avec l’accord de l’auteur. Galerie avec contrôles U/V et emprises au sol. Les cinq tests ciblés et les contrôles d’installation passent ; validation artistique et en jeu toujours ouvertes.

### Révision 3

Angles des murs, balustrades et haies sur cases entières ; trois pièces d’angle ajoutées (38 pièces au total). Deux nouvelles textures de sol à joints réguliers et variantes sans décalage du motif. Map visuelle reconstruite et six tests ciblés réussis ; acceptation artistique et validation en jeu restent ouvertes.


### V4 — reprise complète du kit (23 septembre 2026)

38 pièces refaites et installées ; angles communs pour murs, balustrades et haies, matériaux continus et sols à joints réguliers. La carte de validation couvre les 38 pièces en 359 placements. Chargement et rendu réalisés avec LevelEditor aux échelles 1 et 2 : zéro erreur de contrôle, avertissements existants de gameplay difficult/cover. Installations et budgets HD conformes. Les fichiers installés sont identiques aux images testées dans le moteur. Sources, prompts, rendus et limites du contrôle : `Tools/AssetsHD/Regions/central-empire/capital/Common/V4/README.md`. Validation artistique utilisateur et essai de déplacement restent distincts de ce contrôle visuel.
