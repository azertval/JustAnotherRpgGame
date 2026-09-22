# HMI · Graphics

Tests unitaires — **156 cas** (18 bloquants, 57 critiques, 74 majeurs, 7 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_animation_catalog.cpp`](#test-animation-catalogcpp) | 12 | - | 5 | 7 | - |
| [`test_arena_animation_driver.cpp`](#test-arena-animation-drivercpp) | 8 | - | 3 | 4 | 1 |
| [`test_arena_appearance_catalog.cpp`](#test-arena-appearance-catalogcpp) | 13 | 4 | 2 | 6 | 1 |
| [`test_arena_scene_composer.cpp`](#test-arena-scene-composercpp) | 11 | 5 | - | 6 | - |
| [`test_arena_scene_renderer.cpp`](#test-arena-scene-renderercpp) | 6 | 2 | 3 | - | 1 |
| [`test_asset_gallery.cpp`](#test-asset-gallerycpp) | 7 | 3 | - | 4 | - |
| [`test_asset_gallery_renderer.cpp`](#test-asset-gallery-renderercpp) | 2 | 1 | - | 1 | - |
| [`test_cache_registry.cpp`](#test-cache-registrycpp) | 5 | - | 5 | - | - |
| [`test_camera2d.cpp`](#test-camera2dcpp) | 10 | - | - | 9 | 1 |
| [`test_city_block_render.cpp`](#test-city-block-rendercpp) | 2 | - | - | 2 | - |
| [`test_depth_sort.cpp`](#test-depth-sortcpp) | 5 | - | 4 | 1 | - |
| [`test_entity_markers.cpp`](#test-entity-markerscpp) | 5 | - | 1 | 4 | - |
| [`test_image_encode.cpp`](#test-image-encodecpp) | 5 | - | 1 | 4 | - |
| [`test_maquette_tokens.cpp`](#test-maquette-tokenscpp) | 6 | - | 2 | 4 | - |
| [`test_missing_texture.cpp`](#test-missing-texturecpp) | 5 | - | 2 | 3 | - |
| [`test_poly_quad.cpp`](#test-poly-quadcpp) | 4 | - | 2 | 2 | - |
| [`test_procedural_atlas.cpp`](#test-procedural-atlascpp) | 4 | - | 1 | 2 | 1 |
| [`test_quad_recorder.cpp`](#test-quad-recordercpp) | 7 | - | 3 | 3 | 1 |
| [`test_render_culling.cpp`](#test-render-cullingcpp) | 10 | - | 5 | 4 | 1 |
| [`test_rhi_offscreen.cpp`](#test-rhi-offscreencpp) | 2 | - | 1 | 1 | - |
| [`test_texture_atlas.cpp`](#test-texture-atlascpp) | 1 | - | 1 | - | - |
| [`test_world_scene_composer.cpp`](#test-world-scene-composercpp) | 20 | - | 13 | 7 | - |
| [`test_world_scene_renderer.cpp`](#test-world-scene-renderercpp) | 6 | 3 | 3 | - | - |

## test_animation_catalog.cpp

### AnimationCatalogTest.RoundTripClipsMultiplesDureeParDefautEtOneShot

*Critique · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:31`

Un descripteur valide se relit intégralement, durée par défaut et clip suivant compris.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `description.frameWidth` vaut `16`.
- Vérifie que `description.frameHeight` vaut `16`.
- Vérifie que `description.clips.clipCount()` vaut `3`.
- Vérifie que `closed.frames` vaut `(std::vector<int>{0})`.
- Vérifie que `closed.endMode` vaut `core::ClipEndMode::Loop`.
- Vérifie que `closed.frameDuration` vaut `hmi::AnimationCatalog::DEFAULT_FRAME_DURATION_SECONDS` (comparaison flottante).
- Vérifie que `opening.frames` vaut `(std::vector<int>{1, 2, 3, 4})`.
- Vérifie que `opening.frameDuration` vaut `0.06f` (comparaison flottante).
- Vérifie que `opening.endMode` vaut `core::ClipEndMode::OneShot`.
- Vérifie que `opening.nextClip` vaut `"open"`.
- Vérifie que `description.clips.indexOf(opening.nextClip)` est supérieur ou égal à `0`.

### AnimationCatalogTest.JsonInvalideEstUneErreurExploitable

*Majeur · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:69`

Un JSON invalide donne une erreur d'analyse avec un message exploitable.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `hmi::AnimationCatalogError::ParseError`.
- Vérifie que `result.error.empty()` est faux.

### AnimationCatalogTest.VersionInconnueEstRefusee

*Majeur · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:87`

Une version de format inconnue est refusée.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `hmi::AnimationCatalogError::UnsupportedVersion`.

### AnimationCatalogTest.ClipSuivantInexistantEstRefuse

*Critique · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:108`

Un clip suivant inexistant est refusé au chargement.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `hmi::AnimationCatalogError::MalformedStructure`.

### AnimationCatalogTest.IndiceDImageNegatifEstRefuse

*Majeur · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:130`

Un indice d'image négatif est refusé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `hmi::AnimationCatalogError::MalformedStructure`.

### AnimationCatalogTest.ClipSansFramesEstRefuse

*Majeur · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:151`

Un clip sans images est refusé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `hmi::AnimationCatalogError::MalformedStructure`.

### AnimationCatalogTest.FichierAbsentEstFileNotFoundSansException

*Critique · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:172`

Un fichier absent donne un code dédié, sans exception.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `hmi::AnimationCatalogError::FileNotFound`.

### AnimationCatalogTest.DescriptorFileNameRemplaceLExtension

*Critique · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:190`

Le nom du descripteur remplace l'extension en conservant le chemin.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `hmi::AnimationCatalog::descriptorFileName("water.png")` vaut `"water.anim.json"`.
- Vérifie que `hmi::AnimationCatalog::descriptorFileName("Npc/anariel/attack.png")` vaut `"Npc/anariel/attack.anim.json"`.

### AnimationCatalogTest.CoherenceAvecLePngValideeSurSpritesheetAUnRang

*Majeur · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:206`

Un descripteur cohérent avec les dimensions du PNG est validé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `valid.valid` est vrai.

### AnimationCatalogTest.TailleDImageIncoherenteAvecLePngEstRefusee

*Majeur · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:228`

Des dimensions de PNG incohérentes avec l'image sont refusées.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `wrongHeight.valid` est faux.
- Vérifie que `wrongHeight.message.empty()` est faux.
- Vérifie que `wrongWidth.valid` est faux.

### AnimationCatalogTest.IndiceDImageHorsBornesDeLaSpritesheetReelleEstRefuse

*Critique · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:256`

Un indice d'image hors des bornes réelles de la spritesheet est refusé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.ok()` est vrai.
- Vérifie que `tooShort.valid` est faux.
- Vérifie que `tooShort.message.empty()` est faux.

### AnimationCatalogTest.FrameRegionPremiereEtDerniereImageSpritesheetAUnRang

*Majeur · Unitaire · Catalogue d'animations* — `Source/Test/Unit/HMI/Graphics/test_animation_catalog.cpp:280`

La région d'une image se déduit de son indice par décalage horizontal, ordonnée constante.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `first.x` vaut `0`.
- Vérifie que `first.y` vaut `0`.
- Vérifie que `first.width` vaut `16`.
- Vérifie que `first.height` vaut `16`.
- Vérifie que `last.x` vaut `5 * 16`.
- Vérifie que `last.y` vaut `0`.
- Vérifie que `last.width` vaut `16`.
- Vérifie que `last.height` vaut `16`.

## test_arena_animation_driver.cpp

### ArenaAnimationDriverTest.CombattantNonSuiviResteIdle

*Majeur · Unitaire · Pilote d'animation de l'arène* — `Source/Test/Unit/HMI/Graphics/test_arena_animation_driver.cpp:77`

Un combattant non déclenché ne bouge pas.

**Étapes**

1. Avancer un pilote vide de dix secondes.

**Résultat attendu**

- Vérifie que `driver.snapshot().figures.empty()` est vrai.
- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Idle`.

### ArenaAnimationDriverTest.IdleBoucle

*Majeur · Unitaire · Pilote d'animation de l'arène* — `Source/Test/Unit/HMI/Graphics/test_arena_animation_driver.cpp:93`

La boucle idle ne s'arrête jamais.

**Étapes**

1. Jouer Idle. 2. Avancer de deux durées d'image. 3. Avancer de deux de plus.

**Résultat attendu**

- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `0`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `2`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `0`.
- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Idle`.

### ArenaAnimationDriverTest.TransitionIdleAttackIdle

*Critique · Unitaire · Pilote d'animation de l'arène* — `Source/Test/Unit/HMI/Graphics/test_arena_animation_driver.cpp:115`

Transition idle -> attack -> idle.

**Étapes**

1. Jouer Idle. 2. Jouer Attack. 3. Avancer jusqu'à la fin de la bande d'attaque.

**Résultat attendu**

- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Attack`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `0`.
- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Attack`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `3`.
- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Idle`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `0`.

### ArenaAnimationDriverTest.HitRelanceCoupSurCoup

*Majeur · Unitaire · Pilote d'animation de l'arène* — `Source/Test/Unit/HMI/Graphics/test_arena_animation_driver.cpp:143`

Hit relance toujours, même coup sur coup.

**Étapes**

1. Jouer Hit, avancer d'une image. 2. Rejouer Hit.

**Résultat attendu**

- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `1`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `0`.
- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Hit`.

### ArenaAnimationDriverTest.MortResteFigee

*Critique · Unitaire · Pilote d'animation de l'arène* — `Source/Test/Unit/HMI/Graphics/test_arena_animation_driver.cpp:163`

Franchissement de l'état mort.

**Étapes**

1. Jouer Death, avancer jusqu'à la derniere image. 2. Rejouer Idle puis Attack.

**Résultat attendu**

- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Death`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `3`.
- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Death`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `3`.
- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Death`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `3`.

### ArenaAnimationDriverTest.ActionAbsenteReplieSurIdle

*Majeur · Unitaire · Pilote d'animation de l'arène* — `Source/Test/Unit/HMI/Graphics/test_arena_animation_driver.cpp:190`

Repli sur Idle quand l'action n'existe pas (ex. un ennemi sans attack.png).

**Étapes**

1. Déclarer une figurine avec seulement `idle`. 2. Lui jouer Attack.

**Résultat attendu**

- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Idle`.
- Vérifie que `driver.snapshot().frameOf(HERO)` vaut `0`.

### ArenaAnimationDriverTest.RemoveRetireDuPilotage

*Mineur · Unitaire · Pilote d'animation de l'arène* — `Source/Test/Unit/HMI/Graphics/test_arena_animation_driver.cpp:210`

Un combattant sorti de la grille quitte le pilotage.

**Étapes**

1. Jouer Idle. 2. Retirer le combattant.

**Résultat attendu**

- Vérifie que `driver.snapshot().figures.empty()` est faux.
- Vérifie que `driver.snapshot().figures.empty()` est vrai.
- Vérifie que `driver.actionOf(HERO)` vaut `ArenaFigureAction::Idle`.

### ArenaAnimationDriverTest.PlanchesDUnKitValides

*Critique · Unitaire · Pilote d'animation de l'arène* — `Source/Test/Unit/HMI/Graphics/test_arena_animation_driver.cpp:229`

Les `.anim.json` d'un kit d'arène sont valides.

**Étapes**

1. Lire un héros (cinq actions). 2. Lire un gladiateur au repos seul.

**Résultat attendu**

- Vérifie que `std::filesystem::exists(characters)` est vrai.
- Vérifie que `hero.errors.empty()` est vrai.
- Vérifie que `hero.clips.idle` diffère de `nullptr`.
- Vérifie que `hero.clips.walk` diffère de `nullptr`.
- Vérifie que `hero.clips.attack` diffère de `nullptr`.
- Vérifie que `hero.clips.hit` diffère de `nullptr`.
- Vérifie que `hero.clips.death` diffère de `nullptr`.
- Vérifie que `std::filesystem::exists(enemy)` est vrai.
- Vérifie que `tireur.errors.empty()` est vrai.
- Vérifie que `tireur.clips.idle` diffère de `nullptr`.
- Vérifie que `tireur.clips.attack` vaut `nullptr`.
- Vérifie que `tireur.clips.hit` vaut `nullptr`.
- Vérifie que `tireur.clips.death` vaut `nullptr`.

## test_arena_appearance_catalog.cpp

### ArenaAppearanceCatalogTest.LectureDeReference

*Bloquant · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:45`

Le catalogue de reference se lit sans erreur.

**Étapes**

1. Lire REFERENCE_JSON.

**Résultat attendu**

- Vérifie que `catalog.heroes().size()` vaut `4U`.
- Vérifie que `catalog.gladiators().size()` vaut `4U`.
- Vérifie que `catalog.paleSlabs().size()` vaut `10U`.
- Vérifie que `catalog.heroFrames()` vaut `5`.
- Vérifie que `catalog.enemyFrames()` vaut `8`.

### ArenaAppearanceCatalogTest.ChampsInvalidesRefuses

*Majeur · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:63`

Un catalogue dont un champ obligatoire manque, est vide ou mal type est refuse.

**Étapes**

1. Lire des variantes du JSON de reference, chacune avec un defaut.

**Résultat attendu**

- Vérifie que `result.ok()` est faux.
- Vérifie que `result.errorCode` vaut `hmi::ArenaAppearanceError::MalformedStructure`.

### ArenaAppearanceCatalogTest.LesAnglesPortentUneColonne

*Bloquant · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:91`

Sur une grille 10 x 8 murée, les quatre angles portent `WallFeature::Corner`.

**Étapes**

1. Lire le role des quatre coins, wall = vrai.

**Résultat attendu**

- Vérifie que `role.wall` est vrai.
- Vérifie que `role.wallFeature` vaut `hmi::WallFeature::Corner`.
- Vérifie que `role.gateSpot` est faux.

### ArenaAppearanceCatalogTest.LaBanniereTousLesCinqPas

*Majeur · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:115`

Sur une grille 10 x 8, la banniere est aux colonnes multiples de 5 des bords haut et bas, hors angle ; les autres cases de ces bords sont un pan de mur ordinaire.

**Étapes**

1. Lire le role de chaque case murée des lignes 0 et 7.

**Résultat attendu**

- Vérifie que `role.wallFeature` vaut `attendu`.

### ArenaAppearanceCatalogTest.LaTorcheTousLesQuatrePas

*Majeur · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:140`

Sur une grille 10 x 8, la torche est aux lignes valant 2 modulo 4 des bords gauche et droit, hors angle ; les autres cases de ces bords sont un pan de mur ordinaire.

**Étapes**

1. Lire le role de chaque case muree des colonnes 0 et 9.

**Résultat attendu**

- Vérifie que `role.wallFeature` vaut `attendu`.

### ArenaAppearanceCatalogTest.LaPorteEstSurLeBordSeulement

*Bloquant · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:165`

Sur une grille 10 x 8, toute case non muree du bord porte `gateSpot`, aucune case interieure ne le porte.

**Étapes**

1. Lire le role de la case (0, 3) et de la case (4, 4), wall = faux.

**Résultat attendu**

- Vérifie que `bord.gateSpot` est vrai.
- Vérifie que `bord.wall` est faux.
- Vérifie que `interieur.gateSpot` est faux.

### ArenaAppearanceCatalogTest.LaDalleSuitLaFormuleQml

*Critique · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:188`

Le sol d'une case non muree porte une dalle quand `(colonne*3 + ligne*5 + colonne*ligne) % 7 == 0`, a l'indice `(colonne*3 + ligne*5) % 10`.

**Étapes**

1. Calculer le role de chaque case interieure d'une grille 10 x 8.
2. Comparer a la formule transcrite de la scene QML.

**Résultat attendu**

- Vérifie que `role.slab` vaut `slabAttendu`.
- Vérifie que `role.slabVariant` vaut `(column * 3 + row * 5) % 10`.

### ArenaAppearanceCatalogTest.LaFigurineEstDeterministeEtDependDuCote

*Bloquant · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:218`

Deux appels avec le meme nom et le meme cote rendent la meme figurine ; un allie choisit parmi les heros, un ennemi parmi les gladiateurs.

**Étapes**

1. Demander la figurine de « Gorlak » comme allie, puis comme ennemi, deux fois chacune.

**Résultat attendu**

- Vérifie que `allie1.sheet` vaut `allie2.sheet`.
- Vérifie que `std::find(catalog.heroes().begin(), catalog.heroes().end(), allie1.sheet)` diffère de `catalog.heroes().end()`.
- Vérifie que `allie1.frameCount` vaut `catalog.heroFrames()`.
- Vérifie que `ennemi1.sheet` vaut `ennemi2.sheet`.
- Vérifie que `std::find(catalog.gladiators().begin(), catalog.gladiators().end(), ennemi1.sheet)` diffère de `catalog.gladiators().end()`.
- Vérifie que `ennemi1.frameCount` vaut `catalog.enemyFrames()`.

### ArenaAppearanceCatalogTest.LaFigurineSuitLaFormuleQml

*Majeur · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:248`

Pour plusieurs noms, l'indice de figurine vaut `(longueur*7 + code du premier caractere) % taille du roster`.

**Étapes**

1. Calculer la figurine attendue pour plusieurs noms.
2. Comparer a la formule transcrite de la scene QML.

**Résultat attendu**

- Vérifie que `catalog.figureFor(nom, core::CombatSide::Allies).sheet` vaut `catalog.heroes()[static_cast<std::size_t>(indiceAttendu)]`.

### ArenaAppearanceCatalogTest.NomVideRendLaPremiereFigurine

*Mineur · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:272`

Un combattant sans nom rend la figurine d'indice 0.

**Étapes**

1. Demander la figurine d'un nom vide, des deux cotes.

**Résultat attendu**

- Vérifie que `catalog.figureFor("", core::CombatSide::Allies).sheet` vaut `catalog.heroes().front()`.
- Vérifie que `catalog.figureFor("", core::CombatSide::Enemies).sheet` vaut `catalog.gladiators().front()`.

### ArenaAppearanceCatalogTest.ManifesteDUnKitValide

*Critique · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:287`

Le manifeste du kit d'arene d'essai se lit sans erreur.

**Étapes**

1. Lire le manifeste livre depuis les sources.

**Résultat attendu**

- Vérifie que `std::filesystem::exists(path)` est vrai.
- Vérifie que `result.ok()` est vrai.
- Vérifie que `result.catalog->heroes().empty()` est faux.
- Vérifie que `result.catalog->gladiators().empty()` est faux.
- Vérifie que `result.catalog->paleSlabs().empty()` est faux.
- Vérifie que `result.catalog->heroFrames()` est strictement supérieur à `0`.
- Vérifie que `result.catalog->enemyFrames()` est strictement supérieur à `0`.

### ArenaAppearanceCatalogTest.UnHerosRemplaceLitSesBandesAilleurs

*Majeur · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:312`

`replaceHero` change le dossier rendu par `figureFor`/`sheetDirectory`, rien d'autre.

**Étapes**

1. Lire le dossier par defaut d'un heros et d'un gladiateur.
2. Remplacer ce heros par `../Npc/anariel`, puis tenter un nom inconnu.

**Résultat attendu**

- Vérifie que `avant.directory` vaut `"characters/" + avant.sheet`.
- Vérifie que `catalog.figureFor("Gorlak", core::CombatSide::Enemies).directory` vaut `"enemies/" + catalog.figureFor("Gorlak", core::CombatSide::Enemies).sheet`.
- Vérifie que `catalog.replaceHero(avant.sheet, "../Npc/anariel")` est vrai.
- Vérifie que `apres.sheet` vaut `avant.sheet`.
- Vérifie que `apres.frameCount` vaut `avant.frameCount`.
- Vérifie que `apres.directory` vaut `"../Npc/anariel"`.
- Vérifie que `catalog.sheetDirectory(avant.sheet, core::CombatSide::Allies)` vaut `"../Npc/anariel"`.
- Vérifie que `catalog.heroes()` vaut `referenceCatalog().heroes()`.
- Vérifie que `catalog.replaceHero("inconnu", "../Npc/inconnu")` est faux.
- Vérifie que `catalog.sheetDirectory("inconnu", core::CombatSide::Allies)` vaut `"characters/inconnu"`.

### ArenaAppearanceCatalogTest.LeManifesteDesPnjRemplaceLesHerosNommes

*Majeur · Unitaire · Catalogue d'apparence de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_appearance_catalog.cpp:344`

`applyNpcManifest` : un heros remplace par entree valide, une entree inconnue ou sans slug ignoree, un fichier absent sans effet.

**Étapes**

1. Appliquer un chemin inexistant.
2. Ecrire un manifeste avec `kaelith_voss` -> `anariel`, un heros inconnu et un slug vide ; l'appliquer.

**Résultat attendu**

- Vérifie que `catalog.applyNpcManifest(manifest)` vaut `0`.
- Vérifie que `catalog.sheetDirectory("kaelith_voss", core::CombatSide::Allies)` vaut `"characters/kaelith_voss"`.
- Vérifie que `catalog.applyNpcManifest(manifest)` vaut `1`.
- Vérifie que `catalog.sheetDirectory("kaelith_voss", core::CombatSide::Allies)` vaut `"../Npc/anariel"`.
- Vérifie que `catalog.sheetDirectory("bram", core::CombatSide::Allies)` vaut `"characters/bram"`.

## test_arena_scene_composer.cpp

### ArenaSceneComposerTest.NombreDeQuadsParCalque

*Bloquant · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:207`

La scene composee compte exactement les quads attendus, par calque.

**Étapes**

1. Monter cinq combattants sur la piste 5x4.
2. Composer la scene.

**Résultat attendu**

- Vérifie que `onLayer(scene, RenderLayer::Tile).size()` vaut `static_cast<std::size_t>(FLOOR_QUADS)`.
- Vérifie que `onLayer(scene, RenderLayer::Object).size()` vaut `static_cast<std::size_t>(STRUCTURE_QUADS)`.
- Vérifie que `onLayer(scene, RenderLayer::Player).size()` vaut `5U`.
- Vérifie que `scene.size()` vaut `static_cast<std::size_t>(FLOOR_QUADS + STRUCTURE_QUADS + 5)`.

### ArenaSceneComposerTest.OrdreDesCalques

*Bloquant · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:227`

Le sol est dessine sous tout ; la bande de profondeur est triee par pied.

**Étapes**

1. Composer la scene (triee).

**Résultat attendu**

- Vérifie que `quads.size()` vaut `static_cast<std::size_t>(FLOOR_QUADS + STRUCTURE_QUADS + 5)`.
- Vérifie que `quads[index].layer` vaut `RenderLayer::Tile`.
- Vérifie que `hmi::sortsByDepth(quads[index].layer)` est vrai.
- Vérifie que `quads[index - 1].sortOrder` est inférieur ou égal à `quads[index].sortOrder`.

### ArenaSceneComposerTest.ProfondeurAuPiedDeLaCase

*Majeur · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:253`

La profondeur suit le pied de la case.

**Étapes**

1. Composer la scene.
2. Reperer la figurine de Bram (1, 1), le pan (1, 0), le pan (1, 3) et le pan a torche (4, 2).

**Résultat attendu**

- Vérifie que `found` diffère de `quads.end()`.
- Vérifie que `bram` diffère de `nullptr`.
- Vérifie que `back` diffère de `nullptr`.
- Vérifie que `front` diffère de `nullptr`.
- Vérifie que `torch` diffère de `nullptr`.
- Vérifie que `torch->texture` vaut `sceneTextures.resolve(scenePiece("torch-left")).texture`.
- Vérifie que `indexOf(back)` est strictement inférieur à `indexOf(bram)`.
- Vérifie que `indexOf(bram)` est strictement inférieur à `indexOf(front)`.

### ArenaSceneComposerTest.LeDecorSePoseParSonAncreDansLeBonSens

*Majeur · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:307`

Le decor de l'atelier des textures (LOT-92) est pose par son ancre, dans le bon sens.

**Étapes**

1. Composer la piste 5x4.
2. Relever la piece et le sol de (1, 0), (0, 1), (0, 0), (4, 3), (0, 2).

**Résultat attendu**

- Vérifie que `found` diffère de `composed.quads().end()`.
- Vérifie que `is(back, "wall-right")` est vrai.
- Vérifie que `is(side, "wall-left")` est vrai.
- Vérifie que `side` diffère de `nullptr`.
- Vérifie que `side->sprite.x` vaut `top.x - 34.0f * unitsPerArtPixel`, à `1e-3f` près.
- Vérifie que `side->sprite.y` vaut `top.y - (STANDING_HEIGHT - 42) * unitsPerArtPixel`, à `1e-3f` près.
- Vérifie que `side->sprite.width` vaut `68.0f * unitsPerArtPixel`, à `1e-3f` près.
- Vérifie que `is(pieceAt({.column = 0, .row = 0}, RenderLayer::Object), "wall-corner")` est vrai.
- Vérifie que `is(pieceAt({.column = 4, .row = 3}, RenderLayer::Object), "pillar")` est vrai.
- Vérifie que `is(pieceAt({.column = 0, .row = 2}, RenderLayer::Object), "arch-left")` est vrai.
- Vérifie que `is(pieceAt({.column = 0, .row = 2}, RenderLayer::Tile), "gate-threshold")` est vrai.

### ArenaSceneComposerTest.ATerreEtSorti

*Bloquant · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:359`

Down compose une figurine, Withdrawn aucune.

**Étapes**

1. Demarrer le combat.
2. Abattre Bram (allie) et Orc (ennemi), faire sortir Cid.
3. Composer la scene.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `combat.withdraw(idOf(session, "Cid"))` vaut `core::WithdrawResult::Withdrawn`.
- Vérifie que `combat.find(idOf(session, "Bram"))->status` vaut `core::CombatantStatus::Down`.
- Vérifie que `combat.find(idOf(session, "Orc"))->status` vaut `core::CombatantStatus::Down`.
- Vérifie que `combat.find(idOf(session, "Cid"))->status` vaut `core::CombatantStatus::Withdrawn`.
- Vérifie que `combat.phase()` diffère de `core::CombatPhase::Ended`.
- Vérifie que `onLayer(scene, RenderLayer::Player).size()` vaut `4U`.
- Vérifie que `bram` diffère de `nullptr`.
- Vérifie que `bram->sprite.u0` vaut `4.0f * 48.0f / 240.0f` (comparaison flottante).
- Vérifie que `bram->sprite.u1` vaut `1.0f` (comparaison flottante).
- Vérifie que `bram->sprite.a` vaut `1.0f` (comparaison flottante).
- Vérifie que `orc` diffère de `nullptr`.
- Vérifie que `orc->sprite.u0` vaut `7.0f * 48.0f / 384.0f` (comparaison flottante).
- Vérifie que `orc->sprite.a` vaut `hmi::ARENA_DOWN_ENEMY_ALPHA` (comparaison flottante).
- Vérifie que `cid` diffère de `eve`.
- Vérifie que `withTexture(scene, cid)` vaut `nullptr`.

### ArenaSceneComposerTest.ImageCouranteBornee

*Majeur · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:406`

L'image courante vient de ArenaAnimationState, bornee a la bande.

**Étapes**

1. Donner l'image 3 a Eve et l'image 99 a Rat.
2. Composer la scene.

**Résultat attendu**

- Vérifie que `eve` diffère de `nullptr`.
- Vérifie que `eve->sprite.u0` vaut `144.0f / 240.0f` (comparaison flottante).
- Vérifie que `eve->sprite.u1` vaut `192.0f / 240.0f` (comparaison flottante).
- Vérifie que `eve->sprite.v1` vaut `1.0f` (comparaison flottante).
- Vérifie que `rat` diffère de `nullptr`.
- Vérifie que `rat->sprite.u0` vaut `7.0f * 48.0f / 384.0f` (comparaison flottante).
- Vérifie que `rat->sprite.a` vaut `1.0f` (comparaison flottante).

### ArenaSceneComposerTest.RepliSurLeDamier

*Majeur · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:436`

Repli sur le damier, puis rien.

**Étapes**

1. Composer sans aucune texture chargee, damier fourni.
2. Composer sans aucune texture ni damier.

**Résultat attendu**

- Vérifie que `fallback.size()` vaut `static_cast<std::size_t>(FLOOR_QUADS + STRUCTURE_QUADS + 5)`.
- Vérifie que `std::all_of(fallback.quads().begin(), fallback.quads().end(), [&](const hmi::ComposedQuad& quad) { return quad.texture == onlyMissing.missing.texture; })` est vrai.
- Vérifie que `empty.size()` vaut `0U`.

### ArenaSceneComposerTest.LectureSeule

*Bloquant · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:462`

La composition est une lecture seule (EX-ARCH-012).

**Étapes**

1. Demarrer le combat, relever phase, tour actif, journal et positions.
2. Composer deux fois.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().phase()` vaut `phase`.
- Vérifie que `session.combat().activeCombatant()` vaut `active`.
- Vérifie que `session.journal().size()` vaut `journal`.
- Vérifie que `after` vaut `positions`.
- Vérifie que `first.size()` vaut `second.size()`.
- Vérifie que `a.layer` vaut `b.layer`.
- Vérifie que `a.texture` vaut `b.texture`.
- Vérifie que `a.sortOrder` vaut `b.sortOrder`.
- Vérifie que `a.sprite.x` vaut `b.sprite.x` (comparaison flottante).
- Vérifie que `a.sprite.y` vaut `b.sprite.y` (comparaison flottante).

### ArenaSceneComposerTest.InstantaneSurvitALaSession

*Bloquant · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:507`

L'instantane en valeurs survit a sa session (LOT-86 Phase 5).

**Étapes**

1. Abattre Bram, faire sortir Cid, tirer l'instantane.
2. Detruire une copie de la session, composer l'instantane.

**Résultat attendu**

- Vérifie que `session.start()` est vrai.
- Vérifie que `session.combat().withdraw(idOf(session, "Cid"))` vaut `core::WithdrawResult::Withdrawn`.
- Vérifie que `ephemeral->mount(affrontement()).refusals.empty()` est vrai.
- Vérifie que `ephemeral->start()` est vrai.
- Vérifie que `ephemeral->combat().withdraw(idOf(*ephemeral, "Cid"))` vaut `core::WithdrawResult::Withdrawn`.
- Vérifie que `snapshot` vaut `hmi::snapshotArenaScene(session)`.
- Vérifie que `snapshot.columns` vaut `5`.
- Vérifie que `snapshot.rows` vaut `4`.
- Vérifie que `std::count(snapshot.obstructed.begin(), snapshot.obstructed.end(), true)` vaut `13`.
- Vérifie que `snapshot.figures.size()` vaut `4U`.
- Vérifie que `std::none_of(snapshot.figures.begin(), snapshot.figures.end(), [](const hmi::ArenaFigureSnapshot& f) { return f.name == "Cid"; })` est vrai.
- Vérifie que `bram` diffère de `snapshot.figures.end()`.
- Vérifie que `bram->down` est vrai.
- Vérifie que `bram->anchor` vaut `(core::GridPosition{.column = 1, .row = 1})`.
- Vérifie que `fromSnapshot.size()` vaut `fromSession.size()`.
- Vérifie que `a.texture` vaut `b.texture`.
- Vérifie que `a.sortOrder` vaut `b.sortOrder`.
- Vérifie que `a.sprite.x` vaut `b.sprite.x` (comparaison flottante).
- Vérifie que `a.sprite.u0` vaut `b.sprite.u0` (comparaison flottante).
- Vérifie que `a.sprite.a` vaut `b.sprite.a` (comparaison flottante).

### ArenaSceneComposerTest.ListeDesTexturesCouvreLaComposition

*Majeur · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:566`

La liste des textures couvre tout ce que la composition demande.

**Étapes**

1. Lier chaque chemin de arenaTexturePaths a une texture, et le damier a une autre.
2. Composer la piste avec un allie a terre.

**Résultat attendu**

- Vérifie que `std::adjacent_find(sorted.begin(), sorted.end())` vaut `sorted.end()`.
- Vérifie que `session.start()` est vrai.
- Vérifie que `scene.size()` vaut `static_cast<std::size_t>(FLOOR_QUADS + STRUCTURE_QUADS + 5)`.
- Vérifie que `std::none_of( scene.quads().begin(), scene.quads().end(), [&](const hmi::ComposedQuad& quad) { return quad.texture == listed.missing.texture; })` est vrai.

### ArenaSceneComposerTest.UneFigurineDeRemplacementSuitSaPropreDecoupe

*Majeur · Unitaire · Composeur de la scene de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_composer.cpp:601`

Un heros remplace se dessine depuis `../Npc/<slug>` ; une bande large (96 px) donne un quad deux fois plus large, centre au meme endroit, et sa derniere image a terre.

**Étapes**

1. Remplacer la figurine de Bram par `../Npc/anariel`, avec `idle.png` 288x64 (frameWidth 48) et `death.png` 576x64 (frameWidth 96).
2. Composer debout a l'image 5, puis a terre.

**Résultat attendu**

- Vérifie que `appearance.replaceHero(sheet, "../Npc/anariel")` est vrai.
- Vérifie que `session.start()` est vrai.
- Vérifie que `repos` diffère de `nullptr`.
- Vérifie que `repos->sprite.u0` vaut `5.0f * 48.0f / 288.0f` (comparaison flottante).
- Vérifie que `repos->sprite.u1` vaut `1.0f` (comparaison flottante).
- Vérifie que `session.combat().find(bram)->status` vaut `core::CombatantStatus::Down`.
- Vérifie que `mort` diffère de `nullptr`.
- Vérifie que `mort->sprite.u0` vaut `5.0f * 96.0f / 576.0f` (comparaison flottante).
- Vérifie que `mort->sprite.u1` vaut `1.0f` (comparaison flottante).
- Vérifie que `mort->sprite.width` vaut `repos->sprite.width * 2.0f` (comparaison flottante).
- Vérifie que `mort->sprite.height` vaut `repos->sprite.height` (comparaison flottante).
- Vérifie que `mort->sprite.x + mort->sprite.width / 2.0f` vaut `centreDebout`, à `1e-3f` près.

## test_arena_scene_renderer.cpp

### ArenaSceneRendererTest.CreationLiberationRecreation

*Bloquant · Unitaire · Rendu QRhi de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_renderer.cpp:160`

Le cycle de vie des ressources QRhi de l'arene est sur.

**Étapes**

1. Creer les ressources sur une interface QRhi hors ecran, sans jamais dessiner.
2. Liberer, puis liberer encore.
3. Recreer, dessiner une image, detruire le rendu avant l'interface.

**Résultat attendu**

- Vérifie que `renderer.catalog().heroes().empty()` est faux.
- Vérifie que `renderer.ensureResources(nullptr)` est faux.
- Vérifie que `renderer.created()` est faux.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `renderer.created()` est vrai.
- Vérifie que `renderer.rhi()` vaut `rhi.get()`.
- Vérifie que `renderer.textures().byPath.size()` vaut `hmi::arenaTexturePaths(renderer.catalog()).size()`.
- Vérifie que `renderer.textures().missing.texture` diffère de `nullptr`.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `renderer.textures().resolve("../Scene/bourg/sand.png").texture` vaut `sand`.
- Vérifie que `renderer.created()` est faux.
- Vérifie que `renderer.rhi()` vaut `nullptr`.
- Vérifie que `renderer.textures().byPath.empty()` est vrai.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `image.size()` vaut `QSize(TARGET_SIZE, TARGET_SIZE)`.

### ArenaSceneRendererTest.SceneNonVideSurUneGrilleDeTest

*Bloquant · Unitaire · Rendu QRhi de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_renderer.cpp:213`

La scene de l'arene devient des pixels.

**Étapes**

1. Tirer l'instantane de la piste 5x4 (quatre combattants), puis detruire la session.
2. Dessiner une image hors ecran et la relire.

**Résultat attendu**

- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `renderer.animating()` est vrai.
- Vérifie que `image.size()` vaut `QSize(TARGET_SIZE, TARGET_SIZE)`.
- Vérifie que `renderer.composed().size()` vaut `PISTE_QUADS`.
- Vérifie que `quad.texture` diffère de `nullptr`.
- Vérifie que `quad.texture` diffère de `renderer.textures().missing.texture`.
- Vérifie que `painted` est strictement supérieur à `static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE / 5)`.
- Vérifie que `painted` est strictement inférieur à `static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE)`.
- Vérifie que `renderer.animating()` est faux.
- Vérifie que `renderer.composed().size()` vaut `0U`.
- Vérifie que `paintedPixels(empty)` vaut `0U`.

### ArenaSceneRendererTest.RecreationSurUneAutreInterface

*Critique · Unitaire · Rendu QRhi de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_renderer.cpp:260`

Le rendu de l'arene se recree sur une nouvelle interface QRhi.

**Étapes**

1. Creer et dessiner sur une premiere interface.
2. Appeler ensureResources avec une seconde interface, puis dessiner dessus.

**Résultat attendu**

- Vérifie que `renderer.ensureResources(first.get())` est vrai.
- Vérifie que `paintedPixels(renderFrame(*first, renderer, firstTarget))` est strictement supérieur à `0U`.
- Vérifie que `renderer.ensureResources(second.get())` est vrai.
- Vérifie que `renderer.rhi()` vaut `second.get()`.
- Vérifie que `renderer.textures().byPath.size()` vaut `hmi::arenaTexturePaths(renderer.catalog()).size()`.
- Vérifie que `paintedPixels(renderFrame(*second, renderer, secondTarget))` est strictement supérieur à `0U`.
- Vérifie que `renderer.composed().size()` vaut `PISTE_QUADS`.

### ArenaSceneRendererTest.LeCadrageRameneChaqueCaseAElleMeme

*Critique · Unitaire · Rendu QRhi de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_renderer.cpp:292`

Le centre de chaque case, projete a l'ecran par le cadrage du rendu, redevient la meme case.

**Étapes**

1. Pour plusieurs grilles et surfaces (zoom entier et zoom inferieur a 1), cadrer par arenaCamera.
2. Projeter le centre de chaque case a l'ecran, puis revenir au monde et a la case.
3. Revenir d'un coin de la surface, hors de la scene.

**Résultat attendu**

- Vérifie que `screen.x` est supérieur ou égal à `0.0f`.
- Vérifie que `screen.x` est inférieur ou égal à `static_cast<float>(cas.width)`.
- Vérifie que `screen.y` est supérieur ou égal à `0.0f`.
- Vérifie que `screen.y` est inférieur ou égal à `static_cast<float>(cas.height)`.
- Vérifie que `cell.has_value()` est vrai.
- Vérifie que `cell->column` vaut `column`.
- Vérifie que `cell->row` vaut `row`.
- Vérifie que `projection.worldToTile(camera.screenToWorld({0.0f, 0.0f})).has_value()` est faux.

### ArenaSceneRendererTest.CaptureDeLArenePourRelecture

*Mineur · Unitaire · Rendu QRhi de l'arene* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_renderer.cpp:334`

Capture de l'arène pour relecture à l'œil (LOT-92).

**Étapes**

1. Définir JADG_ARENA_CAPTURE, lancer ce test.
2. Ouvrir l'image.

**Résultat attendu**

- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `image.isNull()` est faux.
- Vérifie que `image.save(destination)` est vrai.

### ArenaSceneRendererTest.LeCombatUtiliseLeDecorEtLesDepartsDeLaZone

*Critique · Unitaire · Rendu du Colisée* — `Source/Test/Unit/HMI/Graphics/test_arena_scene_renderer.cpp:388`

Le combat utilise le décor et les départs de la nouvelle arène.

**Étapes**

1. Charger la zone sable, monter deux concurrents et rendre la scène.

**Résultat attendu**

- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `zone` diffère de `nullptr`.
- Vérifie que `zone->columns` vaut `20`.
- Vérifie que `zone->rows` vaut `14`.
- Vérifie que `core::validateCombatZones("donjon", *loaded.level).empty()` est vrai.
- Vérifie que `mounted.refusals.empty()` est vrai.
- Vérifie que `snapshot.figures.size()` vaut `2U`.
- Vérifie que `snapshot.figures[0].anchor.column` vaut `1`.
- Vérifie que `snapshot.figures[1].anchor.column` vaut `18`.
- Vérifie que `rhi` diffère de `nullptr`.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `renderer.composed().quads().size()` est strictement supérieur à `280U`.
- Vérifie que `quad.texture` diffère de `renderer.textures().missing.texture`.
- Vérifie que `image.save("salle-combat.png")` est vrai.
- Vérifie que `session.start()` est vrai.

## test_asset_gallery.cpp

### AssetGalleryTest.FormeDesBlocs

*Majeur · Unitaire · Galerie des assets* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery.cpp:57`

Un bloc contient son dessin, marge comprise.

**Étapes**

1. Calculer le bloc d'une figure, d'une attaque large, d'un mur, d'une pièce 2×1 et d'une grande pièce.

**Résultat attendu**

- Vérifie que `figure.columns` vaut `3`.
- Vérifie que `figure.rows` vaut `3`.
- Vérifie que `figure.footprintColumn` vaut `1`.
- Vérifie que `figure.footprintRow` vaut `1`.
- Vérifie que `attaque.columns` vaut `4`.
- Vérifie que `attaque.rows` vaut `3`.
- Vérifie que `attaque.footprintColumn` vaut `1`.
- Vérifie que `mur.columns` vaut `3`.
- Vérifie que `mur.rows` vaut `4`.
- Vérifie que `mur.footprintRow` vaut `2`.
- Vérifie que `large.columns` vaut `4`.
- Vérifie que `large.rows` vaut `4`.
- Vérifie que `large.footprintColumn` vaut `1`.
- Vérifie que `large.footprintRow` vaut `2`.
- Vérifie que `piece.columns` vaut `9`.
- Vérifie que `piece.rows` vaut `6`.

### AssetGalleryTest.DispositionEnBandes

*Majeur · Unitaire · Galerie des assets* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery.cpp:95`

La galerie se dispose en bandes, lignes et colonnes.

**Étapes**

1. Disposer deux modèles de deux formes (dont un mur) sur six cases, puis sur cinq.

**Résultat attendu**

- Vérifie que `layout.bands.size()` vaut `1U`.
- Vérifie que `layout.bands[0].row` vaut `0`.
- Vérifie que `layout.blocs.size()` vaut `4U`.
- Vérifie que `layout.blocs[0].entry` vaut `0`.
- Vérifie que `layout.blocs[0].row` vaut `1`.
- Vérifie que `layout.blocs[1].entry` vaut `2`.
- Vérifie que `layout.blocs[1].column` vaut `3`.
- Vérifie que `layout.blocs[1].row` vaut `1`.
- Vérifie que `layout.blocs[2].entry` vaut `1`.
- Vérifie que `layout.blocs[3].entry` vaut `3`.
- Vérifie que `layout.blocs[3].row` vaut `4`.
- Vérifie que `layout.blocs[2].row` vaut `5`.
- Vérifie que `layout.rows` vaut `8`.
- Vérifie que `layout.columns` vaut `6`.
- Vérifie que `narrow.blocs.size()` vaut `4U`.
- Vérifie que `narrow.blocs[1].column` vaut `0`.
- Vérifie que `narrow.blocs[1].row` vaut `4`.

### AssetGalleryTest.VisibiliteParLaVue

*Bloquant · Unitaire · Galerie des assets* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery.cpp:141`

Seuls les blocs à l'écran sont dessinés.

**Étapes**

1. Classer un bloc 3×3 pour une vue qui le couvre, qui s'en écarte de deux cases, puis de sept.

**Résultat attendu**

- Vérifie que `hmi::assetGalleryVisibility(bloc, {0.0, 0.0, 10.0, 10.0})` vaut `hmi::AssetGalleryVisibility::Drawn`.
- Vérifie que `hmi::assetGalleryVisibility(bloc, {5.0, 0.0, 10.0, 10.0})` vaut `hmi::AssetGalleryVisibility::Preloaded`.
- Vérifie que `hmi::assetGalleryVisibility(bloc, {10.0, 0.0, 10.0, 10.0})` vaut `hmi::AssetGalleryVisibility::Unloaded`.

### AssetGalleryTest.ImageJouee

*Majeur · Unitaire · Galerie des assets* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery.cpp:161`

L'image jouée suit le temps.

**Étapes**

1. Demander l'image d'un clip bouclé et d'un clip joué une fois à plusieurs instants.

**Résultat attendu**

- Vérifie que `hmi::assetGalleryFrameRank(boucle, 0.16)` vaut `1`.
- Vérifie que `hmi::assetGalleryFrameRank(boucle, 0.91)` vaut `0`.
- Vérifie que `hmi::assetGalleryFrameRank(unique, 0.35)` vaut `3`.
- Vérifie que `hmi::assetGalleryFrameRank(unique, 0.75)` vaut `3`.
- Vérifie que `hmi::assetGalleryFrameRank(unique, 1.05)` vaut `0`.
- Vérifie que `hmi::assetGalleryFrameRank(entry("fixe", 16, 16), 3.0)` vaut `0`.

### AssetGalleryTest.AssetsDEssai

*Bloquant · Unitaire · Galerie des assets* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery.cpp:184`

La galerie lit les assets d'une racine.

**Étapes**

1. Lire le catalogue de la racine d'essai.

**Résultat attendu**

- Vérifie que `npcs` diffère de `nullptr`.
- Vérifie que `attack` diffère de `npcs->entries.end()`.
- Vérifie que `attack->frameWidth` vaut `96`.
- Vérifie que `attack->frameCount()` vaut `8`.
- Vérifie que `attack->loop` est faux.
- Vérifie que `familyNamed(catalog, "Monstres")` diffère de `nullptr`.
- Vérifie que `familyNamed(catalog, "Scène · bourg")` diffère de `nullptr`.
- Vérifie que `std::filesystem::is_regular_file(root / value.path)` est vrai.
- Vérifie que `value.frameWidth` est strictement supérieur à `0`.
- Vérifie que `value.frameHeight` est strictement supérieur à `0`.
- Vérifie que `hmi::layoutAssetGallery(catalog).blocs.empty()` est faux.

### AssetGalleryTest.ToutAssetLivreEstDansLaGalerie

*Bloquant · Unitaire · Galerie des assets* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery.cpp:226`

Aucun asset livré n'échappe à la galerie.

**Étapes**

1. Lire le catalogue de Source/Elements/Assets. 2. Parcourir toutes les images livrées.

**Résultat attendu**

- Vérifie que `hmi::assetGalleryExcludes("UI/background/menu-scene.png")` est vrai.
- Vérifie que `hmi::assetGalleryExcludes("Maps/world.jpg")` est vrai.
- Vérifie que `hmi::assetGalleryExcludes("Fonts/Cinzel.ttf")` est vrai.
- Vérifie que `hmi::assetGalleryExcludes("Regions/central-empire/capital/martpart/Scene/street.png")` est faux.
- Vérifie que `hmi::assetGalleryExcludes("Npc/figurant/portrait.png")` est faux.

### AssetGalleryTest.FigurinesDeMonstres

*Majeur · Unitaire · Galerie des assets* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery.cpp:256`

Une figurine Grande sans sort paraît dans la galerie.

**Étapes**

1. Écrire un dossier Monsters/ : un manifeste qui nomme idle et cast, un lion qui n'a que idle, en cellules de 96 × 96. 2. Lire le catalogue.

**Résultat attendu**

- Vérifie que `catalog.errors.empty()` est vrai.
- Vérifie que `monsters` diffère de `nullptr`.
- Vérifie que `monsters->directory` vaut `"Monsters"`.
- Vérifie que `monsters->entries.size()` vaut `1U`.
- Vérifie que `idle.model` vaut `"lion"`.
- Vérifie que `idle.form` vaut `"idle"`.
- Vérifie que `idle.path` vaut `"Monsters/lion/idle.png"`.
- Vérifie que `idle.frameWidth` vaut `96`.
- Vérifie que `idle.frameHeight` vaut `96`.
- Vérifie que `idle.frameCount()` vaut `6`.
- Vérifie que `idle.loop` est vrai.

## test_asset_gallery_renderer.cpp

### AssetGalleryRendererTest.ChargementEtLiberation

*Bloquant · Unitaire · Galerie des assets (rendu)* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery_renderer.cpp:111`

La galerie ne garde que les textures voulues.

**Étapes**

1. Dessiner la figure au repos d'un PNJ. 2. Ne plus rien vouloir, 1 s puis 1,5 s.

**Résultat attendu**

- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `renderer.cachedTextureCount()` vaut `1U`.
- Vérifie que `paintedPixels(image)` est strictement supérieur à `100U`.
- Vérifie que `renderer.cachedTextureCount()` vaut `1U`.
- Vérifie que `renderer.cachedTextureCount()` vaut `0U`.

### AssetGalleryRendererTest.ChargementsEtalesEtFichierAbsent

*Majeur · Unitaire · Galerie des assets (rendu)* — `Source/Test/Unit/HMI/Graphics/test_asset_gallery_renderer.cpp:141`

Les chargements s'étalent, un fichier absent ne bloque rien.

**Étapes**

1. Vouloir 30 textures d'un coup. 2. Dessiner une texture absente.

**Résultat attendu**

- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `renderer.cachedTextureCount()` vaut `static_cast<std::size_t>(hmi::AssetGalleryRenderer::UPLOADS_PER_FRAME)`.
- Vérifie que `renderer.loading()` est vrai.
- Vérifie que `renderer.cachedTextureCount()` vaut `28U`.
- Vérifie que `renderer.loading()` est faux.
- Vérifie que `absent.ensureResources(rhi.get())` est vrai.
- Vérifie que `absent.cachedTextureCount()` vaut `1U`.
- Vérifie que `paintedPixels(image)` est strictement supérieur à `100U`.

## test_cache_registry.cpp

### CacheRegistryTest.ChargeUneSeuleFoisPourUneCleRepetee

*Critique · Unitaire · Registre de cache* — `Source/Test/Unit/HMI/Graphics/test_cache_registry.cpp:43`

Une clé demandée deux fois n'est chargée qu'une fois et rend la même entrée.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `first` diffère de `nullptr`.
- Vérifie que `second` diffère de `nullptr`.
- Vérifie que `*first` vaut `42`.
- Vérifie que `second` vaut `first`.
- Vérifie que `loader.callCount()` vaut `1`.

### CacheRegistryTest.InvalidateForceLeRechargementDeLaCleSeule

*Critique · Unitaire · Registre de cache* — `Source/Test/Unit/HMI/Graphics/test_cache_registry.cpp:68`

invalidate ne force le rechargement que de la clé visée, pas des autres.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `loaderA.callCount()` vaut `2`.
- Vérifie que `loaderB.callCount()` vaut `1`.

### CacheRegistryTest.InvalidateAllViseTout

*Critique · Unitaire · Registre de cache* — `Source/Test/Unit/HMI/Graphics/test_cache_registry.cpp:95`

invalidateAll vide le registre et fait relire toutes les clés.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `registry.size()` vaut `2U`.
- Vérifie que `registry.size()` vaut `0U`.
- Vérifie que `loaderA.callCount()` vaut `2`.
- Vérifie que `loaderB.callCount()` vaut `2`.

### CacheRegistryTest.UnEchecEstMemoriseSansRetenterLeChargement

*Critique · Unitaire · Registre de cache* — `Source/Test/Unit/HMI/Graphics/test_cache_registry.cpp:123`

Un échec de chargement est mémorisé : la clé absente n'est pas relue.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `first` vaut `nullptr`.
- Vérifie que `second` vaut `nullptr`.
- Vérifie que `loader.callCount()` vaut `1`.

### CacheRegistryTest.InvalidateSurUnEchecPermetUnNouvelEssai

*Critique · Unitaire · Registre de cache* — `Source/Test/Unit/HMI/Graphics/test_cache_registry.cpp:146`

Invalider une clé en échec autorise un nouvel essai, qui peut réussir.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result` diffère de `nullptr`.
- Vérifie que `*result` vaut `7`.

## test_camera2d.cpp

### Camera2DTest.CentreAuMilieuDeLEcran

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:25`

Le centre de la caméra se projette au centre de l'écran.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `screen.x` vaut `WIDTH * 0.5f`, à `TOLERANCE` près.
- Vérifie que `screen.y` vaut `HEIGHT * 0.5f`, à `TOLERANCE` près.

### Camera2DTest.CenterEtZoomRenvoientLesValeursPosees

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:45`

center() et zoom() renvoient exactement les valeurs posées.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `camera.center().x` vaut `3.0f` (comparaison flottante).
- Vérifie que `camera.center().y` vaut `4.0f` (comparaison flottante).
- Vérifie que `camera.zoom()` vaut `2.5f` (comparaison flottante).

### Camera2DTest.EchelleEtAxeY

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:64`

Une unité monde vaut 16 pixels ; l'axe Y va vers le bas.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `right.x` vaut `WIDTH * 0.5f + 16.0f`, à `TOLERANCE` près.
- Vérifie que `right.y` vaut `HEIGHT * 0.5f`, à `TOLERANCE` près.
- Vérifie que `down.y` vaut `HEIGHT * 0.5f + 16.0f`, à `TOLERANCE` près.

### Camera2DTest.Zoom

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:86`

Le zoom multiplie l'échelle en pixels.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `right.x` vaut `WIDTH * 0.5f + 32.0f`, à `TOLERANCE` près.

### Camera2DTest.ConversionsReciproques

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:104`

`screenToWorld` est la réciproque de `worldToScreen`.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `roundTrip.x` vaut `world.x`, à `TOLERANCE` près.
- Vérifie que `roundTrip.y` vaut `world.y`, à `TOLERANCE` près.

### Camera2DTest.ProjectionCentreVersOrigineClip

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:125`

La matrice de projection envoie le centre de la caméra à l'origine du clip space.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.x` vaut `0.0f`, à `TOLERANCE` près.
- Vérifie que `result.y` vaut `0.0f`, à `TOLERANCE` près.
- Vérifie que `result.w` vaut `1.0f`, à `TOLERANCE` près.

### Camera2DTest.BordEcranVersBordClip

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:152`

Un coin de l'écran correspond à un bord du clip space (±1).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `result.x` vaut `1.0f`, à `TOLERANCE` près.

### Camera2DTest.FitZoomEntierPourPetitNiveau

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:176`

fitZoom reste entier tant que le facteur brut est supérieur ou égal à 1 (petit niveau).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `zoom` est supérieur ou égal à `1.0f`.
- Vérifie que `zoom` vaut `std::floor(zoom)` (comparaison flottante).

### Camera2DTest.FitZoomFractionnairePourGrandNiveau

*Majeur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:195`

fitZoom devient fractionnaire pour un niveau plus grand que la surface disponible.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `zoom` est strictement supérieur à `0.0f`.
- Vérifie que `zoom` est strictement inférieur à `1.0f`.
- Vérifie que `100.0f * hmi::Camera2D::PIXELS_PER_UNIT * zoom` est inférieur ou égal à `1280.0f + TOLERANCE`.
- Vérifie que `100.0f * hmi::Camera2D::PIXELS_PER_UNIT * zoom` est inférieur ou égal à `720.0f + TOLERANCE`.

### Camera2DTest.FitZoomAppliqueLaMarge

*Mineur · Unitaire · Camera2 D* — `Source/Test/Unit/HMI/Graphics/test_camera2d.cpp:216`

fitZoom applique la marge avant l'arrondi.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `zoomSansMarge` vaut `5.0f` (comparaison flottante).
- Vérifie que `zoomAvecMarge` vaut `4.0f` (comparaison flottante).

## test_city_block_render.cpp

### CityBlockRenderTest.LeCadrageContientLIlot

*Majeur · Unitaire · Plan de la ville* — `Source/Test/Unit/HMI/Graphics/test_city_block_render.cpp:36`

Le cadrage d'un ilot couvre son losange englobant, plus la hauteur des pieces.

**Étapes**

1. Cadrer un ilot de 10 x 8 cases, puis un de 20 x 16, sur une carte de 48 x 40.

**Résultat attendu**

- Vérifie que `cadrePetit.pixelWidth` est strictement supérieur à `0`.
- Vérifie que `cadreGrand.pixelWidth` vaut `2 * cadrePetit.pixelWidth`.
- Vérifie que `static_cast<float>(cadrePetit.pixelHeight)` est strictement supérieur à `losange`.
- Vérifie que `cadrePetit.focus.x + cadrePetit.focus.y` est strictement inférieur à `15.0F + 14.0F`.
- Vérifie que `cadrePetit.focus.x - cadrePetit.focus.y` vaut `15.0F - 14.0F`, à `0.01F` près.

### CityBlockRenderTest.UnIlotDevientUneImage

*Majeur · Unitaire · Plan de la ville* — `Source/Test/Unit/HMI/Graphics/test_city_block_render.cpp:67`

Un ilot d'un lieu se dessine hors ecran, a la taille de son cadrage.

**Étapes**

1. Charger une carte de ville, sa table d'apparence et son plus grand ilot.
2. Le dessiner hors ecran.

**Résultat attendu**

- Vérifie que `lu.ok()` est vrai.
- Vérifie que `table.ok()` est vrai.
- Vérifie que `place` diffère de `ilots.end()`.
- Vérifie que `image.width()` vaut `cadrage.pixelWidth`.
- Vérifie que `image.height()` vaut `cadrage.pixelHeight`.
- Vérifie que `peints * 3` est strictement supérieur à `echantillons`.

## test_depth_sort.cpp

### TriParProfondeurTest.TroisPrimitivesSortentParPiedCroissant

*Critique · Unitaire · Tri par profondeur* — `Source/Test/Unit/HMI/Graphics/test_depth_sort.cpp:46`

Trois primitives a Y croissants sortent dans l'ordre de leur pied.

**Étapes**

1. Composer trois primitives de la bande de profondeur, dans le desordre, avec deux textures differentes.
2. Trier la scene.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `3u`.
- Vérifie que `scene.quads()[0].sprite.x` vaut `1.0f` (comparaison flottante).
- Vérifie que `scene.quads()[1].sprite.x` vaut `2.0f` (comparaison flottante).
- Vérifie que `scene.quads()[2].sprite.x` vaut `0.0f` (comparaison flottante).

### TriParProfondeurTest.PersonnageEntreDeuxObjets

*Critique · Unitaire · Tri par profondeur* — `Source/Test/Unit/HMI/Graphics/test_depth_sort.cpp:73`

Le personnage passe derriere un objet plus bas et devant un objet plus haut.

**Étapes**

1. Composer un arbre au-dessus du personnage, le personnage, puis un arbre en dessous.
2. Trier.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `3u`.
- Vérifie que `scene.quads()[0].layer` vaut `hmi::RenderLayer::Object`.
- Vérifie que `scene.quads()[1].layer` vaut `hmi::RenderLayer::Player`.
- Vérifie que `scene.quads()[2].layer` vaut `hmi::RenderLayer::Object`.
- Vérifie que `scene.quads()[2].sprite.x` vaut `2.0f` (comparaison flottante).

### TriParProfondeurTest.PiedEgalConserveLOrdreDeComposition

*Critique · Unitaire · Tri par profondeur* — `Source/Test/Unit/HMI/Graphics/test_depth_sort.cpp:101`

A pied egal, l'ordre de composition est preserve : aucun scintillement.

**Étapes**

1. Composer trois primitives de meme pied, a des positions differentes.
2. Trier.
3. Recomposer et retrier a l'identique.

**Résultat attendu**

- Vérifie que `first` vaut `second`.
- Vérifie que `first.size()` vaut `3u`.
- Vérifie que `first[0]` vaut `3.0f` (comparaison flottante).

### TriParProfondeurTest.QuantificationAuPixel

*Majeur · Unitaire · Tri par profondeur* — `Source/Test/Unit/HMI/Graphics/test_depth_sort.cpp:137`

Un ecart inferieur au pixel ne departage pas deux profondeurs.

**Étapes**

1. Calculer l'ordre de deux pieds distants d'un centieme d'unite.
2. Le comparer a celui de deux pieds distants d'une demi-unite.

**Résultat attendu**

- Vérifie que `hmi::depthSortOrder(4.0f)` vaut `hmi::depthSortOrder(4.01f)`.
- Vérifie que `hmi::depthSortOrder(4.0f)` est strictement inférieur à `hmi::depthSortOrder(4.5f)`.
- Vérifie que `hmi::depthSortOrder(1.0f)` est strictement inférieur à `hmi::depthSortOrder(9.0f)`.

### TriParProfondeurTest.LaProfondeurNeDebordePasDeSaBande

*Critique · Unitaire · Tri par profondeur* — `Source/Test/Unit/HMI/Graphics/test_depth_sort.cpp:155`

La profondeur ne deborde pas de sa bande.

**Étapes**

1. Composer une tuile, un objet tres bas, un element d'interface tres haut.
2. Trier.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `3u`.
- Vérifie que `scene.quads()[0].layer` vaut `hmi::RenderLayer::Tile`.
- Vérifie que `scene.quads()[1].layer` vaut `hmi::RenderLayer::Object`.
- Vérifie que `scene.quads()[2].layer` vaut `hmi::RenderLayer::UI`.
- Vérifie que `recorder.isLayerOrderRespected()` est vrai.

## test_entity_markers.cpp

### EntityMarkersTest.TypeCamelCaseDevientUneCleKebabCase

*Majeur · Unitaire · Marqueurs d'entite* — `Source/Test/Unit/HMI/Graphics/test_entity_markers.cpp:22`

Le type d'entite devient une cle de marqueur en kebab-case.

**Étapes**

1. Convertir spawnPoint, arenaEntry, chest, NPCGuard et chest2.

**Résultat attendu**

- Vérifie que `hmi::entityMarkerKey("spawnPoint")` vaut `"marker/spawn-point"`.
- Vérifie que `hmi::entityMarkerKey("arenaEntry")` vaut `"marker/arena-entry"`.
- Vérifie que `hmi::entityMarkerKey("chest")` vaut `"marker/chest"`.
- Vérifie que `hmi::entityMarkerKey("NPCGuard")` vaut `"marker/npc-guard"`.
- Vérifie que `hmi::entityMarkerKey("chest2")` vaut `"marker/chest2"`.

### EntityMarkersTest.CaracteresInterditsIgnoresEtSeparateursNormalises

*Majeur · Unitaire · Marqueurs d'entite* — `Source/Test/Unit/HMI/Graphics/test_entity_markers.cpp:39`

Caracteres interdits ignores, separateurs normalises.

**Étapes**

1. Convertir des types avec tirets bas, espaces, ponctuation et accents.

**Résultat attendu**

- Vérifie que `hmi::entityMarkerKey("_old__chest_")` vaut `"marker/old-chest"`.
- Vérifie que `hmi::entityMarkerKey(" wooden - sign ")` vaut `"marker/wooden-sign"`.
- Vérifie que `hmi::entityMarkerKey("a.b!c")` vaut `"marker/abc"`.
- Vérifie que `hmi::entityMarkerKey("a.B")` vaut `"marker/a-b"`.
- Vérifie que `hmi::entityMarkerKey("coffre\xC3\xA9")` vaut `"marker/coffre"`.

### EntityMarkersTest.TypeVideDonneLeMarqueurInconnu

*Majeur · Unitaire · Marqueurs d'entite* — `Source/Test/Unit/HMI/Graphics/test_entity_markers.cpp:56`

Un type vide donne le marqueur inconnu.

**Étapes**

1. Convertir "", "---" et "!?".

**Résultat attendu**

- Vérifie que `hmi::entityMarkerKey("")` vaut `"marker/inconnu"`.
- Vérifie que `hmi::entityMarkerKey("---")` vaut `"marker/inconnu"`.
- Vérifie que `hmi::entityMarkerKey("!?")` vaut `"marker/inconnu"`.

### EntityMarkersTest.ChaqueCleEstValideEtSePeint

*Critique · Unitaire · Marqueurs d'entite* — `Source/Test/Unit/HMI/Graphics/test_entity_markers.cpp:72`

Chaque cle de marqueur est une cle d'asset valide qui se peint.

**Étapes**

1. Convertir chaque type de knownEntityKinds et des types hostiles.
2. Valider la cle et peindre le marqueur.

**Résultat attendu**

- Vérifie que `core::isValidAssetKey(key)` est vrai.
- Vérifie que `image.isEmpty()` est faux.

### EntityMarkersTest.PixelsEmpaquetesEnRgba8

*Majeur · Unitaire · Marqueurs d'entite* — `Source/Test/Unit/HMI/Graphics/test_entity_markers.cpp:97`

Les pixels du marqueur sont empaquetes au format de createTexture.

**Étapes**

1. Empaqueter une image 2x1 connue.
2. Empaqueter une image vide.

**Résultat attendu**

- Vérifie que `pixels.size()` vaut `2u`.
- Vérifie que `pixels[0]` vaut `0x44332211u`.
- Vérifie que `pixels[1]` vaut `0xFFCCBBAAu`.
- Vérifie que `hmi::markerPixelsRgba8(core::MarkerImage{}).empty()` est vrai.

## test_image_encode.cpp

### ImageEncode.AllerRetourExactAlphaCompris

*Critique · Unitaire · Encodage image* — `Source/Test/Unit/HMI/Graphics/test_image_encode.cpp:75`

Encoder puis decoder une image restitue exactement les memes pixels, alpha compris.

**Étapes**

1. Encoder une image 2x2 couvrant plusieurs niveaux d'alpha vers un fichier PNG.
2. Decoder ce fichier.

**Résultat attendu**

- Vérifie que `hmi::encodeImageFile(path, original)` est vrai.
- Vérifie que `decoded.has_value()` est vrai.
- Vérifie que `decoded->width` vaut `original.width`.
- Vérifie que `decoded->height` vaut `original.height`.
- Vérifie que `decoded->pixels` vaut `original.pixels`.

### ImageEncode.DossierInexistantEchoueProprement

*Majeur · Unitaire · Encodage image* — `Source/Test/Unit/HMI/Graphics/test_image_encode.cpp:100`

Encoder vers un dossier inexistant echoue proprement, sans exception.

**Étapes**

1. Tenter d'encoder une image vers un chemin dont le dossier parent n'existe pas.

**Résultat attendu**

- `EXPECT_NO_THROW(result = hmi::encodeImageFile(path, original))`
- Vérifie que `result` est faux.

### ImageEncode.DimensionsIncoherentesEchouentProprement

*Majeur · Unitaire · Encodage image* — `Source/Test/Unit/HMI/Graphics/test_image_encode.cpp:119`

Encoder une image aux dimensions incoherentes echoue proprement.

**Étapes**

1. Construire une image dont le nombre de pixels ne correspond pas a largeur*hauteur.
2. Tenter de l'encoder.

**Résultat attendu**

- `EXPECT_NO_THROW(result = hmi::encodeImageFile(path, broken))`
- Vérifie que `result` est faux.
- Vérifie que `fileCount()` vaut `0U`.

### ImageEncode.AucunFichierTemporaireApresSucces

*Majeur · Unitaire · Encodage image* — `Source/Test/Unit/HMI/Graphics/test_image_encode.cpp:142`

Aucun fichier temporaire ne subsiste apres un encodage reussi.

**Étapes**

1. Encoder une image vers un fichier.
2. Lister le dossier de destination.

**Résultat attendu**

- Vérifie que `hmi::encodeImageFile(dir / "asset.png", sampleImage())` est vrai.
- Vérifie que `fileCount()` vaut `1U`.
- Vérifie que `std::filesystem::exists(dir / "asset.png")` est vrai.

### ImageEncode.EcrasementAtomiqueSansResidu

*Majeur · Unitaire · Encodage image* — `Source/Test/Unit/HMI/Graphics/test_image_encode.cpp:160`

Enregistrer par-dessus un asset existant remplace son contenu sans residu.

**Étapes**

1. Encoder une premiere image vers un fichier.
2. Encoder une seconde image differente vers le meme fichier.
3. Decoder le fichier et lister le dossier.

**Résultat attendu**

- Vérifie que `hmi::encodeImageFile(path, sampleImage())` est vrai.
- Vérifie que `hmi::encodeImageFile(path, second)` est vrai.
- Vérifie que `decoded.has_value()` est vrai.
- Vérifie que `decoded->width` vaut `1`.
- Vérifie que `decoded->height` vaut `1`.
- Vérifie que `fileCount()` vaut `1U`.

## test_maquette_tokens.cpp

### MaquetteTokenTest.LaLettreEstLaPremiereAlphanumerique

*Majeur · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_maquette_tokens.cpp:16`

La lettre d'un jeton est la premiere alphanumerique de son nom.

**Étapes**

1. Demander la lettre de plusieurs noms, dont un vide et un sans lettre.

**Résultat attendu**

- Vérifie que `hmi::maquetteTokenLetter("market-mother")` vaut `'M'`.
- Vérifie que `hmi::maquetteTokenLetter("-- arenarea")` vaut `'A'`.
- Vérifie que `hmi::maquetteTokenLetter("2e-porte")` vaut `'2'`.
- Vérifie que `hmi::maquetteTokenLetter("")` vaut `'?'`.
- Vérifie que `hmi::maquetteTokenLetter("---")` vaut `'?'`.

### MaquetteTokenTest.LeCheminSeRelit

*Critique · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_maquette_tokens.cpp:34`

Le chemin d'un jeton se relit en sa nature et sa lettre.

**Étapes**

1. Ecrire puis relire le chemin de chaque nature.
2. Relire des chemins qui n'en sont pas.

**Résultat attendu**

- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `relu->kind` vaut `kind`.
- Vérifie que `relu->letter` vaut `'W'`.
- Vérifie que `hmi::parseMaquetteTokenPath("Scene/coliseum/sand.png").has_value()` est faux.
- Vérifie que `hmi::parseMaquetteTokenPath("Npc/anariel/idle.png").has_value()` est faux.
- Vérifie que `hmi::parseMaquetteTokenPath("Token/inconnu/W.png").has_value()` est faux.
- Vérifie que `hmi::parseMaquetteTokenPath("Token/player/MOT.png").has_value()` est faux.

### MaquetteTokenTest.UneLettreIllisibleDonneUnPointDInterrogation

*Majeur · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_maquette_tokens.cpp:62`

Un nom illisible donne le chemin du point d'interrogation.

**Étapes**

1. Ecrire le chemin d'un jeton dont la lettre est un caractere de ponctuation.

**Résultat attendu**

- Vérifie que `relu.has_value()` est vrai.
- Vérifie que `relu->letter` vaut `'?'`.

### MaquetteTokenTest.LImageEstUnDisqueALettreDeterministe

*Critique · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_maquette_tokens.cpp:79`

L'image d'un jeton est un disque a lettre, deterministe.

**Étapes**

1. Peindre deux fois le meme jeton.
2. Examiner le centre, un coin, et deux lettres differentes.

**Résultat attendu**

- Vérifie que `premiere.isEmpty()` est faux.
- Vérifie que `premiere.width` vaut `44`.
- Vérifie que `premiere.height` vaut `44`.
- Vérifie que `premiere.pixels` vaut `seconde.pixels`.
- Vérifie que `premiere.at(0, 0).a` vaut `0`.
- Vérifie que `premiere.at(43, 43).a` vaut `0`.
- Vérifie que `premiere.at(22, 22).a` vaut `255`.
- Vérifie que `premiere.pixels` diffère de `autre.pixels`.

### MaquetteTokenTest.UneDemandeImpossibleRendUneImageVide

*Majeur · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_maquette_tokens.cpp:112`

Une demande impossible rend une image vide.

**Étapes**

1. Demander un jeton de cote nul, puis l'image d'un chemin de planche.

**Résultat attendu**

- Vérifie que `hmi::maquetteTokenImage(hmi::MaquetteTokenRequest{}, 0).isEmpty()` est vrai.
- Vérifie que `hmi::maquetteTokenImage("Scene/coliseum/sand.png", 44).isEmpty()` est vrai.

### MaquetteTokenTest.LesSixNaturesOntSixTeintes

*Majeur · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_maquette_tokens.cpp:127`

Les six natures de jeton ont six teintes distinctes.

**Étapes**

1. Comparer les teintes des six natures deux a deux.

**Résultat attendu**

- Vérifie que `hmi::maquetteTokenColor(natures[i]) == hmi::maquetteTokenColor(natures[j])` est faux.
- Vérifie que `hmi::maquetteTokenKindKey(natures[i])` diffère de `hmi::maquetteTokenKindKey(natures[j])`.

## test_missing_texture.cpp

### MissingTextureTest.DimensionsAttendues

*Majeur · Unitaire · Missing Texture* — `Source/Test/Unit/HMI/Graphics/test_missing_texture.cpp:30`

Le damier genere a les dimensions demandees.

**Étapes**

1. Generer le damier par defaut, puis un damier de 32 px.

**Résultat attendu**

- Vérifie que `image.width` vaut `hmi::MISSING_TEXTURE_SIZE`.
- Vérifie que `image.height` vaut `hmi::MISSING_TEXTURE_SIZE`.
- Vérifie que `image.pixels.size()` vaut `static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height)`.
- Vérifie que `large.width` vaut `32`.
- Vérifie que `large.height` vaut `32`.

### MissingTextureTest.GenerationDeterministe

*Majeur · Unitaire · Missing Texture* — `Source/Test/Unit/HMI/Graphics/test_missing_texture.cpp:52`

La generation du damier est deterministe.

**Étapes**

1. Generer deux fois le damier.

**Résultat attendu**

- Vérifie que `hmi::buildMissingTextureImage().pixels` vaut `hmi::buildMissingTextureImage().pixels`.

### MissingTextureTest.CarreauxAlternesMagentaEtNoir

*Critique · Unitaire · Missing Texture* — `Source/Test/Unit/HMI/Graphics/test_missing_texture.cpp:66`

Les carreaux du damier alternent magenta et noir.

**Étapes**

1. Generer le damier.
2. Lire un pixel de chacun des quatre premiers carreaux.

**Résultat attendu**

- Vérifie que `pixelAt(image, 0, 0)` vaut `MAGENTA`.
- Vérifie que `pixelAt(image, STEP, 0)` vaut `BLACK`.
- Vérifie que `pixelAt(image, 0, STEP)` vaut `BLACK`.
- Vérifie que `pixelAt(image, STEP, STEP)` vaut `MAGENTA`.

### MissingTextureTest.DamierEntierementOpaque

*Critique · Unitaire · Missing Texture* — `Source/Test/Unit/HMI/Graphics/test_missing_texture.cpp:87`

Tous les pixels du damier sont opaques.

**Étapes**

1. Generer le damier.
2. Verifier le canal alpha de chaque pixel.

**Résultat attendu**

- Vérifie que `pixel >> 24` vaut `0xFFu`.

### MissingTextureTest.AvertissementNommeAsset

*Majeur · Unitaire · Missing Texture* — `Source/Test/Unit/HMI/Graphics/test_missing_texture.cpp:105`

L'avertissement de texture manquante nomme l'asset attendu.

**Étapes**

1. Construire le message pour un asset donne.

**Résultat attendu**

- Vérifie que `message.find("Backgrounds/foret.png")` diffère de `std::string::npos`.

## test_poly_quad.cpp

### PolyQuadTest.BoiteEnglobanteDesSommets

*Majeur · Unitaire · PolyQuad* — `Source/Test/Unit/HMI/Graphics/test_poly_quad.cpp:38`

La boite englobante d'un losange est celle de ses quatre sommets.

**Étapes**

1. Borner un losange de 4 x 2 pose en (10, 20).

**Résultat attendu**

- Vérifie que `bounds.position.x` vaut `10.0f` (comparaison flottante).
- Vérifie que `bounds.position.y` vaut `20.0f` (comparaison flottante).
- Vérifie que `bounds.size.x` vaut `4.0f` (comparaison flottante).
- Vérifie que `bounds.size.y` vaut `2.0f` (comparaison flottante).

### PolyQuadTest.CullingCommeLesAutres

*Critique · Unitaire · PolyQuad* — `Source/Test/Unit/HMI/Graphics/test_poly_quad.cpp:57`

Un losange hors cadrage est ecarte, celui du cadrage est conserve.

**Étapes**

1. Fixer un cadrage.
2. Composer un losange dedans, puis un losange loin dehors.

**Résultat attendu**

- Vérifie que `scene.addPoly(hmi::RenderLayer::Tile, solid, 0, diamond(15.0f, 15.0f, 2.0f, 1.0f))` est vrai.
- Vérifie que `scene.addPoly(hmi::RenderLayer::Tile, solid, 0, diamond(800.0f, 15.0f, 2.0f, 1.0f))` est faux.
- Vérifie que `scene.statistics().considered` vaut `2`.
- Vérifie que `scene.statistics().culled` vaut `1`.
- Vérifie que `scene.statistics().submitted` vaut `1`.

### PolyQuadTest.NatureEtSommetsPreserves

*Majeur · Unitaire · PolyQuad* — `Source/Test/Unit/HMI/Graphics/test_poly_quad.cpp:81`

Le quad compose porte bien la nature Poly et ses sommets intacts.

**Étapes**

1. Composer un losange sans cadrage.

**Résultat attendu**

- Vérifie que `scene.addPoly(hmi::RenderLayer::Tile, solid, 7, given)` est vrai.
- Vérifie que `composed.kind` vaut `hmi::QuadKind::Poly`.
- Vérifie que `composed.sortOrder` vaut `7`.
- Vérifie que `composed.poly.x[i]` vaut `given.x[i]` (comparaison flottante).
- Vérifie que `composed.poly.y[i]` vaut `given.y[i]` (comparaison flottante).

### PolyQuadTest.TrieAvecLesAutresPrimitives

*Critique · Unitaire · PolyQuad* — `Source/Test/Unit/HMI/Graphics/test_poly_quad.cpp:106`

Un losange se trie avec les autres primitives, par calque puis par texture.

**Étapes**

1. Composer un sprite de decor, puis un losange de sol, puis un second losange de sol.
2. Trier.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `3U`.
- Vérifie que `scene.quads()[0].layer` vaut `hmi::RenderLayer::Tile`.
- Vérifie que `scene.quads()[1].layer` vaut `hmi::RenderLayer::Tile`.
- Vérifie que `scene.quads()[2].layer` vaut `hmi::RenderLayer::Object`.
- Vérifie que `scene.batchCount()` vaut `2`.

## test_procedural_atlas.cpp

### ProceduralAtlasTest.DimensionsAttendues

*Majeur · Unitaire · Procedural Atlas* — `Source/Test/Unit/HMI/Graphics/test_procedural_atlas.cpp:21`

L'image générée a les dimensions attendues (grille de tuiles).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `image.width` vaut `gridSide`.
- Vérifie que `image.height` vaut `gridSide`.
- Vérifie que `image.pixels.size()` vaut `static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height)`.

### ProceduralAtlasTest.GenerationDeterministe

*Majeur · Unitaire · Procedural Atlas* — `Source/Test/Unit/HMI/Graphics/test_procedural_atlas.cpp:42`

La génération est déterministe : deux appels produisent des pixels identiques.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `first.width` vaut `second.width`.
- Vérifie que `first.height` vaut `second.height`.
- Vérifie que `first.pixels` vaut `second.pixels`.

### ProceduralAtlasTest.DamierDeTransparenceDansLaDerniereTuile

*Mineur · Unitaire · Procedural Atlas* — `Source/Test/Unit/HMI/Graphics/test_procedural_atlas.cpp:63`

La dernière tuile de la grille contient des pixels opaques et transparents (damier).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `sawOpaque` est vrai.
- Vérifie que `sawTransparent` est vrai.

### ProceduralAtlasTest.ChaqueTypeDeTuileAUneCouleurDeRepliDistincte

*Critique · Unitaire · Atlas procedural* — `Source/Test/Unit/HMI/Graphics/test_procedural_atlas.cpp:107`

Chaque type de tuile a une couleur de repli visible et distincte.

**Étapes**

1. Generer l'atlas procedural.
2. Echantillonner le centre de la case de chaque type de tuile, hors case vide.

**Résultat attendu**

- Vérifie que `color` diffère de `0xFF000000u`.
- Vérifie que `inserted.second` est vrai.

## test_quad_recorder.cpp

### QuadRecorderTest.CalquePrimeSurTexture

*Critique · Unitaire · Quad Recorder* — `Source/Test/Unit/HMI/Graphics/test_quad_recorder.cpp:41`

Le calque prime sur la texture dans l'ordre de soumission.

**Étapes**

1. Composer un quad Player (texture A) puis un quad Tile (texture B).
2. Trier et capturer la scene.

**Résultat attendu**

- Vérifie que `recorder.size()` vaut `2u`.
- Vérifie que `recorder.quads()[0].layer` vaut `hmi::RenderLayer::Tile`.
- Vérifie que `recorder.quads()[1].layer` vaut `hmi::RenderLayer::Player`.
- Vérifie que `recorder.isLayerOrderRespected()` est vrai.

### QuadRecorderTest.TroisCalquesDeuxTexturesOrdonnes

*Critique · Unitaire · Quad Recorder* — `Source/Test/Unit/HMI/Graphics/test_quad_recorder.cpp:67`

Trois calques et deux textures : ordre des calques et groupes de texture contigus.

**Étapes**

1. Composer six quads en alternant calques et textures.
2. Trier et capturer.

**Résultat attendu**

- Vérifie que `recorder.isLayerOrderRespected()` est vrai.
- Vérifie que `recorder.areTextureGroupsContiguous()` est vrai.
- Vérifie que `layers.size()` vaut `3u`.
- Vérifie que `layers[0]` vaut `hmi::RenderLayer::Background`.
- Vérifie que `layers[1]` vaut `hmi::RenderLayer::Tile`.
- Vérifie que `layers[2]` vaut `hmi::RenderLayer::Player`.
- Vérifie que `recorder.countOnLayer(hmi::RenderLayer::Tile)` vaut `3`.
- Vérifie que `recorder.countWithTexture(textureA)` vaut `3`.
- Vérifie que `recorder.countWithTexture(textureB)` vaut `3`.

### QuadRecorderTest.TriStableAClefEgale

*Majeur · Unitaire · Quad Recorder* — `Source/Test/Unit/HMI/Graphics/test_quad_recorder.cpp:104`

Le tri est stable : a cle egale l'ordre de composition est preserve.

**Étapes**

1. Composer trois quads identiques en cle, a des positions differentes.
2. Trier.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `3u`.
- Vérifie que `scene.quads()[0].sprite.x` vaut `3.0f` (comparaison flottante).
- Vérifie que `scene.quads()[1].sprite.x` vaut `1.0f` (comparaison flottante).
- Vérifie que `scene.quads()[2].sprite.x` vaut `2.0f` (comparaison flottante).

### QuadRecorderTest.TriFinParSpriteLayer

*Majeur · Unitaire · Quad Recorder* — `Source/Test/Unit/HMI/Graphics/test_quad_recorder.cpp:128`

Le tri fin par Sprite::layer subsiste a l'interieur d'un calque.

**Étapes**

1. Composer trois quads du meme calque avec des tris fins decroissants.
2. Trier.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `3u`.
- Vérifie que `scene.quads()[0].sortOrder` vaut `0`.
- Vérifie que `scene.quads()[1].sortOrder` vaut `1`.
- Vérifie que `scene.quads()[2].sortOrder` vaut `2`.

### QuadRecorderTest.DeuxTexturesDeuxPasses

*Majeur · Unitaire · Quad Recorder* — `Source/Test/Unit/HMI/Graphics/test_quad_recorder.cpp:152`

Le nombre de passes egale le nombre de groupes de texture.

**Étapes**

1. Composer quatre quads d'un meme calque en alternant deux textures.
2. Trier.

**Résultat attendu**

- Vérifie que `scene.batchCount()` vaut `2`.
- Vérifie que `recorder.textureSequence().size()` vaut `2u`.
- Vérifie que `recorder.areTextureGroupsContiguous()` est vrai.

### QuadRecorderTest.RectanglesEtSegmentsMelanges

*Mineur · Unitaire · Quad Recorder* — `Source/Test/Unit/HMI/Graphics/test_quad_recorder.cpp:179`

Segments et rectangles cohabitent dans la meme scene ordonnee.

**Étapes**

1. Composer un rectangle puis un segment sur le meme calque.
2. Trier et capturer.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `2u`.
- Vérifie que `scene.quads()[0].kind` vaut `hmi::QuadKind::Sprite`.
- Vérifie que `scene.quads()[1].kind` vaut `hmi::QuadKind::Line`.

### QuadRecorderTest.OrdonnancementDeclare

*Critique · Unitaire · Quad Recorder* — `Source/Test/Unit/HMI/Graphics/test_quad_recorder.cpp:208`

L'ordonnancement place l'interface au-dessus du personnage.

**Étapes**

1. Comparer les valeurs declarees de l'enumeration des calques.

**Résultat attendu**

- Vérifie que `hmi::RenderLayer::Background` est strictement inférieur à `hmi::RenderLayer::Shadow`.
- Vérifie que `hmi::RenderLayer::Shadow` est strictement inférieur à `hmi::RenderLayer::Tile`.
- Vérifie que `hmi::RenderLayer::Tile` est strictement inférieur à `hmi::RenderLayer::Object`.
- Vérifie que `hmi::RenderLayer::Object` est strictement inférieur à `hmi::RenderLayer::Player`.
- Vérifie que `hmi::RenderLayer::Player` est strictement inférieur à `hmi::RenderLayer::UI`.
- Vérifie que `hmi::RenderLayer::UI` est strictement inférieur à `hmi::RenderLayer::EditorOverlay`.

## test_render_culling.cpp

### RenderCullingTest.SansCadrageAucunRejet

*Majeur · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:39`

