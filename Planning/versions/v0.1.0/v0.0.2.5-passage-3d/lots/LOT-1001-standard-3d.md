+++
id = "LOT-1001"
titre = "Le standard 3D"
version = "0.0.2.5"
filiere = "standard"
statut = "a-faire"
taille = "M"
resume = "Une seule règle écrite pour tout ce qui se modèle : unités, format, squelette, corps, pièces, matières, lumière — et le standard 2D HD cesse d'être normatif."
prerequis = ["LOT-1000", "LOT-142"]
livrables = [
  "`Planning/standards/style-3d.md` : unités et repère (1 case = 1,5 m), caméra (orthographique, 45° / 38,3°), format d'échange (`.glb`), budget de triangles, matières et textures, lumière, et le chapitre « images tolérées » qui garde les seules règles 2D encore utiles.",
  "`Planning/standards/personnages-3d.md` : le squelette humanoïde (os nommés, pose de liaison), les **huit corps** (homme, femme × maigre, normal, musclé, gros) et leur contrat commun, les emplacements de pièces, la commande de texture au générateur d'images, la fiche d'un personnage.",
  "`Planning/standards/arborescence-assets.md`, `gabarit-commande-zone.md` et `definition-de-livre.md` mis à jour : filières « Assets » et « PNJ » en 3D.",
  "Les exigences réécrites dans `Documentation/Specification/` : `EX-VIS-008`, `EX-VIS-009`, `EX-REN-013`, `EX-REN-014`, `EX-REN-018` ; les textes remplacés rangés dans `exigences-retirees.md`.",
  "`AGENTS.md` : la consigne « Lots d'assets 2D HD » remplacée par celle des lots d'assets 3D.",
  "`scripts/checks/check_orphans.py`, en CI : tout fichier sous `Source/Elements/Assets/` est cité par un manifeste ou une fiche, toute entrée citée existe, et tout script de `scripts/` est appelé par `check.py`, la CI ou un document. C'est lui qui fait tenir la règle « pas d'asset mort » (D-32) lot après lot.",
]
criteres = [
  "Chaque valeur du standard est une valeur **mesurée** au LOT-1000 (taille du corps, nombre d'os, triangles, définition de la texture) ou une décision datée de l'auteur.",
  "`lint_planning.py`, `lint_exigences.py` et `build_docs.py` passent ; aucun lien mort vers les documents archivés.",
  "`git grep -l \"style-2d-hd\"` ne trouve plus, hors `archives/` et fiches livrées, aucun document qui s'y réfère comme à une règle en vigueur.",
  "Les trois scripts de la chaîne de génération de figurines 2D et leurs deux tests n'existent plus ; `uv run scripts/check.py` passe.",
]
+++

## Pourquoi

Le `LOT-101` a écrit le standard 2D HD d'après une maquette ; celui-ci écrit le standard 3D d'après
la preuve du [LOT-1000](LOT-1000-preuve-de-la-chaine-de-personnages.md). Sans lui, chaque lot suivant
redécide l'échelle, le nom d'un os ou la définition d'une texture.

Il attend aussi la recette de la `0.0.2` : tant qu'elle n'est pas livrée, le standard 2D HD est la
règle sous laquelle elle se juge.

## Périmètre

Dedans : les documents, les exigences, la consigne aux agents, et le **contrôle des orphelins**,
posé dès maintenant pour que chaque lot suivant soit jugé dessus.

Dehors : tout code de rendu, et les **contrôles** des nouveaux formats (`check_hd_assets.py`
apprend les maillages au [LOT-1004](LOT-1004-kit-de-la-capitale-en-maillages.md), les personnages au
[LOT-1006](LOT-1006-corps-de-reference.md)).

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Le standard 2D HD et sa consigne de génération | `Planning/standards/style-2d-hd.md`, `consigne-2d-hd.md` → `Planning/standards/archives/` | des fiches livrées les citent : ils s'**archivent**, liens corrigés, et sortent du `README.md` des standards |
| Le workflow de revue des marches | `Planning/standards/workflow-qualite-pnj.md` → `archives/` | il règle un défaut que la 3D supprime à la source |
| La chaîne de génération de figurines 2D | `scripts/assetsGeneration/prepare_envois_figure.py`, `check_figure_walk.py`, `preview_figure_walk.py` ; `scripts/tests/test_check_figure_walk.py`, `test_preview_figure_walk.py` ; leurs appels dans `scripts/check.py` et les hooks | plus aucune figurine ne se commande en bandes ; les bandes déjà installées restent contrôlées par `check_hd_assets.py` jusqu'au LOT-1006 |
| Les textes remplacés des exigences | `Documentation/Specification/vision.md`, `rendu-technique.md` → `exigences-retirees.md` | une exigence qui prescrit un losange de 256 px pour tout l'art de scène est fausse dès ce lot |
| La question `Q-02` (valeurs du standard 2D HD) | `Planning/vision/decisions.md` | sans objet |
| Les consignes 2D de l'atelier local | `Tools/AssetHd/NPC/ManequinNpc/PROMPT-finir-humanoide.md` et les `prompts` de figurines | à archiver par l'auteur |

## Conception

Décisions déjà prises, que le standard écrit sans les rouvrir :

- **Huit corps**, un squelette ([D-31](../../../../vision/decisions.md)). Le contrat commun
  proposé : même topologie et même dépliage pour les huit, de sorte qu'une texture et des poids
  valent pour tous. À confirmer sur le corps de preuve.
- **Portraits et jetons peints**, mobilier et pièces maîtresses **tolérés en image** jusqu'à la
  `0.0.3` ([D-30](../../../../vision/decisions.md)) : d'où le chapitre « images tolérées », qui
  garde le losange de 256 × 159, l'alpha prémultiplié et les ancres.
- **Les effets** (`Common/Fx/`) restent des bandes : un éclair ou un soin est une image animée, pas
  un volume.
- **La facture** : matières peintes, pas de grain ; la lumière vient du moteur
  ([LOT-1007](LOT-1007-eclairage-et-cycle-jour-nuit.md)), plus de la texture.

## Risques et questions ouvertes

- Le contour sombre du standard 2D n'existe pas en 3D sans une passe dédiée : à décider ici — le
  garder (coût au LOT-1003) ou l'abandonner.
- Les non-humanoïdes : un squelette vaut pour une morphologie. Le standard nomme les silhouettes
  (`humanoid`, `quadruped`, `flying`) sans les produire ; elles viennent avec leurs créatures.
