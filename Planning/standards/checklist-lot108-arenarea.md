# LOT-108 — Checklist complète d’Arenarea à valider

**Validée par l’auteur le 24 septembre 2026, livrée le même jour**, dans son périmètre entier : les compléments du commun (postes C et J) relèvent du LOT-108 et s’installent dans le commun de la Capitale ; l’hippodrome (H) et l’Arching Bridge (E05) se produisent dès maintenant. Une case se coche quand le poste est **livré** (installé, en galerie, vu en assemblage). La commande d’Arenarea (`Tools/AssetsHD/Regions/central-empire/capital/arenarea/commande.md`) porte les identifiants de cette liste.

Objectif : disposer du kit nécessaire au **quartier extérieur complet**, au-delà du parcours réduit de la démo. Couvrir les rues principales et secondaires, demeures, jardins, établissements nommés, hippodrome, eau, enceinte et accès. Produire des modules qui permettent plusieurs compositions, plutôt qu’un bâtiment figé par lieu.

**102 postes d’assets et d’assemblages**, plus 10 critères de fabrication/livraison. Plusieurs postes comprennent de nombreuses orientations, états et raccords : ce nombre ne désigne pas le total final de PNG.

## 1. Base de l’audit et état réel

Sources consultées :

- [Référentiel de la Capitale](../referentiels/central-empire/capitale.md), notamment Arenarea et l’Arena of Fate.
- Texte extrait de la double page imprimée 98–99 et image de la carte imprimée 96–97 du Sourcebook, déjà disponibles dans `Tools/AssetsHD/Regions/central-empire/capital/Common/Extension105/ReferenceReview/`. Les pages originales servent au contenu et à la géographie, jamais à fabriquer les images.
- Planche artistique `Tools/AssetsHD/Arenarea/arenarea-planche-reference-v2.png`, examinée visuellement.
- [Standard HD](style-2d-hd.md), [consigne de production](consigne-2d-hd.md), [arborescence](arborescence-assets.md), [audit de l’extension du commun](audit-capital-lot105-extension.md).
- README V4 après reprise du relief, README et assemblage moteur de l’extension 105 ; manifests installés du commun, de la région, d’Arenarea et de l’Arena of Fate.
- [LOT-108](../versions/v0.1.0/v0.0.1-demo/lots/LOT-108-assets-hd-arenarea.md), commande locale d’Arenarea, [LOT-109](../versions/v0.1.0/v0.0.1-demo/lots/LOT-109-carte-arenarea.md), [LOT-106](../versions/v0.1.0/v0.0.3-capitale-intra-muros/lots/LOT-106-assets-hd-arena-of-fate.md), [LOT-129](../versions/v0.1.0/v0.0.1-demo/lots/LOT-129-etages-et-toits.md), plan de principe et quête de la démo.

| Ensemble installé | Constat vérifié | Traitement |
|---|---|---|
| Commun Capitale | **705 entrées : 107 pièces hors toiture + 598 modules de toiture** ; fichiers référencés présents | Réemployer, compléter seulement les fonctions absentes |
| Commun Empire | **4 pièces** : bannière, murs à bannière U/V, colonne emblématique ; fichiers présents | Conserver l’emblème validé : tête de lion couronnée de profil avec laurier |
| Arenarea | Manifeste de scène vide | Les 45 pièces de la commande sont prévues, pas livrées |
| Arena of Fate | **18 pièces** : 12 sols/transitions et 6 murs/angles ; fichiers présents | Réutilisation ou promotion à étudier ; pas un Colisée complet |
| Communs mondiaux Terrain/Nature/Props | Manifests vides | Aucun kit naturel ou mobilier générique supplémentaire à supposer disponible |
| LOT-129 | Statut livré, étages visuels et toitures droites/L/T/X | La suspension historique du LOT-108 pour ce prérequis est dépassée |

Les « 111 pièces communes » des documents précédents correspondent à **107 Capitale + 4 Empire**, hors toits. Les 598 toits sont des découpes de construction, pas 598 bâtiments différents. L’extension 105 est installée et contrôlée selon son README ; cela ne remplace pas la validation artistique utilisateur. La V4 après reprise reste la référence qualité explicitement acceptée.

### Réemploi sans nouvelle génération

