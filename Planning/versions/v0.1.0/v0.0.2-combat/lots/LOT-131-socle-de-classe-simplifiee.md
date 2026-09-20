+++
id = "LOT-131"
titre = "Le socle de classe simplifiée"
version = "0.0.2"
filiere = "moteur"
statut = "a-faire"
taille = "L"
resume = "Une classe est une donnée : une table de progression, des capacités à effets nommés, des ressources qui se dépensent et se récupèrent."
prerequis = ["LOT-130"]
reprend = ["LOT-47 (socle de classe)", "LOT-25 (capacités de classe, en partie)"]
livrables = [
  "Le schéma et le chargement d'une classe simplifiée : dé de vie, maîtrises, table par niveau.",
  "Les **capacités** comme effets branchés sur les crochets du combat (`LOT-20`, `LOT-21`) : modifier un jet, une CA, des dégâts, un déplacement.",
  "L'**incantation simplifiée** : sorts fixés par la table, **deux lancers par jour et par sort**, pas d'emplacements.",
  "La **résistance globale** aux dégâts, et le déplacement **sans attaque d'opportunité**.",
]
criteres = [
  "Une classe de test à trois capacités se charge et agit en combat sans une ligne de C++ qui la nomme.",
  "Le journal de combat nomme chaque capacité qui a joué.",
  "Un sort épuisé ne se propose plus ; un repos long le rend.",
]
sources = [
  "Player's Guide to Tanares, p. 192 (règles communes), 196 et 200 (incantation simplifiée)",
]
+++

## Périmètre

Le niveau **1 à 5** : c'est ce que l'Empire central demande. La table complète jusqu'au niveau 20
est saisie en données, mais seules les capacités des niveaux 1 à 5 sont implémentées et testées
ici ; le reste appartient à la `0.3.0`.
