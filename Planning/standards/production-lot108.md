# LOT-108 — Production coordonnée

Production lancée le 24 septembre 2026 après validation auteur de la checklist complète et demande explicite de répartir le travail entre agents.

## Répartition et propriété des fichiers

| Chantier | Responsable | Postes | Dossier de préparation exclusif |
|---|---|---|---|
| Architecture commune et noble | Agent architecture | C01–C11, M01–M12 | `Production108/architecture/` |
| Jardins, eau, limites et pont | Agent jardins_berges | J01–J16, E01–E08 | `Production108/jardins-berges/` |
| Commerces et vie urbaine | Agent commerces_mobilier | G, D, B, P, U | `Production108/commerces-mobilier/` |
| Sols, grands équipements et intégration | Agent principal | S, H, F, validation V | `Production108/sols/`, puis autres sous-dossiers dédiés |

Racine de préparation : `Tools/AssetsHD/Regions/central-empire/capital/arenarea/Production108/`.
Les agents produisent les sources, les prompts, les variantes et les descripteurs dans leurs propres dossiers. **Un seul responsable installe dans les manifests de production : l’agent principal**, après examen des sorties. Les fichiers existants et les modifications d’autres travaux sont préservés.

## Contrat commun

- Les 102 postes de la [checklist](checklist-lot108-arenarea.md) restent la référence de couverture ; leurs cases signifient livré, pas simplement démarré.
- Outil imagegen intégré pour les nouvelles peintures. Sources conservées et prompts archivés. Réemploi des matières approuvées ; projection et recalage géométriques autorisés, aucune substitution par un dessin procédural générique.
- Grille 256×159, projection orthographique 0,62, hauteur d’étage 224 px, lumière haut-gauche. Angles et raccords continus. Référence V4 après reprise du relief.
- Aucun plafond de poids par kit (D-23). Plafond de 5 Mio par fichier, mesure du poids et optimisation sans perte.
- Les matières et sources de base peuvent être partagées en lecture. Pas de création concurrente d’un même objet commun, pas d’écrasement des fichiers validés.
- Les générations des familles indépendantes avancent en parallèle conformément à la demande auteur. L’installation et les assemblages restent contrôlés progressivement.
- Chaque chantier conserve un `status.json` : postes couverts, fichiers produits, provenance, vérifications, manques. « Source générée », « préparé », « installé » et « livré en assemblage » sont des états différents.
- Réserves de gameplay conservées : ouvertures et collisions à configurer, hauteur de décor différente d’un étage praticable.

## Progression

### Retour auteur — entrée de l’hippodrome

Le 24 septembre 2026, le cheval sculpté au sommet de l’entrée H04 est accepté pour annoncer l’hippodrome. Les bannières doivent en revanche porter **le lion impérial**, fidèle au lampadaire v1 / à la bannière impériale validée. La version source avec chevaux sur les bannières n’est pas une version à installer. Retouche ciblée, original conservé.

La première vague ouvre les matières de sol, les façades nobles, la fontaine et les grilles, ainsi que les enseignes et entrées des établissements. Les autres postes restent à produire jusqu’à preuve de leur présence dans la galerie et les assemblages. Ce document ne déclare aucune livraison anticipée.

### Premier contrôle moteur

Un premier instantané de **719 nouvelles pièces préparées** a été chargé le 24 septembre : architecture 119, commerces 48, sols 284, jardins 153, équipements 115. Les **22 cartes d’essai**, rendues aux échelles 1 et 2, passent le contrôle moteur sans erreur. Preuves conservées dans `Production108/engine-check-wave1.json` et `snapshot-wave1.json`. Il s’agit de modules et variantes, pas de 719 objets distincts.

Ce contrôle ne clôt pas les postes : inspection visuelle, assemblages complets et installation restent distincts. Il a conduit à reprendre la matière de gravier et celle de la piste, l’orientation du deuxième char, ainsi que les linteaux de façade. Les sources rejetées restent archivées. Un contrôle réussi sur un instantané ne valide pas automatiquement les fichiers modifiés ensuite.

### Installation pour revue auteur — 24 septembre 2026

