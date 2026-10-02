# Workflow de qualité des PNJ générés

> **Archivé le 1er octobre 2026 ([LOT-1001](../../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1001-standard-3d.md)).** Ce workflow réglait un défaut des marches **peintes** image par image ; un personnage est désormais un modèle animé par un squelette ([personnages 3D](../personnages-3d.md)), et les scripts qu'il cite (`check_figure_walk.py`) ont été supprimés. Il n'est plus en vigueur.

La validation artistique d'une planche ne valide pas son mouvement en jeu. Le brawler
livré sert de cas de régression signalé par l'auteur : ses huit images à 62,5 ms sont
compatibles avec deux cases/s, mais cette cadence ne garantit ni les appuis ni la fluidité.
Ne pas le prendre comme référence de marche acceptée tant que le problème reste ouvert.

Le 24 septembre 2026, l'auteur refuse explicitement les **quatre marches** : appuis non
alternés, voire immobiles dans certains cas. Ce constat global ne précise pas quelles
directions sont immobiles. Une nouvelle cadence seule ne suffit pas à résoudre un défaut
d'alternance des poses. Ces bandes et leur avis de refus ont été retirés au LOT-1000
(1er octobre 2026) : le brawler livré est désormais rendu depuis son modèle animé.

## Références de mouvement fournies par l'auteur

Le 24 septembre 2026, l'auteur fournit les quatre bandes `run_down.png`,
`run_right.png`, `run_left.png` et `run_up.png` du dossier local
`D:/Telechargement/FREE_Adventurer 2D Pixel Art/Sprites/RUN/` comme exemples du
**type de mouvement attendu**. Elles ne valident ni le pixel art, ni les vues cardinales
pour ce projet. Elles ne remplacent pas les références artistiques et ne sont pas des
assets à installer. Les fichiers source restent à leur emplacement d'origine.

Traduire cette référence en critères de revue :

- Les deux jambes échangent leur rôle au cours du cycle : une jambe porte le corps,
  l'autre se dégage puis revient vers le prochain appui. Les poses intermédiaires doivent
  rendre cette progression lisible, y compris sans déplacement de la figurine.
- Les pieds ne restent pas figés pendant que seuls le torse, les bras ou l'équipement
  bougent. Une variation de silhouette ou de couleur entre images ne prouve pas un pas.
- Le bassin, le torse et les bras accompagnent le mouvement de façon cohérente avec
  l'équipement ; éviter le corps rigide simplement translaté ou agité verticalement.
- Examiner un cycle complet, son raccord et le déplacement sur sol fixe : la lisibilité
  de l'alternance sur place et l'absence de glissement en déplacement sont deux exigences.
- Traduire ce mouvement dans les quatre diagonales isométriques SE/SW/NE/NW et dans
  la facture peinte HD du projet, en conservant l'identité du PNJ entre les images.

Le dossier est nommé `RUN` : ces exemples ne constituent pas une mesure de cadence de
marche. Ne pas reprendre automatiquement leur vitesse, leur inclinaison du corps ou
d'éventuelles phases de vol. Le profil de marche du projet reste celui décrit ci-dessous.
L'auteur approuve ici une intention de mouvement, pas des coordonnées de pieds annotées
ni un étalon quantitatif utilisable directement par le script.

Complément de prompt lorsque ces bandes sont jointes, après les références de style et
d'identité :

```text
MOTION REFERENCE ONLY: the attached adventurer strips demonstrate the desired
clarity of alternating leg action and distinct intermediate poses. Use them
to understand the motion, not the character design, pixel-art style, camera
angle or cardinal facing. Keep the project's approved painted HD appearance,
character identity and requested isometric diagonal facing.

Show a complete readable walking cycle: each leg takes its turn supporting
the body while the other lifts, passes and reaches the next contact. The
pelvis, torso and available arms respond naturally to that weight transfer.
The leg action must read even with root translation disabled. Do not animate
only the upper body over frozen feet. During translation, stance contacts
must compensate for root motion. Adapt the running reference to a grounded
walk at the project's specified cadence, with a continuous last-to-first pose.
```