| Famille du standard | Pièces déjà utilisables |
|---|---|
| 01 Sols | Pavés et dalles ×3, bordures de pavé ×4 ; eau, gazon, terre et bois ×3 ; huit transitions d’allée |
| 02 Façades | Calcaire plein, fenêtre, porte, volets, vitrine, boutique à auvent U/V ; angles ; murs à corniche, pilastre ; quais, soutènements, remparts |
| 03 Colonnes | Colonne architecturale de marbre ; colonne impériale au lion |
| 04 Accès | Passage ouvert U/V, portail de jardin, arche |
| 05 Limites | Balustrades U/V, pilier et angles ; haies U/V et angles |
| 06 Pièces maîtresses | Fontaine publique, statue civique anonyme, puits |
| 07 Végétal | Cyprès, arbre d’ombrage, massif fleuri, vasque, haies |
| 08 Mobilier | Bancs U/V, lampadaire, applique, caisse, tonneau, étal nu, charrette, denrées, poteries, établi, affichage, amarrage, barque |
| 09 Bâtiments | Assemblage des façades avec les toits à deux pans, profondeurs 2–5, jonctions L/T/X de largeur égale |
| 10 Seuils | Quatre escaliers, palier, tabliers de pont U/V, pontons U/V, arche |

## 2. Comment lire et valider l’inventaire

**C** = complément ou variation du commun ; **A** = identité d’Arenarea ; **R** = réemploi/promotion à coordonner avec le Colisée. Les pièces C génériques sont candidates au niveau partagé approprié (ville, région ou monde), selon leurs usages ; pas de déplacement artificiel pour contourner un budget. Les variantes luxueuses utiles à Arenarea, Uptown et Downtown forment un sous-ensemble cohérent du commun de la Capitale. Les motifs et bâtiments nommés restent propres au quartier.

**Origine** : **T** = besoin explicitement établi par le texte ; **P** = lieu ou géographie du plan ; **V** = planche artistique ; **D** = déduction fonctionnelle ou proposition artistique. T/P attestent un besoin ou un lieu, pas le détail inventé de son architecture. Aucune nouvelle héraldique de maison noble, divinité ou organisation n’est réputée canonique.

**Variantes de construction** :

- **4 faces** : les deux axes isométriques, avec les faces opposées nécessaires pour fermer un îlot ou border les deux côtés d’une rue. Créer seulement les vues distinctes ; pas de miroir qui inverse la lumière.
- **Raccords complets** : tronçons droits, quatre positions d’angle sortant et quatre d’angle rentrant, bouts fermés aux deux extrémités de chaque axe. Conserver les éléments existants ; ajouter les positions absentes. Les T/X sont explicitement demandés sur les familles qui en ont l’usage.
- **Longueurs 1/2** : modules d’une et deux cases, pour construire des dimensions impaires sans chevauchement. Les ensembles plus grands se composent sur cette maille.
- **Ouvert/fermé** : états visuels distincts ; l’interaction et le passage doivent être raccordés aux données de la carte. Un PNG ouvert ne garantit pas une ouverture praticable.
- Les fonds répétables ont **au moins trois variantes compatibles**, généralement six pour les grandes surfaces nouvelles. Les incrustations et décors de sol sont des pièces finies compatibles avec le chargeur, pas des superpositions supposées disponibles.

Chaque ligne ci-dessous est un **poste de validation**. Elle peut livrer plusieurs PNG. On ne multiplie pas systématiquement toutes les options entre elles : par exemple lierre et balcon ne nécessitent pas chaque combinaison de volets, enseigne et bannière. Le décompte exact des fichiers vient après résolution des réemplois et des faces manquantes ; les 45 pièces initiales ne constituent plus une limite.

## 3. Sols, voirie et transitions — famille 01