Sans cadrage fixe, aucune primitive n'est ecartee.

**Étapes**

1. Composer un quad tres eloigne dans une scene sans cadrage.

**Résultat attendu**

- Vérifie que `scene.isCullingEnabled()` est faux.
- Vérifie que `scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(9999.0f, 9999.0f))` est vrai.
- Vérifie que `scene.statistics().culled` vaut `0`.

### RenderCullingTest.HorsCadrageEcarte

*Critique · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:57`

Une primitive hors cadrage est ecartee, la meme dans le cadrage est conservee.

**Étapes**

1. Fixer le cadrage de reference.
2. Composer un quad dedans, puis un quad loin dehors.

**Résultat attendu**

- Vérifie que `scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(15.0f, 15.0f))` est vrai.
- Vérifie que `scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(80.0f, 15.0f))` est faux.
- Vérifie que `recorder.size()` vaut `1u`.
- Vérifie que `recorder.containsSpriteAt(15.0f, 15.0f)` est vrai.
- Vérifie que `recorder.statistics().considered` vaut `2`.
- Vérifie que `recorder.statistics().culled` vaut `1`.
- Vérifie que `recorder.statistics().submitted` vaut `1`.

### RenderCullingTest.AChevalSurLaFrontiereConserve

*Critique · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:86`

