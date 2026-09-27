# Sorts

Un sort est une **combinaison déclarée de mécanismes** (`schema/spell.schema.json`, `EX-RPG-050`),
jamais une fonction. Le socle de classe (`LOT-131`) n'en joue qu'un : le sort à **jet d'attaque**
(`attackRoll: true`, des dés et un type de dégâts, une portée en mètres), résolu comme une attaque à
distance au modificateur de la caractéristique d'incantation plus la maîtrise. Les sorts à jet de
sauvegarde, de soin ou de condition déclarent leurs champs et attendent leurs mécanismes avec leur
classe (`LOT-133`, `LOT-134`, `LOT-137`).

L'**incantation simplifiée** du *Player's Guide* (p. 196, 200) ne connaît pas d'emplacements : la
table de progression d'une classe (`classes/*.json`, champs `cantrips` et `spells`) dit quels sorts
sont connus à quel niveau, et chacun se lance `spellcasting.castsPerDay` fois par jour. Le compte est
sur la fiche (`CharacterSheet::knownSpells`) ; un repos long le rend (`core::longRest`).

Le dossier est vide de données tant que les classes ne sont pas livrées : les sorts du Mage et du
Priest arrivent avec les lots `LOT-133` et `LOT-134`. Les sorts d'essai du socle vivent dans la
racine d'essai, `Source/Test/Fixtures/GameData/Rpg/spells/`.
