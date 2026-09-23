# Exigences retirées

> Deux familles d'exigences sont retirées **en entier** par le `LOT-88` : aucune ne décrit plus
> rien du jeu ni du dépôt. Leurs ancres restent ici, jamais renumérotées : les lots livrés s'y
> réfèrent, et un numéro réutilisé ferait mentir leur histoire. Les exigences retirées une à une,
> dans une famille qui vit encore, restent à la fin de leur propre spécification.
>
> Une exigence retirée n'est citée par aucun code : `scripts/checks/lint_exigences.py` le vérifie.

## Solveur d'apprentissage automatique (`EX-IA-*`)

Un agent entraîné sans framework d'apprentissage, capable de terminer un niveau seul et de rejouer
sa solution en jeu. Le programme et son code ont été retirés au `LOT-01`.

- **EX-IA-001** *(retirée au `LOT-88`)* — calcul tensoriel et générateur
  pseudo-aléatoire maison.
- **EX-IA-002** *(retirée au `LOT-88`)* — différentiation automatique.
- **EX-IA-003** *(retirée au `LOT-88`)* — réseaux de neurones.
- **EX-IA-004** *(retirée au `LOT-88`)* — optimiseurs de descente de gradient.
- **EX-IA-005** *(retirée au `LOT-88`)* — jeu sans fenêtre pour l'entraînement.
- **EX-IA-006** *(retirée au `LOT-88`)* — observation de l'état du jeu.
- **EX-IA-007** *(retirée au `LOT-88`)* — espace d'action de l'agent.
- **EX-IA-008** *(retirée au `LOT-88`)* — séquence d'actions gagnante.
- **EX-IA-009** *(retirée au `LOT-88`)* — signal de récompense.
- **EX-IA-010** *(retirée au `LOT-88`)* — journal d'entraînement.
- **EX-IA-011** *(retirée au `LOT-88`)* — algorithme évolutionniste.
- **EX-IA-012** *(retirée au `LOT-88`)* — entraînement niveau par niveau.
- **EX-IA-013** *(retirée au `LOT-88`)* — gradient de politique.
- **EX-IA-014** *(retirée au `LOT-88`)* — réduction de variance.
- **EX-IA-015** *(retirée au `LOT-88`)* — apprentissage par valeur.
- **EX-IA-016** *(retirée au `LOT-88`)* — harnais de benchmark.
- **EX-IA-017** *(retirée au `LOT-88`)* — taux de réussite d'un modèle.
- **EX-IA-018** *(retirée au `LOT-88`)* — validation d'un fichier de rejeu.
- **EX-IA-019** *(retirée au `LOT-88`)* — rejeu en partie réelle.
- **EX-IA-020** *(retirée au `LOT-88`)* — exécutable en ligne de commande.
- **EX-IA-021** *(retirée au `LOT-88`)* — garde-fou d'intégration continue.
- **EX-IA-022** *(retirée au `LOT-88`)* — écran « Mode IA ».
- **EX-IA-023** *(retirée au `LOT-88`)* — champ de distances de la récompense de
  progression.

## Décors et plans picturaux (`EX-DEC-*`)

Des images peintes couvrant un niveau entier, avec densité, profondeur et parallaxe, peintes dans
l'éditeur ; et avant elles des décors-sprites manipulables. Le lieu d'une carte se dessine
désormais avec les pièces de sa planche (`EX-VIS-008`, `EX-REN-010`).

- **EX-DEC-001** *(retirée au `LOT-88`)* — décor posé librement.
- **EX-DEC-002** *(retirée au `LOT-88`)* — décors devant ou derrière le
  personnage.
- **EX-DEC-003** *(retirée au `LOT-88`)* — plan affiché fidèle à l'asset.
- **EX-DEC-004** *(retirée au `LOT-88`)* — décors en entités de la simulation.
- **EX-DEC-005** *(retirée au `LOT-88`)* — collision propre à un décor.
- **EX-DEC-006** *(retirée au `LOT-88`)* — parallaxe d'un décor.
- **EX-DEC-010** *(retirée au `LOT-88`)* — édition des décors.
- **EX-DEC-020** *(retirée au `LOT-88`)* — manipulation des décors en jeu.
- **EX-DEC-021** *(retirée au `LOT-88`)* — déterminisme de cette manipulation.
- **EX-DEC-030** *(retirée au `LOT-88`)* — conversion d'une photo en pixel art.
- **EX-DEC-031** *(retirée au `LOT-88`)* — paramètres de cette conversion.
- **EX-DEC-032** *(retirée au `LOT-88`)* — enregistrement de l'image convertie.
- **EX-DEC-040** *(retirée au `LOT-88`)* — plan pictural couvrant le niveau.
- **EX-DEC-041** *(retirée au `LOT-88`)* — densité d'un plan.
- **EX-DEC-042** *(retirée au `LOT-88`)* — profondeur d'un plan.
- **EX-DEC-043** *(retirée au `LOT-88`)* — parallaxe d'un plan.
- **EX-DEC-044** *(retirée au `LOT-88`)* — coût borné des plans d'un niveau.
- **EX-DEC-045** *(retirée au `LOT-88`)* — peinture des plans dans l'éditeur.