Une primitive a cheval sur la frontiere du cadrage est conservee.

**Étapes**

1. Fixer le cadrage de reference.
2. Composer un quad chevauchant le bord droit, puis un quad entierement dans la marge.

**Résultat attendu**

- Vérifie que `scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(29.5f, 15.0f))` est vrai.
- Vérifie que `scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(30.2f, 15.0f))` est vrai.
- Vérifie que `scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(31.5f, 15.0f))` est faux.

### RenderCullingTest.SegmentHorizontalConserve

*Majeur · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:109`

Un segment horizontal traversant le cadrage est conserve.

**Étapes**

1. Fixer le cadrage de reference.
2. Composer un segment horizontal a l'interieur, puis un segment horizontal tres au-dessus.

**Résultat attendu**

- Vérifie que `scene.addLine(hmi::RenderLayer::EditorOverlay, texture, 0, inside)` est vrai.
- Vérifie que `scene.addLine(hmi::RenderLayer::EditorOverlay, texture, 0, outside)` est faux.

### RenderCullingTest.PrimitiveEtireeConservee

*Critique · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:139`

Une primitive etiree dont l'ancrage est hors cadrage reste soumise.

**Étapes**

1. Fixer le cadrage de reference.
2. Composer un quad ancre en (0, 0) et large de 200 unites.

