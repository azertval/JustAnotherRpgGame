+++
id = "LOT-03"
titre = "Agrégat `LevelData`"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "S"
resume = "Un niveau se construit depuis un agrégat nommé, ce qui laisse le RPG y ajouter ses champs sans repasser par un constructeur positionnel illisible."
prerequis = ["LOT-02"]
livrables = [
  "`struct LevelData`, agrégat public dans `Source/Core/Levels/Level.h`, chaque champ documenté.",
  "`core::Level` construit depuis un `LevelData&&` ; constructeur positionnel retiré d'emblée, sans étape `[[deprecated]]`.",
  "Les dix sites de construction adaptés (un dans `LevelLoader`, neuf dans les tests).",
  "Exigence `EX-LVL-*` nouvelle : « un niveau se construit depuis un agrégat nommé ».",
]
criteres = [
  "Plus aucun appel au constructeur positionnel ; le constructeur `[[deprecated]]` a disparu.",
  "Refactoring à **comportement constant** : les tests passent, adaptés mécaniquement (la forme de l'appel change, jamais l'intention du test).",
  "Doxygen vert (chaque champ de `LevelData` documenté, plus de `@param` orphelin).",
]
+++

## Pourquoi

Remplacer le constructeur positionnel de `core::Level` par un agrégat nommé, **avant** que le RPG
n'y ajoute ses propres champs.

### Le problème

Le constructeur de `core::Level` comptait **19 paramètres** ; le `LOT-01` l'a ramené à 11 en
retirant le gameplay de plateforme, mais la dette de fond demeure — elle est actée dans l'en-tête
lui-même (`Source/Core/Levels/Level.h`). Le RPG va rajouter au moins six champs : couches de
tuiles (`LOT-04`), entités placées, connexions de carte, zones de rencontre, points d'apparition,
drapeaux. On repasserait au-dessus de 17 paramètres positionnels, où deux `std::optional<std::string>`
voisins s'intervertissent sans que le compilateur bronche.

Solder maintenant, alors que la liste est au plus court, coûte le minimum.

## Périmètre

- `struct LevelData` : agrégat public portant les mêmes champs, dans `Source/Core/Levels/Level.h`.
- `Level` construit depuis un `LevelData&&`. Les *designated initializers* C++20 rendent chaque
  site de construction lisible (`.name = …, .tileMap = …`).
- Constructeur positionnel **retiré d'emblée**, sans passer par l'étape `[[deprecated]]` prévue :
  la purge du `LOT-01` avait déjà ramené la liste de 19 à 11 paramètres, et il ne restait que
  **dix sites de construction** (un dans `LevelLoader`, neuf dans les tests). Avec si peu
  d'appels, la béquille coûtait plus qu'elle ne rapportait — et le lot se termine sans API
  dépréciée à nettoyer plus tard.

## Conception

### La décision qui porte le lot

**`tileMap` n'a volontairement pas de valeur par défaut.** `core::TileMap` n'étant pas
constructible par défaut, l'omettre dans l'agrégat est une **erreur de compilation**, jamais une
grille vide silencieuse — exactement la propriété qu'on attend d'un champ obligatoire. Tous les
autres champs ont un défaut utile, si bien qu'un site minimal tient en deux lignes :

@code
core::Level level(core::LevelData{.name = "village", .tileMap = std::move(map)});
@endcode

## Exigences couvertes

`EX-LVL-*` nouvelle (« un niveau se construit depuis un agrégat nommé »), `EX-ARCH-*`.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, 899 tests verts
sans modification de l'intention d'un seul test, format et linters verts. Le prérequis `LOT-02`
était un confort d'outillage, pas une dépendance stricte.