- [x] **S01 · A · V/T** — Dallage noble de marbre ivoire : six fonds 1×1, joints communs, sans cadre ; matière et relief adaptés aux grandes places.
- [x] **S02 · A · V** — Incrustations bourgogne : bandes, angles rentrants/sortants, terminaisons, T et croisement ; raccords aux dalles neutres. Remplace le simple jeu initial de huit bordures.
- [x] **S03 · A · V/D** — Deux compositions de place démontables : médaillon géométrique carré et octogonal, centres et pourtours séparés ; pas d’emblème noble inventé.
- [x] **S04 · C · D** — Transitions marbre/pavé et marbre/dallage commun : quatre côtés, angles intérieurs/extérieurs, pièces de seuil. Ne pas refaire les fonds communs.
- [x] **S05 · C · D** — Bordures de trottoir : droites longueurs 1/2, angles, bouts, abaissements pour passage piéton et entrée de cour ; matière calcaire V4.
- [x] **S06 · C · D** — Pavés de ruelles légèrement usés : trois variations dérivées du pavé commun, joints inchangés ; usure propre et localisée, pas de quartier délabré.
- [x] **S07 · C · T/D** — Allée claire de gravier fin pour jardins : trois fonds, jonctions vers terre/gazon et pavage, quatre côtés et angles ; pas de grain parasite.
- [x] **S08 · C · D** — Compléments des allées existantes : angles concaves, bouts, passages étroits, T et croisement pour tracer des jardins complets.
- [x] **S09 · C · D** — Caniveaux de pierre : droits, angles, T, terminaison et bouche d’évacuation ; grilles de drain et regard intégrés au pavage.
- [x] **S10 · C · D** — Seuils de sol : porte simple, double porte, portail carrossable, passage sous arche ; versions raccordées au pavé commun et au marbre.
- [x] **S11 · A/R · T/D** — Sol de piste de l’hippodrome : six variantes terre/sable tassé, versions sobres à traces de roues et sabots ; évaluer d’abord les sables du Colisée et ne dériver que le nécessaire.
- [x] **S12 · C · P/D** — Transitions de rive : eau/terre, eau/gazon, eau/pierre ; tronçons, angles convexes/concaves et bouts, eau commune conservée.

## 4. Architecture commune à compléter — familles 02, 03, 09

- [x] **C01 · C · D** — Murs calcaires : modules courts d’une case, extrémités finies, raccords en T/X pour ailes mitoyennes et clôture de cours ; compléter les orientations des angles existants.
- [x] **C02 · C · D** — Façades arrière et latérales : variantes sobres des portes, fenêtres et volets, faces opposées manquantes ; murs aveugles déjà existants réemployés.
- [x] **C03 · C · D** — Portes de service : simple bois et double livraison, ouvertes/fermées, quatre faces utiles ; raccord exact au calcaire commun.
- [x] **C04 · C · D** — Fenêtres communes complémentaires : volets ouverts/fermés, baie étroite et soupirail de soubassement ; quatre faces pour les façades réellement utilisées.
- [x] **C05 · C · D** — Socles et soubassements : longueurs 1/2, angles et retours aux portes ; continuité entre façade sur rue, jardin et cour.
- [x] **C06 · C · D** — Corniches et pilastres : pièces courtes, retours d’angle manquants, extrémités, raccord à l’étage et au pignon. Ne pas dupliquer les murs à corniche installés.
- [x] **C07 · C · D** — Accents de toiture : cheminée simple et double, lucarne sobre, évent ; vues sur les deux pans, scellement sans trou dans la couverture.
- [x] **C08 · C · D** — Finitions des toits contre murs : solins, raccord d’appentis de service et jonctions de niveaux différents utilisées par les annexes ; conserver les 598 modules déjà produits.
- [x] **C09 · C · D** — Petite couverture de perron et appentis : éléments modulaires, deux axes, rives et fermeture latérale ; pour entrées secondaires et livraisons.
- [x] **C10 · C · D** — Auvents communs complémentaires : rétracté, étendu uni et rayé ; faces opposées, bout gauche/droit et angle de boutique, en modules compatibles avec les murs existants.
- [x] **C11 · C · D** — Supports d’enseigne et appliques : orientations manquantes, version intégrée au mur de boutique pour le moteur actuel ; enseigne générique vierge.

## 5. Manoirs et architecture noble — familles 02, 03, 04, 09

Le marbre, les colonnes et les jardins sont attestés ; le vocabulaire précis vient de la planche et de la proposition artistique. Les composants sans identité locale sont proposés pour le commun noble ; les compositions de manoir restent propres à Arenarea.

