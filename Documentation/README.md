# Documentation/

Documentation technique au format **Doxygen** (outils et code).

## Objet
Expliquer *comment* le code et les outils fonctionnent : modules, classes, fonctions, pipeline de build, dépendances DirectX.

## Mise en place
- Le `Doxyfile` de ce dossier configure la génération ; il ne se lance pas directement mais via
  `python scripts/build_docs.py`, qui y injecte le numéro de version lu dans le `CMakeLists.txt`
  racine.
- Le code source de `../Source/` porte les balises Doxygen (`@brief`, `@param`, `@return`,
  `@file`). `WARN_AS_ERROR = FAIL_ON_WARNINGS` : un renvoi cassé ou un `@param` en conflit fait
  échouer la génération, donc la CI.
- La sortie HTML est écrite dans `generated/`, ignoré par Git.

## Convention de commentaire
```cpp
/**
 * @brief Description courte.
 * @param nom Description du paramètre.
 * @return Description de la valeur de retour.
 */
```

## Conventions de code
Les règles de style, de nommage et d'architecture sont détaillées dans [`Specification/conventions.md`](Specification/conventions.md).

## Cartes jouables

- [Arena of Brave — carte du Colisée](arena-of-brave-map.md) : accès, édition et vérifications.
