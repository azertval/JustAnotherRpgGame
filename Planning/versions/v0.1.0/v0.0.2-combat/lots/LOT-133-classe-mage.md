+++
id = "LOT-133"
titre = "Classe — Mage"
version = "0.0.2"
filiere = "regles"
statut = "livre"
taille = "M"
resume = "Le Mage se joue du niveau 1 au niveau 5, capacités en main."
prerequis = ["LOT-131"]
livrables = [
  "`Rpg/classes/mage.json` : la table complète des niveaux 1 à 20, saisie de la page.",
  "Les capacités et les sorts des niveaux 1 à 5, avec leur effet en combat et leur ligne de journal.",
  "Les icônes de capacité et de sort, à la charte v2.",
]
criteres = [
  "La fiche préfabriquée du Mage se joue en combat avec toutes ses capacités de niveau 1.",
  "Chaque capacité a un test qui la déclenche et vérifie son effet.",
  "Monter du niveau 1 au niveau 5 donne ce que dit la table.",
]
sources = ["Player's Guide to Tanares, p. 196-199"]
+++

## Les capacités des niveaux 1 à 5

- N1 tours *fire bolt*, *light* ; sorts *detect magic*, *magic missile* (2 par jour chacun).
- N2 **Arcane Protection** : sans armure, CA = 13 + Dex.
- N3 *mage hand*, *invisibility*, *scorching ray*.
- N5 *fireball*, *fly*.

*Fireball* demande les zones d'effet du `LOT-22` ; *fly* demande le vol, que la grille tactique connaît déjà comme attribut.

La progression complète est dans [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).

## Décisions de réalisation

Livré le 27 septembre 2026. La table du Mage est reprise **niveau par niveau** de la page 197 :
les identifiants fusionnés du `LOT-36` (`experience-arcane-protection`…) deviennent les
capacités qu'ils nommaient, et chaque ligne porte ses sorts mineurs et ses sorts (`cantrips`,
`spells`) jusqu'au niveau 20. Les capacités et les sorts des niveaux 1 à 5 sont au catalogue.

1. **Arcane Protection** (N2) est une formule de CA sans armure, 13 + Dex : la fiche de la page
   199 passe de 12 à 15. **Simplified Spellcasting** et **Specific Cantrips** sont narratives :
   l'incantation se joue par la table et `spellcasting.castsPerDay` (`LOT-131`), elles ne
   portent aucun effet à elles seules.
2. **Neuf sorts** entrent au catalogue `Rpg/spells/`, nommés et relevés dans le *Manuel des
   Joueurs* (`source: phb-fr`, page PDF dans le texte). *Lumière*, *main du mage* et *détection
   de la magie* sont narratifs : le grimoire de combat les tait, et le chargement du héros les
   nomme au journal de l'application (`EX-RPG-051`).
3. **Les mécanismes de sort** que le Mage demande, déclarés en données (`spell.schema.json`) et
   choisis par `core::spellMechanism`, jamais par le nom d'un sort :
   - `projectiles` : un jet d'attaque par rayon (*rayon ardent*, 3), chacun sa ligne « 1/3 : » ;
     le moteur les dirige sur une seule cible, et ceux qui restent quand elle tombe sont
     « perdus », écrits au journal ;
   - `autoHit` : les dés de chaque projectile sans jet, en une salve (*projectile magique*,
     3 × 1d4+1 de force) ;
   - `savingThrow` + `saveEffect` + `area` : les dés lancés une fois, puis chaque créature de la
     **sphère** — alliés et lanceur compris — jette sa sauvegarde contre le DD du lanceur
     (8 + maîtrise + Int), une ligne chacune ; `half` divise par deux, `negates` annule
     (*boule de feu*). La sphère se centre sur une créature visée : viser une case vide n'est
     pas encore joué. Une autre forme de zone se charge et ne se joue pas ;
   - `effect` + `target` : un **effet qui dure** posé sur une créature de son camp
     (`ArenaEffect`). *Vol* change la locomotion et le budget (18 m, 12 cases ;
     `CombatState::setLocomotion` garde ce qui a déjà été marché) ; *invisibilité* désavantage
     qui l'attaque, avantage ses attaques, soustrait aux attaques d'opportunité, et cesse quand
     son porteur attaque ou lance un sort ;
   - `cantripScaling` : les dés d'un sort mineur montent aux niveaux 5, 11 et 17 (*trait de feu*
     à 2d10 au niveau 5).
4. **La concentration** : un second sort de concentration met fin au premier, un lanceur qui
   tombe perd la sienne, l'échéance d'une durée se compte au round ; chaque fin s'écrit
   « fin de l'effet … (raison) ». Le jet de concentration après des dégâts est au `LOT-137`
   (`mecanismesRequis: jet-de-concentration`).
5. **Les sauvegardes** entrent dans le profil de combat (`CombatantProfile::savingThrows`) :
   modificateur plus maîtrise pour une fiche, modificateur nu pour une créature du bestiaire,
   qui ne porte pas encore ses sauvegardes maîtrisées.
6. **L'écran de combat** lance un sort choisi dans la barre sur la créature cliquée, alliée ou
   non — le sort sait qui il vise, et son refus le dit — et affiche la ligne du sort
   (`ArenaAttack::summary`), un sort sans jet n'ayant pas d'issue d'attaque.
7. **Icônes** : les trois capacités entrent à la pièce `ui/icon/capacity`, et une pièce
   `ui/icon/spell` naît au cahier avec un membre par sort ; les douze clés sont « non livrées »
   dans le manifeste, et les envois au générateur préparés sur le poste
   (`Tools/Envois/LOT-133/`).

Tests : `test_class_mage.cpp` — la fiche N1 et son grimoire, un test par sort joué et par
capacité, la montée de 1 à 5, le refus d'un effet inconnu.

## Ce qui n'est pas ici

- Répartir les projectiles entre plusieurs cibles, viser un point vide, lancer un sort à un
  niveau supérieur en dépensant un autre sort (*Player's Guide*, p. 197).
- Le jet de concentration après des dégâts : `LOT-137`.
- La prévisualisation d'une zone à l'écran : `LOT-140`.
- Les capacités et sorts des niveaux 6 à 20 : la `0.3.0`.
- Les images des icônes : en attente de génération par l'auteur.

## Exigences

- `EX-RPG-024` — *Arcane Protection* est un effet nommé.
- `EX-RPG-025` — chaque sort du Mage garde son compte de deux lancers par jour.
- `EX-RPG-050` — chaque sort est une combinaison déclarée de mécanismes ; `EX-RPG-051` — les
  sorts sans effet en combat se déclarent narratifs.