**1 491 nouveaux PNG sont installés** : 1 029 dans le commun Capitale et 462 dans Arenarea. Leurs identifiants, emprises, ancres, poids, provenance et destinations sont consignés dans `Production108/installation.json`. Les 705 PNG communs antérieurs sont préservés ; les variantes de toiture qui entraient en collision ont été renommées avant installation. `appearance.json` fournit les variantes de marbre et de piste propres à la zone.

Les trois agents ont produit l’architecture, les commerces et les jardins/berges ; l’agent principal a produit sols et équipements puis assuré l’installation. L’audit des 102 postes a conduit à ajouter les compositions murales, angles du casino, dos de l’entrée d’hippodrome, raccords de la porte du triomphe, toiture avec solin et approches opposées du pont.

La galerie `Production108/galerie.html` présente toutes les pièces et les assemblages. Les preuves moteur incluent demeures, rues commerçantes, jardin fermé, quai/pont, écurie couverte, hippodrome, arène sur quatre étages, deux parvis et quartier témoin. Les contrôles moteur et HD sont conservés dans `delivery-report.json`, `engine-check-wave2.json`, `engine-check.json`, `production-check.json` et `hd-check.md`. Le chargement hiérarchique commun/zone a aussi été vérifié avec les fichiers installés.

Poids après installation : commun Capitale **101,9 Mio**, Arenarea **56,6 Mio**, sans plafond par kit et sans dépassement de 5 Mio par PNG d’asset. Les grandes images de contrôle restent dans l’atelier, hors des kits du jeu.

État : **installé pour revue artistique de l’auteur**. Les cases de la checklist ne sont pas cochées automatiquement par les compteurs. Les cartes témoins sont des compositions de contrôle, pas la carte canonique du LOT-109 ni une preuve de gameplay vertical. Les contraintes d’occupation des ouvertures et les réemplois sont documentés dans le README et l’inventaire.


### Reprise après la revue auteur — 24 septembre 2026

Les réserves sur le quadrillage du marbre, la lecture des tribunes, l’approche de l’Arching Bridge et l’absence de quai continu sont reprises dans `Production108/Review108/`. La matière de marbre S01 est repeinte avec imagegen intégré ; ses transitions et seuils sont reconstruits. Les tribunes possèdent des assises distinctes et font face à la piste dans le témoin. L’approche montante du pont comporte des marches et des parapets raccordés. Le quai est complété par les modules existants. Les grandes places du quartier témoin emploient désormais S01 ; le pavage commun LOT-105 reste intact.

87 PNG modifiés, 93 pièces ciblées contrôlées ; les 1 398 autres pièces installées sont vérifiées identiques à l’inventaire antérieur. Versions précédentes, scripts, sources et prompts sont conservés. Les contrôles moteur et de chargement hiérarchique sont consignés dans `Review108/`, avec comparaison avant/après ×1 et ×2. Cette reprise reste **proposée à la validation artistique de l’auteur**, sans clôture automatique de la checklist ni validation de gameplay.


### Deuxième retour auteur — S01 et terminaisons gauches C10/B04

La première reprise du marbre ne satisfaisait pas S01 : deux fichiers dupliqués et un relief trop pâle. `Production108/Review108B/` contient six fonds issus de six peintures distinctes, avec joints communs et sans cadre par case. Le contraste de luminance mesuré sur les PNG installés est compris entre 12,56 et 15,43. Le contrôle exige désormais six tableaux de pixels distincts ; les raccords et la proximité de luminosité moyenne sont vérifiés.

Huit terminaisons gauches d’auvents reçoivent un ourlet en volume, lisible même lorsque la face latérale est occultée. Les modules courants et droits sont conservés. 73 PNG ciblés sont mis à jour ; les 1 418 autres pièces de l’inventaire sont vérifiées identiques, dont le pont, le quai et les tribunes. La répétition de marbre, le quartier témoin et les assemblages d’auvents passent le contrôle moteur et disposent de rendus ×1/×2. Sources, prompts, anciennes versions et résultats chiffrés sont conservés dans le dossier de reprise. La validation artistique reste ouverte.
