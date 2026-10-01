+++
id = "LOT-1000"
titre = "La preuve de la chaîne de personnages"
version = "0.0.2.5"
filiere = "pnj"
statut = "livre"
taille = "M"
resume = "Deux personnages générés par Meshy d'après les figurines validées, préparés pour l'animation et rendus en bandes : preuve de la chaîne de personnages 3D."
prerequis = []
livrables = [
  "Dans l'atelier local (hors dépôt) : les modèles maîtres Meshy validés du brawler et du scoundrel, leurs références et paramètres ; des copies préparées avec squelette et les cinq animations que lit le moteur (repos, marche, attaque, touché, mort). La marche est l'animation jugée ; les quatre autres sont des poses clés simples.",
  "Le **brawler** : géométrie Meshy fidèle à la figurine, texture peinte, équipement lié correctement au squelette et export `.glb`. Les sources de textures déjà acceptées sont conservées.",
  "Un **deuxième personnage** (le scoundrel, corps différent) passé par les mêmes scripts, sans les modifier.",
  "`scripts/assetsGeneration/render_character_strips.py` : le modèle animé rendu par Blender au format des bandes actuelles (8 images, 4 orientations, cellule du standard), et ses tests ; `install_hd_asset.py` accepte une bande `placed`, déjà au format du moteur.",
  "Les 20 bandes du brawler ainsi rendues, installées dans `Common/Characters/Heroes/brawler/` ; le kit `Common` republié.",
  "Le **verdict** de l'auteur, écrit en fin de fiche : la version s'ouvre, ou s'abandonne.",
]
criteres = [
  "Le modèle du brawler se reconnaît à côté de son portrait (jugement de l'auteur).",
  "Dans le jeu, le brawler marche dans les quatre diagonales avec des appuis alternés, sans glissement, sans retouche à la main d'aucune image.",
  "Le deuxième personnage passe par la chaîne sans changer une ligne des scripts : seuls sa fiche, sa texture et ses pièces diffèrent.",
  "À 100 px par case, le brawler rendu tient à côté du décor actuel (jugement de l'auteur).",
  "Aucune bande générée du brawler ne reste, ni dans le kit ni dans le dépôt : `check_hd_assets.py` passe.",
]
+++

## Pourquoi

**Décision actualisée du 1er octobre 2026 :** après refus des corps MPFB et des
essais TripoSR, l'auteur valide les deux géométries Meshy présentées sans texture,
active un abonnement et demande de réaliser le LOT-1000 avec Meshy comme
générateur. Cette décision remplace l'hypothèse initiale du corps commun ci-dessous.
La validation des formes ne vaut pas encore recette de la marche ni du rendu dans
le jeu. Blender demeure l'atelier de préparation, d'animation et de rendu.

Tout le passage à la 3D repose sur une hypothèse que rien n'a encore prouvé dans ce projet : qu'un
personnage se fabrique à partir d'un **corps commun**, d'une **texture peinte** et de **pièces
d'équipement**, et qu'il reste reconnaissable. Si elle tient, le générateur d'images ne dessine
plus jamais un mouvement — la cause des marches refusées le 24 septembre. Si elle ne tient pas, il
vaut mieux le savoir avant d'avoir touché au moteur.

## Périmètre

