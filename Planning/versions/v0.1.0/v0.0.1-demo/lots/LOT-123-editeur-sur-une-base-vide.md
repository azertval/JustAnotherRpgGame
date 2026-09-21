+++
id = "LOT-123"
titre = "L'éditeur debout sur une base vide"
version = "0.0.1"
filiere = "editeur"
statut = "a-faire"
taille = "M"
resume = "L'éditeur, son contrôle et ses tests ne dépendent plus d'aucune carte ni d'aucune planche livrée : la table rase peut passer sans le casser."
prerequis = ["LOT-100"]
livrables = [
  "`LevelEditor --check` rend 0 sur une base **sans aucune carte** (il dit « 0 map »), et `resolveDataRoot` admet un dossier `Levels/` vide, gardé par son `README.md`.",
  "Une racine de données de test complète sous `Source/Test/Fixtures/EditorData/` : deux cartes reliées, une planche et son manifeste, un dialogue, une rencontre, ses textes — assez pour tous les contrôles.",
  "Les dix-huit tests de `Source/Test/Unit/Editor/` et les trois tests système ou d'intégration rebranchés dessus ; `test_shipped_maps` garde son mécanisme (toute carte livrée s'ouvre et se réenregistre à l'octet) et admet qu'il n'y en ait aucune.",
  "Le scénario `--apply` de référence (`martpart-rue.json`) rejoué sur la carte de test, fichier attendu régénéré.",
  "`bench_canvas` et `bench_levels` réécrits sur la carte de test.",
]
criteres = [
  "Sur une branche où `Levels/` et `Assets/Scene/` sont vidés à la main, la CI entière de l'éditeur est verte : tests, `--check`, `--render`, mesures.",
  "`git grep -i \"martpart\\|coliseum\\|arenarea\" Source/Test/Unit/Editor Source/Benchmark` ne trouve plus rien.",
  "Aucun test n'a été supprimé : la liste des rebranchements est dans la PR.",
]
+++

## Pourquoi

Le [LOT-102](LOT-102-table-rase-assets-et-cartes.md) promet « l'éditeur s'ouvre sur une carte
vierge ; `LevelEditor --check` passe ». L'[audit de l'éditeur](../../../../standards/audit-editeur.md)
(constats T1 à T6) montre que c'est faux aujourd'hui : `--check` rend une erreur quand il n'y a
aucune carte, et dix-huit tests que l'audit du moteur n'a pas comptés lisent Martpart ou le Colisée.
Ce lot passe **avant** la table rase, pour qu'elle reste un retrait et non un chantier.

## Périmètre

Les tests, le contrôle et les mesures. **Pas dedans** : la nouvelle arborescence (LOT-124), le rendu
HD (LOT-125). La carte de test reste au format et au style actuels : elle n'est pas un asset du
jeu, et le LOT-124 la déplacera sous le nouvel arbre.
