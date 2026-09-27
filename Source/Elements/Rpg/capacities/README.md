# Capacités de classe

Une capacité est une liste d'**effets nommés** (`schema/capacity.schema.json`, `LOT-131`) : un
bonus au jet d'attaque, une formule de classe d'armure sans armure, une résistance, une vitesse, des
dés de dégâts en plus, l'immunité aux attaques d'opportunité, des attaques en plus à l'action
*Attaquer*. La table de progression d'une classe
(`classes/*.json`, champ `features`) les désigne par identifiant ; le moteur les charge
(`core::loadCapacities`) et branche leurs effets sur les crochets du combat sans connaître la classe.

Les capacités des niveaux 1 à 5 y entrent classe par classe : le Brawler au `LOT-132` (*Tough as
Nails*, *Hit the Mark*, *Extra Attack*), le Mage, le Priest et le Scoundrel aux lots `LOT-133` à
`LOT-135`. *Experience* et *Ability Score Improvement*, communes aux quatre classes, sont
**narratives** : un choix du joueur au passage de niveau, déclaré comme mécanisme requis. Les
capacités d'une classe simplifiée sont provisoires comme elle (`status`). Une capacité que la table
nomme et que ce dossier ne porte pas est **signalée** au chargement de la fiche
(`LoadedCharacterSheet::warnings`), jamais jouée en silence.

L'icône d'une capacité est le membre de même identifiant de la pièce `ui/icon/capacity` du cahier
des assets de la charte v2.

La classe d'essai du socle et ses capacités vivent dans la racine d'essai,
`Source/Test/Fixtures/GameData/Rpg/`.