Dedans : un corps, un squelette, la **marche** soignée, deux personnages, le rendu en bandes,
l'installation du brawler. Les quatre autres animations du moteur (repos, attaque, touché, mort)
sont posées aussi, simplement : sans elles le brawler n'aurait pas de figurine entière — le moteur
dessine un damier à la place d'une bande absente — et ses bandes générées ne pourraient pas partir
(décision de l'auteur, 30 septembre 2026).

Dehors, nommément :

- le moteur — pas une ligne de C++ ne change ; le modèle se juge **dans le jeu 2D**, par ses bandes
  rendues ;
- les sept autres corps, le sort (`cast`) et la reprise soignée des animations
  ([LOT-1006](LOT-1006-corps-de-reference.md)) ;
- le standard écrit ([LOT-1001](LOT-1001-standard-3d.md)), qui se rédige **d'après** ce que ce lot
  mesure, comme le `LOT-101` l'a fait d'après sa maquette.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Les 20 bandes **générées** du brawler et leurs `.anim.json` | `Source/Elements/Assets/Common/Characters/Heroes/brawler/` | les bandes rendues les remplacent, fichier pour fichier ; deux jeux de bandes pour un héros serait un asset mort |
| L'avis de refus des marches | `Planning/quality/brawler-walk-review.json`, et le dossier `Planning/quality/` s'il reste vide | il porte sur des fichiers qui n'existent plus |
| Les sources 2D du brawler | atelier local, `Tools/AssetHd/NPC/Classes/LOT-136-v1/brawler/` | à archiver hors du poste de travail par l'auteur |

Si la porte **ne s'ouvre pas**, rien n'est supprimé : les bandes rendues ne sont pas installées, le
lot passe `abandonne` avec sa raison, et la version avec lui.

## Conception

- **Blender est l'atelier, pas le générateur.** Il se pilote par script, sans fenêtre (vérifié le
  30 septembre : maillage, squelette, poids automatiques, animation, export `.glb`, Blender 5.2.2).
- **La forme vient du corps**, la même pour tous ; l'identité vient de la texture et des pièces.
  Cheveux et barbe sont des pièces, pas de la texture.
- **La texture** se commande au générateur d'images en deux vues, face et dos, dans la pose de
  liaison du corps ; un script la projette sur le dépliage du corps. Le générateur n'a qu'une image
  fixe à réussir.
- **Le corps vient de MPFB** (MakeHuman pour Blender, extension de `extensions.blender.org` ; code
  GPL, maillages humains CC0), choisi par l'auteur le 30 septembre : sexe, musculature et poids sont
  des curseurs d'**un seul** maillage, donc les huit corps du LOT-1006 partagent topologie, dépliage
  et squelette (`game_engine`, 53 os). Le script applique les formes et retire la géométrie d'aide ;
  le `.glb` n'emporte rien de MPFB.
- **Les poses se disent en cibles**, pas en angles d'os : une cheville à atteindre (deux os résolus
  analytiquement, genou vers l'avant), une main à placer, une direction où pointer l'arme. Les mêmes
  fonctions animent tous les corps du squelette.
- **La marche tient la règle du moteur** : un cycle couvre une case de 1,5 m en 0,5 s. Le pied posé
  recule exactement de cette vitesse (attaque du talon, déroulé, départ sur la plante) ; le bassin
  descend juste assez pour que les deux jambes atteignent leurs chevilles.
- **La caméra du rendu est celle du jeu** : orthographique, 45°, inclinée de asin 0,62 ≈ 38,3°, à
  120,7 px par mètre (une case de 1,5 m fait 256 px, un corps de 1,80 m 170 px — la hauteur du
  standard, sans réglage). Le pied tombe au milieu de la cellule sur la ligne de sol par
  construction ; la bande s'installe **`placed`**, copiée telle quelle, sans le recentrage sur la
  boîte que demandaient les images du générateur (il déplacerait une figurine qui porte une hache).
  Un pied posé devant ou à côté passe sous la ligne de sol à l'écran : la cellule s'allonge par pas
  de 8 px, la règle d'installation existante.
- **Le rendu en bandes** reprend exactement le format lu par le moteur : le jugement se fait donc à
  la vitesse du jeu, sur une carte, à côté du décor. Ce rendu reste un **repli** si la suite de la
  version glisse : il donne déjà la production stable, sans jour / nuit.

## Risques et questions ouvertes

