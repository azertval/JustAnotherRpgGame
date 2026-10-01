# Version 0.0.2.5 — passage à la 3D

Le jeu garde sa vue isométrique et change de matière : le décor d'architecture devient des
maillages, les personnages des modèles animés par un squelette commun. La version se loge entre la
recette de la `0.0.2` et la `0.0.3` ([D-29](../../../vision/decisions.md)), parce que la `0.0.3`
produit les assets de six quartiers : tout ce qui serait produit avant la bascule serait à
convertir après.

Deux buts, tous deux de l'auteur :

- **une production de personnages stable.** Le générateur d'images ne tient ni l'identité ni la
  marche d'une planche à l'autre (les quatre marches du brawler ont été refusées le 24 septembre).
  En 3D il ne dessine plus le mouvement : il ne peint qu'une texture ;
- **un cycle jour / nuit**, qui demande que le moteur éclaire lui-même la scène.

L'instruction complète est dans l'[audit de faisabilité](https://claude.ai/code/artifact/9cffa31b-7b23-4e0a-8df9-1c0cb9534523)
du 30 septembre 2026.

## Ce qui ne bouge pas

- `Source/Core` : règles, combat, grille, quêtes, cartes. La grille reste de 1,5 m, le combat ne
  voit pas le rendu.
- Le **format de carte v4** : une case cite une clé de pièce, l'étage est déjà une donnée. Les
  quatre cartes livrées se rechargent sans retouche.
- La **vue** : caméra orthographique tournée de 45° et inclinée de 38,3°, ce qui redonne exactement
  le losange de rapport 0,62 (sin 38,3° = 0,62). Pas de rotation libre.
- L'**interface** QML, les **portraits** et les **jetons**, qui restent peints
  ([D-30](../../../vision/decisions.md)).

## Ce dont est fait un humanoïde

**Révision du 1er octobre 2026 (LOT-1000) :** l'auteur a refusé les corps communs
de preuve puis les essais TripoSR, validé les deux géométries Meshy et choisi Meshy
comme générateur de personnages. Le tableau ci-dessous décrit l'hypothèse
initiale D-31 ; la preuve utilise désormais des maillages propres aux figurines.
Le contrat de squelette, les animations et l'organisation des pièces seront
ajustés d'après cette preuve lors du LOT-1001. Les modèles statiques sont validés,
la chaîne animée produit les quarante bandes, le brawler est installé et le kit
`Common@5` publié. L'auteur a accepté le résultat le 1er octobre 2026 : la version
s'ouvre (détail dans le LOT-1000).

Seule la texture du corps est propre au personnage ([D-31](../../../vision/decisions.md)).

| Élément | Combien | Partagé ou propre |
|---|---|---|
| Squelette et six animations (repos, marche, attaque, sort, touché, mort) | 1 pour tous les humanoïdes | partagé |
| Corps : homme ou femme × maigre, normal, musclé, gros | **8 maillages**, liés au même squelette | partagé |
| Texture du corps : peau, visage, vêtements près du corps | 1 par personnage | propre |
| Pièces d'équipement : casque, épaulières, cape, robe, cheveux, barbe, armes | une bibliothèque ; chaque pièce modelée une fois, accrochée à un os | partagé |
| Texture ou teinte d'une pièce | 1 par pièce, avec variantes de couleur | partagé, parfois propre |

Un personnage est une **fiche** : son corps, sa texture, ses pièces, leur os et leur teinte.

## L'ordre, et pourquoi

| # | Lot | Filière | Taille | Attend |
|---|---|---|---|---|
| 1 | [LOT-1000](lots/LOT-1000-preuve-de-la-chaine-de-personnages.md) — la preuve de la chaîne de personnages | pnj | M | — |
| 2 | [LOT-1001](lots/LOT-1001-standard-3d.md) — le standard 3D | standard | M | LOT-1000, LOT-142 |
| 3 | [LOT-1002](lots/LOT-1002-canevas-sur-le-rendu-du-jeu.md) — le canevas de l'éditeur sur le rendu du jeu | editeur | L | LOT-142 |
| 4 | [LOT-1003](lots/LOT-1003-maillages-et-profondeur.md) — maillages et profondeur | moteur | L | LOT-1001, LOT-1002 |
| 5 | [LOT-1004](lots/LOT-1004-kit-de-la-capitale-en-maillages.md) — le kit de la Capitale en maillages | assets | L | LOT-1003 |
| 6 | [LOT-1005](lots/LOT-1005-squelette-et-animations.md) — squelette et animations | moteur | L | LOT-1003 |
| 7 | [LOT-1006](lots/LOT-1006-corps-de-reference.md) — les corps de référence | pnj | L | LOT-1005 |
| 8 | [LOT-1007](lots/LOT-1007-eclairage-et-cycle-jour-nuit.md) — éclairage et cycle jour / nuit | moteur | L | LOT-1004 |
| 9 | [LOT-1008](lots/LOT-1008-atelier-des-assets-3d.md) — l'atelier des assets 3D | editeur | XL | LOT-1004, LOT-1006 |
| 10 | [LOT-1009](lots/LOT-1009-les-quatre-heros.md) — les quatre héros | pnj | M | LOT-1008 |
| 11 | [LOT-1010](lots/LOT-1010-recette-et-version-0-0-2-5.md) — recette et version | version | M | LOT-1007, LOT-1009 |

Trois règles ont fixé cet ordre.

1. **La preuve d'abord.** Le [LOT-1000](lots/LOT-1000-preuve-de-la-chaine-de-personnages.md) est une
   **porte** : il ne touche pas au moteur, et si ses critères ne sont pas tenus la version est
   abandonnée sans avoir rien cassé.
2. **L'éditeur avant le moteur.** Le canevas peint aujourd'hui la scène par `QPainter`, qui ne sait
   pas dessiner un maillage. Le [LOT-1002](lots/LOT-1002-canevas-sur-le-rendu-du-jeu.md) le branche
   sur le rendu du jeu **tant que ce rendu est encore en 2D** : quand le moteur apprend les
   maillages, l'éditeur suit sans un lot de retard, et il n'y a jamais deux rendus à tenir.
3. **Le moteur avant l'asset, et l'asset emporte l'ancien.** Un lot de moteur apprend une forme
   nouvelle sur des données d'essai ; le lot d'assets qui suit installe les vrais fichiers et
   **supprime dans la même PR** ce qu'ils remplacent — images et code.

Après le LOT-1003, deux filières avancent en parallèle : le décor et la lumière (404, 407), les
personnages (405, 406) ; elles se rejoignent à l'atelier (408).

## Ce que chaque lot retire

C'est la règle de la version ([D-32](../../../vision/decisions.md)) : **un lot qui remplace
quelque chose le supprime dans sa propre PR**, et sa fiche le nomme sous « À supprimer ». Rien ne
reste « pour plus tard » sans être écrit dans la dette ci-dessous.

| Lot | Ce qu'il apporte | Ce qu'il supprime |
|---|---|---|
| LOT-1000 | Le brawler en modèle, rendu en bandes dans le jeu 2D | Les bandes **générées** du brawler ; `Planning/quality/` (l'avis de refus de ses marches) |
| LOT-1001 | `style-3d.md`, exigences réécrites | Le standard et la consigne 2D HD (archivés, plus normatifs) ; le workflow de revue des marches ; la chaîne de **génération de figurines 2D** (3 scripts, 2 tests) ; les exigences remplacées (vers `exigences-retirees.md`) |
| LOT-1002 | Canevas `QRhiWidget`, `--render` hors écran | `ScenePainter`, `SceneImages`, leurs tests, la parité GPU / `QPainter`, `bench_canvas_paint` |
| LOT-1003 | Passe de maillages, caméra 3D, chargeur glTF | `Camera2D` comme caméra du lieu. Presque rien d'autre : tant qu'il reste des pièces en image, l'ordre du peintre sert encore — c'est dit |
| LOT-1004 | Sols, murs, balustrades, haies, escaliers et toits en maillages | Leurs **1 340 PNG** au moins, dont les 686 de toits ; le mécanisme d'étage 2D (rang de tri, occlusion, effacement) ; le recouvrement des dalles ; la maquette 2D HD et son test |
| LOT-1005 | Déformation par os, clips d'animation | Rien d'installé : les bandes vivent un lot de plus, leur retrait est au LOT-1006 |
| LOT-1006 | Huit corps, un squelette, six animations | **Toutes** les bandes de figurine (132 images et leurs `.anim.json`) ; la cellule et les quatre orientations de figurine dans le rendu ; les bandes de figurine des données d'essai ; la suite 2D du LOT-145. Les 24 bandes d'effets de `Common/Fx/` et `AnimationCatalog`, qui les lit, **restent** |
| LOT-1007 | Heure du monde, soleil, ombres, lumières de nuit | L'ombre propre cuite dans les textures du kit |
| LOT-1008 | L'atelier à trois vues | La saisie à la main des manifestes de personnages ; le descripteur d'installation des portraits |
| LOT-1009 | Brawler, Mage, Priest, Scoundrel assemblés | Le **portrait d'attente** (liste `portraits`) ; le brawler de la preuve |
| LOT-1010 | Le contrôle des orphelins, la recette | Ce que le contrôle trouve encore ; les références d'image périmées |

## La dette déclarée pour la 0.0.3

Deux décisions de l'auteur ([D-30](../../../vision/decisions.md)) laissent des images dans une
scène 3D. Elles ne sont pas mortes — le jeu les affiche — mais elles ont une date de retrait.

| Ce qui reste en image | Jusqu'à | Retiré par |
|---|---|---|
| Le **mobilier** et les **pièces maîtresses** peints du kit de la Capitale (bancs, lampadaires, fontaine, statues…) | la `0.0.3` | le kit commun complété ([LOT-151](../v0.0.3-capitale-intra-muros/lots/LOT-151-kit-commun-intra-muros.md)) |
| Les kits d'**Arenarea**, de l'**Arena of Fate** et de **Martpart** | la `0.0.3` | les lots de leur quartier (LOT-147, LOT-106, LOT-110) |
| L'**ordre du peintre** pour ces images (tri par le pied, `ComposedScene`) | le retrait de la dernière image de décor | le dernier des lots ci-dessus |

Une pièce en image se dresse face à la caméra : sous une caméra orthographique fixe, elle occupe
exactement les pixels qu'elle occupe aujourd'hui. Elle ne reçoit que la teinte de l'heure, pas le
soleil. Les lots de zone de la `0.0.3` sont à **réécrire** pour commander des maillages : à la recette
([D-35](../../../vision/decisions.md)).

## Ce que la version fait aux lots ouverts

- [LOT-142](../v0.0.2-combat/lots/LOT-142-recette-et-version-0-0-2.md), la recette de la `0.0.2`,
  se livre **en 2D**, sans attendre.
- [LOT-145](../v0.0.2-combat/lots/LOT-145-mannequins-de-remplacement.md), les mannequins 2D, est
  **clos en l'état** ([D-34](../../../vision/decisions.md)) : livré sur son humanoïde ; les mannequins
  quadrupède et volant ne sont pas produits en bandes. Sa règle de repli (`hmi::FigureResolver`,
  propriété `silhouette`) reste, et une silhouette désignera un squelette.

## Risques

| Risque | Ce qui le réduit |
|---|---|
| Un personnage fait d'un corps, d'une texture et de pièces **ne tient pas** à côté de son portrait | la porte du LOT-1000 ; en repli, rouvrir D-31 : un générateur image → maillage, lié au même squelette |
| Le décor en maillages **perd la facture peinte** validée le 23 septembre | le LOT-1004 compare, carte pour carte, le rendu d'avant et d'après |
| Les images dressées **se croisent mal** avec les volumes | emprises 1 × 1 seulement en image au-delà de la `0.0.3` ; le LOT-1003 le mesure sur la carte d'Arenarea |
| La version ne « se joue » pas, contre la règle de la trajectoire | sa recette rejoue la quête et le combat de la `0.0.2` : même jeu, autre matière |