## Références de mouvement pour les attaques

Le 24 septembre 2026, l'auteur fournit également `attack1_down.png`,
`attack1_left.png`, `attack1_right.png` et `attack1_up.png`, dans
`D:/Telechargement/FREE_Adventurer 2D Pixel Art/Sprites/ATTACK 1/`.
Comme les bandes RUN, elles servent de **références de mouvement uniquement**.
Conserver les originaux à leur emplacement ; ne pas installer ces sprites dans le jeu.
Leur ajout ne constitue pas une validation des attaques actuelles du brawler.

Les bandes montrent un coup avec une trajectoire soulignée par un arc clair, suivi de
poses de récupération. Retenir la lisibilité du geste et son déroulement. L'arc visuel
ne doit pas servir à masquer une arme ou un corps immobiles. Adapter le geste à l'arme
réellement portée par le PNJ, à son poids et à sa prise ; ne pas remplacer son équipement
par celui de la référence.

Critères de revue pour chaque attaque SE, SW, NE et NW :

- **Préparation lisible** : une pose arme le coup et annonce sa direction, avec une
  posture distincte de l'impact. Éviter huit variantes presque identiques d'une garde.
- **Frappe et impact** : la main entraîne l'arme sur une trajectoire cohérente vers la
  cible. Identifier l'image ou la fenêtre d'impact pour la comparer au déclenchement de
  l'effet en jeu ; ne pas déduire cette synchronisation de la seule planche PNG.
- **Engagement du corps** : épaules, torse, bassin et appuis accompagnent l'effort.
  Les pieds peuvent rester plantés pendant une frappe : l'alternance exigée pour la
  marche ne s'applique pas ici. Un pivot ou un pas doit être intentionnel et cohérent.
- **Continuité de l'arme et de l'anatomie** : même main porteuse, même prise, mêmes
  proportions et même équipement. Pas de lame qui change de côté, de membre ajouté,
  d'arme disparue ou de téléportation entre deux poses. Les occultations restent plausibles.
- **Fin du geste** : dépassement de la cible, décélération puis récupération lisibles,
  avec un retour cohérent vers le repos ou la garde. Examiner la transition attaque→repos
  dans le moteur ; ne pas exiger artificiellement une boucle attaque 7→0.
- **Orientation et cadrage** : attaque dirigée vers la diagonale demandée, avec le même
  personnage reconnaissable de dos. Respecter les cellules larges 384 × 256 du standard,
  sans couper l'arme, les mains ou une éventuelle traînée dans les poses extrêmes.
- **Lecture à la taille du jeu** : vérifier les huit images puis l'action à vitesse
  normale et au ralenti, à 100 et 200 px par case. Le coup doit se comprendre par ses
  poses et sa trajectoire ; un effet graphique ne remplace pas une animation corporelle.

Les PNG seuls ne fournissent pas la durée des poses ni le moment d'application des
dégâts. Garder huit images conformément au standard, mais définir la cadence pour
l'attaque et l'essayer dans le moteur ; ne pas lui appliquer les 62,5 ms de la marche
par défaut. Un maintien volontaire de pose peut être pertinent : le rejet des images
adjacentes identiques du contrôleur de marche ne doit pas être transposé aveuglément.
Ces critères sont une consigne de génération et de revue ; `check_figure_walk.py` ne
contrôle pas les attaques.

Complément de prompt à joindre aux blocs de style, de vue et d'identité :