- L'auteur a tranché que le corps commun, la texture et les pièces **suffisent** (D-31). Si le
  brawler ne se reconnaît pas, la décision se rouvre : le repli est un générateur image → maillage
  lié au même squelette par script. Aucun n'a été essayé ; c'est un autre lot, pas un prolongement de
  celui-ci.
- Les vêtements amples (la robe du mage) ne se peignent pas sur des jambes : ils demandent une
  pièce liée aux os des jambes. Ce lot ne le prouve pas — le brawler et le scoundrel n'en portent
  pas ; le [LOT-1009](LOT-1009-les-quatre-heros.md) le rencontrera.

## Réalisation locale — 1er octobre 2026

L'auteur a refusé les premiers corps et demandé une reprise d'après les silhouettes des figurines
de référence : brawler massif avec un visage de demi-orc, scoundrel avec vêtements et bottes en
volume. Cette demande remplace, pour cette preuve, l'hypothèse d'un habillage entièrement peint
sur le corps nu. Les premiers essais sont conservés dans
`Tools/Assets3D/Versions/LOT-1000-refuse-2026-10-01/`.

La proposition suivante reste dans `Tools/Assets3D/Characters/`, hors dépôt :

- `brawler/build/brawler.blend` et `.glb`, `scoundrel/build/scoundrel.blend` et `.glb` ;
- morphologies reprises avec déformation cohérente du maillage et du squelette de repos ;
- textures peintes avec imagegen intégré, projetées et incorporées aux exports ;
- enveloppes de vêtements pesées sur le squelette, chaussures fermées, défenses du brawler,
  coiffures et équipements séparés ;
- `apercu_modeles.py` pour les vues face, dos et trois-quarts ; `verifier_exports.py` pour contrôler
  les données binaires GLB, les poids, les indices, les 53 os et les cinq animations ;
- `preparer_revue.py` pour la revue locale `revue-v2/index.html` et sa planche.
- vingt bandes rendues par personnage dans `<nom>/bandes-v2/`, avec descripteurs `placed`,
  contrôlées par l'installateur en mode mesure ; aperçus de marche dans `<nom>/revue/marche-v2.webp`.

Les 65 tests du rendu en bandes et de l'installation passent, dont un nouveau cas qui garantit
que le corps massif couché reste entier. La réserve sous la cellule de rendu passe de 128 à
192 px ; l'échelle et l'ancre restent inchangées. La mesure des appuis sur
les squelettes remodelés donne un glissement maximal de **0,45 px d'art pour le brawler** et
**0,53 px pour la scoundrel**, sur six paires d'appui chacune. Ce contrôle du squelette ne remplace
ni le jugement visuel des semelles et des vêtements animés, ni un essai dans le jeu.

### Verdict de l'auteur

**Textures acceptées, maillages V2 refusés.** L'auteur demande de retrouver les formes et la
précision des figurines 2D. Les volumes procéduraux actuels ne satisfont pas ce critère ; augmenter
leur subdivision ne constitue pas une correction suffisante. Le lot reste `en-cours`.

Les textures et les modèles correspondant à ce verdict sont conservés dans
`Tools/Assets3D/Versions/LOT-1000-v2-textures-validees-modeles-refuses/`, avec leurs empreintes.
Préserver les textures sources validées ; une nouvelle topologie pourra demander leur transfert
vers de nouveaux UV, sans changer leur direction artistique.

La prochaine proposition doit être jugée aussi en matériau neutre : anatomie et visage de
demi-orc, mèches et tresses, plis et épaisseur des vêtements, revers des bottes et profils des
armes doivent exister dans le maillage. Comparer les silhouettes aux figurines d'origine et
présenter des gros plans avant de relancer les quarante bandes. L'hypothèse D-31 de suffisance
du corps commun et de pièces simples est donc rouverte. Une reconstruction image vers maillage
est une piste à essayer, pas un résultat acquis ni une garantie de fidélité.

