# Télécharger et lancer le jeu

Le jeu est publié en deux formes, toutes deux **autonomes** (aucune installation
requise : ni Visual Studio, ni Qt, ni redistribuable) :

| Forme | Quand l'utiliser | Où |
|-------|------------------|-----|
| **Version publiée** (`vX.Y.Z`) | Le cas normal : une version arrêtée et annoncée. | [Dernière version](https://github.com/azertval/JustAnotherRpgGame/releases/latest) |
| **Préversion roulante** (`debug-latest`) | Pour essayer le tout dernier état du projet, entre deux versions publiées. | [debug-latest](https://github.com/azertval/JustAnotherRpgGame/releases/tag/debug-latest) |

## Prérequis
- **Windows 64 bits** (x64).

## Étapes
1. Ouvrir la page de la dernière version publiée :
   <https://github.com/azertval/JustAnotherRpgGame/releases/latest>
2. Dans la section **Assets**, télécharger **`JustAnotherRpgGame-vX.Y.Z-release.zip`**.
   (L'archive **`-debug`** de la même version contient les informations de
   débogage : plus lente, mais utile pour signaler un problème — voir plus bas.)
3. **Décompresser** l'archive dans un dossier de votre choix (clic droit →
   *Extraire tout…*).
4. Ouvrir le dossier extrait et double-cliquer sur **`JustAnotherRpgGame.exe`**.

Une fenêtre de jeu s'ouvre. Pour quitter : fermer la fenêtre (croix) ou appuyer
sur **Échap**. Le jeu se joue au clavier et à la souris ; une **manette** (XInput) navigue dans les menus et au
Colisée — voir [Jouer](jouer.md) pour le détail des contrôles.

## Remarques
- Une version publiée (`vX.Y.Z`) ne change plus une fois parue : c'est celle à
  citer pour signaler un problème. La liste des nouveautés de chaque version est
  dans le [CHANGELOG](https://github.com/azertval/JustAnotherRpgGame/blob/main/CHANGELOG.md).
- La version « roulante » (`debug-latest`) correspond toujours au **dernier état
  intégré** du projet ; elle est marquée *pre-release* et peut changer d'un jour à
  l'autre.
- **Pour signaler un problème**, préférez l'archive **`-debug`** : elle produit des
  messages de diagnostic plus précis, qui aident à localiser la cause.
- Windows SmartScreen peut afficher un avertissement pour un exécutable non signé :
  *Informations complémentaires* → *Exécuter quand même*.
