# Référence du code

Cette partie du site est **engendrée par Doxygen** depuis les commentaires de `Source/` : classes,
espaces de noms, fichiers, et pour chaque fonction son rôle, ses paramètres et sa valeur de retour.
C'est l'**annexe du guide** : le guide explique *comment le moteur marche* ; la référence dit *ce que
chaque symbole fait exactement*.

## S'y retrouver

| Pour trouver… | Aller à… |
|---|---|
| une classe ou une structure par son nom | [Classes](annotated.html), ou la recherche en haut à droite |
| ce que contient un module (`core::`, `hmi::`) | [Espaces de noms](namespaces.html) |
| le contenu d'un fichier d'en-tête | [Fichiers](files.html) |
| qui hérite de qui | [Hiérarchie des classes](hierarchy.html) |

## Les modules

| Espace de noms | Dossier | Ce qu'il porte |
|---|---|---|
| `core` | `Source/Core/` | La simulation, sans fenêtre ni GPU : ECS, cartes, règles d20, combat, monde, données. |
| `hmi` | `Source/HMI/` | La présentation : rendu QRhi, entrées, audio, interface Qt Quick, modes de jeu. |
| `hmi` et `core` | `Source/Editor/` | L'éditeur de cartes (`LevelEditor`) : document, gestes, canevas, contrôle du contenu. |

Le sens des dépendances est `hmi → core`, jamais l'inverse
(`EX-ARCH-001`, dans les spécifications).

## Convention de commentaire

```cpp
/**
 * @brief Description courte.
 * @param nom Description du paramètre.
 * @return Description de la valeur de retour.
 */
```

La génération échoue au moindre avertissement (`WARN_AS_ERROR`) : un `@param` oublié ou en conflit
casse le job `docs` de la CI. La lancer avant d'ouvrir une PR : `python scripts/build_docs.py`.