**Résultat attendu**

- Vérifie que `scene.addSprite(hmi::RenderLayer::Background, texture, 0, background)` est vrai.

### RenderCullingTest.SpriteQuadBoundsSansRotationEstLeRectangleBrut

*Majeur · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:163`

spriteQuadBounds sans rotation vaut le rectangle brut.

**Étapes**

1. Calculer la boite englobante d'un quad de rotation nulle.

**Résultat attendu**

- Vérifie que `bounds.position.x` vaut `5.0f` (comparaison flottante).
- Vérifie que `bounds.position.y` vaut `3.0f` (comparaison flottante).
- Vérifie que `bounds.size.x` vaut `4.0f` (comparaison flottante).
- Vérifie que `bounds.size.y` vaut `2.0f` (comparaison flottante).

### RenderCullingTest.SpriteQuadBoundsA90DegresPermuteLargeurEtHauteur

*Critique · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:189`

spriteQuadBounds a 90 degres permute largeur et hauteur.

**Étapes**

1. Calculer la boite englobante d'un quad large tourne de 90 degres.

**Résultat attendu**

- Vérifie que `bounds.size.x` vaut `2.0f`, à `1e-3f` près.
- Vérifie que `bounds.size.y` vaut `4.0f`, à `1e-3f` près.
- Vérifie que `bounds.position.x + bounds.size.x * 0.5f` vaut `2.0f`, à `1e-3f` près.
- Vérifie que `bounds.position.y + bounds.size.y * 0.5f` vaut `1.0f`, à `1e-3f` près.

