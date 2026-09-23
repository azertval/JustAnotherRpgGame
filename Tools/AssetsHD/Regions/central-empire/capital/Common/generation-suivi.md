# LOT-105 — Production du 23 septembre 2026

28 sources PNG produites avec imagegen intégré, à partir des envois de `commande.md` et de la planche originale d'Arenarea. Les références de chaque série ont été jointes dans l'ordre prévu. Aucun extrait du livre n'a servi d'image source.

## Première livraison

- 24 sources sous ce dossier (Sols, Facades, Balustrades, Vegetal, Mobilier), donnant 31 pièces installées.
- 4 sources impériales sous `../../Common/Bannieres/`, donnant 4 pièces installées dans le commun régional.
- Les deux manifestes de scène et leurs PNG sous `Source/Elements/Assets/Regions/central-empire/`.
- Galerie `apercus/index.html`, reconstruite avec `python scripts/build_capital_gallery.py` : 35 pièces, fond clair/sombre, filtre par famille, composition de douze cases de long sur trois de large avec les ancres installées.
- Prompts effectivement employés dans `generation-prompts.json` (sources de travail locales). Les orientations V ont reçu une précision sur le sens de leur diagonale.

## Reprises et installation

Le mur V a été régénéré pour corriger son orientation. Le massif fleuri a été régénéré pour respecter une emprise carrée et une bordure basse. Le lampadaire a été régénéré sans la bannière ajoutée au premier essai. Les essais écartés ne sont pas installés.

L'installateur du projet découpe les trois planches de sols, nettoie l'alpha, réduit et inscrit les pièces. Aucune retouche dessinée ni miroir n'a été appliqué aux sources. Les objets dont la silhouette ne décrit pas correctement le socle ont des `scale` et `anchorOffset` explicites dans les descripteurs : pilier, cyprès, lampadaire, vasque, tonneau, caisse, bannière et colonne. Les sols installés mesurent 256 × 159 px.

## Contrôles

`scripts/check_hd_assets.py` passe. Les deux commandes `install_hd_asset.py …/install.json --check` passent : 31 + 4 pièces à jour des sources. Poids total installé d'environ 2,6 Mio, inférieur au budget de 40 Mio.

## Relecture encore ouverte

Le lot reste en cours : l'acceptation artistique appartient à l'auteur. Vérifier dans la galerie les raccords entre variantes de pavés, la hauteur des angles par rapport aux murs droits, les raccords des balustrades et haies, ainsi que la palette et la lumière. La composition de galerie n'est pas une validation du rendu dans le moteur ; aucun test de scène en jeu n'a été effectué durant cette production.

## Révision 2 — raccords et grille (23 septembre 2026)

Cette révision remplace les réglages de la première livraison. L’auteur a explicitement autorisé le « recalage précis par script » pour la projection et les raccords.

- Bannière régénérée depuis `References/lampadaire-v1.png` : bourgogne, liseré ancien or, tête de lion couronnée de profil et feuillage. Le même motif est installé sur les deux murs à bannière.
- Quatre nouvelles sources de murs droits et fenêtres, sans piliers terminaux, avec profils communs.
- 35 pièces recalées et réinstallées : plans projetés sur 256 × 159, contacts au sol et ancres explicites. Les bancs et l’étal gardent leurs montants verticaux.
- Sols redressés et bords opposés harmonisés par mélange des textures existantes ; variantes avec périmètre commun. Balustrades et haies projetées avec les mêmes axes.
- Originaux conservés. Les PNG `Calibres/` sont reproductibles par `scripts/rectify_capital_kit.py` ; les deux descripteurs d’installation les référencent.
- Prompts des cinq nouvelles générations : `generation-prompts-v2.json`.
- Galerie reconstruite, directions U/V et grille sur chaque fiche ; composition fixe `apercus/rue-v2.png`.

Les cinq tests ciblés de projection, raccord, masque et verticalité passent, ainsi que le contrôle HD global et les deux vérifications d’installation (31 + 4 pièces). Inspection visuelle effectuée sur les compositions locales ; la vérification automatique du navigateur local est bloquée par sa politique d’URL. Validation artistique et rendu en jeu restent ouverts.

