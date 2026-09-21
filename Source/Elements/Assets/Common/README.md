<!-- Dossier encore vide : il attend le lot qui le remplira. -->

# Le commun du monde

Ce qui existe **partout**, dans n'importe quelle région : sols naturels (`Terrain/`),
arbres et rochers (`Nature/`), mobilier et objets génériques (`Props/`), effets de combat et de
sort (`Fx/`), et les personnages que l'on retrouve d'un bout à l'autre du monde (`Characters/` :
héros jouables, peuples, bêtes, monstres sans région).

Un asset n'arrive ici que par **promotion** : il naît dans sa zone, et le jour où une deuxième
zone en a besoin, il monte au premier niveau commun aux deux — jamais copié. La promotion est un
renommage outillé (`LevelEditor --replace-piece`), qui réécrit les cartes qui le citent.

Voir [l'arborescence](../../../../Planning/standards/arborescence-assets.md).