```text
ATTACK MOTION REFERENCE ONLY: the attached adventurer attack strips demonstrate
a readable strike, weapon trajectory and recovery. Preserve the project's
approved painted HD style, character identity, equipment and requested
isometric diagonal facing. Do not copy the pixel-art rendering, cardinal
camera views, adventurer design or reference weapon.

Produce eight ordered poses of ONE attack adapted to this character's actual
weapon, grip and weight. Show clear preparation, a committed strike through
the target, follow-through and controlled recovery toward the approved guard.
Coordinate shoulders, torso, pelvis and supporting feet. Keep the weapon in
the same hand with a consistent grip and continuous trajectory. Any pivot or
step must be deliberate; planted feet are appropriate when supporting a strike.

The attack must read from the body and weapon poses, without relying on a
slash effect to create the impression of motion. If a weapon trail is part
of the approved design, it must follow the actual strike and remain secondary.
Keep the whole weapon, body and any approved trail within every wide frame.
Preserve plausible occlusion in rear views. Make the recovery connect naturally
to idle; do not force this one-shot action into a walking-style loop.
```

## Chaîne de production

1. **Contrat du personnage.** Conserver prompt, références approuvées, identité, équipement,
   orientation et palette. Appliquer `style-2d-hd.md` et `consigne-2d-hd.md`.
2. **Candidat en atelier.** Générer une bande complète et versionnée. Faire normaliser le
   candidat par l'installateur existant dans un dossier de travail hors des assets livrés.
   Conserver les sources. Contrôler les fichiers *après* découpe, réduction et recalage :
   ce sont eux que le moteur verra. Ne pas masquer un défaut de démarche par un recentrage
   indépendant des pieds sur chaque image.
3. **Précontrôle automatique.** Exécuter `check_figure_walk.py` sur les quatre marches
   normalisées. Il produit une galerie animée sur grille fixe, un rapport JSON, un gabarit
   de revue et une liste de reprises. Les directions manquantes sont un refus.
4. **Revue par l'agent chargé des assets.** Examiner les huit poses, la boucle 7→0,
   l'alternance des appuis et la conservation de l'identité. Utiliser la galerie à vitesse
   normale et au ralenti, à 100 et 200 px par case. L'utilisateur ne doit pas découvrir
   les premiers défauts évidents. L'agent renseigne uniquement les critères effectivement
   examinés ; toute ambiguïté reste en attente. Le script seul n'exécute pas cette revue.
5. **Appuis et essai moteur.** Renseigner les points de contact et les preuves de revue,
   puis tester le candidat dans une scène de validation isolée : quatre directions,
   départ/arrêt, virages et déplacement continu. Vérifier le comportement au contact d'un
   obstacle. Une simulation HTML ne prouve pas le comportement du moteur.
6. **Reprise bornée.** Refus → diagnostiquer pose, découpe/recalage ou synchronisation moteur
   avant de relancer le générateur. Joindre les références, le candidat refusé et le défaut
   précis. Retoucher seulement la bande concernée. Maximum trois candidats par bande dans
   une session automatique ; ensuite présenter le blocage et les preuves à l'auteur.
   Une preuve absente ne justifie pas une régénération. Conserver les versions validées.
7. **Livraison.** N'accepter que le code 0 pour la marche, puis compléter les contrôles
   des autres animations et le contrôle HD habituel avant installation. Montrer à l'auteur
   le candidat retenu et ses preuves pour la validation artistique finale.

## Commande reproductible

Depuis la racine, sur le brawler actuellement livré (lecture seule) :

```powershell
python scripts/assetsGeneration/check_figure_walk.py Source/Elements/Assets/Common/Characters/Heroes/brawler --manifest Source/Elements/Assets/Common/Characters/manifest.json --out build/quality/brawler-walk
```

Pour un PNJ, remplacer le dossier et son manifeste par les fichiers normalisés de l'atelier.
Ouvrir `index.html`. Copier `review-template.json` sous `review.json`, compléter la revue,
puis relancer la même commande avec `--review chemin/review.json`.

| Code | Résultat | Conséquence |
|---|---|---|
| 0 | accepted | Contrôles de marche et preuves renseignées satisfaits |
| 1 | rejected | Au moins un défaut mesuré, une entrée invalide ou un avis négatif |
| 2 | pending | Aucune acceptation : il manque des preuves ou un point reste incertain |

