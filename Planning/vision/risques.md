# Les risques

Ce qui peut faire grossir le projet ou l'arrêter, et le garde-fou posé en face de chacun.

| # | Risque | Ce qui le rend probable | Garde-fou |
|---|---|---|---|
| R-01 | **Le volume d'assets.** Une centaine de zones, des centaines de PNJ, tous à produire | aucune zone HD n'existe : le coût d'une zone est inconnu | le kit commun d'abord (LOT-105) ; la règle du commun vers le propre ; le bilan chiffré de chaque version ; aucune date avant la `0.0.3` |
| R-02 | **La cohérence du style.** La facture peinte varie d'une génération à l'autre bien plus que le pixel art quantifié | le générateur est un outil manuel, sans graine fixée | la planche de référence comme ancre ; une palette par région ; une relecture sur la galerie à chaque lot d'assets |
| R-03 | **Le poids du dépôt.** Sans budget par zone (D-23), l'Empire dépasse le gibioctet, le monde en pèse plusieurs | git versionne mal le binaire | 5 Mio par fichier (`check_binary_files.py`) ; poids de chaque zone publié en CI (LOT-104) ; décision de stockage à la `0.0.3` (Q-08) ; l'arborescence par région permet de déplacer sans tout casser |
| R-04 | **La table rase casse la CI** plus longtemps que prévu | trente tests, quatre scripts, trois règles CMake lisent les fichiers supprimés | un lot dédié, dans l'ordre : rebrancher, admettre le vide, supprimer (LOT-102) |
| R-05 | **Le rendu HD** révèle d'autres hypothèses que les quatre de l'audit | l'audit a lu le code, il n'a rien rendu | la maquette du LOT-101 rendue par le moteur est le critère du LOT-103 |
| R-06 | **Le combat sur la carte** est la pièce de moteur la plus lourde de la démo | il n'a jamais existé : le combat ne se joue qu'en arène | il est sur le chemin critique dès le départ (LOT-118), en parallèle des assets |
| R-07 | **La figurine par combinaison** explose à la `0.3.0` : 21 espèces × 20 classes | chaque figurine est une planche animée en quatre orientations | décision « figurine composée » instruite dès la `0.0.2` (Q-07) |
| R-08 | **Le contenu sans texte** : dix-sept villages, les docks, plusieurs forts n'ont qu'un nom | la carte du monde nomme plus que le livre ne décrit | décor par défaut (Q-10) ; rien ne s'invente sans lot |
| R-09 | **Les sorts du *Manuel*** : le scan efface les cellules valant 1 | constaté au `LOT-36` | toute table numérique se recoupe à la main ; les *Basic Rules*, propres, priment quand elles couvrent |
| R-12 | **L'éditeur freine les cartes.** Chaque carte passe par lui, et il a été fait pour trois cartes en pixel art | l'[audit](../standards/audit-editeur.md) : un manifeste par carte, un canevas au plus proche voisin, huit lots jamais passés à la souris | la filière `editeur`, dont les lots sont des **prérequis déclarés** des cartes ; l'éditeur est complet pour l'Empire à la `0.0.5` |
| R-10 | **La dérive de la planification elle-même** | l'ancienne feuille de route a atteint 2 600 lignes | des fiches courtes, un lint, une seule version détaillée, un bilan par version |
| R-11 | **Le projet d'un seul auteur** : la lassitude devant cent zones | | des versions courtes qui se **jouent** ; chaque zone est une version qu'on tague |