L'installation du brawler, la suppression des anciennes bandes et de leur avis de refus, puis
la republication de `Common` restent conditionnées au verdict favorable prévu par cette fiche.
Aucun remplacement des assets du jeu n'a été effectué pendant cette reprise.

### Essai local TripoSR — 1er octobre 2026

À la demande de l'auteur, un essai local est réalisé avant d'envisager Meshy.
Environnement isolé et résultats : `Tools/Assets3D/TripoSR-test/` (hors dépôt).
Les deux images `master-se.png` du LOT-136-v1 alimentent le modèle officiel
TripoSR ; les images restent locales. Poids téléchargés et empreinte vérifiée.

L'inférence fonctionne sur la RTX 4060 Ti 8 Go : pic alloué par PyTorch d'environ
2 507 Mio. Après chargement initial, génération et extraction à résolution 256
prennent 24,2 s pour le brawler (première inférence) et 6,9 s pour la scoundrel.
L'extraction de surface est exécutée sur CPU avec scikit-image ; les formes
sont calculées par TripoSR sur CUDA. Les détails de reproduction et les versions
sont conservés avec les résultats.

Exports expérimentaux : 71 180 triangles pour le brawler et 49 262 pour la
scoundrel, fichiers GLB et Blender, sans squelette ni animations. Une planche
et une revue locale montrent les références, les couleurs inférées et les
maillages en argile sous quatre angles. Les textures précédemment acceptées
restent intactes ; les couleurs de cet essai ne les remplacent pas.

**Constat de revue de l'agent, pas verdict de l'auteur :** les silhouettes sont
partiellement retrouvées, mais les visages et détails sont émoussés ; les armes
présentent des déformations et discontinuités. Ce test prouve la faisabilité
locale, pas l'atteinte de la qualité visuelle demandée. Aucun passage à Meshy,
installation dans le jeu ou remplacement des modèles courants n'est effectué.

### Essais approfondis TripoSR — exploitation de la marge matérielle

L'auteur juge le premier résultat inutilisable et demande de pousser la solution
locale avant de la rejeter. Le constat précédent est donc limité au réglage initial.
La nouvelle revue est conservée dans `Tools/Assets3D/TripoSR-test/quality-study/`.

Pour chaque personnage, 19 variantes supplémentaires explorent les extractions
512 et 1 024, les cadrages 75 %, 85 % et 95 %, les seuils 10, 25 et 50, et les
entrées 768, 1 024, 1 280 et 1 536 en plus de l'entrée native 512. Le dernier essai
combine entrée 1 536 et extraction 1 024. Les tailles d'entrée supérieures à 512
sont expérimentales et ne constituent pas des modes officiels de qualité.

La résolution d'extraction correspond à de nouveaux échantillons du champ prédit,
sans subdivision artificielle. La grille 1 024 évalue 64 fois plus de positions que
la grille 256. Les blocs de calcul ont été calibrés jusqu'à 4 194 304 points et
5 772 Mio alloués par PyTorch. L'entrée 1 536 atteint 9 666 Mio d'allocations CUDA :
ce compteur dépasse les 8 Go physiques et ne représente pas une quantité de VRAM
entièrement résidente. Ces essais ont tous terminé.

La combinaison la plus poussée exporte **1 141 746 triangles pour le brawler** et
**738 548 pour la scoundrel**. La revue compare 40 versions au total, résultats
initiaux compris, sous le même éclairage neutre, avec corps entier, visage et dos.
Les paramètres, temps, empreintes et fichiers GLB sont conservés pour chaque essai.

**Constat de l'agent :** les entrées plus grandes améliorent notamment la forme de
la hache du brawler ; les extractions fines rendent les surfaces plus précises.
Elles ne suffisent cependant pas à restituer les visages de référence ; les mains
et certaines armes restent déformées ou interrompues. Les réglages les plus élevés
ne sont pas systématiquement les plus fidèles. Ce constat porte sur les références
et la matrice testées, pas sur toutes les utilisations possibles de TripoSR.
Le verdict de l'auteur sur ces variantes reste à recueillir. Les textures validées
et les assets installés sont préservés ; le lot demeure `en-cours`.