### RenderCullingTest.QuadTourneChevauchantLeCadrageResteSoumis

*Critique · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:217`

Un quad tourne dont la boite reelle chevauche le cadrage reste soumis.

**Étapes**

1. Fixer un cadrage de reference.
2. Composer un quad tourne de 45 degres juste a l'exterieur du rectangle non tourne, mais dont la diagonale rentre dans le cadrage.

**Résultat attendu**

- Vérifie que `scene.addSprite(hmi::RenderLayer::EditorOverlay, texture, 0, quad)` est vrai.

### RenderCullingTest.MargeAppliqueeSurLesQuatreCotes

*Mineur · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:246`

Le cadrage de culling est le cadrage visible elargi de la marge.

**Étapes**

1. Fixer le cadrage de reference.
2. Lire le rectangle de culling.

**Résultat attendu**

- Vérifie que `bounds.left()` vaut `VISIBLE_BOUNDS.left() - MARGIN` (comparaison flottante).
- Vérifie que `bounds.top()` vaut `VISIBLE_BOUNDS.top() - MARGIN` (comparaison flottante).
- Vérifie que `bounds.right()` vaut `VISIBLE_BOUNDS.right() + MARGIN` (comparaison flottante).
- Vérifie que `bounds.bottom()` vaut `VISIBLE_BOUNDS.bottom() + MARGIN` (comparaison flottante).