- [x] **M01 · C · T/V** — Mur noble en marbre : longueurs 1/2, quatre faces, raccords complets et extrémités ; rez et étage compatibles avec une hauteur de 224 px.
- [x] **M02 · C · V/D** — Soubassement noble, bandeau d’étage et corniche dorée : droits, angles, retours et bouts ; ils doivent fermer tout le bâtiment.
- [x] **M03 · C · V** — Fenêtre cintrée haute : quatre faces, vitrage clair et volets fermés ; appui sculpté et version avec jardinière de fleurs blanches.
- [x] **M04 · C · D** — Fenêtres secondaires de manoir : baie rectangulaire double et petite baie de service, quatre faces ; contraste entre façade d’apparat et arrière.
- [x] **M05 · C · V** — Balcon : module simple et balcon filant, quatre faces, retours, angle et extrémités ; plancher, dessous et côtés en volume, porte-fenêtre intégrée.
- [x] **M06 · C · V** — Porte noble : double porte de bois et bronze avec encadrement de marbre, ouverte/fermée, quatre faces ; version neutre et version avec bannière impériale existante.
- [x] **M07 · C · V** — Colonne cannelée à chapiteau doré et demi-colonne engagée ; variante de pilastre assortie, sans emblème ajouté.
- [x] **M08 · C · V** — Colonnade modulaire : travée de 1 case et ensemble de 3 cases, deux axes, angle, extrémités et entablement ; colonnes réellement espacées et passages définissables.
- [x] **M09 · C · V/D** — Portique et loggia : travée ouverte, retour et angle, plafond/sous-face et balustrade associée ; à combiner avec les étages et toits existants.
- [x] **M10 · C · V** — Façades avec lierre : deux densités maîtrisées, quatre faces, coins et pied de mur ; variantes composées avec le mur, pas un calque de décor au rez supposé superposable.
- [x] **M11 · C · D** — Fronton noble : triangulaire et cintré, quatre faces utiles ; géométrie nue et motif végétal, sans fausse armoirie.
- [x] **M12 · A · T/D** — Trois manoirs témoins assemblés : résidence à portique, demeure à balcon avec cour, manoir à ailes en L/U avec jardin. Préfabriqués ou recettes d’assemblage des pièces précédentes ; aucun nouveau PNG de bâtiment entier.

## 6. Clôtures, jardins, bassins — familles 04, 05, 06, 07

- [x] **J01 · C · V/D** — Balustrades V4 : modules courts, bouts fermés et orientations d’angles absentes ; version de raccord aux escaliers. Garder les balustres en volume validés.
- [x] **J02 · C · V/D** — Haies V4 : courtes, bouts arrondis, orientations d’angles absentes, T et croisement ; continuité du volume et de la texture.
- [x] **J03 · C · V** — Grille de jardin bronze sombre : longueurs 1/2, quatre faces, angles, extrémités et piliers de raccord ; pieds compatibles avec muret et sol.
- [x] **J04 · C · D** — Murets de jardin : calcaire V4, droits 1/2, angles, bouts et pilier ; versions pleines et surmontées de la grille.
- [x] **J05 · C · V/D** — Portail existant : compléter orientation opposée et ouvert/fermé ; ajouter portillon d’une case et grande entrée carrossable, avec raccords aux grilles/haies/murets.
- [x] **J06 · C · V** — Pilier couronné d’une vasque fleurie et pilier à boule ; complément du pilier nu existant, sans le remplacer.
- [x] **J07 · A · V** — Fontaine signature à trois vasques, bassin octogonal et quatre jardinières, 3×3 cases ; conserver la fontaine publique pour les places secondaires.
- [x] **J08 · C · V/D** — Bassin d’ornement modulaire : margelles droites, angles convexes/concaves, bouts, quarts arrondis sur emprise entière ; réemploi de l’eau commune.
- [x] **J09 · C · D** — Décor d’eau sobre : bouche de fontaine murale et petit déversoir de bassin, deux axes ; versions composées avec mur/margelle pour garantir la pose.
- [x] **J10 · C · V** — Topiaires : spirale, boule sur tige et cône ; version plantée et en bac pour entrées et terrasses.
- [x] **J11 · C · V/D** — Rosier bourgogne et blanc, arbuste fleuri bas ; trois silhouettes compatibles, sans simples recolorations systématiques.
- [x] **J12 · C · V/D** — Parterres modulaires : bandes de fleurs blanches et bourgogne, angles, extrémités, petit massif isolé ; compléter le massif commun au lieu de le refaire.
- [x] **J13 · C · T/D** — Variations de végétation structurante : cyprès élancé/court et arbre d’ombrage à deux silhouettes complémentaires ; même famille peinte que l’existant.
- [x] **J14 · C · D** — Pergola : travée, angle, terminaison, treille végétale ; deux axes, version couverte et ouverte, pour diversifier les jardins de façade.
- [x] **J15 · C · D** — Mobilier de jardin : banc courbe par segments, banc de pierre noble, table basse et sièges assortis ; compléter les deux bancs communs.
- [x] **J16 · C · V/D** — Bacs : carré bas, rectangulaire et urne haute ; nus, fleuris ou avec topiaire selon les trois usages retenus, sans produit cartésien de toutes les plantes.