### Passage à Meshy — décision de l'auteur

L'auteur juge les essais approfondis TripoSR non convaincants et demande de passer
à Meshy. Les références originales et leurs empreintes sont préparées dans
`Tools/Assets3D/Meshy-test/`. Les textures validées restent conservées.
Après connexion et choix explicite de l'essai gratuit sous licence CC BY 4.0,
deux modèles ont été générés avec Meshy 7.1 Flagship, Détails élevés, résolution
Standard, sans texture ni amélioration d'image, depuis les figurines SE.
Coût : 40 crédits au total. Le visualiseur indique 1 510 770 faces pour le brawler
et 1 249 566 pour le scoundrel. Des captures de face, de profil et de dos sont
conservées dans `Tools/Assets3D/Meshy-test/`.

Les volumes et accessoires apparaissent nettement mieux définis que dans les
essais locaux ; leur fidélité reste à valider par l'auteur. Le téléchargement GLB
ouvre une offre d'abonnement : aucun export local obtenu, aucun achat effectué.
L'intégration et l'animation restent donc à réaliser après validation et accès
à l'export. Les textures acceptées sont préservées ; le lot reste `en-cours`.

### Modèles Meshy validés — poursuite du lot

L'auteur confirme : « les deux models sont bon », annonce avoir ajouté un
abonnement et demande de faire le lot avec Meshy comme générateur. Les deux
géométries de l'essai deviennent les maîtres approuvés ; aucune régénération de
leur forme n'est nécessaire. L'abonnement est observé actif dans l'espace Meshy.

La réception locale des exports, la préparation des textures et du squelette,
les cinq animations, les quarante bandes de contrôle, l'installation des vingt
bandes du brawler et la publication du kit Common restent à vérifier. Le lot ne
sera clos qu'après ces livrables et la recette visuelle en mouvement.

Point de reprise actualisé : l'auteur a déposé deux GLB dans l'atelier. Le brawler
est la copie Meshy à 102 894 faces ; le scoundrel est le maître à 1 249 566 faces.
Ces fichiers ne contiennent aucune texture, aucun squelette et aucune animation.

Deux copies texturées 4K ont ensuite été produites dans Meshy, depuis les figurines
SE originales, pour 20 crédits au total. La copie du scoundrel est préalablement
réduite par Meshy à 102 988 faces. Les captures `texture-meshy.png` des deux dossiers
en conservent le résultat ; le téléchargement piloté ne remet toujours aucun
fichier. Les deux GLB **texturés** restent à récupérer, puis à contrôler localement.
Les anciennes textures acceptées sont conservées.

Des copies locales de travail possèdent un squelette commun de 53 os et cinq clips
de preuve. Dix bandes SE neutres ont été rendues. Les poids sont normalisés ; un
contrôle du contact de la surface avec le sol a conduit à corriger la translation
verticale de la racine à chaque image. Après correction, les 40 poses échantillonnées
par personnage sont finies ; la pénétration maximale résiduelle est de 1,3 mm pour
le brawler et inférieure à 0,1 mm pour le scoundrel. Ces mesures ne sont pas une
validation visuelle des mouvements.

**Limite bloquante pour la recette des animations :** les bras et les armes suivent
encore le buste. Cette preuve conserve les prises sculptées mais ne constitue pas
une attaque finale. Il reste à articuler correctement les bras et leurs équipements,
contrôler les textures exportées, produire et examiner les quarante bandes, puis
effectuer l'intégration et la recette prévues. Aucun asset du jeu n'a été remplacé.

### Livraison Meshy texturée et intégration locale — 1er octobre 2026

