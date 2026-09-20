# LOT-CREATION-ASSETS — Produire selon les régions jouées

> Pilote T00 en cours, 19 septembre 2026 : premier essai généré, aucune intégration autorisée. Bilan : `Tools/AssetFactory/bilan-poc.md` (hors dépôt).
> Chantier autonome hors roadmap et numérotation gameplay ; remplace la proposition LOT-97 abandonnée.
> Ce chat local orchestre l’outil de génération intégré. Aucune clé API, SDK payant, budget dollars ou réservation artificielle d’usage.

## Progression et tâches

Priorité obligatoire après le pilote : carte de test en jeu T01, reprise exhaustive Martpart/Arenarea/Colisée CE-R01 et refonte Ironhand CE-R02. Ces remises en conformité précèdent les ajouts CE-01/CE-02 ci-dessous. Toute texture future reste soumise à T01.

Calibrer le socle sur les cinq PNJ existants, puis servir **Martpart/Myr et Arenarea/Galender**. Les autres régions forment une réserve documentée, activée par une scène réellement choisie. Une série mêle PNJ, monstres, décor, objets, symboles ou carte selon son besoin ; **dix assets métier maximum, jamais un minimum**. Un PNJ complet est une unité (portrait + six bandes et descripteurs détaillés), une tuile/pièce/carte/variante distincte est une unité. Aucun « pack » illimité. Une branche et une PR par série cohérente ; ne pas répartir artificiellement un PNJ entre plusieurs PR.

| Tâche | Objet | Unités proposées |
|---|---|---:|
| [T00](tache-00-socle-pilote-pnj.md) | Socle, Anariel/Jade/Lizz/Nakral/Xorius | 5 |
| [T01](tache-01-carte-test-textures.md) | Carte de test en jeu, témoin de taille et raccords | Outillage |
| [CE-R01](tache-ce-r01-reprise-trois-sites.md) | Reprise intégrale des trois sites, sous-séries régionales | ≤ 10 par série |
| [CE-R02](tache-ce-r02-refonte-ironhand.md) | Refaire le soldat Ironhand complet | 1 |
| [CE-01](tache-ce-01-martpart-myr.md) | Boucle de marché et enquête Myr | 6 |
| [CE-02](tache-ce-02-arenarea-galender.md) | Entraînement et Arena of Fate | 6 |

[Préparation commune](preparation-regionale.md) : corpus, panthéon, factions, registre et limites.
[Draconic Council](transversal/draconic-council/preparation.md) : organisation transversale.

| Dossier régional | Déclenchement |
|---|---|
| [Central Empire](regions/central-empire/preparation.md) | Reprises CE-R01/CE-R02, puis CE-01/CE-02 |
| [Republic of Freelands](regions/republic-of-freelands/preparation.md) | Réserve conditionnelle |
| [Imperial Ben’net](regions/imperial-benenet/preparation.md) | Réserve conditionnelle |
| [Kingdom of Kolbjörn](regions/kingdom-of-kolbjorn/preparation.md) | Réserve conditionnelle |
| [Taii’Maku](regions/taii-maku-city-states/preparation.md) | Réserve conditionnelle |
| [Theocracy of Kepesh](regions/theocracy-of-kepesh/preparation.md) | Réserve conditionnelle |
| [Tsvetan](regions/tsvetan/preparation.md) | Réserve conditionnelle |
| [Yama](regions/yama/preparation.md) | Réserve conditionnelle |
| [Magocracy of Mage Tower](regions/magocracy-of-mage-tower/preparation.md) | Réserve conditionnelle |
| [Seashores](regions/seashores/preparation.md) | Réserve conditionnelle |
| [Sindile Forest](regions/sindile-forest/preparation.md) | Réserve conditionnelle |
| [Stravian Domains](regions/stravian-domains/preparation.md) | Réserve conditionnelle |
| [Storm Islands](regions/storm-islands/preparation.md) | Réserve conditionnelle |
| [Undertanares](regions/undertanares/preparation.md) | Réserve conditionnelle |
| [Darkall](regions/darkall/preparation.md) | Réserve conditionnelle |
| [Mystical / Wasteland](regions/mystical-wasteland/preparation.md) | Réserve conditionnelle |
| [Pénombre](regions/penumbra/preparation.md) | Réserve conditionnelle |

Les treize régions JSON restent distinguées des marges Undertanares/Darkall/Mystical-Wasteland et du plan Pénombre. La région est un contexte d’usage, pas l’identité ou le dossier canonique du fichier final. Les données de lieux peuvent contenir un objet, un PNJ ou du texte de règle : vérifier le type avant de commander un paysage.

## Cycle de travail et mémoire

