# Créer et partager une carte (sans ligne de commande)

Ce guide s'adresse aux **créateurs de cartes** sans connaissance en programmation. Il explique
comment créer une carte dans l'éditeur, puis la partager avec le reste de l'équipe via une
interface Git graphique — **jamais de ligne de commande**.

## 1. Récupérer le projet

1. Installer [GitHub Desktop](https://desktop.github.com/) (gratuit).
2. Ouvrir GitHub Desktop, se connecter avec un compte GitHub (en créer un si besoin — gratuit).
3. **File → Clone repository**, choisir `azertval/JustAnotherRpgGame`, puis un dossier sur votre
   ordinateur. Le bouton **Clone** télécharge tout le projet.
4. Récupérer les **images** : elles ne font pas partie de ce que le clone télécharge (elles sont
   trop lourdes pour y vivre, et sont publiées à part). Dans le dossier du projet, lancer une fois
   `python scripts/fetch_assets.py` — ou `scripts/setup_dev.ps1`, qui le fait avec le reste de
   l'installation. Sans cette étape, l'éditeur ouvre les cartes mais les montre **sans texture** :
   des cases nues et des marqueurs à la place des pièces. La même commande se relance quand un
   message signale des images manquantes ou périmées après une mise à jour du projet.

## 2. Lancer l'éditeur

L'éditeur est un programme à part, livré à côté du jeu : **`LevelEditor.exe`**, dans le même
dossier que `JustAnotherRpgGame.exe` — voir [Télécharger et lancer le jeu](telecharger-et-lancer.md).

## 3. Créer une carte

La fenêtre montre la carte ouverte au centre, entourée de quatre panneaux qu'on peut déplacer,
redimensionner ou détacher :

- **Cartes** — les cartes enregistrées, avec un champ de recherche. On y **crée**, **renomme**,
  **duplique** et **supprime** une carte.
- **Palette** — les types de case : vide, plein, entrée, herbe, terre, sable, eau, eau profonde,
  mur, falaise, pont, escalier.
- **Couches** — les couches de la carte (sol, décor) : en ajouter, en retirer, choisir celle qu'on
  peint.
- **Entités** — ce qu'on pose sur la carte : coffre, panneau, PNJ, rencontre, portail, point
  d'arrivée, entrée d'arène…

**F1** affiche la liste complète des raccourcis.

| Action | Comment |
|--------|---------|
| Peindre une case | Choisir un type dans la **palette**, puis cliquer (ou cliquer-glisser) sur la grille avec l'outil **Pinceau**. |
| Remplir une zone | Outil **Rectangle** : cliquer-glisser d'un coin à l'autre, relâcher pour remplir. |
| Copier / coller une zone | Outil **Sélection** : cliquer-glisser pour définir la zone, **Ctrl+C** pour la copier, **Ctrl+V** pour la coller à l'endroit survolé. |
| Placer l'entrée | Choisir *Entrée* dans la palette et cliquer la case voulue. Une carte a **une seule** entrée : l'ancienne se déplace. |
| Poser une entité | Outil **Entité** : choisir sa sorte dans le panneau **Entités**, cliquer la case. Ses champs (portail de destination, dialogue d'un PNJ…) se remplissent dans le panneau. **Suppr** retire l'entité sélectionnée. |
| Déplacer la vue / zoomer | Cliquer-glisser avec le **bouton droit** ; **molette** pour zoomer ; **0** pour revenir au cadrage automatique. |
| Afficher un quadrillage | **F10**. |
| Changer la taille de la carte | Menu **Redimensionner…** : taper la nouvelle largeur et la nouvelle hauteur ; si la réduction supprimerait l'entrée ou des entités, une confirmation est demandée. |
| Renommer la carte | **F2**. |
| Annuler / refaire | **Ctrl+Z** / **Ctrl+Y**. |
| Essayer la carte | **P** — la carte se joue dans l'éditeur, comme en jeu : **↑ ↓ ← →**, **ZQSD** ou **WASD** pour marcher, **E** ou **Espace** pour interagir, **Échap** pour revenir à l'édition. Rien n'est perdu. |
| Enregistrer | **Ctrl+S** — un message confirme l'enregistrement, ou explique ce qui manque (par exemple : aucune entrée). |

La carte est enregistrée à côté de l'exécutable, dans le dossier `Levels`.

## 4. Publier votre carte

1. Copiez le fichier de votre carte (`Levels\<nom>.json`, à côté de `LevelEditor.exe`) dans le
   dossier `Source/Elements/Levels/` de votre copie du projet (celle clonée à l'étape 1).
2. Ouvrez **GitHub Desktop** : votre nouveau fichier apparaît dans la liste des changements.
3. En bas à gauche, donnez un court résumé (ex. « Ajout de la carte du port ») et cliquez
   **Commit to main**.
4. Cliquez **Push origin** (en haut) : votre carte est envoyée sur GitHub, visible par toute
   l'équipe.

## 5. Récupérer les cartes des autres

Dans GitHub Desktop, cliquez **Fetch origin** puis **Pull origin** : les cartes ajoutées par
d'autres membres de l'équipe apparaissent dans votre dossier `Source/Elements/Levels/`, prêtes à
être ouvertes dans l'éditeur.

## En cas de problème

- **L'enregistrement affiche un message d'erreur** : le message précise le problème (par
  exemple : aucune entrée). Corrigez et recommencez **Ctrl+S**.
- **GitHub Desktop signale un conflit** : deux personnes ont modifié le *même* fichier de carte.
  Donnez des **noms de fichiers différents** à vos cartes pour éviter ce cas.