L'auteur dépose les deux exports texturés : `Meshy_AI_Braided_Axe_Warlord_1001121243_texture.glb`
et `Meshy_AI_Hooded_Duelist_Ranger_1001121251_texture.glb`. Chacun contient une texture
4096 × 4096 incorporée. Les géométries utilisées ont respectivement 102 894 et
102 988 triangles ; aucune réduction supplémentaire n'est appliquée à ces exports.

Les livrables sont dans `Tools/Assets3D/Meshy-test/<personnage>/animation-texturee/` :
Blender, GLB autonome, paramètres et rapports. Les deux personnages passent par
`preparer_animation.py` avec leurs seules fiches `rig-config.json`. Les bras sont
désormais articulés par cibles ; la hache et ses deux prises suivent une transformation
coordonnée, les deux lames du scoundrel suivent leurs mains. Les masques d'équipement
ont été corrigés après observation des premières déformations. La réaction au coup
du scoundrel est réduite pour tenir dans la cellule standard sans couper son épée.
Les quatre clips autres que la marche restent des poses simples, conformément au périmètre.

Les **40 bandes** (5 clips × 4 orientations × 2 personnages) sont rendues en huit
images. Tous les contrôles de cadrage passent. La marche dure 0,5 s, soit une case
de 1,5 m à deux cases/s ; ses images durent 62,5 ms. Les exports vérifiés contiennent
53 os, cinq clips, UV, texture incorporée, indices valides et poids normalisés.
Les contrôles de contact sur les maillages évalués conservent une pénétration
résiduelle maximale de 1,3 mm pour le brawler, inférieure à 0,1 mm pour le scoundrel.
Les 65 tests du rendu et de l'installation passent.

Les vingt bandes du brawler sont installées par `install_hd_asset.py`, avec leur
provenance sous `Tools/AssetsHD/Common/Characters/Heroes/brawler/lot1000-meshy/`.
Le portrait et le jeton installés sont inchangés. Les anciens fichiers et l'avis
de refus sont sauvegardés dans `Tools/Assets3D/Meshy-test/integration/avant-meshy/` ;
l'avis portant sur les anciennes marches est retiré du planning actif.
`check_hd_assets.py` passe (six lieux). La capture D3D11 du jeu charge Arenarea
et montre le nouveau brawler dans le décor : `revue/brawler-moteur.png`.
Le premier essai hors écran ne dessinait pas le décor ; il n'est pas retenu comme preuve.

La revue `Tools/Assets3D/Meshy-test/revue/index.html` présente les deux personnages,
les cinq clips, les quatre vues, une pause image par image, le ralenti et l'échelle
du jeu. Elle inclut une simulation de marche sur la maquette, explicitement distinguée
de la capture moteur. **Aucun verdict visuel de l'auteur sur la marche n'est déduit
de ces contrôles techniques.** Le scoundrel reste livré dans l'atelier, conformément
au périmètre d'installation limité au brawler.

**Publication :** `Common@5`, 164 images, 34,4 Mio,
SHA-256 `7499757565ce5c2169faedfe31604613b0d16fbc6e939d101a48076d1ef1d6a1`,
publié par l'auteur le 1er octobre 2026 sur la release `assets-common` ;
`kits.lock.json` le verrouille.

### Verdict final de l'auteur — 1er octobre 2026

**La version s'ouvre.** L'auteur accepte le brawler installé en l'état (« ok comme ça »)
et demande la livraison du lot. Trois constats de revue lui ont été présentés et restent
ouverts pour le [LOT-1001](LOT-1001-standard-3d.md) et le
[LOT-1006](LOT-1006-corps-de-reference.md) : le buste du maillage Meshy est vrillé
d'environ 45° par rapport aux pieds (pose de la figurine d'origine) ; le visage se lit
mal sous la caméra du jeu ; la texture Meshy est plus pâle que la figurine peinte.
Les quatre clips autres que la marche restent des poses simples, conformément au périmètre.