Le code de sortie permet à un orchestrateur de bloquer sa prochaine étape. Cet outil ne
modifie pas l'installateur existant et ne constitue donc pas encore un verrou global sur
toutes les installations. Il ne lance aucune génération, aucun service distant, aucune
validation visuelle par modèle en arrière-plan. La CI peut tester le contrôleur avec des
images synthétiques ; elle ne doit pas lancer de génération d'assets.

## Mesures et preuves

- Contrôles déterministes : PNG RGBA, huit cellules 192 × 256, marges, cellules non vides,
  séquence 0…7, boucle, cadence d'une case/cycle, images adjacentes identiques.
- Diagnostic indicatif : différence de pixels au raccord 7→0 par rapport aux autres
  transitions. Une grande différence demande une inspection, ce n'est pas une mesure
  d'anatomie, de fluidité ou de glissement.
- Chaque pied reçoit huit entrées `contacts.left` / `contacts.right`. Une entrée `[x, y]`
  désigne **le même point anatomique de la semelle en appui**, dans la cellule normalisée,
  origine en haut à gauche ; `null` signifie pied en l'air ou contact incertain. Indices 0…7.
  Deux entrées adjacentes non nulles affirment un appui continu du même pied, y compris 7→0.
  Il faut au moins deux transitions mesurables par pied. Une annotation artificielle peut
  tromper le contrôle : l'agent doit vérifier ces points visuellement, sans les inventer
  pour obtenir un résultat vert.
- Mesure du glissement : `pied[i+1] - pied[i] + déplacement_monde_par_image`.
  À 2 cases/s et 62,5 ms/image, ce déplacement vaut `(±16, ±9,9375)` pixels d'art.
  Un pied fixe dans sa cellule glisse donc d'environ 18,83 pixels d'art par transition.
  Le seuil initial est 4 px d'art (1,56 px à 1080p), à calibrer sur une marche réellement
  acceptée. Il mesure le résidu entre débuts d'images, pas le glissement intra-image
  inévitable d'une animation discrète. Il ne suffit pas pour juger toute la fluidité.
- `support.left` et `support.right` : huit phases `stance` (appui), `swing` (pied en
  retour aérien), ou `null` (inconnu). Ne jamais déduire « pied levé » d'un point de
  contact absent. Le profil exige un début d'appui par pied et par cycle, au moins trois
  poses en appui et deux en retour, et un appui unilatéral de chaque côté. Les phases
  de vol sont refusées pour cette marche. Les phases et les coordonnées doivent être
  cohérentes. Ces critères portent sur les annotations : ils ne détectent pas seuls
  des pieds immobiles dans les pixels. `visual.alternating_support` exige aussi leur
  examen visuel. Une marche stylisée hors de ce profil demande un autre contrat.
- `visual` : identité, direction, anatomie/équipement, transfert du poids, continuité de
  boucle et mouvement en jeu. `true` = examiné et conforme, `false` = refus, `null` = inconnu.
  `reviewer` identifie l'auteur ou l'agent ; `evidence` décrit les captures/essais et leurs
  emplacements. Ce sont des attestations de revue, pas une preuve vérifiée par le script.
- L'empreinte lie la revue aux PNG, animations, manifeste et version de la politique.
  Modifier l'un invalide la revue. Ne jamais recopier des avis positifs sur un nouveau hash.

## Étendre le contrôle à tous les PNJ

Cette première implémentation couvre la marche humanoïde standard à quatre directions.
Une attaque, une mort ou une créature atypique demandent d'autres critères : impact et
anticipation, maintien de la dernière pose, nombre et nature des appuis, etc. Ne pas leur
appliquer aveuglément les seuils de marche. Pour réduire davantage la revue manuelle,
l'étape suivante est un évaluateur visuel capable de proposer les repères anatomiques et
de commenter les défauts, évalué contre des exemples acceptés/refusés par l'auteur ; ses
incertitudes doivent continuer à bloquer l'acceptation automatique.
