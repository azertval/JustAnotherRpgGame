+++
id = "LOT-157"
titre = "Donjon — les catacombes du colisée"
version = "0.0.3"
filiere = "cartes"
statut = "a-faire"
taille = "L"
resume = "Sous l'Arena of Fate, les catacombes où les quatre Ungods sont représentés enchaînés, tenues par le Culte de l'Aile d'Ombre."
prerequis = ["LOT-107", "LOT-151"]
livrables = [
  "`Levels/central-empire/capital/arenarea/arena-of-fate/catacombs.json`, dessinée **dans l'éditeur**.",
  "`Regions/central-empire/capital/arenarea/arena-of-fate/catacombs/Scene/` : les pièces propres aux catacombes, `manifest.json`, `appearance.json`.",
  "Les **quatre Ungods enchaînés** : C'thraxis, Droggath, Krynnethoth, Z'ulvath — une pièce maîtresse chacun.",
  "L'ouverture de l'**escalier des catacombes** au fond de la prison (`LOT-107` l'a posé condamné).",
  "La page de galerie de la zone.",
]
criteres = [
  "`LevelEditor --check` passe : aucune case inatteignable, aucun portail sans arrivée — y compris l'escalier de la prison, désormais relié.",
  "Les quatre Ungods sont posés, nommés, et paraissent dans la galerie.",
  "On descend depuis la prison de l'arène et on remonte, sans quitter le jeu.",
]
sources = [
  "Tanares Sourcebook, ch. 4 (p. 68-86) : les quatre Ungods",
  "Tanares Sourcebook, ch. 2 (p. 44-45) : le Culte de l'Aile d'Ombre, ses quatre sectes",
]
+++

## Pourquoi

La [DA de l'Arena of Fate](../../../../referentiels/central-empire/capitale.md#larena-of-fate--architecture-et-iconographie-da-de-lauteur-21-sept-2026)
(D-18) sort les Ungods et le Culte du colisée : **ni statue, ni loge, ni bannière au-dessus du
sable**. Ce qui est chassé de l'arène est sous elle. Les catacombes sont le revers du monument —
l'Empire montre quatorze dieux au public et tient les quatre autres enchaînés sous la prison — et
elles valent un donjon à elles seules, pas une salle de plus.

Sans ce lot, l'escalier posé au fond de la prison par le `LOT-107` reste une porte condamnée.

## Périmètre

**Dedans** : la carte des catacombes, ses pièces de scène propres, les quatre Ungods enchaînés,
l'iconographie du Culte, la liaison avec la prison.

**Dehors** : les monstres et les PNJ du Culte (lot de PNJ de la version), la quête qui y mène,
et tout ce qui touche au niveau 0 ou −1 du colisée — c'est le `LOT-106` et le `LOT-107`.

## Conception

Le seul accès est **par la prison** de l'arène, et il est gardé : on n'entre pas dans les
catacombes par la rue. Le donjon est un lieu clos à plusieurs salles, sous la ville, au-dessous du
niveau −1 du colisée.

Les **quatre Ungods** y sont **représentés, enchaînés** — des représentations, pas les entités.
Chacun est une pièce maîtresse, à sa mesure : C'thraxis, Droggath, Krynnethoth, Z'ulvath.

## Risques et questions ouvertes

- **La version.** Le donjon est posé en `0.0.3` parce qu'Arenarea est intra-muros et que le kit
  commun intra-muros (`LOT-151`) lui sert de base. À déplacer si l'auteur préfère le rattacher à la
  quête qui l'ouvrira.
- **Le contenu du donjon** — nombre de salles, rencontres, récompense — n'est pas arrêté : il se
  décide au démarrage du lot, avec la quête qui y conduit.
