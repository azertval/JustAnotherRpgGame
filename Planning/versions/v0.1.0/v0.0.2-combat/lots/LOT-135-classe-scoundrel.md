+++
id = "LOT-135"
titre = "Classe — Scoundrel"
version = "0.0.2"
filiere = "regles"
statut = "livre"
taille = "M"
resume = "Le Scoundrel se joue du niveau 1 au niveau 5, capacités en main."
prerequis = ["LOT-131"]
livrables = [
  "`Rpg/classes/scoundrel.json` : la table complète des niveaux 1 à 20, saisie de la page.",
  "Les capacités et les sorts des niveaux 1 à 5, avec leur effet en combat et leur ligne de journal.",
  "Les icônes de capacité et de sort, à la charte v2.",
]
criteres = [
  "La fiche préfabriquée du Scoundrel se joue en combat avec toutes ses capacités de niveau 1.",
  "Chaque capacité a un test qui la déclenche et vérifie son effet.",
  "Monter du niveau 1 au niveau 5 donne ce que dit la table.",
]
sources = ["Player's Guide to Tanares, p. 204-207"]
+++

## Les capacités des niveaux 1 à 5

- N1 **Sneak Attack Simplified** : +1d8 à la première touche du tour contre une cible **adjacente à un allié** ; **Scoundrel's Agility** : pas d'attaque d'opportunité, vitesse +10 ft.
- N3 +2d8, **Adventurer's Aptitude**.
- N5 +3d8, +2 CA, **Precise Striker**.

L'attaque sournoise lit l'adjacence d'un allié : c'est le calcul de la prise en tenaille du `LOT-23`, réemployé. Elle n'a de sens qu'en **groupe** — le Scoundrel seul n'en profite jamais.

La progression complète est dans [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).

## Décisions de réalisation

Livré le 27 septembre 2026. La table du Scoundrel est reprise niveau par niveau des pages 204-205 :
les identifiants fusionnés du `LOT-36` (`sneak-attack-simplified-scoundrel-s-agility`…) deviennent
les capacités qu'ils nommaient, et la colonne de l'attaque sournoise devient une capacité par
palier (`sneak-attack-simplified-2d8`… `-10d8`), chacune **remplaçant** la précédente
(`replaces`). Les capacités des niveaux 1 à 5 sont au catalogue.

1. **Sneak Attack Simplified** est un `extra-damage` une fois par tour, avec une condition
   nouvelle, `allyAdjacentToTarget` : la cible doit être adjacente à un **allié debout** de
   l'attaquant (`core::isAdjacentToAllyOf`, la moitié « allié au contact » de la prise en
   tenaille du `LOT-23`, sans la géométrie des côtés opposés). Sans allié, les dés ne s'ajoutent
   pas **et** la fois du tour n'est pas consommée ; un allié à terre ne compte pas. 1d8 au
   niveau 1, 2d8 au 3, 3d8 au 5, du type de l'arme, nommés au journal.
2. **Scoundrel's Agility** : pas d'attaque d'opportunité et +3 m, deux effets que le socle savait
   jouer ; la fiche de la page 207 passe à **40 ft** (l'écart n° 9 du registre du `LOT-130` est
   refermé). Au niveau 5, la capacité du palier suivant la remplace et ajoute +2 à la CA (16).
3. **Adventurer's Aptitude** est le genre d'effet nouveau `proficient-check-bonus` : +1 aux tests
   de caractéristique **maîtrisés**, lu par `core::skillModifier` ; les paliers de 6 à 15 sont à
   la `0.3.0`.
4. **Precise Striker** est un `attack-bonus` de 1, nommé au jet.
5. **Icônes** : une par capacité de base — une amélioration (`replaces`) garde l'icône de ce
   qu'elle remplace. Les quatre icônes de la pièce `ui/icon/capacity`, générées par l'auteur sur
   les envois du poste (`Tools/Envois/LOT-135/`, jamais livré), sont recadrées à 128 px avec
   4 px de marge, installées par `receive_ui_assets.py` et publiées dans le kit `UI@5`.

Tests : `test_class_scoundrel.cpp` — la fiche N1, l'attaque sournoise avec et sans allié au
contact, ses paliers, l'agilité, l'aptitude, *Precise Striker*, la montée de 1 à 5.

## Ce qui n'est pas ici

- *Evasion* (N7), les paliers d'*Adventurer's Aptitude* et de *Precise Striker*, *Scoundrel's
  Fortune* : la `0.3.0`.
- L'attaque sournoise en jeu suppose un groupe : la démo joue le Brawler seul, le groupe est au
  `LOT-138`.

## Exigences

- `EX-RPG-024` — chaque capacité du Scoundrel est un effet nommé, et le journal la nomme quand
  elle joue.
- `EX-RPG-023` — la classe entre sans une ligne de C++ qui la nomme ; seuls la condition
  `allyAdjacentToTarget` et le genre `proficient-check-bonus` sont ajoutés au moteur.
