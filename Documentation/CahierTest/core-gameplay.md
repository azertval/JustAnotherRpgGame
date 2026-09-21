# Core · Gameplay

Tests unitaires — **9 cas** (5 critiques, 3 majeurs, 1 mineur). [Retour à la synthèse](README.md).

## test_interaction.cpp

### InteractionTest.LaCaseViseeSuitLaDirectionDominante

*Majeur · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:69`

La case visee suit la direction dominante de l'orientation.

**Étapes**

1. Viser avec quatre orientations cardinales, puis une orientation a 30 degres.
2. Viser avec une orientation nulle.

**Résultat attendu**

- Vérifie que `core::aimedCell(depart, {1.0F, 0.0F})` vaut `(core::GridPosition{3, 2})`.
- Vérifie que `core::aimedCell(depart, {-1.0F, 0.0F})` vaut `(core::GridPosition{1, 2})`.
- Vérifie que `core::aimedCell(depart, {0.0F, 1.0F})` vaut `(core::GridPosition{2, 3})`.
- Vérifie que `core::aimedCell(depart, {0.0F, -1.0F})` vaut `(core::GridPosition{2, 1})`.
- Vérifie que `core::aimedCell(depart, {0.866F, 0.5F})` vaut `(core::GridPosition{3, 2})`.
- Vérifie que `core::aimedCell(depart, {0.5F, 0.866F})` vaut `(core::GridPosition{2, 3})`.
- Vérifie que `core::aimedCell(depart, {0.0F, 0.0F})` vaut `depart`.

### InteractionTest.UnCoffreNeDonneSonButinQuUneFois

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:97`

Un coffre ne donne son butin qu'a la premiere ouverture.

**Étapes**

1. Interagir deux fois avec le meme coffre.

**Résultat attendu**

- Vérifie que `premiere.found()` est vrai.
- Vérifie que `butin.happened` est vrai.
- Vérifie que `butin.consumed` est vrai.
- Vérifie que `butin.type` vaut `"chest"`.
- Vérifie que `seconde.found()` est faux.
- Vérifie que `rien.happened` est faux.

### InteractionTest.LEtatDUnCoffreSurvitAUnAllerRetourDeCarte

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:128`

Un coffre ouvert le reste apres avoir quitte la carte et y etre revenu.

**Étapes**

1. Ouvrir un coffre, puis DETRUIRE les entites de la carte et les recreer depuis la couche objects, comme le fait un changement de carte.

**Résultat attendu**

- Vérifie que `core::spawnMapEntities(monde, niveau, niveau.name())` vaut `1U`.
- Vérifie que `objets.size()` vaut `1U`.
- Vérifie que `cible.found()` est vrai.
- Vérifie que `core::interact(cible, drapeaux).consumed` est vrai.
- Vérifie que `core::spawnMapEntities(monde, niveau, niveau.name())` vaut `1U`.
- Vérifie que `objets.size()` vaut `1U`.
- Vérifie que `cible.found()` est faux.

### InteractionTest.LInteractionNeTraversePasUnMur

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:184`

Un objet place sur une case pleine n'est pas atteignable.

**Étapes**

1. Poser un coffre sur une case, puis rendre cette case pleine.

**Résultat attendu**

- Vérifie que `core::findInteractionTarget({2, 2}, {1.0F, 0.0F}, carte, candidats(objets), drapeaux) .found()` est vrai.
- Vérifie que `derriereLeMur.found()` est faux.
- Vérifie que `derriereLeMur.aimedCell` vaut `(core::GridPosition{3, 2})`.

### InteractionTest.LaCibleEstDeterministeAEgalite

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:211`

Deux objets sur la meme case designent toujours le meme.

**Étapes**

1. Poser deux objets sur la case visee et designer la cible dix fois.

**Résultat attendu**

- Vérifie que `cible.found()` est vrai.
- Vérifie que `cible.index` vaut `0U`.
- Vérifie que `cible.interactable->type` vaut `"chest"`.

### InteractionTest.UnPanneauSeRelitIndefiniment

*Majeur · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:236`

Une entite non consommable reste une cible apres interaction.

**Étapes**

1. Interagir trois fois avec un panneau.

**Résultat attendu**

- Vérifie que `cible.found()` est vrai.
- Vérifie que `lecture.happened` est vrai.
- Vérifie que `lecture.consumed` est faux.
- Vérifie que `lecture.promptKey` vaut `"interaction.sign"`.
- Vérifie que `drapeaux.size()` vaut `0U`.

### InteractionTest.DeuxCartesNeSeMarchentPasDessus

*Critique · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:262`

Deux coffres de cartes differentes a la meme case ont des drapeaux distincts.

**Étapes**

1. Fabriquer la cle de deux coffres a la case (3, 2), sur deux cartes.
2. Ouvrir le premier.

**Résultat attendu**

- Vérifie que `cleVillage` diffère de `cleDonjon`.
- Vérifie que `cible.found()` est vrai.
- Vérifie que `core::interact(cible, drapeaux).consumed` est vrai.
- Vérifie que `core::findInteractionTarget({2, 2}, {1.0F, 0.0F}, carte, candidats(auDonjon), drapeaux) .found()` est vrai.

### InteractionTest.UnTypeInconnuProduitUneEntiteNonInteractive

*Majeur · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:294`

Un objet de type inconnu apparait sur la carte sans etre interactif.

**Étapes**

1. Peupler un monde depuis une couche objects portant un type inconnu.

**Résultat attendu**

- Vérifie que `core::spawnMapEntities(monde, niveau, niveau.name())` vaut `2U`.
- Vérifie que `interactif.type` vaut `"chest"`.
- Vérifie que `interactifs` vaut `1U`.

### InteractionTest.LesDrapeauxSeRelisentTries

*Mineur · Unitaire · Interaction* — `Source/Test/Unit/Core/Gameplay/test_interaction.cpp:322`

Les drapeaux acquis se relisent dans un ordre stable.

**Étapes**

1. Lever trois drapeaux dans le desordre, puis les relire.
2. Lever deux fois le meme.

**Résultat attendu**

- Vérifie que `drapeaux.set("village/chest@3,2")` est vrai.
- Vérifie que `drapeaux.set("donjon/chest@1,1")` est vrai.
- Vérifie que `drapeaux.set("village/chest@0,0")` est vrai.
- Vérifie que `drapeaux.set("village/chest@3,2")` est faux.
- Vérifie que `drapeaux.size()` vaut `3U`.
- Vérifie que `tous.size()` vaut `3U`.
- Vérifie que `std::is_sorted(tous.begin(), tous.end())` est vrai.
- Vérifie que `drapeaux.isSet("donjon/chest@1,1")` est faux.
- Vérifie que `drapeaux.size()` vaut `2U`.
