# Couverture des exigences

**33 exigences en vigueur sur 298** sont citées par au moins un cas de test. Cette matrice est **engendrée** avec le reste du cahier : la colonne de gauche vient des déclarations des [spécifications](../Specification/README.md), celle de droite des identifiants `EX-…` que les tests citent dans leur commentaire ou leur corps. Une exigence sans cas est une exigence que **rien ne garde** : le cahier ne la cache pas, il la montre. Les exigences retirées ne sont pas comptées — aucun test ne doit les citer.

## Par famille

| Famille | Déclarée dans | En vigueur | Citées par un test | Sans test |
|---|---|---|---|---|
| `EX-ARCH` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | 13 | 0 | 13 |
| `EX-BUILD` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | 1 | 0 | 1 |
| `EX-CBT` | [Combat tactique](../Specification/combat.md) | 14 | 2 | 12 |
| `EX-CNT` | [Contenu et données](../Specification/contenu.md) | 22 | 4 | 18 |
| `EX-CTRL` | [Contrôles & entrées](../Specification/controles.md) | 7 | 0 | 7 |
| `EX-EDIT` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | 76 | 8 | 68 |
| `EX-EXP` | [Exploration](../Specification/exploration.md) | 12 | 1 | 11 |
| `EX-GP` | [Gameplay](../Specification/gameplay.md) | 7 | 1 | 6 |
| `EX-IHM` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | 32 | 2 | 30 |
| `EX-INV` | [Inventaire et économie](../Specification/inventaire.md) | 8 | 0 | 8 |
| `EX-LVL` | [Cartes & format](../Specification/niveaux.md) | 20 | 7 | 13 |
| `EX-NFR` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | 19 | 2 | 17 |
| `EX-REG` | [Règles d20](../Specification/regles-d20.md) | 15 | 1 | 14 |
| `EX-REN` | [Rendu & cible technique](../Specification/rendu-technique.md) | 26 | 4 | 22 |
| `EX-RPG` | [Personnage et progression](../Specification/rpg.md) | 17 | 1 | 16 |
| `EX-VIS` | [Vision & périmètre](../Specification/vision.md) | 9 | 0 | 9 |
| **Total** | | **298** | **33** | **265** |

## Exigence par exigence

Un cas se lit dans la page de son domaine ; le lien y mène. « — » : aucun cas ne cite l'exigence.

### `EX-ARCH`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-ARCH-001` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-010` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-011` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-012` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-020` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-021` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-022` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-030` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-040` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-050` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-060` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-070` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |
| `EX-ARCH-080` | [Architecture (décisions dimensionnantes)](../Specification/architecture.md) | — |

### `EX-BUILD`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-BUILD-010` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |

### `EX-CBT`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-CBT-001` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-010` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-011` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-012` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-020` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-021` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-022` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-030` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-031` | [Combat tactique](../Specification/combat.md) | [`DamageTest.LeCritiqueDoubleLesDesPasLeModificateur`](core-combat.md#damagetestlecritiquedoublelesdespaslemodificateur) |
| `EX-CBT-032` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-040` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-041` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-042` | [Combat tactique](../Specification/combat.md) | — |
| `EX-CBT-050` | [Combat tactique](../Specification/combat.md) | [`EnemyAiTest.LIaNeLitQueLEtatEnsanglante`](core-combat.md#enemyaitestlianelitqueletatensanglante) |

### `EX-CNT`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-CNT-001` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-002` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-010` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-011` | [Contenu et données](../Specification/contenu.md) | [`DiceTest.AllerRetourDesCaracteristiques`](core-rpg.md#dicetestallerretourdescaracteristiques), [`RpgEnumsTest.UnNomInconnuEstRefuse`](core-rpg.md#rpgenumstestunnominconnuestrefuse), [`RpgEnumsTest.LesEnumerationsCoincidentAvecLesSchemas`](core-rpg.md#rpgenumstestlesenumerationscoincidentaveclesschemas), [`AtlasTest.LesNotesDuMoteurCoincidentAvecCellesDuSchema`](core-world.md#atlastestlesnotesdumoteurcoincidentaveccellesduschema) |
| `EX-CNT-012` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-020` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-021` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-022` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-023` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-030` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-031` | [Contenu et données](../Specification/contenu.md) | [`BestiaryTest.LesMecanismesExigesSontAnnonces`](core-rpg.md#bestiarytestlesmecanismesexigessontannonces), [`CharacterOptionsTest.LesMecanismesExigesSontAnnonces`](core-rpg.md#characteroptionstestlesmecanismesexigessontannonces) |
| `EX-CNT-032` | [Contenu et données](../Specification/contenu.md) | [`CharacterOptionsTest.LesClassesProvisoiresNeSontReferenceesParRien`](core-rpg.md#characteroptionstestlesclassesprovisoiresnesontreferenceesparrien) |
| `EX-CNT-040` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-041` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-042` | [Contenu et données](../Specification/contenu.md) | [`AssetGalleryTest.ToutAssetLivreEstDansLaGalerie`](hmi-graphics.md#assetgallerytesttoutassetlivreestdanslagalerie), [`AssetGalleryTest.UnHerosOrienteRangeParClasse`](hmi-graphics.md#assetgallerytestunherosorienterangeparclasse) |
| `EX-CNT-050` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-060` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-061` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-062` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-070` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-071` | [Contenu et données](../Specification/contenu.md) | — |
| `EX-CNT-072` | [Contenu et données](../Specification/contenu.md) | — |

