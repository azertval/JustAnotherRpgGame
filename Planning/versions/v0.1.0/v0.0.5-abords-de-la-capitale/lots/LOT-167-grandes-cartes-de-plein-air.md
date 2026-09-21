+++
id = "LOT-167"
titre = "Éditeur — les grandes cartes de plein air"
version = "0.0.5"
filiere = "editeur"
statut = "a-faire"
taille = "L"
resume = "Une prairie, une route, une lisière se dessinent sur une carte bien plus grande qu'un quartier, sans que le canevas ralentisse ni que le sol se pose case par case."
prerequis = ["LOT-165"]
livrables = [
  "La **taille maximale** d'une carte, déclarée dans `core`, refusée au-delà par le chargeur, par `--apply` et par les deux boîtes de dialogue (qui portent aujourd'hui un 100 que rien d'autre ne connaît) ; sa valeur sort de la mesure, pas de la feuille de route.",
  "La composition et la peinture **bornées au cadrage** : un geste sur une grande carte ne recompose que ce qui se voit.",
  "Un **pinceau de terrain** : herbe, terre, chemin, eau, roche, avec leurs **raccords** tirés du kit de nature — les transitions sont des pièces du manifeste, choisies par le voisinage.",
  "Des **variantes de sol** tirées à graine notée, pour qu'une prairie ne montre pas sa trame.",
  "La mini-carte et le navigateur tiennent une grande carte ; `bench_canvas` la mesure.",
]
criteres = [
  "Une carte à la taille maximale, couverte de pièces du kit de nature, se peint et se fait défiler de façon fluide sur le poste de référence ; la mesure est publiée.",
  "Une route de quarante cases à travers une prairie se trace en un geste, raccords compris ; rejouée par `--apply`, elle rend le même fichier.",
  "Un fichier plus grand que la borne est refusé avec un message qui la nomme.",
]
+++

## Pourquoi

Jusqu'à la `0.0.4` toutes les cartes sont des quartiers : la plus grande fait 34 × 24. Les abords
de la Capitale, puis la côte, la forêt de Bak et les hautes terres sont du **plein air**. L'éditeur
n'y est pas prêt ([audit](../../../../standards/audit-editeur.md), §3) : la borne de 128 × 128 promise
par sa feuille de route n'existe nulle part dans le code, la composition reprend toute la carte à
chaque geste, et les raccords de tuiles ont été retirés au `LOT-88` « à revoir pour le terrain ».

## Périmètre

Le sol et la taille. Les arbres, rochers et buissons sont au [LOT-168](LOT-168-semis-assiste.md).

## Risques

- Les raccords demandent au [LOT-170](LOT-170-kit-commun-nature.md) de **produire** les pièces de
  transition : la liste se fixe avec lui, avant la commande du kit.
