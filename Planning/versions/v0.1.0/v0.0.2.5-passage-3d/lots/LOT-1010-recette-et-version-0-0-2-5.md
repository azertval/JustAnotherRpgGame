+++
id = "LOT-1010"
titre = "Recette et version 0.0.2.5"
version = "0.0.2.5"
filiere = "version"
statut = "a-faire"
taille = "M"
resume = "Le jeu de la 0.0.2, rejoué en 3D de bout en bout, de jour comme de nuit — et un dépôt où plus rien de l'ancien rendu ne traîne."
prerequis = ["LOT-1007", "LOT-1009"]
livrables = [
  "La recette : la quête « Des pommes pour l'arène » par ses trois issues et un combat à quatre contre quatre, joués en 3D au clavier et à la manette, à midi et de nuit.",
  "Le **balayage final** : le contrôle des orphelins, les recherches de la liste ci-dessous, et la suppression de ce qu'ils trouvent encore.",
  "Les références d'image et les captures du guide régénérées ; le cahier de test régénéré.",
  "`bilan.md` dans le dossier de la version : ce que chaque lot a coûté, le coût mesuré d'un personnage, le poids des kits avant et après, et ce que la `0.0.3` en retient.",
  "Les lots d'assets, de PNJ et de cartes de la `0.0.3` relus : ceux qui commandent encore des images 2D sont réécrits, ceux des versions suivantes marqués à réécrire (D-35).",
  "L'installeur éprouvé sur un poste vierge ; le tag `v0.0.2.5`.",
]
criteres = [
  "Les critères de sortie de la version sont tenus.",
  "`check_orphans.py` passe : aucun fichier installé sans citation, aucune citation sans fichier, aucun script sans appelant.",
  "Les seules images de décor qui restent sont celles de la dette déclarée dans le `README.md` de la version ; chacune a un lot de la `0.0.3` qui la retire.",
  "Plus aucun document en vigueur ne prescrit la 2D HD : standards, spécifications, guides, `AGENTS.md`, fiches de lots à faire.",
  "Le poids des kits publiés et la cadence de `bench_world_frame` sont écrits au bilan, avant et après la version.",
]
+++

## Pourquoi

Une version qui change la matière du jeu sans en changer une règle ne se juge que d'une façon : on
rejoue ce qu'on jouait. Et la règle de la version — chaque lot retire ce qu'il remplace — se
vérifie une dernière fois d'un bloc, parce qu'un oubli se voit mieux à la fin qu'en cours de route.

## À supprimer

Ce lot ne devrait rien avoir à supprimer : s'il trouve quelque chose, c'est un oubli d'un lot
précédent, et le bilan le nomme.

| Recherche | Attendu |
|---|---|
| `git grep -n "ScenePainter\|SceneImages"` | rien (LOT-1002) |
| `git ls-files "Source/Elements/Assets/**/Characters/**/*.anim.json"` | rien (LOT-1006) |
| `git grep -n "\"portraits\"" Source scripts` | rien (LOT-1009) |
| `git grep -ln "2D HD\|style-2d-hd" -- . ":!Planning/standards/archives" ":!Planning/versions/v0.0.0" ":!CHANGELOG.md"` | seulement des fiches **livrées** et les décisions datées |
| PNG sous `floors/`, `walls/`, `balustrades/`, `stairs/`, `roofs/` du kit de la Capitale | rien (LOT-1004) |
| Scripts de `scripts/` sans appelant | rien (`check_orphans.py`) |
| Tests désactivés ou références d'image que plus aucun test ne lit | rien |
| Anciennes versions des kits dans `kits.lock.json` | une entrée par kit, la dernière |

Ce qui reste **à dessein**, et que le balayage ne doit pas emporter : les bandes d'effets de
`Common/Fx/`, les portraits et jetons peints, les cartes peintes, l'interface, les documents
archivés, et la dette déclarée pour la `0.0.3`.

## Risques et questions ouvertes

- Les lots de zone de la `0.0.3` ont été écrits pour des kits 2D. Les relire ici évite d'ouvrir la
  `0.0.3` sur des fiches fausses ; c'est la décision D-35. Si la réécriture dépasse
  la taille de ce lot, elle devient un lot à part, avant le tag.
