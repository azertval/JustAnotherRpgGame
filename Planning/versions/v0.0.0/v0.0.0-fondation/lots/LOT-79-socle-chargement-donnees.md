+++
id = "LOT-79"
titre = "Socle de chargement de données"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "Une seule routine de lecture JSON, qui annonce le fichier et la ligne de l'erreur, et la capacité de test paramétré sur dossier de fixtures, avant que la filière contenu n'ajoute quinze lecteurs."
prerequis = []
livrables = [
  "`core::JsonDocument` (`Source/Core/Data/JsonDocument.{h,cpp}`) : `readJsonObject`, `readJsonObjectFromFile`, `core::JsonReadError`, `positionOf`.",
  "Les six lecteurs migrés : `hmi::SkinCatalog`, `hmi::AnimationCatalog`, `hmi::SoundCatalog`, `hmi::PixelPalette`, `core::LevelLoader`, `core::LevelSequenceLoader`.",
  "`Source/Test/Fixtures/Json/` (six fixtures) et le premier `TEST_P` de la suite, plus le test des fixtures orphelines.",
  "`nlohmann_json` en dépendance PUBLIC de `Core`.",
]
criteres = [
  "Les six lecteurs passent par la brique, et leurs tests restent verts.",
  "Une donnée invalide produit un message portant le **fichier et la ligne**.",
  "Un test paramétré balaie un dossier de fixtures et échoue en nommant l'entrée fautive.",
  "`ctest` : **943/943** (927 avant le lot, plus 16).",
  "Build `/W4 /WX` sans avertissement, `clang-format` propre, cahier de test régénéré.",
]
+++

## Pourquoi

Écrire **une** routine de lecture JSON avant que la filière contenu n'en ajoute quinze, et créer la
capacité de **test paramétré** que la suite n'avait pas.

Le §10 de la feuille de route attribuait ces deux travaux au `LOT-32` — mais son périmètre ne livre
que des schémas, un script Python et un test d'énumérations : **personne ne portait le travail
C++**. C'est l'audit qui l'a trouvé, et ce lot qui le fait.

## Ce qui existait

Six réimplémentations de `loadFromFile`, chacune avec sa validation écrite à la main :
`hmi::SkinCatalog`, `hmi::AnimationCatalog`, `hmi::SoundCatalog`, `hmi::PixelPalette`,
`core::LevelLoader`, `core::LevelSequenceLoader`. Toutes répétaient la même séquence — `accept()`
puis `parse()`, racine objet, champ de version absent valant 1, version supérieure refusée — et
quatre d'entre elles redéfinissaient les **mêmes cinq catégories d'échec** sous quatre noms
différents.

Aucune ne disait **où** était l'erreur. `nlohmann` ne rapporte qu'un décalage en octets, et les six
lecteurs le jetaient : le message était « JSON malformé », devant un catalogue de mille lignes.

## Périmètre

**`core::JsonDocument`** (`Source/Core/Data/JsonDocument.{h,cpp}`) :

- `readJsonObject` et `readJsonObjectFromFile` portent l'enveloppe commune — JSON bien formé,
  racine objet, garde de version — et ne lèvent jamais (`EX-NFR-040`) ;
- `core::JsonReadError` porte les cinq catégories **une seule fois** ;
- `positionOf` convertit le décalage de `nlohmann` en **ligne et colonne**, si bien qu'un échec
  s'annonce désormais `sounds.json:12:5 : …`. C'est ce qu'`EX-CNT-010` demande, et c'est la seule
  différence qui se voit à l'usage : « JSON malformé » ne se corrige pas, « ligne 12 » si ;
- `supportedVersion = 0` **désactive** la garde de version, pour l'appelant qui porte la sienne avec
  sa propre catégorie d'échec — c'est le cas de `core::LevelLoader`.

**Les six lecteurs migrés.** Chacun garde son énumération publique, documentée pour son format, et
traduit depuis la catégorie partagée par un `switch` **exhaustif et sans `default`** : ajouter une
catégorie d'un côté fait échouer la compilation plutôt que de tomber dans un cas par défaut. C'est
la même discipline que `TileTypeName.cpp`.

**La capacité de test paramétré.** `Source/Test/` ne comptait **aucun** `TEST_P` ni aucun parcours
de dossier de fixtures. Le lot en pose le premier : `Source/Test/Fixtures/Json/` porte six fichiers
— valide, tronqué, virgule en trop, racine tableau, version future, version non entière — et un
`TEST_P` vérifie que chacun produit la catégorie annoncée par son nom. Un test supplémentaire
vérifie qu'**aucune fixture du dossier n'est orpheline** : un fichier de fixture qu'aucun test ne lit
ne protège de rien.

C'est la capacité qui compte plus que ces six cas : un catalogue de 176 créatures se teste en
balayant un dossier, pas en écrivant 176 `TEST`.

## Décisions de réalisation

**`nlohmann_json` devient une dépendance PUBLIC de `Core`.** `JsonDocument.h` expose l'arbre parsé à
ses appelants : la bibliothèque fait donc partie de l'interface de `Core`, et non de son
implémentation. La seule façon de l'éviter aurait été d'écrire une façade typée par-dessus
`nlohmann` — soit un second modèle d'arbre à maintenir, pour ne rien gagner d'observable.

**La brique parse en laissant l'exception se produire, à l'intérieur.** Le mode non-lançant de
`nlohmann` renvoie « document rejeté » sans dire **où**. L'exception est donc capturée dans la
brique pour en extraire la position, et aucune ne franchit la frontière — la garantie d'`EX-NFR-040`
est tenue au même endroit pour les six lecteurs, au lieu de six fois.

**Le défaut de version absente passe de 0 à 1 pour `LevelSequenceLoader`.** Sans effet observable :
la version gérée valant 1, les deux valeurs franchissent la même garde. C'est noté ici parce qu'un
changement sans effet aujourd'hui en a un le jour où la version gérée monte.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 943/943, `clang-format`, lint d'exigences, lint des lots, cahier de test et Doxygen verts ; tous les critères d'acceptation sont cochés dans l'epic d'origine. Ce lot est prérequis du `LOT-32`.
