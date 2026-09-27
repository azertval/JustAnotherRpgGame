# Capacités de classe

Une capacité est une liste d'**effets nommés** (`schema/capacity.schema.json`, `LOT-131`) : un
bonus au jet d'attaque, une formule de classe d'armure sans armure, une résistance, une vitesse, des
dés de dégâts en plus, l'immunité aux attaques d'opportunité. La table de progression d'une classe
(`classes/*.json`, champ `features`) les désigne par identifiant ; le moteur les charge
(`core::loadCapacities`) et branche leurs effets sur les crochets du combat sans connaître la classe.

Le dossier est vide de données tant que les classes ne sont pas livrées : les capacités du Brawler,
du Mage, du Priest et du Scoundrel arrivent avec les lots `LOT-132` à `LOT-135`. Une capacité que la
table nomme et que ce dossier ne porte pas est **signalée** au chargement de la fiche
(`LoadedCharacterSheet::warnings`), jamais jouée en silence.

La classe d'essai du socle et ses capacités vivent dans la racine d'essai,
`Source/Test/Fixtures/GameData/Rpg/`.