## 7. Rues, places et vie quotidienne — familles 06, 08, 10

- [x] **U01 · C · D** — Lampadaire commun : version sans bannière et version à deux lanternes ; appliques des faces absentes, variantes éteintes pour mêmes objets si utiles au cycle visuel.
- [x] **U02 · C · V/D** — Brasero de bronze sur pied : allumé/éteint ; brasero mural, quatre faces ; même famille de bronze que le lampadaire.
- [x] **U03 · C · D** — Bornes de rue et chasse-roues : pierre simple, bronze et chaîne basse, deux axes et terminaisons ; protéger les angles des manoirs et organiser le parvis.
- [x] **U04 · C · D** — Barrières temporaires : bois peint/cordon de file, deux axes, bout et retour ; ouverte ou fermant le passage, sans collision implicite.
- [x] **U05 · C · D** — Panneaux d’information : version murale et pupitre, silhouettes d’affiches sans texte lisible ; réemploi du tableau d’affichage existant.
- [x] **U06 · C · D** — Plaques de rue et poteau directionnel : supports nus, deux axes ; noms ajoutés par l’interface/données si nécessaire, pas générés dans la peinture.
- [x] **U07 · C · D** — Mobilier de livraison : caisse ouverte, pile de caisses, sacs, panier vide/plein et coffre de transport fermé ; deux orientations pour les grands assemblages.
- [x] **U08 · C · D** — Charrette commune : orientation manquante et chargement de ballots ; chariot couvert noble distinct, roues et brancards lisibles.
- [x] **U09 · C · D** — Attache pour montures, auge et râtelier à foin : deux axes ; servir l’hippodrome et les entrées carrossables, sans produire d’animaux.
- [x] **U10 · C · D** — Entretien discret : seau, balai, arrosoir, outils de jardin rangés ; petites compositions d’une case pour cours de service.
- [x] **U11 · A · D** — Statue du champion anonyme en bronze sur socle : 2×2 ; socle seul et orientations utiles au parvis. Aucun héros nommé improvisé.
- [x] **U12 · A · D** — Repères de quête : panier de pommes plein/renversé et petit groupe de pommes au sol ; accessoires facultatifs de mise en scène, sans modifier le scénario du LOT-120.

## 8. Établissements nommés — familles 02, 04, 08, 09

Les noms ci-dessous sont attestés. Leur détail visuel est une proposition. Calice, balance devant un soleil couchant et brasseur encapuchonné sont des pictogrammes proposés, pas des blasons officiels du livre. La Blood Bound reste clandestine : aucun logo mafieux omniprésent sur les rues.

### Golden Chalice Casino — T/P

- [x] **G01 · A · T/D** — Entrée luxueuse : porte ouverte/fermée, marquise bourgogne/or et fronton au calice, quatre faces ; cohérence avec le kit noble.
- [x] **G02 · A · T/D** — Façade distinctive : baie haute de casino, panneaux décoratifs au calice, balcon de façade et angles ; modules réutilisables en plusieurs longueurs.
- [x] **G03 · A · T/D** — Enseigne suspendue et enseigne murale au calice, deux axes et faces visibles nécessaires.
- [x] **G04 · A · D** — Accueil extérieur : pupitre, cordons et tapis de seuil bourgogne (droit, bout, angle), jardinières nobles réemployées ; aucune salle de jeu complète dans ce lot.

### Dusk of Justice Betting House — T/P

- [x] **D01 · A · T/D** — Devanture de paris : porte, guichet fermé/ouvert, baie grillagée et auvent distinct ; quatre faces utiles.
- [x] **D02 · A · T/D** — Enseigne balance/soleil couchant : suspendue et murale, deux axes ; à différencier d’un tribunal officiel.
- [x] **D03 · A · D** — Affichage des rencontres et comptoir extérieur de mise : tableau sans texte lisible, coffre fermé, jetons/bourses rangés en une composition ; pas de mécanique de pari ajoutée.

### Inlet’s Bazaar Block — T/P

