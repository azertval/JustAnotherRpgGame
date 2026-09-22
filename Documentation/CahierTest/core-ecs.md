# Core · Ecs

Tests unitaires — **35 cas** (5 critiques, 30 majeurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_animation_clip.cpp`](#test-animation-clipcpp) | 7 | - | 5 | 2 | - |
| [`test_component_pool.cpp`](#test-component-poolcpp) | 7 | - | - | 7 | - |
| [`test_entity_manager.cpp`](#test-entity-managercpp) | 6 | - | - | 6 | - |
| [`test_sprite.cpp`](#test-spritecpp) | 3 | - | - | 3 | - |
| [`test_view.cpp`](#test-viewcpp) | 5 | - | - | 5 | - |
| [`test_world.cpp`](#test-worldcpp) | 7 | - | - | 7 | - |

## test_animation_clip.cpp

### AnimationClipTest.ClipExistantResoluParNom

*Critique · Unitaire · Clip d'animation* — `Source/Test/Unit/Core/Ecs/test_animation_clip.cpp:32`

Un clip existant est résolu par son nom.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `clips.clipCount()` vaut `2`.
- Vérifie que `clips.indexOf("idle")` est supérieur ou égal à `0`.
- Vérifie que `clips.indexOf("run")` est supérieur ou égal à `0`.
- Vérifie que `clips.clipAt(clips.indexOf("idle")).name` vaut `"idle"`.
- Vérifie que `clips.clipAt(clips.indexOf("run")).name` vaut `"run"`.

### AnimationClipTest.ClipInexistantRepliDeterministe

*Critique · Unitaire · Clip d'animation* — `Source/Test/Unit/Core/Ecs/test_animation_clip.cpp:55`

Un nom inconnu ou un index hors bornes retombe sur le premier clip, sans plantage.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `clips.indexOf("inconnu")` vaut `-1`.
- Vérifie que `clips.clipAt(-1).name` vaut `"idle"`.
- Vérifie que `clips.clipAt(42).name` vaut `"idle"`.

### AnimationClipTest.JeuVideRepliSurUnClipParDefaut

*Critique · Unitaire · Clip d'animation* — `Source/Test/Unit/Core/Ecs/test_animation_clip.cpp:78`

Un jeu de clips vide se replie sur un clip par défaut portant au moins une image.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `clips.clipCount()` vaut `0`.
- Vérifie que `fallback.name.empty()` est vrai.
- Vérifie que `fallback.frames.empty()` est faux.
- Vérifie que `fallback.frameDuration` vaut `0.0f`.

### AnimationClipTest.ClipAUneSeuleImage

*Majeur · Unitaire · Clip d'animation* — `Source/Test/Unit/Core/Ecs/test_animation_clip.cpp:98`

Un clip d'une seule image est un clip valide.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `clip.frames.size()` vaut `1u`.

### AnimationClipTest.DureesInegalesEntreClips

*Majeur · Unitaire · Clip d'animation* — `Source/Test/Unit/Core/Ecs/test_animation_clip.cpp:115`

Chaque clip garde sa propre durée d'image.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `clips.clipAt(clips.indexOf("idle")).frameDuration` vaut `0.5f` (comparaison flottante).
- Vérifie que `clips.clipAt(clips.indexOf("run")).frameDuration` vaut `0.1f` (comparaison flottante).

### AnimationClipTest.ClipJoueUneFoisAvecClipSuivant

*Critique · Unitaire · Clip d'animation* — `Source/Test/Unit/Core/Ecs/test_animation_clip.cpp:135`

Un clip joué une fois désigne un clip suivant, résolu dans le même jeu.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `opening.endMode` vaut `core::ClipEndMode::OneShot`.
- Vérifie que `opening.nextClip` vaut `"open"`.
- Vérifie que `clips.indexOf(opening.nextClip)` est supérieur ou égal à `0`.

### AnimationClipTest.AjouterUnClipDeMemeNomLeRemplace

*Critique · Unitaire · Clip d'animation* — `Source/Test/Unit/Core/Ecs/test_animation_clip.cpp:157`

Ajouter un clip de même nom remplace l'existant, à index constant.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `clips.clipCount()` vaut `1`.
- Vérifie que `clips.indexOf("idle")` vaut `firstIndex`.
- Vérifie que `clips.clipAt(firstIndex).frames.size()` vaut `3u`.

## test_component_pool.cpp

### ComponentPoolTest.AjoutPuisAcces

*Majeur · Unitaire · Component Pool* — `Source/Test/Unit/Core/Ecs/test_component_pool.cpp:28`

`add` puis `get` renvoie la valeur stockée ; `has` est cohérent.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `pool.has(entity)` est faux.
- Vérifie que `pool.has(entity)` est vrai.
- Vérifie que `pool.get(entity).x` vaut `3`.
- Vérifie que `pool.get(entity).y` vaut `4`.
- Vérifie que `pool.size()` vaut `1u`.

### ComponentPoolTest.GetRenvoieReferenceModifiable

*Majeur · Unitaire · Component Pool* — `Source/Test/Unit/Core/Ecs/test_component_pool.cpp:52`

`get` renvoie une référence modifiable sur le composant stocké.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `pool.get(entity).x` vaut `42`.

### ComponentPoolTest.RemoveAuMilieuSwapAndPop

*Majeur · Unitaire · Component Pool* — `Source/Test/Unit/Core/Ecs/test_component_pool.cpp:74`

`remove` d'un élément au milieu (swap-and-pop) laisse les autres composants accessibles et corrects, et le stockage dense reste contigu.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `pool.has(middle)` est faux.
- Vérifie que `pool.has(first)` est vrai.
- Vérifie que `pool.has(last)` est vrai.
- Vérifie que `pool.get(first)` vaut `10`.
- Vérifie que `pool.get(last)` vaut `30`.
- Vérifie que `pool.size()` vaut `2u`.
- Vérifie que `pool.components().size()` vaut `2u`.
- Vérifie que `pool.entities().size()` vaut `2u`.

### ComponentPoolTest.RemoveDernierElement

*Majeur · Unitaire · Component Pool* — `Source/Test/Unit/Core/Ecs/test_component_pool.cpp:110`

Retirer le dernier élément ne perturbe pas les précédents.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `pool.has(a)` est vrai.
- Vérifie que `pool.has(b)` est faux.
- Vérifie que `pool.get(a)` vaut `1`.
- Vérifie que `pool.size()` vaut `1u`.

### ComponentPoolTest.HandlePerimeNePossedePasLeComposant

*Majeur · Unitaire · Component Pool* — `Source/Test/Unit/Core/Ecs/test_component_pool.cpp:137`

Un handle périmé (index recyclé, génération différente) ne possède pas le composant de l'ancienne entité.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `pool.has(original)` est vrai.
- Vérifie que `pool.has(recycled)` est faux.

### ComponentPoolTest.RemoveIfPresent

*Majeur · Unitaire · Component Pool* — `Source/Test/Unit/Core/Ecs/test_component_pool.cpp:164`

`removeIfPresent` retire si le composant existe, sinon ne fait rien.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `pool.removeIfPresent(entity)` est faux.
- Vérifie que `pool.removeIfPresent(entity)` est vrai.
- Vérifie que `pool.has(entity)` est faux.

### ComponentPoolTest.GetSurEntiteAbsenteViolePrecondition

*Majeur · Unitaire · Component Pool* — `Source/Test/Unit/Core/Ecs/test_component_pool.cpp:185`

`get` sur une entité absente viole une précondition (assertion en Debug).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que l'opération lève bien une exception `std::runtime_error`.

## test_entity_manager.cpp

### EntityManagerTest.CreeEntitesVivantesEtDistinctes

*Majeur · Unitaire · Entity Manager* — `Source/Test/Unit/Core/Ecs/test_entity_manager.cpp:16`

`create` renvoie des entités vivantes et distinctes.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `manager.isAlive(a)` est vrai.
- Vérifie que `manager.isAlive(b)` est vrai.
- Vérifie que `a` diffère de `b`.
- Vérifie que `manager.aliveCount()` vaut `2u`.

### EntityManagerTest.DestructionInvalideLeHandle

*Majeur · Unitaire · Entity Manager* — `Source/Test/Unit/Core/Ecs/test_entity_manager.cpp:37`

Après destruction, l'ancien handle n'est plus vivant.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `manager.isAlive(entity)` est faux.
- Vérifie que `manager.aliveCount()` vaut `0u`.

### EntityManagerTest.RecyclageChangeLaGeneration

*Majeur · Unitaire · Entity Manager* — `Source/Test/Unit/Core/Ecs/test_entity_manager.cpp:58`

Un index recyclé produit une génération différente : l'ancien handle reste invalide, le nouveau est valide.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `recycled.index` vaut `first.index`.
- Vérifie que `recycled.generation` diffère de `first.generation`.
- Vérifie que `manager.isAlive(first)` est faux.
- Vérifie que `manager.isAlive(recycled)` est vrai.

### EntityManagerTest.DestructionHandlePerimeSansEffet

*Majeur · Unitaire · Entity Manager* — `Source/Test/Unit/Core/Ecs/test_entity_manager.cpp:86`

Détruire un handle périmé est sans effet (idempotent) et ne touche pas l'entité vivante qui occupe désormais le même index.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `manager.isAlive(live)` est vrai.
- Vérifie que `manager.aliveCount()` vaut `1u`.

### EntityManagerTest.EntiteInvalideJamaisVivante

*Majeur · Unitaire · Entity Manager* — `Source/Test/Unit/Core/Ecs/test_entity_manager.cpp:110`

L'entité invalide conventionnelle n'est jamais vivante.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `manager.isAlive(core::INVALID_ENTITY)` est faux.
- Vérifie que `manager.isAlive(created)` est vrai.
- Vérifie que `manager.isAlive(core::INVALID_ENTITY)` est faux.

### EntityManagerTest.EgaliteHandleInvalide

*Majeur · Unitaire · Entity Manager* — `Source/Test/Unit/Core/Ecs/test_entity_manager.cpp:128`

Le handle invalide se compare comme tel.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `core::INVALID_ENTITY` vaut `core::INVALID_ENTITY`.
- Vérifie que `core::INVALID_ENTITY.index` vaut `core::Entity::INVALID_INDEX`.

## test_sprite.cpp

### SpriteTest.ValeursParDefaut

*Majeur · Unitaire · Sprite* — `Source/Test/Unit/Core/Ecs/test_sprite.cpp:15`

Un sprite par défaut a une teinte blanche opaque, la couche 0 et une région nulle.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `sprite.layer` vaut `0`.
- Vérifie que `sprite.region.x` vaut `0`.
- Vérifie que `sprite.region.y` vaut `0`.
- Vérifie que `sprite.region.width` vaut `0`.
- Vérifie que `sprite.region.height` vaut `0`.
- Vérifie que `sprite.tint.r` vaut `1.0f` (comparaison flottante).
- Vérifie que `sprite.tint.g` vaut `1.0f` (comparaison flottante).
- Vérifie que `sprite.tint.b` vaut `1.0f` (comparaison flottante).
- Vérifie que `sprite.tint.a` vaut `1.0f` (comparaison flottante).

### SpriteTest.ChampsAssignables

*Majeur · Unitaire · Sprite* — `Source/Test/Unit/Core/Ecs/test_sprite.cpp:40`

Les champs sont librement assignables (donnée pure).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `sprite.region.x` vaut `16`.
- Vérifie que `sprite.region.y` vaut `32`.
- Vérifie que `sprite.region.width` vaut `16`.
- Vérifie que `sprite.region.height` vaut `16`.
- Vérifie que `sprite.layer` vaut `2`.
- Vérifie que `sprite.tint.g` vaut `0.5f` (comparaison flottante).
- Vérifie que `sprite.tint.a` vaut `0.8f` (comparaison flottante).

### SpriteTest.CopieValeur

*Majeur · Unitaire · Sprite* — `Source/Test/Unit/Core/Ecs/test_sprite.cpp:65`

Le composant est utilisable comme un composant d'ECS (stockage/copie de données pures).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `copy.layer` vaut `5`.
- Vérifie que `copy.region.width` vaut `16`.
- Vérifie que `copy.tint.r` vaut `0.2f` (comparaison flottante).

## test_view.cpp

### ViewTest.SelectionneUniquementLIntersection

*Majeur · Unitaire · View* — `Source/Test/Unit/Core/Ecs/test_view.cpp:30`

Une vue <A, B> itère exactement les entités possédant A et B.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `visited.size()` vaut `1u`.
- Vérifie que `visited.front()` vaut `both`.

### ViewTest.ComposantsCorrespondentALEntite

*Majeur · Unitaire · View* — `Source/Test/Unit/Core/Ecs/test_view.cpp:61`

Les composants fournis par la vue correspondent bien à l'entité itérée.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `velocity.value` vaut `position.value * 10`.
- Vérifie que `position.value` vaut `1`.
- Vérifie que `position.value` vaut `2`.

### ViewTest.ModificationViaVueEstVisible

*Majeur · Unitaire · View* — `Source/Test/Unit/Core/Ecs/test_view.cpp:95`

La modification d'un composant via la vue est visible ensuite (référence).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `positions.get(entity).value` vaut `8`.

### ViewTest.VueVideNIterePas

*Majeur · Unitaire · View* — `Source/Test/Unit/Core/Ecs/test_view.cpp:121`

Une vue sans entité correspondante s'itère sans erreur (aucune visite).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `visits` vaut `0`.

### ViewTest.IterationForEtEachCoherentes

*Majeur · Unitaire · View* — `Source/Test/Unit/Core/Ecs/test_view.cpp:148`

L'itération par `for` visite les mêmes entités que `each`.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `fromEach.size()` vaut `2u`.
- Vérifie que `fromFor` vaut `fromEach`.

## test_world.cpp

### WorldTest.CycleComposant

*Majeur · Unitaire · World* — `Source/Test/Unit/Core/Ecs/test_world.cpp:56`

Le cycle add/has/get/remove d'un composant est cohérent via le `World`.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `world.hasComponent<Health>(entity)` est faux.
- Vérifie que `world.hasComponent<Health>(entity)` est vrai.
- Vérifie que `world.getComponent<Health>(entity).value` vaut `100`.
- Vérifie que `world.getComponent<Health>(entity).value` vaut `60`.
- Vérifie que `world.hasComponent<Health>(entity)` est faux.

### WorldTest.HasComponentSansPool

*Majeur · Unitaire · World* — `Source/Test/Unit/Core/Ecs/test_world.cpp:83`

`hasComponent` est faux quand aucune pool du type n'existe encore.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `world.hasComponent<Health>(entity)` est faux.

### WorldTest.DestroyEntityRetireTousLesComposants

*Majeur · Unitaire · World* — `Source/Test/Unit/Core/Ecs/test_world.cpp:99`

`destroyEntity` retire l'entité de toutes les pools et la rend non vivante.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `world.isAlive(entity)` est faux.
- Vérifie que `world.hasComponent<Health>(entity)` est faux.
- Vérifie que `world.hasComponent<Position>(entity)` est faux.

### WorldTest.SystemesExecutesDansLOrdre

*Majeur · Unitaire · World* — `Source/Test/Unit/Core/Ecs/test_world.cpp:122`

Les systèmes enregistrés s'exécutent dans l'ordre d'enregistrement.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `world.systemCount()` vaut `3u`.
- Vérifie que `log` vaut `(std::vector<int>{1, 2, 3})`.

### WorldTest.UpdateNFoisExecuteNFois

*Majeur · Unitaire · World* — `Source/Test/Unit/Core/Ecs/test_world.cpp:145`

`update` appelé N fois exécute chaque système N fois (cadencement déterministe).

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `log.size()` vaut `static_cast<std::size_t>(calls)`.

### WorldTest.FixedDeltaTransmisAuxSystemes

*Majeur · Unitaire · World* — `Source/Test/Unit/Core/Ecs/test_world.cpp:169`

Le `fixedDelta` passé à `update` est transmis tel quel aux systèmes.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `observed` vaut `0.25f` (comparaison flottante).

### WorldTest.ViewViaWorld

*Majeur · Unitaire · World* — `Source/Test/Unit/Core/Ecs/test_world.cpp:189`

La vue exposée par le `World` itère l'intersection des composants.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `visited.size()` vaut `1u`.
- Vérifie que `visited.front()` vaut `both`.
