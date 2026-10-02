+++
id = "LOT-1007"
titre = "Éclairage et cycle jour / nuit"
version = "0.0.2.5"
filiere = "moteur"
statut = "a-faire"
taille = "L"
resume = "Le monde a une heure : le soleil tourne, les ombres s'allongent, et la nuit les lampadaires et les fenêtres éclairent la rue."
prerequis = ["LOT-1003"]
livrables = [
  "L'**heure du monde** dans `Source/Core` : elle avance avec le temps de jeu, se fige en combat, se lit et se règle par une commande de débug ; rien d'autre n'en dépend dans cette version.",
  "Un **shader éclairé** pour les maillages : lumière d'ambiance et soleil directionnel, dont la couleur et la direction suivent l'heure par une table en données.",
  "Les **ombres portées** du soleil (carte de profondeur), sur le décor et les personnages.",
  "Les **lumières de nuit** : une pièce déclare la lumière qu'elle émet (position, couleur, portée) dans le manifeste du lieu ; elles s'allument au crépuscule.",
  "La **teinte de l'heure** appliquée aux images tolérées (mobilier, kits non convertis) et aux effets.",
  "Des références d'image à quatre heures — aube, midi, crépuscule, nuit — sur la carte d'essai.",
]
criteres = [
  "Sur la carte d'Arenarea, l'auteur juge un cycle complet accéléré : pas de saut de lumière, une nuit où l'on lit encore la grille et les personnages.",
  "À midi, le rendu d'un mur et d'un toit en maillage de la carte d'essai (`Source/Test/Fixtures/Meshes`) est conforme à leur rendu sans lumière : la lumière n'a pas changé la facture.",
  "Un combat lancé de nuit se joue et se lit comme de jour : cases de portée, cibles et zones restent visibles.",
  "`bench_world_frame` tient la cadence avec ombres et huit lumières de nuit à l'écran.",
  "Aucune texture de **modèle** ne porte d'ombre propre cuite : la même matière sert à toute heure. Celles du kit de la Capitale, resté en images (D-43), partent avec lui à la `0.0.3`.",
]
+++

## Pourquoi

C'est le second but de la version. Il n'est possible que parce que le décor a désormais des faces
orientées : une image peinte garde sa lumière « du haut à gauche » à toute heure.

> **Amendé le 2 octobre 2026** ([D-43](../../../../vision/decisions.md)) : le
> [LOT-1004](LOT-1004-kit-de-la-capitale-en-maillages.md) est clos sans modification, le kit de la
> Capitale reste en images jusqu'à la `0.0.3`. Ce lot éclaire donc les **personnages** et les
> maillages de la carte d'essai, et **teinte** tout le décor ; le soleil sur l'architecture viendra
> avec le kit repris ([LOT-151](../../v0.0.3-capitale-intra-muros/lots/LOT-151-kit-commun-intra-muros.md)).

## Périmètre

Dedans : l'heure, le soleil, les ombres, les lumières de nuit, la teinte des images.

Dehors, nommément :

- ce que l'heure **déclenche** dans le jeu — présence des PNJ, boutiques fermées, rencontres de
  nuit : ce sont des règles, elles viendront avec les zones ;
- la météo ;
- l'éclairage des images tolérées : elles ne reçoivent que la teinte. C'est une raison de plus de
  les modeler à la `0.0.3`.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| ~~L'ombre propre posée par sommet au LOT-1004~~ | — | sans objet : le kit n'a pas été converti (D-43) ; le LOT-151 produit ses maillages sans ombre cuite |
| La règle « lumière du haut à gauche, ombre propre peinte » pour ce qui est modelé | `Planning/standards/style-3d.md` | elle ne vaut plus que pour les images tolérées |
| ~~Les références d'image du LOT-1004 rendues sans lumière~~ | — | sans objet (D-43) |

Rien d'autre : ce lot ajoute une capacité, il ne remplace pas un mécanisme existant — `Core` n'a
aujourd'hui aucune heure du monde.

## Conception

Quatre niveaux, dans l'ordre où ils se livrent ; chacun laisse le jeu dans un état montrable.

| Niveau | Ce qu'on voit |
|---|---|
| 1 — Teinte | l'image entière vire au bleu la nuit, à l'or au couchant |
| 2 — Soleil | les faces des murs et des toits s'éclairent selon l'heure |
| 3 — Ombres portées | bâtiments et personnages jettent une ombre qui s'allonge |
| 4 — Lumières de nuit | lampadaires, fenêtres, torches éclairent autour d'eux |

Le standard 2D le prévoyait à moitié : « pas d'ombre portée dans la pièce, l'ombre au sol est
l'affaire du moteur ».

## Risques et questions ouvertes

- **La lisibilité du combat la nuit** passe avant l'ambiance : c'est un critère, et la nuit se
  règle par sa table de couleurs, pas par le code.
- Un lampadaire est aujourd'hui une **image** (mobilier toléré) : il émet de la lumière sans être
  éclairé. Acceptable jusqu'à la `0.0.3`.
- Le nombre de lumières simultanées borne le shader : huit au critère, à confirmer sur la place la
  plus éclairée.