### `EX-CTRL`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-CTRL-001` | [Contrôles & entrées](../Specification/controles.md) | — |
| `EX-CTRL-002` | [Contrôles & entrées](../Specification/controles.md) | — |
| `EX-CTRL-010` | [Contrôles & entrées](../Specification/controles.md) | — |
| `EX-CTRL-011` | [Contrôles & entrées](../Specification/controles.md) | — |
| `EX-CTRL-012` | [Contrôles & entrées](../Specification/controles.md) | — |
| `EX-CTRL-020` | [Contrôles & entrées](../Specification/controles.md) | — |
| `EX-CTRL-022` | [Contrôles & entrées](../Specification/controles.md) | — |

### `EX-EDIT`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-EDIT-001` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-002` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-004` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | [`LevelDraftTest.SetEntryDeplaceLEntreeExistante`](core-levels.md#leveldrafttestsetentrydeplacelentreeexistante) |
| `EX-EDIT-005` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-006` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-007` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | [`LevelDraftTest.ToLevelSansEntreeEchoueProprement`](core-levels.md#leveldrafttesttolevelsansentreeechoueproprement) |
| `EX-EDIT-008` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-009` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-010` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-011` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | [`CouchesDeCarteTest.BrouillonDEditionPreserveCouchesEtEntites`](core-levels.md#couchesdecartetestbrouillondeditionpreservecouchesetentites) |
| `EX-EDIT-012` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-013` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-014` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-015` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-017` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-018` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-020` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-021` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-022` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-023` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-030` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-031` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-043` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-048` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-049` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-050` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-051` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-052` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-053` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-054` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-055` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-056` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-057` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-058` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-059` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-060` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-061` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-062` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-063` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-064` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-065` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | [`LevelDraftPiecesTest.PeindreLaCollisionForceOuLibereLaCase`](core-levels.md#leveldraftpiecestestpeindrelacollisionforceouliberelacase) |
| `EX-EDIT-066` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-067` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-068` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-069` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-070` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-071` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-072` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-073` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | [`FamillesDEntitesTest.ToutFamilleLueParLeJeuEstDansLaTable`](core-world.md#famillesdentitestesttoutfamillelueparlejeuestdanslatable) |
| `EX-EDIT-074` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-075` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-076` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-077` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | [`ContentCheckTest.UneCarteNeuveASonNomDansChaqueCatalogue`](editor.md#contentchecktestunecarteneuveasonnomdanschaquecatalogue) |
| `EX-EDIT-078` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-079` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-080` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-081` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-082` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-083` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | [`LevelDraftPiecesTest.RemplacerUnePieceEnUnPasLaCollisionSuit`](core-levels.md#leveldraftpiecestestremplacerunepieceenunpaslacollisionsuit), [`Donnees.RemplacerUnePieceSurToutesLesCartes`](editor.md#donneesremplacerunepiecesurtouteslescartes) |
| `EX-EDIT-084` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | [`LevelDraftPiecesTest.ChangerDePlancheTraduitLesPiecesEtRededuitLaCollision`](core-levels.md#leveldraftpiecestestchangerdeplanchetraduitlespiecesetrededuitlacollision), [`Donnees.UneCarteChangeDePlancheSansEtreRepeinte`](editor.md#donneesunecartechangedeplanchesansetrerepeinte) |
| `EX-EDIT-085` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-086` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-087` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-088` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-089` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-090` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-091` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-092` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-093` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-094` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-095` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-096` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-097` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-098` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-099` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |
| `EX-EDIT-100` | [Éditeur de cartes](../Specification/editeur-niveaux.md) | — |

### `EX-EXP`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-EXP-001` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-002` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-003` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-004` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-005` | [Exploration](../Specification/exploration.md) | [`ScenePainterTest.UneCarteSansAucuneImageSeVoitDansLesDeuxRendus`](editor.md#scenepaintertestunecartesansaucuneimagesevoitdanslesdeuxrendus) |
| `EX-EXP-006` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-007` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-008` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-009` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-010` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-011` | [Exploration](../Specification/exploration.md) | — |
| `EX-EXP-012` | [Exploration](../Specification/exploration.md) | — |

### `EX-GP`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-GP-001` | [Gameplay](../Specification/gameplay.md) | — |
| `EX-GP-002` | [Gameplay](../Specification/gameplay.md) | — |
| `EX-GP-014` | [Gameplay](../Specification/gameplay.md) | — |
| `EX-GP-040` | [Gameplay](../Specification/gameplay.md) | — |
| `EX-GP-041` | [Gameplay](../Specification/gameplay.md) | [`FixedTimestepTest.PauseSansAppelNAccumuleAucunPas`](core-time.md#fixedtimesteptestpausesansappelnaccumuleaucunpas) |
| `EX-GP-070` | [Gameplay](../Specification/gameplay.md) | — |
| `EX-GP-071` | [Gameplay](../Specification/gameplay.md) | — |

### `EX-IHM`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-IHM-001` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-002` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-003` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-004` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-011` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-021` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-040` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-041` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-050` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-051` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-052` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-053` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-062` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-070` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-071` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-072` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-075` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-076` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-080` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-081` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-082` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-083` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-090` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | [`ScreenFlowTest.EcranDuRpgRevientVersSonEcranDOrigine`](hmi-interface.md#screenflowtestecrandurpgrevientverssonecrandorigine) |
| `EX-IHM-091` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | [`ScreenFlowTest.LesEcransDeFinFermentLaPartie`](hmi-interface.md#screenflowtestlesecransdefinfermentlapartie), [`EncounterModelTest.DuDeclenchementAuRetourALExploration`](hmi-runtime.md#encountermodeltestdudeclenchementauretouralexploration) |
| `EX-IHM-100` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-101` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-102` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-103` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-104` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-105` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-106` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |
| `EX-IHM-107` | [Interface utilisateur (IHM)](../Specification/interface-ihm.md) | — |

### `EX-INV`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-INV-001` | [Inventaire et économie](../Specification/inventaire.md) | — |
| `EX-INV-010` | [Inventaire et économie](../Specification/inventaire.md) | — |
| `EX-INV-011` | [Inventaire et économie](../Specification/inventaire.md) | — |
| `EX-INV-020` | [Inventaire et économie](../Specification/inventaire.md) | — |
| `EX-INV-030` | [Inventaire et économie](../Specification/inventaire.md) | — |
| `EX-INV-031` | [Inventaire et économie](../Specification/inventaire.md) | — |
| `EX-INV-040` | [Inventaire et économie](../Specification/inventaire.md) | — |
| `EX-INV-041` | [Inventaire et économie](../Specification/inventaire.md) | — |

### `EX-LVL`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-LVL-001` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-002` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-003` | [Cartes & format](../Specification/niveaux.md) | [`TerrainRpgTest.AllerRetourSurChaqueTypeDeTerrain`](core-levels.md#terrainrpgtestallerretoursurchaquetypedeterrain) |
| `EX-LVL-004` | [Cartes & format](../Specification/niveaux.md) | [`CouchesDeCarteTest.TuileHorsBornesDansUneCoucheRefusee`](core-levels.md#couchesdecartetesttuilehorsbornesdansunecoucherefusee) |
| `EX-LVL-005` | [Cartes & format](../Specification/niveaux.md) | [`LevelLoaderTest.NiveauSansVersionSeChargeSansErreur`](core-levels.md#levelloadertestniveausansversionsechargesanserreur), [`LevelLoaderTest.VersionSuperieureALaVersionGereeEchoueProprement`](core-levels.md#levelloadertestversionsuperieurealaversiongereeechoueproprement), [`CouchesDeCarteTest.CarteDUneVersionFutureRefuseeAvecUnMessageExplicite`](core-levels.md#couchesdecartetestcarteduneversionfuturerefuseeavecunmessageexplicite) |
| `EX-LVL-016` | [Cartes & format](../Specification/niveaux.md) | [`CouchesDeCarteTest.CarteVersion2PromueEnCoucheLegacyUnique`](core-levels.md#couchesdecartetestcarteversion2promueencouchelegacyunique), [`CouchesDeCarteTest.CarteVersion2ReecriteSansTableauDeCouches`](core-levels.md#couchesdecartetestcarteversion2reecritesanstableaudecouches), [`CouchesDeCarteTest.AllerRetourSurTroisCouchesEtDeuxEntites`](core-levels.md#couchesdecartetestallerretoursurtroiscouchesetdeuxentites), [`CouchesDeCarteTest.LaCoucheDeCollisionEstLaGrilleDuGameplay`](core-levels.md#couchesdecartetestlacouchedecollisionestlagrilledugameplay), [`CouchesDeCarteTest.BrouillonDEditionPreserveCouchesEtEntites`](core-levels.md#couchesdecartetestbrouillondeditionpreservecouchesetentites), [`CouchesDeCarteTest.TuilePeinteAtteintLaCoucheDeCollision`](core-levels.md#couchesdecartetesttuilepeinteatteintlacouchedecollision), [`CouchesDeCarteTest.RedimensionnementEmporteCouchesEtEntites`](core-levels.md#couchesdecartetestredimensionnementemportecouchesetentites), [`CouchesDeCarteTest.CoucheDeCollisionDeclareeRefusee`](core-levels.md#couchesdecartetestcouchedecollisiondeclareerefusee) |
| `EX-LVL-017` | [Cartes & format](../Specification/niveaux.md) | [`CouchesDeCarteTest.AllerRetourSurTroisCouchesEtDeuxEntites`](core-levels.md#couchesdecartetestallerretoursurtroiscouchesetdeuxentites), [`CouchesDeCarteTest.EntiteHorsBornesRefusee`](core-levels.md#couchesdecartetestentitehorsbornesrefusee), [`CouchesDeCarteTest.RedimensionnementEmporteCouchesEtEntites`](core-levels.md#couchesdecartetestredimensionnementemportecouchesetentites) |
| `EX-LVL-018` | [Cartes & format](../Specification/niveaux.md) | [`CouchesDeCarteTest.ChampsInconnusDUneCouchePreservesALaReecriture`](core-levels.md#couchesdecartetestchampsinconnusdunecouchepreservesalareecriture), [`CouchesDeCarteTest.ChampsInconnusDUneEntitePreservesALaReecriture`](core-levels.md#couchesdecartetestchampsinconnusduneentitepreservesalareecriture) |
| `EX-LVL-019` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-020` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-021` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-022` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-023` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-024` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-025` | [Cartes & format](../Specification/niveaux.md) | [`LevelDraftPiecesTest.UnEtageNeBloqueAucuneCase`](core-levels.md#leveldraftpiecestestunetagenebloqueaucunecase) |
| `EX-LVL-026` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-027` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-028` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-029` | [Cartes & format](../Specification/niveaux.md) | — |
| `EX-LVL-030` | [Cartes & format](../Specification/niveaux.md) | — |

### `EX-NFR`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-NFR-001` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-002` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | [`DiceTest.RejouabiliteStricte`](core-rpg.md#dicetestrejouabilitestricte) |
| `EX-NFR-003` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-004` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-005` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-010` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-011` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-012` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-013` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-020` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-022` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-023` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-024` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-030` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-031` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-032` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-040` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | [`AnimationClipTest.ClipInexistantRepliDeterministe`](core-ecs.md#animationcliptestclipinexistantreplideterministe), [`InteractionTest.UnTypeInconnuProduitUneEntiteNonInteractive`](core-gameplay.md#interactiontestuntypeinconnuproduituneentitenoninteractive), [`LevelWriterTest.SaveToFileVersDossierInexistantEchoueProprement`](core-levels.md#levelwritertestsavetofileversdossierinexistantechoueproprement), [`CouchesDeCarteTest.CarteDUneVersionFutureRefuseeAvecUnMessageExplicite`](core-levels.md#couchesdecartetestcarteduneversionfuturerefuseeavecunmessageexplicite), [`CouchesDeCarteTest.RoleDeCoucheInconnuRetombeSurLeSol`](core-levels.md#couchesdecartetestroledecoucheinconnuretombesurlesol), [`LevelScan.DossierAbsentNeContientRien`](hmi-game.md#levelscandossierabsentnecontientrien), [`ProceduralAtlasTest.ChaqueTypeDeTuileAUneCouleurDeRepliDistincte`](hmi-graphics.md#proceduralatlastestchaquetypedetuileaunecouleurdereplidistincte) |
| `EX-NFR-041` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |
| `EX-NFR-042` | [Exigences non fonctionnelles](../Specification/exigences-non-fonctionnelles.md) | — |

### `EX-REG`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-REG-001` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-002` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-003` | [Règles d20](../Specification/regles-d20.md) | [`AttackTest.ChaqueJetProduitUneEntreeDeJournalComplete`](core-combat.md#attacktestchaquejetproduituneentreedejournalcomplete), [`CheckTest.ModificateursEtRestitution`](core-rpg.md#checktestmodificateursetrestitution) |
| `EX-REG-010` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-011` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-012` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-020` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-021` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-030` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-031` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-032` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-040` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-041` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-050` | [Règles d20](../Specification/regles-d20.md) | — |
| `EX-REG-051` | [Règles d20](../Specification/regles-d20.md) | — |

### `EX-REN`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-REN-001` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-002` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-003` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-005` | [Rendu & cible technique](../Specification/rendu-technique.md) | [`WorldSceneComposerTest.LaCadenceEstCelleQueDitLaBande`](hmi-graphics.md#worldscenecomposertestlacadenceestcellequeditlabande) |
| `EX-REN-007` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-010` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-011` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-012` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-013` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-014` | [Rendu & cible technique](../Specification/rendu-technique.md) | [`TriParProfondeurTest.LaProfondeurNeDebordePasDeSaBande`](hmi-graphics.md#triparprofondeurtestlaprofondeurnedebordepasdesabande), [`QuadRecorderTest.OrdonnancementDeclare`](hmi-graphics.md#quadrecordertestordonnancementdeclare) |
| `EX-REN-018` | [Rendu & cible technique](../Specification/rendu-technique.md) | [`TriParProfondeurTest.TroisPrimitivesSortentParPiedCroissant`](hmi-graphics.md#triparprofondeurtesttroisprimitivessortentparpiedcroissant), [`TriParProfondeurTest.PersonnageEntreDeuxObjets`](hmi-graphics.md#triparprofondeurtestpersonnageentredeuxobjets), [`TriParProfondeurTest.PiedEgalConserveLOrdreDeComposition`](hmi-graphics.md#triparprofondeurtestpiedegalconservelordredecomposition), [`TriParProfondeurTest.QuantificationAuPixel`](hmi-graphics.md#triparprofondeurtestquantificationaupixel) |
| `EX-REN-019` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-020` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-021` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-022` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-023` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-030` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-031` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-032` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-033` | [Rendu & cible technique](../Specification/rendu-technique.md) | [`LocalizationTest.LesDeuxCataloguesDeclarentLesMemesCles`](hmi-localization.md#localizationtestlesdeuxcataloguesdeclarentlesmemescles) |
| `EX-REN-041` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-042` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-043` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-047` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-048` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |
| `EX-REN-050` | [Rendu & cible technique](../Specification/rendu-technique.md) | — |

### `EX-RPG`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-RPG-001` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-002` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-010` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-011` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-020` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-021` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-022` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-023` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-030` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-031` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-032` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-040` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-041` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-042` | [Personnage et progression](../Specification/rpg.md) | [`DialogueTest.UnDialogueEstRefuseFauteDeLangueCommune`](core-rpg.md#dialoguetestundialogueestrefusefautedelanguecommune) |
| `EX-RPG-050` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-051` | [Personnage et progression](../Specification/rpg.md) | — |
| `EX-RPG-052` | [Personnage et progression](../Specification/rpg.md) | — |

### `EX-VIS`

| Exigence | Spécification | Cas de test |
|---|---|---|
| `EX-VIS-001` | [Vision & périmètre](../Specification/vision.md) | — |
| `EX-VIS-002` | [Vision & périmètre](../Specification/vision.md) | — |
| `EX-VIS-003` | [Vision & périmètre](../Specification/vision.md) | — |
| `EX-VIS-004` | [Vision & périmètre](../Specification/vision.md) | — |
| `EX-VIS-005` | [Vision & périmètre](../Specification/vision.md) | — |
| `EX-VIS-006` | [Vision & périmètre](../Specification/vision.md) | — |
| `EX-VIS-007` | [Vision & périmètre](../Specification/vision.md) | — |
| `EX-VIS-008` | [Vision & périmètre](../Specification/vision.md) | — |
| `EX-VIS-009` | [Vision & périmètre](../Specification/vision.md) | — |
