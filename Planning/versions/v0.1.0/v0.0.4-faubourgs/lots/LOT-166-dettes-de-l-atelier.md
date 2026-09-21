+++
id = "LOT-166"
titre = "Éditeur — les dettes de l'atelier"
version = "0.0.4"
filiere = "editeur"
statut = "a-faire"
taille = "M"
resume = "Ce que neuf cartes dessinées ont appris de l'outil : les petites gênes notées « pas fait » dans ses lots, et celles que la recette a trouvées."
prerequis = ["LOT-156"]
livrables = [
  "Un **verrou de session** : deux éditeurs ouverts ne se proposent plus les brouillons l'un de l'autre ; les onglets de la dernière séance se rouvrent.",
  "Le panneau « Problems » contrôle le **brouillon**, pas seulement le fichier enregistré.",
  "Les préfabriqués se renomment, se déplacent d'un niveau à l'autre et se suppriment depuis la fenêtre ; les `carte#id` internes à un tampon sont réécrits à la pose.",
  "Un point s'insère au milieu d'un trajet ; le miroir accepte une ligne de grille ; le seau pose des pièces larges.",
  "`--apply` rejoue les couches, le redimensionnement et la pose de l'entrée ; son format a un schéma JSON publié.",
  "Les touches d'outils passent par `EditorKeyBindings`.",
  "Les anomalies inscrites par le [LOT-127](../../v0.0.1-demo/lots/LOT-127-recette-de-l-editeur-a-la-main.md).",
]
criteres = [
  "Chaque ligne « pas fait » des dossiers `LOT-EDITOR-01` à `14` est levée, ou écartée avec sa raison dans la feuille de route de l'éditeur.",
  "Un scénario `--apply` par retouche de logique ; le cahier de recette de l'éditeur est rejoué par l'auteur.",
]
+++

## Pourquoi

Aucune de ces dettes n'empêche de dessiner une carte ; toutes coûtent un peu à chaque carte. Le lot
est placé **après** la `0.0.3` exprès : neuf cartes faites diront lesquelles pèsent vraiment, et
celles qui ne gênent personne seront écartées plutôt que faites. La liste est dans
l'[audit](../../../../standards/audit-editeur.md), §4.

## Périmètre

Le confort de l'atelier. Les limites du **jeu** relevées par l'audit — seule la première couche de
décor est lue, la gêne et l'abri déduits d'une pièce ne sont pas joués — ne sont pas ici.