## Map visuelle de validation

`apercus/validation.html` réunit les 35 assets installés en 353 placements, sur quatre plateaux : sols et bordures, murs U/V et angles, balustrades et haies, mobilier et emblèmes. Zoom, grille, ancres, filtre de zone et sélection par nom. Les ancres des manifestes sont appliquées telles quelles pour exposer les défauts restants. Le clic utilise l’alpha si le navigateur autorise la lecture des PNG locaux ; sinon il utilise leur rectangle.

Reconstruction : `python scripts/build_capital_validation.py`. Données : `apercus/validation-map.json`. Export pleine taille : `apercus/validation-map.png`. Ce banc visuel local n’est pas une carte jouable du moteur. Couverture 35/35 vérifiée, fichiers présents, placements dans les limites et syntaxe JavaScript contrôlée ; vue complète inspectée en image.

Correction des assemblages : angle rentrant à la jonction des deux grandes rangées de murs, angle sortant raccordé à deux murs droits, angle de haie au changement de direction. Les 35 assets restent présents. L’angle sortant actuel longe les bords éloignés de sa case : ses murs voisins exigent un décalage de 7/8 de case sur leur axe transversal ; ce placement explicite expose une convention à harmoniser pour une pose entièrement sur cases entières. Vue complète reconstruite et inspectée.

## Révision 3 — angles et répétition du sol

Les trois familles modulaires partagent maintenant un axe au centre de case : murs, balustrades et haies. Les angles rentrants partent vers U+/V+ ; les sortants reçoivent U-/V-. Les raccords utilisent exclusivement des coordonnées entières, sans le décalage de 7/8 de case de la révision précédente. Les angles de haie ne possèdent plus le tronçon superflu en T.

Trois pièces ajoutées : `balustrade-limestone-corner-inner`, `balustrade-limestone-corner-outer`, `plant-hedge-corner-outer`. Le kit compte désormais 38 pièces (34 Capitale, 4 Empire). La map de validation les emploie toutes dans des assemblages avec leurs voisins.

Deux textures planes de sols générées par imagegen intégré, sauvegardées dans `Sols/floor-paving-texture-v3.png` et `Sols/floor-flagstone-texture-v3.png`. Prompts : `generation-prompts-v3.json`. Palette calcaire moins contrastée, joints réguliers conservés à la même position dans toutes les variantes ; seuls les tons minéraux varient légèrement. Suppression du décalage interne des anciennes variantes qui cassait les joints. Harmonisation des bords par script et projection sur 256 × 159.

Six tests ciblés passent, dont les douze raccords U/V des angles rentrants/sortants des trois familles. Les vérifications des deux installations et du budget HD passent. La nouvelle map a été inspectée en image ; le rendu en jeu reste à valider.


## Révision 4 — 23 septembre 2026

Les 38 pièces sont remplacées par une nouvelle version issue de 19 sources imagegen et de neuf reprises de nettoyage. Reconstruction géométrique commune des angles de murs, balustrades et haies ; textures périodiques en coordonnées de grille, sols à joints fixes et faible variation de teinte. Colonne et bannières utilisent la tête de lion couronnée avec laurier. Version précédente conservée dans `Versions/before-v4`.

Installation vérifiée pour les 38 pièces, budgets conformes, quatre tests ciblés réussis. Les 359 placements de la map couvrent toutes les pièces. Le véritable LevelEditor a appliqué la carte et produit deux rendus ; contrôle : zéro erreur, avertissements sur le gameplay difficult/cover non encore implémenté. Égalité pixel par pixel des PNG installés avec ceux testés dans le moteur, et égalité des ancres. Ce contrôle ne remplace pas un essai de déplacement en partie. Galerie et map interactive régénérées, cache des images indexé par empreinte source.

Détails, sources, prompts et preuves : [V4](V4/README.md). L’appréciation artistique finale reste à confirmer par l’utilisateur.


### V4 — reprise ciblée du relief

Sols moins lisses, petits piliers des balustrades construits en volume, buissons avec couronne arrondie. Reprise demandée par l’utilisateur après validation du reste du kit. Sources et prompts dans `V4/`, état précédent dans `V4/BeforeRelief/`.