### RenderCullingTest.CadrageDeLaCamera

*Majeur · Unitaire · Render Culling* — `Source/Test/Unit/HMI/Graphics/test_render_culling.cpp:267`

La camera fournit le rectangle monde qu'elle cadre.

**Étapes**

1. Centrer une camera 320x160 sur (10, 5) au zoom 1.
2. Lire son cadrage.

**Résultat attendu**

- Vérifie que `bounds.left()` vaut `0.0f` (comparaison flottante).
- Vérifie que `bounds.top()` vaut `0.0f` (comparaison flottante).
- Vérifie que `bounds.size.x` vaut `20.0f` (comparaison flottante).
- Vérifie que `bounds.size.y` vaut `10.0f` (comparaison flottante).
- Vérifie que `camera.visibleBounds().size.x` vaut `10.0f` (comparaison flottante).

## test_rhi_offscreen.cpp

### RhiOffscreenTest.ZoomEntierResteNetEnFiltrageNearest

*Critique · Unitaire · Rendu QRhi* — `Source/Test/Unit/HMI/Graphics/test_rhi_offscreen.cpp:89`

Le rendu QRhi conserve le pixel art net a zoom entier.

**Étapes**

1. Rendre hors ecran une texture temoin 4x4 agrandie 4 fois.
2. Relire les pixels de la cible.

