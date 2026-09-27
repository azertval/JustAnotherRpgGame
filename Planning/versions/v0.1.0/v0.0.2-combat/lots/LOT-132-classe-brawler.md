+++
id = "LOT-132"
titre = "Classe — Brawler"
version = "0.0.2"
filiere = "regles"
statut = "livre"
taille = "M"
resume = "Le Brawler se joue du niveau 1 au niveau 5, capacités en main."
prerequis = ["LOT-131"]
livrables = [
  "`Rpg/classes/brawler.json` : la table complète des niveaux 1 à 20, saisie de la page.",
  "Les capacités et les sorts des niveaux 1 à 5, avec leur effet en combat et leur ligne de journal.",
  "Les icônes de capacité et de sort, à la charte v2.",
]
criteres = [
  "La fiche préfabriquée du Brawler se joue en combat avec toutes ses capacités de niveau 1.",
  "Chaque capacité a un test qui la déclenche et vérifie son effet.",
  "Monter du niveau 1 au niveau 5 donne ce que dit la table.",
]
sources = ["Player's Guide to Tanares, p. 192-195"]
+++

## Les capacités des niveaux 1 à 5

- N1 **Tough as Nails** : sans armure, CA = 10 + Dex + Con ; **résistance à tous les types de dégâts**.
- N3 **Hit the Mark** : +2 aux jets d'attaque.
- N5 **Extra Attack**.

La résistance à tout est la capacité la plus forte du niveau 1 : l'équilibrage des rencontres en dépend.

La progression complète est dans [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).

## Décisions de réalisation

Livré le 27 septembre 2026 (**PR #148**). La table des niveaux 1 à 20 était déjà saisie depuis le `LOT-36` ;
ce lot écrit ses capacités des niveaux 1 à 5 dans le catalogue du `LOT-131`
(`Source/Elements/Rpg/capacities/`), et le moteur n'y gagne qu'**un genre d'effet**.

1. **Tough as Nails** n'a demandé aucun code : une formule de CA sans armure (10 + Dex + Con,
   bouclier permis) et une résistance à tous les types, deux effets que le socle savait brancher.
   La fiche de la page 195 retrouve sa **CA 14** : l'écart n° 1 du registre du `LOT-130` est
   refermé, et le test des fiches lit désormais les capacités (`loadCharacterOptions(RPG)`).
2. **Hit the Mark** est un `attack-bonus` de 2, nommé au journal (« + 2 (Hit the Mark) »).
3. **Extra Attack** est le genre d'effet nouveau, `extra-attack` (`value` : les attaques ajoutées).
   Quand l'action *Attaquer* se dépense, la session **octroie** ces attaques
   (`EXTRA_ATTACK_RESOURCE`, un octroi de l'économie d'action que le prochain début de tour
   efface) ; l'attaque suivante du tour les consomme avant l'action et le journal écrit
   « attaque supplementaire Grom Tranche-Écaille (Extra Attack) ». Elles ne servent qu'à
   attaquer : un sort, une esquive ou une précipitation prennent l'action sans rien octroyer, et
   la barre d'actions ne propose alors que les attaques. Deux sources ne s'additionnent pas
   (`extraAttacksFrom` prend la plus généreuse) ; une valeur nulle refuse la capacité.
4. **Experience** et **Ability Score Improvement**, communes aux quatre classes, entrent au
   catalogue comme capacités **narratives** qui déclarent leur mécanisme requis
   (`choix-au-passage-de-niveau`, `augmentation-de-caracteristique-au-choix`) : monter du
   niveau 1 au niveau 5 ne laisse plus d'avertissement, et le choix du joueur reste déclaré, pas
   tranché à sa place.
5. Les capacités d'une classe simplifiée sont **provisoires comme elle** (`status`,
   `EX-CNT-032`) : elles partent avec la dernière classe qui les nomme.
6. **Icônes** : la pièce `ui/icon/capacity` entre au cahier des assets de la charte v2, avec un
   membre par capacité. Les cinq icônes, générées par l'auteur sur les envois du poste
   (`Tools/Envois/LOT-132/`, jamais livré), sont recadrées à 128 px avec 4 px de marge, installées
   par `receive_ui_assets.py` (`illustrations.json`, `Artwork.qml`) et publiées dans le kit
   `UI@2`.

Tests : `test_class_brawler.cpp` (un test par capacité, la montée de 1 à 5, le refus d'une
attaque en plus nulle), sur la fiche pré-tirée et les catalogues du jeu
(`Source/Test/Support/ClassArena.h`, partagé par les lots des trois autres classes).

## Ce qui n'est pas ici

- Les capacités des niveaux 6 à 20 (*Physical Might*, *Powerful Legs*, *Deadly*, *Barbaric
  Mastery*) : la `0.3.0`. La table nomme deux fois `hit-the-mark-improvement` (N9 à +3, N13 à
  +4) ; la seconde devra prendre son propre identifiant quand ces niveaux se joueront.
- Le choix de la compétence d'*Experience* et de l'augmentation : l'écran de passage de niveau
  n'existe pas.

## Exigences

- `EX-RPG-024` — Tough as Nails, Hit the Mark et Extra Attack sont des effets nommés, et le
  journal nomme chacune quand elle joue.
- `EX-RPG-023` — la classe entre sans une ligne de C++ qui la nomme ; seul le genre d'effet
  `extra-attack` est ajouté au moteur.