- [x] **B01 · A · T/D** — Portique d’entrée de bazar et galerie de boutiques : fronton sobre, travées et angles ; fonds bâtis et toits communs.
- [x] **B02 · C · T/D** — Étals de luxe : bijoux, parfums/flacons, étoffes/tapis, objets d’art/petite statuaire et vaisselle fine ; cinq compositions sur support commun, deux orientations.
- [x] **B03 · C · T/D** — Présentoirs : vitrine basse, armoire ouverte, portants à étoffes et tapis suspendu ; versions finies avec leur support pour la pose au rez.
- [x] **B04 · C · D** — Auvents de bazar : toile unie, rayée et lambrequin raffiné ; raccords de travées et angles, variantes repliées ; réemploi des versions C10 quand identiques.
- [x] **B05 · C · D** — Réserve marchande extérieure : ballots de soie, coffrets et paniers emballés ; compléter U07/U08, sans nouvelle caisse ou nouveau tonneau identique.

### Cloaked Brewer Pub — P

- [x] **P01 · A · P/D** — Entrée et baie de pub : bois sombre, calcaire commun, encadrements distincts, volet de service ouvert/fermé ; quatre faces utiles.
- [x] **P02 · A · P/D** — Enseigne du brasseur encapuchonné : pictogramme sans lettres, deux axes ; version murale intégrée.
- [x] **P03 · C · D** — Terrasse : table ronde, table longue, chaise et tabouret, deux orientations ; compositions table garnie de chopes/cruche et table vide.
- [x] **P04 · C · D** — Livraison de boissons : petit fût sur chevalet, casier de bouteilles et tonneaux groupés ; réemploi du tonneau individuel commun.

## 9. Hippodrome — lieu T/P, architecture détaillée proposée D

Prévoir de quoi représenter **l’ensemble extérieur et la piste**, pas seulement une enseigne. Le dessin exact du circuit reste à la carte ; ces pièces permettent un ovale allongé sur la grille.

- [x] **H01 · A · D** — Enceinte basse de piste : droites, angles, sections courbes et raccords droit/courbe sur emprises entières, deux rayons modulaires compatibles.
- [x] **H02 · A · D** — Tribune de course : gradin droit, segment courbe, extrémités, angle, escalier et passage d’accès ; mêmes niveaux de hauteur d’une travée à l’autre.
- [x] **H03 · A · D** — Loge d’honneur et auvent de tribune : travée centrale, côtés, supports et bouts ; couverture cohérente avec les toits communs.
- [x] **H04 · A · D** — Entrée de l’hippodrome : arche à motif équin proposé, portail ouvert/fermé, guichet et enseigne ; quatre faces utiles.
- [x] **H05 · A · D** — Départ et arrivée : stalles de départ fermées/ouvertes, grille modulaire, poteaux d’arrivée sans texte.
- [x] **H06 · A · D** — Séparation centrale de piste : muret droit, bouts arrondis, bornes de virage et petit monument central anonyme ; forme inspirée d’un hippodrome antique, explicitement non canonique.
- [x] **H07 · C · D** — Écuries extérieures : façade de box ouvert/fermé, cloison, porte double et clôture de paddock ; modules assemblés sous les toits existants, auge/râtelier U09.
- [x] **H08 · A · D** — Accessoires de course : char de course stationné, roues de rechange et harnachement rangé ; deux orientations ; ni chevaux ni course animée dans le lot de scène.
- [x] **H09 · A · D** — Raccords de piste : sable/terre vers herbe, drainage latéral, seuil des stalles et raccord aux tribunes ; matière S11, pas un second jeu de sable.

## 10. Natural Pool, promenade et limites du quartier — familles 01, 02, 05, 07, 10

- [x] **E01 · A/C · P/D** — Natural Pool : bord rocheux bas et rive végétalisée, tronçons et angles, petites avancées irrégulières sur grille ; même eau que le commun.
- [x] **E02 · C · P/D** — Végétation de berge : roseaux, touffes humides, arbuste bas ; deux silhouettes de chaque, rochers bas de trois tailles ; à distinguer des parterres taillés.
- [x] **E03 · C · P/D** — Promenade sur quai : modules courts et extrémités des quais/soutènements, angles manquants, raccord quai/berge et descente vers l’eau ; balustrade commune réemployée.
- [x] **E04 · C · P/D** — Pontons : bouts finis, angles et raccord terre/platelage ; ne pas refaire les tabliers, borne d’amarrage ou barque existants.
- [x] **E05 · A/C · P/D** — Arching Bridge : arche porteuse, piles, culées, tablier compatible et parapets modulaires, deux axes ; construire au moins l’amorce lisible côté Arenarea, kit capable de prolonger le pont vers Oldtown.
- [x] **E06 · C · P/D** — Remparts : modules courts, angles manquants, retours sur porte et chemin de ronde visuel ; variantes avec contrefort, raccord aux tours.
- [x] **E07 · C · P/D** — Tour d’enceinte carrée : base, corps d’étage, couronnement et toiture, faces/angles ; architecture modulaire, pas une tour entière figée.
- [x] **E08 · A · P/D** — **Arena Gate, porte de ville vers Arching Bridge/Oldtown** : passage monumental, piliers, attique, battants ouverts/fermés, quatre faces et raccords aux remparts ; identité distincte de l’entrée du Colisée.

