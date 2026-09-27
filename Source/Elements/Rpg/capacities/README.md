# Capacités de classe

Une capacité est une liste d'**effets nommés** (`schema/capacity.schema.json`, `LOT-131`) : un
bonus au jet d'attaque, une formule de classe d'armure sans armure, une résistance, une vitesse, des
dés de dégâts en plus, l'immunité aux attaques d'opportunité, des attaques en plus à l'action
*Attaquer*. La table de progression d'une classe
(`classes/*.json`, champ `features`) les désigne par identifiant ; le moteur les charge
(`core::loadCapacities`) et branche leurs effets sur les crochets du combat sans connaître la classe.

Les capacités des niveaux 1 à 5 des quatre classes y sont : le Brawler (`LOT-132` : *Tough as
Nails*, *Hit the Mark*, *Extra Attack*), le Mage (`LOT-133` : *Arcane Protection*), le Priest
(`LOT-134`) et le Scoundrel (`LOT-135` : *Sneak Attack Simplified* et ses paliers, *Scoundrel's
Agility*, *Adventurer's Aptitude*, *Precise Striker*). Un palier supérieur est une capacité qui
**remplace** la précédente (`replaces`). *Experience* et *Ability Score Improvement*, communes aux quatre classes, sont
**narratives** : un choix du joueur au passage de niveau, déclaré comme mécanisme requis. Les
capacités d'une classe simplifiée sont provisoires comme elle (`status`). Une capacité que la table
nomme et que ce dossier ne porte pas est **signalée** au chargement de la fiche
(`LoadedCharacterSheet::warnings`), jamais jouée en silence.

L'icône d'une capacité est le membre de même identifiant de la pièce `ui/icon/capacity` du cahier
des assets de la charte v2 ; une capacité qui en remplace une autre garde l'icône de celle-ci.

La classe d'essai du socle et ses capacités vivent dans la racine d'essai,
`Source/Test/Fixtures/GameData/Rpg/`.
