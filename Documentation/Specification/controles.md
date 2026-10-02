# Contrôles & entrées

> Statut : **livré** pour l'exploration, les écrans et le combat, au clavier et à la souris.
> Dépend de [`gameplay.md`](gameplay.md).

## 1. Périphériques
- **EX-CTRL-001** — Le jeu doit être jouable **entièrement au clavier**.

## 2. Commandes du jeu

Les entrées sont traduites en **commandes nommées** : un écran réagit à « déplacer », « valider »
ou « retour », jamais à une touche en particulier.

| Commande | Clavier |
|----------|---------|
| Se déplacer (exploration) | ← ↑ → ↓, ZQSD ou WASD |
| Interagir (`EX-CTRL-022`) | E ou Espace |
| Pause (exploration) | Échap |
| Passer la main au suivant du groupe (exploration, `EX-EXP-014`) | Tab |
| Écran du groupe (exploration) | G |
| Qui parle pour le groupe (dialogue, D-28) | Tab, Maj+Tab |
| Groupe : prendre ou laisser, mener, avancer, reculer | Entrée, M, Page préc., Page suiv. |
| Naviguer (écrans, combat) | ← ↑ → ↓ |
| Valider | Entrée |
| Retour | Échap |

- **EX-CTRL-010** — Chaque commande du jeu est une **commande nommée**, dissociée de la touche
  physique qui la déclenche : plusieurs touches peuvent mener à la même intention de jeu.
- **EX-CTRL-012** — Les raccourcis de l'**éditeur** doivent être
  **reconfigurables** par fichier (`Settings/keybindings.json`) ; un fichier absent ou partiel
  retombe sur les valeurs par défaut.
- **EX-CTRL-022** — **Interagir** doit être une commande dédiée (E ou Espace)
  qui déclenche l'entité à portée du héros — à moins de 1,5 case, celle qu'il regarde d'abord —
  dialogue, coffre, portail (`EX-EXP-004`).

Les événements Qt sont traduits en intentions avant d'atteindre `Core`, qui ne connaît aucun
périphérique physique.

## 3. Réactivité
- **EX-CTRL-020** — La latence entre une entrée et son effet ne doit pas
  dépasser **un pas** de simulation : une touche enfoncée est lue au pas suivant, jamais perdue
  entre deux.

## Exigences retirées {#ctrl-retirees}

> Ancres conservées, jamais renumérotées : les lots livrés s'y réfèrent.

- **EX-CTRL-013** *(retirée au `LOT-88`)* — commande de ruée (dash) : le jeu n'a
  pas de ruée.
- **EX-CTRL-002** *(retirée le 2 octobre 2026)* — prise en charge de la manette XInput.
- **EX-CTRL-011** *(retirée le 2 octobre 2026)* — échantillonnage des fronts d'un bouton de manette.
- **EX-CTRL-021** *(retirée au `LOT-88`)* — échantillonnage des entrées par la
  boucle d'une fenêtre native : Qt distribue désormais les événements.

## Traçabilité
L'acquisition des touches du jeu relève des événements Qt Quick. `Source/HMI/Input` conserve la
traduction des touches de l'éditeur ; `Source/Core` ne reçoit que des intentions
(`core::ExplorationIntent`), sans dépendance inverse.
Voir [`exigences-non-fonctionnelles.md`](exigences-non-fonctionnelles.md) pour l'architecture.