**Résultat attendu**

- Vérifie que `target->create()` est vrai.
- Vérifie que `renderTarget->create()` est vrai.
- Vérifie que `rhi->beginOffscreenFrame(&commandBuffer)` vaut `QRhi::FrameOpSuccess`.
- Vérifie que `source.has_value()` est vrai.
- Vérifie que `rhi->endOffscreenFrame()` vaut `QRhi::FrameOpSuccess`.
- Vérifie que `readback.pixelSize` vaut `QSize(TARGET_SIZE, TARGET_SIZE)`.
- Vérifie que `pixel.red()` vaut `red`.
- Vérifie que `pixel.green()` vaut `green`.
- Vérifie que `pixel.blue()` vaut `blue`.

### RhiOffscreenTest.TeinteMultiplieeEtEffacementConserve

*Majeur · Unitaire · Rendu QRhi* — `Source/Test/Unit/HMI/Graphics/test_rhi_offscreen.cpp:180`

La teinte multiplie la texture et l'effacement subsiste hors du quad.

**Étapes**

1. Rendre hors ecran un quad teinte couvrant un quart de la cible.
2. Relire un pixel dans le quad et un pixel hors du quad.

**Résultat attendu**

- Vérifie que `target->create()` est vrai.
- Vérifie que `renderTarget->create()` est vrai.
- Vérifie que `rhi->beginOffscreenFrame(&commandBuffer)` vaut `QRhi::FrameOpSuccess`.
- Vérifie que `source.has_value()` est vrai.
- Vérifie que `rhi->endOffscreenFrame()` vaut `QRhi::FrameOpSuccess`.
- Vérifie que `inside.red()` vaut `0`.
- Vérifie que `inside.green()` vaut `255`.
- Vérifie que `inside.blue()` vaut `0`.
- Vérifie que `outside.red()` vaut `0`.
- Vérifie que `outside.green()` vaut `0`.
- Vérifie que `outside.blue()` vaut `255`.

## test_texture_atlas.cpp

### TextureAtlasTest.TileRenvoieLeRectangleAttendu

*Critique · Unitaire · Texture Atlas* — `Source/Test/Unit/HMI/Graphics/test_texture_atlas.cpp:18`

tile(colonne, ligne) renvoie un rectangle de 16x16 pixels à l'origine attendue.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `origin.x` vaut `0`.
- Vérifie que `origin.y` vaut `0`.
- Vérifie que `origin.width` vaut `hmi::TextureAtlas::TILE_SIZE`.
- Vérifie que `origin.height` vaut `hmi::TextureAtlas::TILE_SIZE`.
- Vérifie que `secondRow.x` vaut `2 * hmi::TextureAtlas::TILE_SIZE`.
- Vérifie que `secondRow.y` vaut `1 * hmi::TextureAtlas::TILE_SIZE`.
- Vérifie que `secondRow.width` vaut `hmi::TextureAtlas::TILE_SIZE`.
- Vérifie que `secondRow.height` vaut `hmi::TextureAtlas::TILE_SIZE`.

## test_world_scene_composer.cpp

### ScenePiecePlacement.PreservesLegacyPlacementWithoutOptIn

*Critique · Unitaire · Rendu du Colisée* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:37`

Le placement historique reste inchangé.

**Étapes**

1. Lire un manifeste sans placementVersion et composer une pièce.

**Résultat attendu**

- Vérifie que `hmi::scenePieceAnchor(manifest, "wall.png")` est faux.
- Vérifie que `quad.x` vaut `32` (comparaison flottante).
- Vérifie que `quad.y` vaut `-228` (comparaison flottante).

### ScenePiecePlacement.AlignsFractionalOriginWithoutChangingDimensions

*Critique · Unitaire · Rendu du Colisée* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:56`

Une ancre fractionnaire aligne la pièce.

**Étapes**

1. Lire une origine fractionnaire puis composer la pièce.

**Résultat attendu**

- Vérifie que `anchor` est vrai.
- Vérifie que `quad.x + anchor->x * 2` vaut `100` (comparaison flottante).
- Vérifie que `quad.y + anchor->y * 2` vaut `200` (comparaison flottante).
- Vérifie que `quad.width` vaut `512` (comparaison flottante).
- Vérifie que `quad.height` vaut `512` (comparaison flottante).
- Vérifie que `hmi::scenePieceAnchor(manifest, "unknown.png")` est faux.

### ScenePiecePlacement.RejectsMalformedAnchor

*Critique · Unitaire · Rendu du Colisée* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:80`

Une ancre invalide est ignorée.

**Étapes**

1. Lire une ancre contenant une chaîne à la place d’un nombre.

**Résultat attendu**

- Vérifie que `hmi::scenePieceAnchor(manifest, "wall.png")` est faux.

### ScenePiecePlacement.ProjectionIsOptInAndRejectsInvalidRatios

*Critique · Unitaire · Rendu du Colisée* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:96`

La projection explicite est contrôlée.

**Étapes**

1. Lire les tables sans ratio, avec ratio valide et avec ratios invalides.

**Résultat attendu**

- Vérifie que `normal.ok()` est vrai.
- Vérifie que `normal.appearance.diamondRatio()` vaut `core::ARENA_DIAMOND_RATIO` (comparaison flottante).
- Vérifie que `modular.ok()` est vrai.
- Vérifie que `modular.appearance.diamondRatio()` vaut `42.0F / 68.0F` (comparaison flottante).
- Vérifie que `hmi::PlaceAppearance::loadFromString(R"({"version":1,"place":"bad","diamondRatio":0})") .ok()` est faux.
- Vérifie que `hmi::PlaceAppearance::loadFromString( R"({"version":1,"place":"bad","diamondRatio":"wrong"})") .ok()` est faux.

### WorldSceneComposerTest.LInstantaneTireLeSolDuTypeEtLeReliefDeLaCase

*Critique · Unitaire · Lieu compose* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:183`

Le sol vient du type de tuile, le relief de la piece nommee a la case.

**Étapes**

1. Batir une carte de sable avec une case de pierre, un mur de decor et une piece nommee a une case du decor.
2. En tirer l'instantane.

**Résultat attendu**

- Vérifie que `instantane.columns` vaut `4`.
- Vérifie que `instantane.rows` vaut `3`.
- Vérifie que `instantane.place` vaut `"coliseum"`.
- Vérifie que `instantane.floorAt({0, 0})` vaut `"sand"`.
- Vérifie que `instantane.floorAt({1, 0})` vaut `"sand-2"`.
- Vérifie que `instantane.floorAt({2, 0})` vaut `"sand-3"`.
- Vérifie que `instantane.floorAt({3, 0})` vaut `"stone-slab"`.
- Vérifie que `instantane.floorAt({0, 0})` vaut `hmi::snapshotWorldScene(carte(), table(), {}).floorAt({0, 0})`.
- Vérifie que `instantane.reliefAt({0, 0})` vaut `"wall-left"`.
- Vérifie que `instantane.reliefAt({2, 2})` vaut `"torch-left"`.
- Vérifie que `instantane.reliefAt({1, 1}).empty()` est vrai.
- Vérifie que `instantane.floorAt({9, 9}).empty()` est vrai.

### WorldSceneComposerTest.LesCheminsCouvrentLeLieuEtLesFigurines

*Majeur · Unitaire · Lieu compose* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:216`

La liste des textures a charger couvre exactement ce que la composition resout.

**Étapes**

1. Tirer l'instantane d'une carte avec une figurine.
2. Lister les chemins de texture.

**Résultat attendu**

- Vérifie que `std::ranges::is_sorted(chemins)` est vrai.
- Vérifie que `std::ranges::find(chemins, "Scene/coliseum/sand.png")` diffère de `chemins.end()`.
- Vérifie que `std::ranges::find(chemins, "Scene/coliseum/stone-slab.png")` diffère de `chemins.end()`.
- Vérifie que `std::ranges::find(chemins, "Scene/coliseum/wall-left.png")` diffère de `chemins.end()`.
- Vérifie que `std::ranges::find(chemins, "Scene/coliseum/torch-left.png")` diffère de `chemins.end()`.
- Vérifie que `std::ranges::find(chemins, "Npc/anariel/idle.png")` diffère de `chemins.end()`.
- Vérifie que `std::ranges::find(chemins, "Npc/anariel/walk.png")` diffère de `chemins.end()`.
- Vérifie que `std::ranges::count(chemins, "Scene/coliseum/sand.png")` vaut `1`.

### WorldSceneComposerTest.LaCompositionPoseChaquePieceSurSonCalque

*Critique · Unitaire · Lieu compose* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:245`

Sol, relief et figurine tombent sur les calques Tile, Object et Player.

**Étapes**

1. Composer une carte de douze cases avec un mur, une torche et une figurine.

**Résultat attendu**

- Vérifie que `sols` vaut `12`.
- Vérifie que `reliefs` vaut `2`.
- Vérifie que `figurines` vaut `1`.
- Vérifie que `quad.sprite.y + quad.sprite.height` est strictement inférieur à `piedDeLaCase`.

### WorldSceneComposerTest.UneFigurineSansImageAUneCleDeMarqueur

*Majeur · Unitaire · Rendu du lieu* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:295`

La cle du marqueur d'une figurine se tire de son chemin de bande.

**Étapes**

1. Demander la cle de marqueur de chemins de figurine, de piece et de chemins malformes.

**Résultat attendu**

- Vérifie que `hmi::figureMarkerKey("Npc/sentinelle-ironhand/idle.png")` vaut `"npc/sentinelle-ironhand"`.
- Vérifie que `hmi::figureMarkerKey("Npc/sentinelle-ironhand/walk.png")` vaut `"npc/sentinelle-ironhand"`.
- Vérifie que `hmi::figureMarkerKey("Scene/martpart/street.png")` vaut `""`.
- Vérifie que `hmi::figureMarkerKey("Npc/")` vaut `""`.
- Vérifie que `hmi::figureMarkerKey("Npc//idle.png")` vaut `""`.
- Vérifie que `hmi::figureMarkerKey("Npc/jade")` vaut `""`.
- Vérifie que `hmi::figureMarkerKey("Monsters/ironhand-soldier/idle.png")` vaut `"monsters/ironhand-soldier"`.

### WorldSceneComposerTest.UneFigurineSeNommeParSlugOuParDossier

*Majeur · Unitaire · Scène du monde* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:318`

Le soldat Ironhand se lit dans les monstres, Anariel dans les PNJ.

**Étapes**

1. Demander la bande idle d'« anariel », puis celle de « Monsters/ironhand-soldier ».

**Résultat attendu**

- Vérifie que `hmi::figureStripPath("anariel", "idle")` vaut `"Npc/anariel/idle.png"`.
- Vérifie que `hmi::figureStripPath("anariel", "")` vaut `"Npc/anariel/idle.png"`.
- Vérifie que `hmi::figureStripPath("Monsters/ironhand-soldier", "walk")` vaut `"Monsters/ironhand-soldier/walk.png"`.

### WorldSceneComposerTest.LaPieceNommeeLEmporteSousSonNomCourant

*Critique · Unitaire · Lieu compose* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:337`

La pièce nommée l'emporte, sous son nom courant.

**Étapes**

1. Nommer `stone-slab` sur une case de sable, et `old-torch`, ancien nom de `torch-left`, sur le décor.
2. Tirer l'instantané avec le manifeste des alias.

**Résultat attendu**

- Vérifie que `manifeste.ok()` est vrai.
- Vérifie que `instantane.floorAt({0, 1})` vaut `"stone-slab"`.
- Vérifie que `instantane.reliefAt({2, 2})` vaut `"torch-left"`.

### WorldSceneComposerTest.UnePieceLargeSeTrieAuPiedDeSonEmprise

*Majeur · Unitaire · Lieu compose* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:366`

Une pièce large se trie au pied de son emprise.

**Étapes**

1. Poser un étal 2 × 1 et un mur 1 × 1 sur la même case d'ancrage de deux cartes.
2. Composer les deux.

**Résultat attendu**

- Vérifie que `manifeste.ok()` est vrai.
- Vérifie que `large.footprints.at("stall")` vaut `(core::PieceFootprint{.columns = 2, .rows = 1})`.
- Vérifie que `ordreDuRelief(large)` est strictement supérieur à `ordreDuRelief(simple)`.

### ScenePiecePlacement.DepthRequiresOptInAndValidAnchor

*Critique · Unitaire · Rendu du Colisée* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:407`

