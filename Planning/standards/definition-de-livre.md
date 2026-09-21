# Ce que « livré » veut dire

Un lot passe à `statut = "livre"` quand **tous** ses critères d'acceptation sont tenus **et** que
les conditions de sa filière le sont. Un lot « presque fini » est `en-cours`.

## Pour tout lot

- La PR est fusionnée sur `main`, CI verte, entrée au `CHANGELOG.md`.
- `uv run scripts/check.py` et `scripts/build_docs.py` passent en local avant la PR.
- La fiche porte le numéro de la PR et, s'il y en a, ses **décisions de réalisation**.
- Ce que le lot n'a pas fait et devait faire est **écrit** : dans un autre lot, ou dans une question ouverte.

## Par filière

| Filière | Conditions |
|---|---|
| **Assets HD** | chaque image est au [standard](style-2d-hd.md), citée par un manifeste, visible dans la galerie de débug ; aucune n'est une image du corpus ; la zone tient son budget de poids ; les sources sont rangées dans `Tools/AssetsHD/` sous le même arbre |
| **Cartes** | dessinée dans l'éditeur ; `LevelEditor --check` passe ; aucune case inatteignable ; le rendu de la carte est joint à la PR ; l'image de l'onglet « Carte » existe |
| **PNJ** | figurine, portrait et jeton ; une fiche quand le PNJ peut combattre ; placé sur sa carte ; ses textes en français et en anglais |
| **Quêtes et dialogues** | chaque issue se joue en test, sans fenêtre, à graine fixée ; aucun drapeau lu sans être posé ; textes dans les deux langues |
| **Moteur** | tests unitaires et d'intégration ; pas de régression des mesures de performance ; la spécification dit ce que le code fait |
| **Règles et données** | chaque valeur vient d'une page citée ; schéma validé ; un test recalcule les valeurs dérivées ; les écarts au livre sont écrits |
| **Interface** | à la charte v2 ; capture de référence QML ; jouable sans souris ; textes dans les deux langues |
| **Éditeur** | logique en fonctions pures testées ; un scénario `--apply` par outil, comparé à un fichier attendu ; une famille ou une propriété nouvelle passe par `EntityKinds`, sans code par famille ; le guide d'usage est à jour ; **le geste est fait à la main par l'auteur** ; textes en anglais, hors charte v2 |
| **Recette et version** | les critères de sortie de la version sont tenus ; installeur éprouvé sur un poste vierge ; tag posé ; **bilan** écrit dans le dossier de la version |

## La vérification manuelle

Plusieurs lots passés ont été livrés « vérification à la souris due ». La règle change : ce qui
demande une main — un parcours à la manette, un contrôle visuel du scintillement, l'approbation
d'une maquette — est un **critère d'acceptation**, et le lot reste `en-cours` tant qu'il n'est pas
tenu.
