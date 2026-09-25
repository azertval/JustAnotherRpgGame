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

La fenêtre montre la carte ouverte au centre, sur le **canevas**, entouré de cinq panneaux qu'on
peut déplacer, redimensionner ou détacher — six panneaux en tout, les mêmes que décrit geste par
geste [Utiliser l'éditeur de niveaux](utiliser-l-editeur.md) :

- **Maps** — les cartes du dépôt, avec un champ de recherche. On y **crée** (**New**), **renomme**
  (**Rename**), **duplique** et **supprime** une carte ; l'onglet *Graph* relie deux cartes d'un
  geste, l'onglet *City* montre la ville par quartiers.
- **Palette** — ce qu'on peint. L'onglet *Pieces* offre les pièces de la planche du lieu (sols,
  murs, étals, gradins…), l'onglet *Prefabs* les morceaux de carte enregistrés pour ce lieu, et
  l'onglet *Types* les types de case d'une carte sans lieu ou de la couche de collision — leur
  liste, et l'usage de chacun, sont dans [Utiliser l'éditeur de niveaux](utiliser-l-editeur.md).
- **Layers** — les couches de la carte (`sol`, `relief`, collision) : en ajouter, en retirer,
  choisir celle qu'on peint.
- **Entities · Inspector** — ce qu'on pose sur la carte et ce qu'on en dit : coffre, panneau, PNJ,
  rencontre, portail (ouvert ou condamné), point d'arrivée, zone de combat, zone déclencheuse,
  décor (`prop`)…
- **Problems** — ce qui cloche sur la carte ouverte, à lire avant de la partager.

Les menus et les boutons de l'éditeur sont en anglais ; ils sont cités ici tels qu'il les affiche.
**F1** (*Shortcuts overview*) affiche la liste complète des raccourcis.

| Action | Comment |
|--------|---------|
| Peindre une case | Choisir une pièce dans la **Palette**, puis cliquer (ou cliquer-glisser) sur la grille avec le **pinceau** (**B**). |
| Remplir une zone | Outil **rectangle** (**R**) : cliquer-glisser d'un coin à l'autre, relâcher pour remplir ; le **seau** (**G**) remplit une surface d'un seul sol. |
| Copier / coller une zone | Outil **sélection** (**S**) : cliquer-glisser pour définir la zone, **Ctrl+C** (*Copy*) pour la copier, **Ctrl+V** (*Paste*) pour la coller à l'endroit survolé. |
| Placer l'entrée | Panneau **Layers**, couche *Collision*, puis dans la **Palette**, onglet *Types*, le marqueur **Entry** : cliquer la case voulue. Une carte a **une seule** entrée : l'ancienne se déplace. |
| Poser une entité | Outil **Entité** (**O**) : choisir sa sorte dans la liste *Place* du panneau **Entities**, cliquer la case. Ses champs (carte visée par un portail, dialogue d'un PNJ…) se remplissent dans l'**Inspector** ; le bouton **Remove** retire l'entité sélectionnée. |
| Déplacer la vue / zoomer | Cliquer-glisser avec le **bouton droit** ; **molette** pour zoomer ; **0** (*Reset camera*) pour revenir au cadrage automatique. |
| Afficher un quadrillage | **F10** (*Grid*). |
| Changer la taille de la carte | *File* › **Resize…** : taper la nouvelle largeur et la nouvelle hauteur ; si la réduction supprimait l'entrée ou des entités, une confirmation est demandée. |
| Renommer la carte | **F2** (*Rename*) : le nouvel identifiant, dossier compris. |
| Annuler / refaire | **Ctrl+Z** / **Ctrl+Y** (*Undo*, *Redo*). |
| Essayer la carte | **P** (*Playtest*) — la carte se joue dans l'éditeur, comme en jeu : **↑ ↓ ← →**, **ZQSD** ou **WASD** pour marcher, **E** ou **Espace** pour interagir, **Échap** pour revenir à l'édition. Rien n'est perdu. |
| Enregistrer | **Ctrl+S** (*Save*) — un message confirme l'enregistrement, ou explique ce qui manque (par exemple : aucune entrée). |

Où la carte s'enregistre dépend de l'éditeur qu'on lance. Le `LevelEditor.exe` d'une archive
publiée ouvre et écrit le dossier `Levels` posé à côté de lui ; l'éditeur construit depuis les
sources, lui, ouvre et écrit directement l'arbre des sources, `Source/Elements/Levels/`, et la
carte est alors déjà dans le projet. Dans les deux cas, la carte va dans le sous-dossier de son
lieu, `Levels/<région>/<ville>/` — par exemple `Levels/central-empire/capital/echoppe.json`.

## 4. Publier votre carte

1. Si vous avez travaillé avec le `LevelEditor.exe` d'une archive, copiez le fichier de votre
   carte (`Levels\central-empire\capital\<nom>.json`, à côté de `LevelEditor.exe`) au même
   endroit sous `Source/Elements/Levels/` de votre copie du projet (celle clonée à l'étape 1).
   Avec l'éditeur construit depuis les sources, la carte y est déjà.
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