1. Relier chaque unité à une scène/quête/UI et au registre global ; décider créer/réutiliser/adapter. Documenter faits textuels, observations visuelles, choix artistiques et inconnues dans la fiche. Sources SB/CC/PG avec numéro PDF et imprimé.
2. Les **images de référence du corpus sont autorisées comme entrées au générateur**, dans ce chat. PDF, rendus, recadrages et images d’entrée du corpus restent hors dépôt, ou en `.cache` effectivement ignoré. Versionner document/page/hash/coordonnées du crop, pas l’image. Aucune copie de visuel du corpus dans Assets ou Documentation, aucun calque.
3. Conserver fiche, prompt B, palette, portrait original retenu, ancres, A (style) + B (identité) + C (format), historique exact prompt/réponse/verdict et empreintes. Mémoire proposée `Tools/AssetFactory/assets/<famille>/<slug>/` ; registre global à ce niveau, jamais un doublon par région. Journal : **à faire / généré / rejeté / validé / intégré**, étape et raison d’attente.
4. Générer portrait puis animations lorsqu’un personnage est requis. Une animation fausse repart seule en passe X : aucune copie de frame d’une autre animation, aucun compte artificiellement complété. Les tours précédents restent traçables. Réviser une fiche/prompt/ancre invalide les verdicts dépendants.
5. QC mesuré puis planche-contact, GIF et ouverture des images (Read image/view_image). Incertitude ou contrôle manquant bloque l’intégration et requiert revue humaine. Un code de retour ne remplace pas la revue. Rapport validés/rejetés/à revoir, par fichier et animation.
6. Tester les textures candidates en espace isolé avec [T01](tache-01-carte-test-textures.md), le rendu réel du jeu et le témoin de taille figé : mesures de raccord, parcours et revue des captures, puis vérification des cartes consommatrices. Un test graphique manquant ou sauté bloque la validation. Intégrer uniquement sorties validées dans `Source/Elements/Assets/<Famille>/<slug>/`, manifeste et `replaces` préservés, installation préparée et atomique. Mettre à jour galerie EX-CNT-042, charger réellement les `.anim.json`, attendre textures avant captures, crédits et changelog.

Les outils futurs `py -3.13` préparent/réceptionnent/normalisent/contrôlent/intègrent ; **le chat effectue génération et relecture**. Aucune promesse de CLI autonome accédant à cet outil intégré. Quota atteint : sauver l’étape et reprendre ; réponse perdue = incertaine. Enregistrer modèle/usage seulement si exposés, jamais inventer coût ou tokens. Après échecs identiques, consigner le blocage et revoir le brief.

## Contrat de qualité PNJ

| Animation | Images | Cellule | Bande | Durée/image | Boucle |
|---|---:|---|---|---:|---|
| idle | 6 | 48 × 64 | 288 × 64 | 0,15 s | oui |
| walk | 8 | 48 × 64 | 384 × 64 | 0,10 s | oui |
| hit | 4 | 48 × 64 | 192 × 64 | 0,08 s | non |
| death | 6 | 96 × 64 | 576 × 64 | 0,12 s | non |
| attack | 8 | 96 × 64 | 768 × 64 | 0,08 s | non |
| cast | 8 | 96 × 64 | 768 × 64 | 0,10 s | non |

PNG RGBA ; alpha final 0 ou 255 ; pas de halo, fond résiduel, cellule vide, compte complété
artificiellement ou partie utile rognée. Pose de garde : 45 pixels d'art. Sol : y = 63 ;
ancre x = 24 pour idle/walk/hit, 48 pour death, 32 pour attack/cast.
Portrait PNG RGBA transparent, 1024 × 1024.

Le pilote chiffre puis fige les seuils de palette, de raccord de boucle, de dérive de taille,
de position et de silhouette. Il mesure le corps séparément des effets ; la chute et la
reptation de Lizz ont des contrôles adaptés. Mesurer avant et après normalisation empêche
celle-ci de masquer une erreur. Aucun seuil non étalonné n'est présenté comme une garantie.

La revue vérifie aussi l'arme unique, sa main, la locomotion, le style, les effets et l'identité.
Le rapport Markdown ou HTML classe validés / rejetés avec raisons / à revoir et expose les
mesures par animation. Aucun asset avec contrôle manquant ou indécis n'est intégré.

Les tailles hors gabarit attendent un profil validé ; aucune réduction artificielle pour terminer une série. Les profils monstres, décors, tuiles, cartes, objets et PJ sont ajoutés lorsque la série régionale en a besoin, avec leurs propres dimensions et contrôles.

## Capacités, branches et validation

Pas de lots de stock par famille. Au premier besoin régional d’un nouveau profil, prévoir son format, ses ancres et son QC dans la tâche. Code futur dans `scripts/` ou `Tools/`, tests Python dans `scripts/tests/` et tests du jeu dans `Source/Test/`, aucun code d’implémentation dans ce lot documentaire. Réemplois possibles : pilote PNJ validé, lion/loup et autres éléments après audit. Les trois sites Martpart/Arenarea/Colisée exigent CE-R01 ; le soldat Ironhand doit être refait via CE-R02 avant réemploi. La galerie seule ne suffit pas à valider les textures.

PJ : périmètre demandé **12 classes × somme des variantes de races × 2 (homme/femme)** ; inventorier les variantes attestées, produire seulement les combinaisons consommées par création de personnage/rencontres et réparties selon régions. Ne pas fabriquer toute la matrice, ne pas changer les classes gameplay du projet.

Branche par série `lot/LOT-CREATION-ASSETS-<zone>-<serie>`, PR avec résumé QC et rapport complet ; Conventional Commits, `Source/Elements/Assets/CREDITS.md`, changelog, contrôles requis, tests pertinents et `scripts/build.ps1`, CI verte. Aucun commit/push ni modification de roadmap dans cette préparation.