La profondeur exige un manifeste de placement valide.

**Étapes**

1. Lire une pièce valide, une inconnue et un manifeste historique.

**Résultat attendu**

- Vérifie que `hmi::scenePieceDepthOffset(manifest, "gate.png")` vaut `4.5F`.
- Vérifie que `hmi::scenePieceDepthOffset(manifest, "unknown.png")` est faux.
- Vérifie que `hmi::scenePieceDepthOffset(legacy, "gate.png")` est faux.

### MaquetteRenderTest.UneCarteSansLieuSeComposeEnLosangesDeCouleur

*Critique · Unitaire · Rendu de maquette* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:453`

Une carte sans lieu se compose en losanges de couleur.

**Étapes**

1. Composer une carte de deux cases qui ne nomme aucun lieu.

**Résultat attendu**

- Vérifie que `instantane.typeAt({0, 0})` vaut `core::TileType::Water`.
- Vérifie que `instantane.typeAt({1, 0})` vaut `core::TileType::Wall`.
- Vérifie que `scene.size()` vaut `4U`.
- Vérifie que `quad.kind` vaut `hmi::QuadKind::Poly`.
- Vérifie que `quad.texture` vaut `aplat()`.
- Vérifie que `scene.quads()[0].layer` vaut `hmi::RenderLayer::Tile`.
- Vérifie que `scene.quads()[0].poly.r` vaut `eau.r` (comparaison flottante).
- Vérifie que `scene.quads()[0].poly.b` vaut `eau.b` (comparaison flottante).
- Vérifie que `scene.quads()[i].layer` vaut `hmi::RenderLayer::Object`.

### MaquetteRenderTest.UnTypeNonCouvertParLeLieuPrendLaMaquette

*Critique · Unitaire · Rendu de maquette* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:490`

Un type absent de la table du lieu prend le rendu de maquette.

**Étapes**

1. Peindre une case d'eau sur la carte du Colisee, dont la table ne couvre que le sable et la pierre.
2. Composer.

**Résultat attendu**

- Vérifie que `instantane.floorAt({0, 0}).empty()` est faux.
- Vérifie que `instantane.floorAt({1, 0}).empty()` est vrai.
- Vérifie que `scene.size()` vaut `2U`.
- Vérifie que `sprites` vaut `1`.
- Vérifie que `losanges` vaut `1`.

### MaquetteRenderTest.SansAplatRienNEstCompose

*Majeur · Unitaire · Rendu de maquette* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:532`

Sans aplat, la maquette ne compose rien.

**Étapes**

1. Composer une carte sans lieu avec une table de textures sans aplat.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `0U`.

### MaquetteRenderTest.UnMurSeComposeEnBlocDeTroisFaces

*Critique · Unitaire · Rendu de maquette* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:551`

Un mur se compose en bloc de trois faces, haut d'une case.

**Étapes**

1. Composer une carte d'une seule case de mur, sans lieu.

**Résultat attendu**

- Vérifie que `scene.size()` vaut `3U`.
- Vérifie que `quad.layer` vaut `hmi::RenderLayer::Object`.
- Vérifie que `quad.kind` vaut `hmi::QuadKind::Poly`.
- Vérifie que `premiere` diffère de `deuxieme`.
- Vérifie que `deuxieme` diffère de `troisieme`.
- Vérifie que `plusHaut` vaut `bounds.position.y - bounds.size.y` (comparaison flottante).

### MaquetteRenderTest.LEauProfondeNeSExtrudePas

*Majeur · Unitaire · Rendu de maquette* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:597`

L'eau profonde reste un losange plat, plus sombre que l'eau vive.

**Étapes**

1. Interroger l'extrusion et la palette pour l'eau profonde.

**Résultat attendu**

- Vérifie que `hmi::maquetteExtrudes(core::TileType::DeepWater)` est faux.
- Vérifie que `hmi::maquetteExtrudes(core::TileType::Wall)` est vrai.
- Vérifie que `hmi::maquetteExtrudes(core::TileType::Solid)` est vrai.
- Vérifie que `hmi::maquetteExtrudes(core::TileType::Cliff)` est vrai.
- Vérifie que `profonde.r + profonde.g + profonde.b` est strictement inférieur à `vive.r + vive.g + vive.b`.

### MaquetteRenderTest.LaCouleurDuJetonSeDeduitDeLEntite

*Critique · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:619`

La couleur d'un jeton se deduit de ce que le format dit deja.

**Étapes**

1. Poser un PNJ avec dialogue, un sans, un avec figurine, une rencontre, deux entrees d'arene, un point d'apparition, un portail et un coffre.
2. En tirer les marques.

**Résultat attendu**

- Vérifie que `marques.tokens.size()` vaut `8U`.
- Vérifie que `marques.tokens[0].kind` vaut `hmi::MaquetteTokenKind::Talker`.
- Vérifie que `marques.tokens[0].letter` vaut `'M'`.
- Vérifie que `marques.tokens[1].kind` vaut `hmi::MaquetteTokenKind::Neutral`.
- Vérifie que `marques.tokens[1].letter` vaut `'N'`.
- Vérifie que `marques.tokens[2].kind` vaut `hmi::MaquetteTokenKind::Hostile`.
- Vérifie que `marques.tokens[2].letter` vaut `'W'`.
- Vérifie que `marques.tokens[3].kind` vaut `hmi::MaquetteTokenKind::Hostile`.
- Vérifie que `marques.tokens[4].kind` vaut `hmi::MaquetteTokenKind::Player`.
- Vérifie que `marques.tokens[5].kind` vaut `hmi::MaquetteTokenKind::Player`.
- Vérifie que `marques.tokens[5].letter` vaut `'G'`.
- Vérifie que `marques.tokens[6].kind` vaut `hmi::MaquetteTokenKind::Portal`.
- Vérifie que `marques.tokens[6].letter` vaut `'A'`.
- Vérifie que `marques.tokens[6].arrow` est vrai.
- Vérifie que `marques.tokens[7].kind` vaut `hmi::MaquetteTokenKind::Object`.

### MaquetteRenderTest.LesTracesNeParaissentQuEnMaquette

*Critique · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:669`

Une carte habillee garde ses jetons mais perd ses traces.

**Étapes**

1. Tirer les marques d'un portail, d'une zone de combat et d'un trajet, en maquette puis hors maquette.

**Résultat attendu**

- Vérifie que `maquette.tokens.size()` vaut `1U`.
- Vérifie que `maquette.tokens.front().arrow` est vrai.
- Vérifie que `maquette.traces.size()` vaut `2U`.
- Vérifie que `maquette.traces[0].shape` vaut `hmi::MaquetteTraceShape::Outline`.
- Vérifie que `maquette.traces[0].cells.size()` vaut `6U`.
- Vérifie que `maquette.traces[1].shape` vaut `hmi::MaquetteTraceShape::Path`.
- Vérifie que `habillee.tokens.size()` vaut `1U`.
- Vérifie que `habillee.tokens.front().arrow` est faux.
- Vérifie que `habillee.traces.empty()` est vrai.

### MaquetteRenderTest.LesCheminsContiennentLesJetons

*Majeur · Unitaire · Jetons de maquette* — `Source/Test/Unit/HMI/Graphics/test_world_scene_composer.cpp:712`

Les chemins de textures d'une carte contiennent ceux de ses jetons.

**Étapes**

1. Batir une carte sans lieu portant une rencontre.
2. Lister ses chemins de texture.

**Résultat attendu**

- Vérifie que `std::ranges::find(chemins, hmi::maquetteTokenPath(hmi::MaquetteTokenKind::Hostile, 'W'))` diffère de `chemins.end()`.

## test_world_scene_renderer.cpp

### WorldSceneRendererTest.CreationLiberationRecreation

*Bloquant · Unitaire · Rendu QRhi d'un lieu* — `Source/Test/Unit/HMI/Graphics/test_world_scene_renderer.cpp:134`

Le cycle de vie des ressources QRhi d'un lieu est sur.

**Étapes**

1. Creer les ressources hors ecran, sans dessiner.
2. Liberer, puis liberer encore.
3. Recreer, dessiner une image d'un lieu, detruire le rendu avant l'interface.

**Résultat attendu**

- Vérifie que `renderer.ensureResources(nullptr)` est faux.
- Vérifie que `renderer.created()` est faux.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `renderer.created()` est vrai.
- Vérifie que `renderer.rhi()` vaut `rhi.get()`.
- Vérifie que `renderer.textures().missing.texture` diffère de `nullptr`.
- Vérifie que `renderer.textures().byPath.empty()` est vrai.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `renderer.textures().byPath.empty()` est faux.
- Vérifie que `renderer.textures().byPath.size()` vaut `chargees`.
- Vérifie que `renderer.requested().size()` vaut `demandees`.
- Vérifie que `renderer.created()` est faux.
- Vérifie que `renderer.rhi()` vaut `nullptr`.
- Vérifie que `renderer.textures().byPath.empty()` est vrai.
- Vérifie que `renderer.requested().empty()` est vrai.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `image.size()` vaut `QSize(TARGET_SIZE, TARGET_SIZE)`.

### WorldSceneRendererTest.UnLieuDevientDesPixels

*Bloquant · Unitaire · Rendu QRhi d'un lieu* — `Source/Test/Unit/HMI/Graphics/test_world_scene_renderer.cpp:194`

Le donjon d'essai se dessine, sans une seule piece manquante.

**Étapes**

1. Charger la carte du donjon d'essai et la table d'apparence du lieu.
2. Dessiner une image hors ecran, cadree sur le heros a la porte.

**Résultat attendu**

- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `image.size()` vaut `QSize(TARGET_SIZE, TARGET_SIZE)`.
- Vérifie que `renderer.composed().size()` est strictement supérieur à `700U`.
- Vérifie que `quad.texture` diffère de `nullptr`.
- Vérifie que `quad.texture` diffère de `renderer.textures().missing.texture`.
- Vérifie que `paintedPixels(image)` est strictement supérieur à `static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE / 4)`.
- Vérifie que `renderer.composed().size()` vaut `0U`.
- Vérifie que `paintedPixels(empty)` vaut `0U`.

### WorldSceneRendererTest.LaCameraSuitLeHerosSansSortirDeLaCarte

*Critique · Unitaire · Rendu QRhi d'un lieu* — `Source/Test/Unit/HMI/Graphics/test_world_scene_renderer.cpp:236`

Le cadrage d'un lieu suit le heros, borne a la scene, a un agrandissement entier.

**Étapes**

1. Cadrer une grande carte sur son centre, puis sur un coin.
2. Cadrer une carte plus petite que la vue.

**Résultat attendu**

- Vérifie que `suivie.zoom()` vaut `1.0F` (comparaison flottante).
- Vérifie que `suivie.center().x` vaut `centre.x`, à `0.001F` près.
- Vérifie que `suivie.center().y` vaut `centre.y`, à `0.001F` près.
- Vérifie que `bornee.center().x` est strictement supérieur à `coin.x`.
- Vérifie que `bornee.visibleBounds().position.x` est supérieur ou égal à `-0.001F`.
- Vérifie que `bornee.visibleBounds().position.x + bornee.visibleBounds().size.x` est inférieur ou égal à `grande.sceneSize().x + 0.001F`.
- Vérifie que `petiteVue.zoom()` vaut `2.0F` (comparaison flottante).
- Vérifie que `petiteVue.center().x` vaut `petite.sceneSize().x / 2.0F`, à `0.001F` près.
- Vérifie que `petiteVue.center().y` vaut `petite.sceneSize().y / 2.0F`, à `0.001F` près.

### WorldSceneRendererTest.DeuxLieuxDeviennentDesPixels

*Bloquant · Unitaire · Rendu QRhi d'un lieu* — `Source/Test/Unit/HMI/Graphics/test_world_scene_renderer.cpp:276`

Deux lieux se dessinent sans une piece sur le damier, sentinelles sous les traits de leur figurine.

**Étapes**

1. Charger deux cartes d'essai, et la table du lieu.
2. Dessiner chacune hors ecran, cadre sur une sentinelle, le heros a cote.

**Résultat attendu**

- Vérifie que `table.ok()` est vrai.
- Vérifie que `carte.ok()` est vrai.
- Vérifie que `figurines.empty()` est faux.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `quad.texture` diffère de `renderer.textures().missing.texture`.
- Vérifie que `soldat` diffère de `renderer.textures().byPath.end()`.
- Vérifie que `soldat->second.texture` diffère de `nullptr`.
- Vérifie que `soldat->second.texture` diffère de `renderer.textures().missing.texture`.
- Vérifie que `paintedPixels(image)` est strictement supérieur à `static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE / 4)`.

### WorldSceneRendererTest.LesCartesSeSauvegardentEtSeRendentAvecLeurKit

*Critique · Unitaire · Rendu du donjon d'essai* — `Source/Test/Unit/HMI/Graphics/test_world_scene_renderer.cpp:342`

Les trois cartes se sauvegardent et se rendent avec leur kit.

**Étapes**

1. Charger et enregistrer chaque carte ; comparer les textures du jeu et de l’éditeur.

**Résultat attendu**

- Vérifie que `rhi` diffère de `nullptr`.
- Vérifie que `table.ok()` est vrai.
- Vérifie que `loaded.ok()` est vrai.
- Vérifie que `saved.ok()` est vrai.
- Vérifie que `saved.level->layers().size()` vaut `loaded.level->layers().size()`.
- Vérifie que `saved.level->entities().size()` vaut `loaded.level->entities().size()`.
- Vérifie que `renderer.ensureResources(rhi.get())` est vrai.
- Vérifie que `paintedPixels(image)` est strictement supérieur à `static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE / 4)`.
- Vérifie que `quad.texture` diffère de `renderer.textures().missing.texture`.
- Vérifie que `editor.depthOffset` vaut `gpu.depthOffset`.
- Vérifie que `editor.anchor.has_value()` vaut `gpu.anchor.has_value()`.
- Vérifie que `editor.anchor->x` vaut `gpu.anchor->x` (comparaison flottante).
- Vérifie que `editor.anchor->y` vaut `gpu.anchor->y` (comparaison flottante).
- Vérifie que `image.save(QString::fromStdString( std::string(name).substr(std::string(name).find('/') + 1) + "-renderer.png"))` est vrai.

### WorldSceneRendererTest.TousLesPortailsSeTraversent

*Critique · Unitaire · Rendu du donjon d'essai* — `Source/Test/Unit/HMI/Graphics/test_world_scene_renderer.cpp:397`

Tous les portails des cartes se traversent.

**Étapes**

1. Entrer dans chaque carte et traverser chaque portail avec WorldTravel.

**Résultat attendu**

- Vérifie que `travel.enter(id, {})` vaut `core::TravelResult::Moved`.
- Vérifie que `travel.enter(id, {})` vaut `core::TravelResult::Moved`.
- Vérifie que `travel.cross(entity.position, flags)` vaut `core::TravelResult::Moved`.
- Vérifie que `core::isSolid(travel.currentMap()->tileMap().tile(pos.column, pos.row))` est faux.
- Vérifie que `core::portalAt(*travel.currentMap(), pos).has_value()` est faux.
