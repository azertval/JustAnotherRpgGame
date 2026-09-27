# Sorts

Un sort est une **combinaison déclarée de mécanismes** (`schema/spell.schema.json`, `EX-RPG-050`),
jamais une fonction. Le moteur en joue quatre (`core::spellMechanism`) : le **jet d'attaque**
(`attackRoll`, un par projectile), le sort qui **touche sans jet** (`autoHit`), le **jet de
sauvegarde** d'une cible ou de chaque créature d'une **sphère** (`savingThrow`, `saveEffect`,
`area`), et l'**effet qui dure** posé sur une créature de son camp (`effect`, `target` : le vol,
l'invisibilité), sous concentration. Un sort sans effet en combat se déclare narratif
(`EX-RPG-051`) ; ce qu'il exige et que le moteur n'honore pas encore, il le déclare
(`mecanismesRequis`).

L'**incantation simplifiée** du *Player's Guide* (p. 196, 200) ne connaît pas d'emplacements : la
table de progression d'une classe (`classes/*.json`, champs `cantrips` et `spells`) dit quels sorts
sont connus à quel niveau, et chacun se lance `spellcasting.castsPerDay` fois par jour. Le compte est
sur la fiche (`CharacterSheet::knownSpells`) ; un repos long le rend (`core::longRest`).

Les sorts du Mage (`LOT-133`) et du Priest (`LOT-134`) y sont, nommés comme dans le *Manuel des
Joueurs*. Le Priest ajoute le **soin** (`healing`), les effets à plusieurs cibles (`maxTargets`),
l'**action bonus** (`bonusAction`) et l'attaque de sort au corps à corps (`attackKind`). L'icône d'un sort est le membre de même identifiant de la pièce
`ui/icon/spell` du cahier des assets de la charte v2. Les sorts d'essai du socle vivent dans la
racine d'essai, `Source/Test/Fixtures/GameData/Rpg/spells/`.
