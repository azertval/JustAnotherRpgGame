# Tests système

Tests système — **1 cas** (1 critique). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_parcours_edition_rpg.cpp`](#test-parcours-edition-rpgcpp) | 1 | - | 1 | - | - |

## test_parcours_edition_rpg.cpp

### ParcoursEditionSysteme.ProduitUneCarteDuRpgSansEcrireDeJson

*Critique · Système · Éditeur de niveaux* — `Source/Test/Systeme/test_parcours_edition_rpg.cpp:63`

Produire une carte du RPG dans l'editeur, sans JSON ecrit a la main.

**Étapes**

1. Creer une carte, poser l'entree et un couloir de murs dans la collision.
2. Ajouter un sol (qui reprend l'image) puis un decor, peindre l'un et l'autre.
3. Poser un PNJ au dialogue du garde, un coffre, un point d'arrivee et un portail qui y mene, une rencontre en terrain ouvert et une dans le couloir.
4. Valider contre les catalogues d essai et analyser le terrain.
5. Enregistrer, recharger, construire le graphe du monde et peupler le monde ECS.
6. Tout annuler.

**Résultat attendu**

- Vérifie que `ground && decor` est vrai.
- Vérifie que `draft.layers()[*ground].tiles.tile(12, 5)` vaut `core::TileType::Wall`.
- Vérifie que `draft.paintLayerRegion(*ground, 0, 0, std::vector<std::vector<core::TileType>>( 4, std::vector<core::TileType>(6, core::TileType::Grass)))` est vrai.
- Vérifie que `draft.paintLayerTile(*decor, 3, 3, core::TileType::Water)` est vrai.
- Vérifie que `draft.paintLayerTile(*decor, 3, 4, core::TileType::Entry)` est faux.
- Vérifie que `core::validateMapEntities(draft.entities(), context).empty()` est vrai.
- Vérifie que `terrains.size()` vaut `2U`.
- Vérifie que `terrains[0].entityIndex` vaut `open`.
- Vérifie que `terrains[0].valid()` est vrai.
- Vérifie que `terrains[1].entityIndex` vaut `corridor`.
- Vérifie que `terrains[1].valid()` est faux.
- Vérifie que `validated.ok()` est vrai.
- Vérifie que `core::LevelWriter::saveToFile(*validated.level, directory / "parcours-lot-11.json")` est vrai.
- Vérifie que `reloaded.ok()` est vrai.
- Vérifie que `reloaded.level->layers().size()` vaut `3U`.
- Vérifie que `reloaded.level->layers()[*decor].tiles.tile(3, 3)` vaut `core::TileType::Water`.
- Vérifie que `reloaded.level->entities().size()` vaut `6U`.
- Vérifie que `world.portals.size()` vaut `1U`.
- Vérifie que `world.portals.front().status` vaut `core::PortalLinkStatus::Resolved`.
- Vérifie que `core::spawnMapEntities(ecs, *reloaded.level, reloaded.level->name())` vaut `6U`.
- Vérifie que `interactables` vaut `(std::vector<std::string>{"chest", "npc"})`.
- Vérifie que `draft.undo()` est vrai.
- Vérifie que `draft.entities().empty()` est vrai.
- Vérifie que `draft.layers().empty()` est vrai.