## 11. Parvis et interface avec l’Arena of Fate — familles 02, 04, 06, 10

Le LOT-106 conserve les intérieurs, gradins complets du Colisée, statues des dieux, sous-sols et catacombes. Ici, couvrir ce qui est visible et nécessaire depuis la rue. Une pièce partagée doit avoir une seule source et une seule clé adaptée au niveau de partage.

- [x] **F01 · A · V/D** — Escalier monumental : volées de largeurs 2 et 4 cases, quatre directions, marches latérales, joues et raccords à balustrade ; hauteur visuelle compatible avec les 64 px des paliers communs.
- [x] **F02 · A/C · V/D** — Parvis : paliers répétables en marbre, nez de terrasse, coins, joues et pied de soutènement ; jonctions avec S01/S02 et escalier F01.
- [x] **F03 · R · T/D** — **Porte du triomphe / accès de l’arène** : coordonner sa façade côté ville avec le LOT-106, ouverte/fermée et orientée vers le parvis ; ne pas la remplacer par l’Arena Gate.
- [x] **F04 · R · T/D** — Façade du Colisée visible depuis Gauntlet St : réexaminer les six murs/angles installés ; compléter les travées et courbes nécessaires, les trois ordres d’arcades et l’attique selon la DA existante, en coordination avec LOT-106.
- [x] **F05 · A/C · D** — Accueil de foule : guichet/billetterie, petit abri de garde, contrôle d’entrée et poste d’affichage ; réemployer cordons, barrières, braseros et panneaux précédents.
- [x] **F06 · A · D** — Deux parvis témoins : accès cérémoniel dégagé pour garde/enfant et entrée latérale de service ; compositions de validation, pas deux images de décor aplaties.

## 12. Couverture géographique à obtenir par assemblage

Cette grille évite de livrer un catalogue généreux qui ne permettrait pourtant pas de terminer une rue.

| Secteur | Assemblage attendu | Postes principaux |
|---|---|---|
| Herofate Avenue | Chaussée, trottoirs, entrées nobles, carrefour, début/fin d’avenue | S, C, M, U |
| Gauntlet St | Rue ceinturant l’arène, courbes/retours, accès public et service | S, U, F |
| Deux îlots de manoirs et leurs jardins | Façades avant/arrière, toits fermés, portails, jardins complets | C, M, J |
| Golden Chalice | Façade identifiable, entrée, accueil et livraison | G, M, U |
| Dusk of Justice | Devanture identifiable et guichet de paris | D, C, U |
| Inlet’s Bazaar | Galerie, étals variés et desserte | B, C, U |
| Cloaked Brewer | Pub, terrasse et accès de service | P, C, U |
| Hippodrome | Circuit, tribunes, départ, entrée, écuries | H, S11, U09 |
| Natural Pool et promenade | Rive fermée, eau continue, berges et quai | E01–E04, S12, J |
| Arena Gate / Arching Bridge | Porte de ville, remparts, amorce de pont et transition Oldtown | E05–E08 |
| Mapleleaf Plaza et petites places | Pavage, plantations, fontaine ou statue, bancs, sorties multiples | S, J, U |
| Rues secondaires nommées | Sanguine Lane, Greenwater St, Lost Troll Corner, Twin Corner, Murkye End, Eaglenest St, Greatboulder St, Griffon St, Ayefall St, Lost Bird Path, Orion St, Mason’s St | Réemploi des mêmes modules ; pas un kit différent par nom de rue |
| Lisières vers les quartiers voisins | Raccord de matériaux, bâti fermé et sorties visuellement lisibles | S04, C, U ; localisation finale selon la carte de référence |

Les noms Mapleleaf, Griffon ou Lost Troll n’imposent pas, à eux seuls, une statue, un arbre particulier ou une créature. Ces décors ne sont donc pas ajoutés automatiquement au lore.

