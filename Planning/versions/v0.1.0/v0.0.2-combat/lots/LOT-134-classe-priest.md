+++
id = "LOT-134"
titre = "Classe — Priest"
version = "0.0.2"
filiere = "regles"
statut = "livre"
taille = "M"
resume = "Le Priest se joue du niveau 1 au niveau 5, capacités en main."
prerequis = ["LOT-131"]
livrables = [
  "`Rpg/classes/priest.json` : la table complète des niveaux 1 à 20, saisie de la page.",
  "Les capacités et les sorts des niveaux 1 à 5, avec leur effet en combat et leur ligne de journal.",
  "Les icônes de capacité et de sort, à la charte v2.",
]
criteres = [
  "La fiche préfabriquée du Priest se joue en combat avec toutes ses capacités de niveau 1.",
  "Chaque capacité a un test qui la déclenche et vérifie son effet.",
  "Monter du niveau 1 au niveau 5 donne ce que dit la table.",
]
sources = ["Player's Guide to Tanares, p. 200-203"]
+++

## Les capacités des niveaux 1 à 5

- N1 tours *light*, *sacred flame* ; sorts *bless*, *cure wounds* (2 par jour chacun).
- N3 *spare the dying*, *lesser restoration*, *spiritual weapon*.
- N5 *daylight*, *revivify*.

*Bless* demande la concentration ; *revivify* demande les règles de mort du LOT-137.

La progression complète est dans [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).

## Décisions de réalisation

Livré le 27 septembre 2026 (**PR #150**). La table du Priest est reprise niveau par niveau de la page 201, sorts
mineurs et sorts compris jusqu'au niveau 20 ; ses capacités des niveaux 1 à 5 (*Simplified
Spellcasting*, *Specific Cantrips*, *Experience*, *Ability Score Improvement*) étaient déjà au
catalogue. Neuf sorts entrent au catalogue `Rpg/spells/`, nommés d'après le *Manuel des Joueurs*.

1. **Flamme sacrée** réemploie la sauvegarde du `LOT-133`, avec `saveEffect: negates` : une
   réussite ne coûte rien ; le sort mineur monte à 2d8 au niveau 5.
2. **Soin des blessures** est le mécanisme nouveau `healing` : 1d8 plus le modificateur
   d'incantation (`addsAbilityModifier`), rendus à une créature de son camp au contact. Un allié
   **à terre** est une cible valide du soin, et il se relève (`CombatState::heal`) ; le journal
   écrit « soin … -> … : 1d8+3 : … ; PV a -> b ».
3. **Bénédiction** est un effet qui dure (`bless`, `dice: 1d4`, dix rounds, concentration) posé
   sur **trois** créatures (`maxTargets`) : la cible choisie, puis les alliés debout les plus
   proches du lanceur, à portée — lui compris. Le d4 se lance à chaque jet d'attaque (crochet
   `BeforeRoll`) et à chaque sauvegarde de sort, nommé « Benediction » ; l'effet cesse à
   l'échéance, écrite au journal.
4. **Arme spirituelle** se lance par une **action bonus** (`bonusAction`) : une attaque de sort
   **au corps à corps** (`attackKind: melee`, la portée sert d'allonge), 1d8 + Sag de force, puis
   l'arme reste (`spiritual-weapon`, dix rounds). Tant qu'elle dure, relancer le sort la fait
   frapper de nouveau par une action bonus **sans** dépenser de lancer ; la barre d'actions la
   propose même lancers épuisés. Le moteur la fait frapper une créature à portée du lanceur, sans
   la déplacer case par case.
5. *Lumière du jour* est narratif. *Épargner les mourants*, *restauration inférieure* et
   *revigorer* attendent le `LOT-137` — l'agonie, les états en combat, la mort — et le déclarent
   (`mecanismesRequis`) ; le grimoire de combat les tait.
6. **Icônes** : les huit sorts nouveaux entrent à la pièce `ui/icon/spell` du cahier. Générées
   par l'auteur sur les envois du poste (`Tools/Envois/LOT-134/`, jamais livré), elles sont
   recadrées à 128 px avec 4 px de marge, installées par `receive_ui_assets.py` et publiées dans
   le kit `UI@4`.

Tests : `test_class_priest.cpp` — la fiche N1 et son grimoire, un test par sort joué, la montée de
1 à 5 et les sorts qui attendent le `LOT-137`.

## Ce qui n'est pas ici

- *Épargner les mourants*, *restauration inférieure*, *revigorer* : `LOT-137`.
- Choisir les trois cibles de *bénédiction* une à une, déplacer l'arme spirituelle : `LOT-140`.
- *Divine Cure* et *Holy Shield* (niveaux 6 et 10) : la `0.3.0`.

## Exigences

- `EX-RPG-025` — chaque sort du Priest garde son compte de deux lancers par jour ; l'arme
  spirituelle qui frappe de nouveau n'en dépense pas.
- `EX-RPG-050` — soin, bénédiction et arme spirituelle sont des combinaisons déclarées de
  mécanismes ; `EX-RPG-051` — ceux qui attendent un autre lot le déclarent.
