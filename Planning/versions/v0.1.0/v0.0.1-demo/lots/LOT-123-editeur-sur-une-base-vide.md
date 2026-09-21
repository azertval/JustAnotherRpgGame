+++
id = "LOT-123"
titre = "L'éditeur debout sur une base vide"
version = "0.0.1"
filiere = "editeur"
statut = "livre"
taille = "M"
resume = "L'éditeur, son contrôle et ses tests ne dépendent plus d'aucune carte ni d'aucune planche livrée : la table rase peut passer sans le casser."
prerequis = ["LOT-100"]
livrables = [
  "`LevelEditor --check` rend 0 sur une base **sans aucune carte** (il dit « 0 map »), et `resolveDataRoot` admet un dossier `Levels/` vide, gardé par son `README.md`.",
  "Une racine de données de test complète sous `Source/Test/Fixtures/GameData/` : deux cartes reliées, une planche et son manifeste, un dialogue, une rencontre, ses textes — assez pour tous les contrôles.",
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

## Décisions de réalisation

### D-123-1 — Trois cartes, pas deux

La fiche promettait « deux cartes reliées ». Il en faut **trois**, et chacune répond à un test que
la paire ne couvrait pas :

| Carte | Identifiant | Ce qu'elle porte | Ce qu'elle rend possible |
|---|---|---|---|
| La Place | `bourg/place` | 48 × 40, dans un **sous-dossier** ; la porte de départ de la ville | un identifiant à chemin (`documentLabel`, annexe, renommage de dossier) |
| La Cave | `cave` | 48 × 40, **à la racine** ; reliée à la Place dans les deux sens ; un déclencheur de rencontre | les deux cartes reliées ; le changement de planche (elle seule pose `street-2`) |
| Le Donjon | `donjon` | 40 × 34 ; une zone de combat et ses **huit entrées d'arène** ; d'aucune ville | `--link-maps`, qui exige deux cartes que **rien** ne relie ; le verdict d'une zone ; une carte hors ville |

Sans la troisième, `WorldLinks` posait un second lien au lieu du premier et comptait deux portails
là où le test en attendait un ; le redimensionnement d'une zone de combat n'avait pas d'entrées à
faire sortir ; et `cityOfMap` ne pouvait plus rendre « aucune ville ».

### D-123-2 — La géométrie est reprise, l'art ne l'est pas

Les trois cartes reprennent le **relief** des cartes livrées au `LOT-96` et au `LOT-09` — leur
grille, leurs pièces posées, leur collision. Rien d'autre : ni nom de lieu, ni dialogue, ni figurine,
ni quartier d'atlas. C'est ce qui permet aux tests de garder leurs **coordonnées** (l'étal 2 × 1 en
(21, 24), la rue des lignes 2 à 5, la poignée est de la zone en (29, 16)) et donc de continuer à
prouver la même chose.

La **planche**, elle, est synthétique : un losange plein par pièce de sol, un pan dressé par pièce
haute, à la taille, à l'ancre, à l'emprise et au miroir que déclarent les manifestes livrés. Fondre
les deux planches livrées en une seule (leurs trois pièces communes ont la même géométrie) donne à
la fixture un lieu que les trois cartes partagent — ce qui met à l'épreuve, au passage, le
préfabriqué découpé sur une carte et posé sur une autre.

Ces images ne servent qu'à peser : ce que les tests de rendu comparent, c'est le **peintre de
l'éditeur** au **rendu GPU du jeu**, sur la même image. Un losange plat le prouve aussi bien qu'une
façade peinte, et 300 Kio suffisent là où il en aurait fallu 2 Mio.

### D-123-3 — « 0 map » n'est pas une erreur

`--check` rendait 1, avec `error: no map under …`, quand `Levels/` n'avait aucune carte : l'étape
CI du contrôle aurait été rouge le jour de la table rase. L'absence de carte se **dit** désormais
— `no map under …`, puis `checked 0 maps: 0 errors, 0 warnings` — et rend 0. Ce qui garde contre un
dossier mal désigné reste `--data`, qui nomme la racine.

`resolveDataRoot` acceptait déjà un `Levels/` vide ; ce qui manquait, c'est que rien ne le disait ni
ne l'éprouvait. Le `README.md` du dossier est nommé **gardien** dans son propre texte et dans
l'en-tête de `DataRoot.h` : git ne gardant pas un dossier vide, le retirer ferait retomber la fenêtre,
en silence, sur la copie que la construction refait à côté de l'exécutable. Un test le verrouille.

### D-123-4 — Un seul test lit encore les cartes livrées, et c'est son objet

`test_shipped_maps.cpp` : **toute** carte livrée s'ouvre en brouillon, se réenregistre à l'octet, se
retouche et se défait. Il perd son plancher de trois cartes — zéro carte est un état légitime — et
garde tout le reste. Deux autres tests suivent la même règle, pour la même raison :
`MapFormatTest.LesCartesLivreesPassentLeControle` (ce que la CI exige des cartes livrées) et
`MapFormatTest.ChaqueCarteMigreeSeJoueALIdentique`, qui ne nomme plus trois cartes mais balaie
celles qu'il trouve.

### D-123-5 — Ce qui n'est pas du ressort de ce lot

Trois tests de `Source/Test/Unit/Core/` tombent quand on vide `Levels/` et `Assets/Scene/` à la
main : `ArenaTest.LaPremiereCarteSeChargeEtAccueilleLesDeuxCamps` et les deux
`ScenePieceManifestTest`. Ils lisent la donnée **livrée** de propos délibéré, ce sont des tests du
**moteur**, et l'audit du moteur les compte : ils partent avec le [LOT-102](LOT-102-table-rase-assets-et-cartes.md).
La CI de l'**éditeur** — les tests de `Unit/Editor`, les tests d'intégration et système, `--check`,
`--render`, les deux mesures — est verte sur une base vidée.

## Ce que la vérification a donné

Sur une copie de travail où `Source/Elements/Levels` ne garde que son `README.md` et où
`Source/Elements/Assets/Scene` est vide :

- `LevelEditor --data Source/Elements --check` → `no map under …` / `checked 0 maps: 0 errors,
  0 warnings`, code **0** ;
- `UnitTests` : aucun échec sous `Unit/Editor` ; `IntegrationTests` et `SystemTests` verts ;
- `Benchmarks` : `ComposeTestMap` et `LoadTestLevel` mesurent la carte d'essai (1,17 k primitives,
  458 µs ; 2,86 ms) ;
- `git grep -i "martpart\|coliseum\|arenarea" Source/Test/Unit/Editor Source/Benchmark` : rien.

Aucun test supprimé : 912 `TEST` avant, 913 après — celui de la base vide.

---

Livré le 21 septembre 2026 par la [PR #105](https://github.com/azertval/JustAnotherRpgGame/pull/105).