## 13. Conditions de fabrication et de livraison à inclure dans la validation

- [x] **V01 — Préserver l’existant.** Sauvegarder sources/versions/prompts ; aucune reprise globale de la V4 validée. Les variantes nouvelles n’écrasent pas leurs modèles.
- [x] **V02 — Raccords réels.** Projection orthographique 0,62, case 256×159, emprises entières, ancres communes et étage 224 px ; matières projetées avec phase identique ; coins par union de volumes, pas par collage approximatif.
- [x] **V03 — Facture.** Relief lisible, matière propre, lumière haut-gauche, balustres/haies/colonnes en volume, pas de grain ni d’ombre portée ; emblème impérial repris exactement.
- [x] **V04 — Compatibilité de pose.** Le README de l’extension signale une seule couche de décor affichée au rez : livrer les façades composées nécessaires (fenêtre+lierre, mur+enseigne, etc.) ou vérifier une évolution effective du moteur avant de compter sur des calques indépendants. Ne pas multiplier toutes les combinaisons inutiles.
- [x] **V05 — Passage et hauteur.** Définir les emprises bloquantes autour des ouvertures ; escaliers, tribunes et terrasses sont des assets visuels. Le LOT-129 ne livre pas le déplacement des personnages en hauteur. Ne pas annoncer un étage jouable sans travail de carte/moteur associé.
- [x] **V06 — Réemploi Colisée.** Promouvoir les seules pièces réellement partagées avec conservation des références, ou décider d’une référence commune prise en charge ; ne pas supposer qu’Arenarea hérite automatiquement des assets de sa sous-zone.
- [x] **V07 — Suivi du poids sans limitation du kit.** La décision **D-23 du 24 septembre 2026**, ajoutée au projet pendant cet audit, supprime le budget de 40 Mio par zone, sous-zone ou commun. Le README historique de l’extension indique environ **39,5 Mio pour le commun Capitale**. Mesurer et publier les poids, optimiser les PNG sans perte, conserver le plafond de **5 Mio par fichier**, mais ne supprimer aucun besoin pour faire tenir le kit dans un budget de zone. Le classement reste fonctionnel, conforme à l’arborescence.
- [x] **V08 — Galerie et carte exhaustive.** Présenter toutes les pièces, états et orientations, avec inventaire des manques ; fonds sur 12×12, raccords droits/angles/bouts/T/X, deux étages sous toiture, bassin fermé, boutiques et hippodrome assemblés.
- [x] **V09 — Essai de quartier.** Ajouter une scène cohérente reliant avenue, manoirs, jardins, casino, parvis et au moins un secteur étendu ; contrôler le chargement moteur, les occultations et le rendu à 1080p/2160p. Distinguer ce contrôle d’un essai de gameplay.
- [x] **V10 — Inventaire traçable.** Après validation de cette liste, établir les identifiants définitifs, nombres de PNG, emprises, états, orientations, dépendances, coût estimé et destination de chaque poste ; réviser la commande initiale de 45 pièces et la mention de suspension devenue obsolète du LOT-108.

## 14. Ordre proposé après validation

1. Résoudre les réemplois, la pose et les identifiants, estimer le poids ; produire les matières manquantes et les sols/transitions.
2. Fermer les trous du commun : raccords, accès, clôtures, façades, finitions de toiture et voirie.
3. Construire les manoirs/jardins et la fontaine signature ; valider un îlot complet.
4. Produire les établissements nommés, le mobilier et leurs variantes.
5. Étendre à l’hippodrome, Natural Pool, porte de ville et pont ; coordonner la façade du Colisée.
6. Monter les assemblages exhaustifs puis la scène de quartier, contrôler et soumettre le rendu artistique.

**Périmètre proposé : tous les postes S, C, M, J, U, G, D, B, P, H, E et F ci-dessus**, y compris les compléments communs. Les postes R exigent une coordination avec le LOT-106 ; ils ne constituent pas une seconde commande concurrente du même monument. Les intérieurs complets du casino/pub/manoirs, personnages, animaux animés, interfaces, carte finale LOT-109 et gameplay restent des travaux distincts. Les accessoires extérieurs, cours, écuries visibles et tribunes de l’hippodrome sont bien inclus.

La validation peut porter sur l’ensemble ou sur les identifiants à retirer/modifier. Aucune image, aucun manifeste de production et aucun asset existant n’a été modifié pour préparer cette proposition.
