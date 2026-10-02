# Les personnages 3D

> **Mise à jour de l'auteur — 1er octobre 2026, production LOT-1009.**
> Les nouveaux humanoïdes, brawler compris, sont demandés en **T-pose via l'option
> Meshy**, avec le **rig réalisé dans Meshy également**. Les armes peuvent être
> produites **séparément** pour améliorer l'animation et permettre ultérieurement
> leur changement par l'inventaire. Pour cette production, les personnages sont donc
> préparés mains libres et les armes et boucliers comme assets indépendants ; le
> bouclier Ironhand porte aussi la paume ouverte.
> Ces décisions remplacent les consignes de production antérieures ci-dessous sur
> la pose en A, l'absence de changement de pose et les armes fusionnées au personnage.
> Le squelette Meshy n'est pas encore contrôlé contre les 53 os du squelette du jeu :
> compatibilité, conversion et accroches d'équipement restent à mesurer avant
> l'installation. Aucun changement d'arme par inventaire n'est encore implémenté.

Un personnage est **un modèle** : un maillage texturé qui lui est propre, lié au **squelette
humanoïde commun**, animé par les **clips communs**. Cette page fixe ce contrat. Elle complète le
[standard 3D](style-3d.md) et s'écrit, comme lui, d'après ce que le
[LOT-1000](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1000-preuve-de-la-chaine-de-personnages.md)
a mesuré sur le brawler et le scoundrel, et d'après les décisions datées de l'auteur.

> **Ce que la preuve a changé.** La décision [D-31](../vision/decisions.md) composait un personnage
> d'un **corps parmi huit**, d'une texture et de pièces d'une bibliothèque. L'auteur a refusé ces
> corps le 1er octobre 2026, puis les reconstructions locales, et validé les maillages produits par
> **Meshy** depuis les figurines peintes. Le même jour il tranche : **un maillage par personnage**
> ([D-38](../vision/decisions.md)). Les huit corps et la bibliothèque de pièces sortent du standard.
> Ce qui reste commun est ce qui faisait le but de la version : **le squelette et les animations**.

## 1. Ce dont est fait un personnage

| Élément | Combien | Partagé ou propre |
|---|---|---|
| Maillage texturé, vêtements et coiffure compris, **sans arme** | **1 par personnage**, généré d'après son image de référence | propre |
| Squelette `humanoid` (53 os) | 1 pour tous les humanoïdes | partagé |
| Clips d'animation | 1 jeu pour tous les humanoïdes | partagé |
| Fiche de liaison : où sont les articulations dans **ce** maillage | 1 par personnage | propre |
| Portrait (512 × 512) et jeton (128 × 128), **peints** | 1 par personnage | propre |

Le générateur d'images ne dessine plus jamais un mouvement : il peint **une image fixe**. C'est ce
qui rend la production stable — les quatre marches peintes du brawler avaient été refusées le
24 septembre 2026.

## 2. La chaîne

| Étape | Qui | Ce qui en sort |
|---|---|---|
| 1. **Peindre** le portrait, puis l'image de référence ([§3](#3-limage-de-référence)) | le générateur d'images, sur commande de Claude ; l'auteur valide | deux images, dans l'atelier |
| 2. **Générer** le maillage, puis sa texture ([§4](#4-la-génération)) | Meshy, lancé par l'auteur | un `.glb` texturé, déposé par l'auteur dans l'atelier |
| 3. **Juger la forme** en matériau neutre, puis texturée : face, profil, dos, gros plan du visage | l'auteur | un verdict — un maillage refusé se **régénère**, il ne se retouche pas |
| 4. **Lier** au squelette et poser les clips ([§5](#5-le-squelette), [§6](#6-la-liaison), [§7](#7-les-clips)) | `scripts/assetsGeneration/rig_character.py`, d'après la fiche de liaison (depuis le LOT-1005 : un calcul, sans Blender) | un `.glb` autonome : maillage, texture, squelette, clips ; et `skeleton.json` |
| 5. **Contrôler** l'export ([§9](#9-les-contrôles)), et le **montrer** | `scripts/checks/check_character_model.py` ; `scripts/assetsGeneration/render_character_review.py` rend chaque clip en huit poses sous la caméra du jeu | un relevé, conservé avec le modèle ; les planches que l'auteur juge |
| 6. **Installer** et **publier** le kit | `install_hd_asset.py`, `publish_asset_kit.py` | l'asset dans le jeu, le kit verrouillé |

Jusqu'au [LOT-1006](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1006-corps-de-reference.md), le
moteur n'anime pas de modèle : l'étape 6 **rend** le modèle en bandes
(`scripts/assetsGeneration/render_character_strips.py`, caméra du jeu, huit images, quatre
orientations) et installe ces bandes en mode `placed`. C'est ainsi que le brawler est dans le jeu
depuis le LOT-1000 (`Common@5`).

L'atelier est local (`Tools/Assets3D/`, jamais livré) : références, exports reçus, fiches de
liaison, scripts Blender, relevés. La provenance d'un modèle — tâche Meshy, réglages, empreintes —
y est conservée avec lui.

## 3. L'image de référence

Décision de l'auteur, 1er octobre 2026 : le maillage se génère depuis **une vue de face en pose
neutre**, peinte par le générateur d'images d'après la figurine ou le portrait validé.

- **De face**, le regard droit, les deux pieds à plat, écartés de la largeur des épaules.
- **Pose en A** : bras tendus, écartés du corps d'environ 45°, doigts détendus.
- **Sans arme, les mains vides** (décision de l'auteur, 1er octobre 2026,
  [D-42](../vision/decisions.md)) : rien de tenu, rien au dos ni à la ceinture — ni arme, ni
  bouclier, ni carquois, ni fourreau. Mains ouvertes, doigts lisibles. Vêtements, armure,
  coiffure, barbe, ceinture et sacoches se portent.
- Fond uni, personnage entier, sans ombre au sol ; la facture et la palette du portrait.

Pourquoi : la preuve a généré le brawler depuis sa figurine **sud-est**, et Meshy a reproduit la
pose — le buste du maillage est **vrillé d'environ 45°** sur ses pieds, la hache est soudée aux
deux mains, et le visage, de trois quarts, se lit mal sous la caméra du jeu. L'auteur a accepté ce
brawler en l'état ; la règle vaut pour les suivants.

> **Non mesuré.** Aucun maillage n'a encore été généré depuis une telle image : la première série
> ([LOT-1009](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1009-les-quatre-heros.md)) l'éprouve
> sur son premier personnage, et corrige cette section s'il le faut.

## 4. La génération

Les réglages **mesurés** au LOT-1000, sur deux personnages validés par l'auteur :

| Étape Meshy | Réglage | Coût relevé |
|---|---|---:|
| Image vers 3D | Meshy 7.1 Flagship, détails élevés, résolution Standard ; **amélioration d'image désactivée**, sans texture, sans changement de pose | 20 crédits |
| Réduction | une **copie** à la cible de 100 000 triangles ; le maître (1,2 à 1,5 million de faces) est conservé | 0 |
| Texture | 4096 × 4096, depuis la **même** image de référence ; PBR et multi-vues désactivés | 10 crédits |
| Export | `.glb` texturé, téléchargé **par l'auteur** (le navigateur piloté ne reçoit pas le fichier) | — |

Soit **30 crédits par personnage**. Les copies exportées font 102 894 et 102 988 triangles ; elles
ne sont pas réduites davantage ([budget](style-3d.md#3-le-poids-dun-modèle) : 100 000 triangles
et 2048 px de texture au plus, fixé au LOT-1005).

> **Décision de l'auteur, 2 octobre 2026 — la réduction se fait par script.** Le maître téléchargé
> de Meshy (de 10 000 à 2,6 millions de triangles, mesuré sur les 44 modèles de la démo) passe par
> `scripts/assetsGeneration/reduce_model.py` : Blender sans fenêtre le pose au sol, ne lui laisse
> que sa couleur de base, le décime à 100 000 triangles au plus, et la texture du maître est
> remise octet pour octet dans la copie. Jugé par l'auteur sur une planche comparant le maître et
> la copie de trois modèles sous la caméra du jeu : à la taille du jeu, moins de 0,4 % des pixels
> diffèrent de plus de 8 niveaux à 1080p, moins de 0,7 % à 2160p ; en gros plan, la décimation
> laisse de petits points clairs aux coutures de texture, acceptés. La copie réduite dans Meshy
> (ligne « Réduction » ci-dessus) n'est plus l'étape de la chaîne ; le maître reste conservé.

La reconstruction locale (TripoSR, 40 variantes par personnage) a été essayée et **écartée** par
l'auteur le même jour : visages émoussés, armes interrompues. Elle ne se réessaie pas sans raison
nouvelle.

## 5. Le squelette

Un squelette par **silhouette**. La seule produite est `humanoid` ; `quadruped` se construit au
[LOT-1009](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1009-les-quatre-heros.md), avec le lion et le loup de l'arène,
d'après une image de trois quarts — rien n'en est encore mesuré ; `flying` est nommée et viendra
avec ses créatures ([D-34](../vision/decisions.md)).

Le squelette `humanoid` est le `game_engine` de MPFB : **53 os**, une racine, noms fixes —
c'est par eux que les clips, la fiche de liaison et le moteur se retrouvent.

| Chaîne | Os |
|---|---|
| Tronc | `Root`, `pelvis`, `spine_01`, `spine_02`, `spine_03`, `neck_01`, `head` |
| Bras (× `_l`, `_r`) | `clavicle`, `upperarm`, `lowerarm`, `hand` |
| Doigts (× `_l`, `_r`) | `thumb`, `index`, `middle`, `ring`, `pinky`, chacun en `_01`, `_02`, `_03` |
| Jambe (× `_l`, `_r`) | `thigh`, `calf`, `foot`, `ball` |

- **La hauteur.** Le maillage est normalisé à la hauteur de son personnage, du sol au sommet du
  crâne : **1,80 m** pour les deux de la preuve. Un personnage petit ou grand garde le même
  squelette : ce sont ses articulations, dans la fiche de liaison, qui sont plus proches ou plus
  éloignées — pas un squelette de plus.
- **Le sol** est à la hauteur zéro de la racine ; l'origine est entre les pieds.
- **La pose de liaison** est la pose du maillage généré, donc celle de l'image de référence : d'où
  la pose en A.
- **Les doigts** gardent la pose sculptée : aucun clip ne les anime.

## 6. La liaison

Le squelette est commun, **sa position dans le maillage ne l'est pas** : chaque personnage a sa
fiche de liaison, lue par le script de préparation. Mesuré au LOT-1000 — les deux personnages sont
passés par le même script, seules leurs fiches diffèrent.

| Champ | Ce qu'il dit |
|---|---|
| `source_pattern` | le `.glb` reçu de Meshy |
| `ground`, `head_top`, `center_x` | le sol, le sommet du crâne et l'axe du corps dans le maillage reçu : la mise à l'échelle et le recentrage s'en déduisent |
| `joints` | la position de chaque articulation du tronc et des jambes |
| `arms` | les quatre points de chaque bras : épaule, coude, poignet, bout de la main |
| `arm_radius` | le rayon de la peau qui suit un bras |
| `weapons` | les volumes — boîtes ou capsules — de ce qui suit **rigidement** un os |

Les poids sont calculés par le script, puis **normalisés** : quatre os au plus par sommet, somme
égale à 1. Une arme, un fourreau ou une pièce rigide est pesé à **1 sur un seul os** : il ne se
déforme pas.

Aucune retouche de poids à la main. Un défaut de déformation se corrige dans la fiche — une
articulation déplacée, un volume ajusté — et le script se rejoue.

## 7. Les clips

Les animations sont posées **une fois**, sur le squelette, et rejouées par tous les humanoïdes.

| Clip | Durée mesurée | Boucle | État à la preuve |
|---|---:|---|---|
| `idle` | 1,000 s | oui | pose simple |
| `walk` | 0,500 s | oui | **soigné** : c'est l'animation jugée par l'auteur |
| `attack` | 0,875 s | non | pose simple |
| `hit` | 0,703 s | non | pose simple |
| `death` | 0,844 s | non | pose simple |
| `cast` | — | non | **non produit** : le sixième clip du moteur, à poser au LOT-1006 |

- **La marche tient la règle du moteur** : un cycle couvre **une case de 1,5 m en 0,5 s**, soit
  deux cases par seconde. Le pied posé recule exactement à cette vitesse ; le bassin descend juste
  assez pour que les deux jambes atteignent leurs chevilles.
- **Les poses se disent en cibles**, pas en angles d'os : une cheville à atteindre (deux os résolus,
  genou vers l'avant), une main à placer, une direction où pointer. Les mêmes fonctions animent
  tous les maillages liés au squelette.
- **Le contact au sol** se corrige par clip : la hauteur de la racine est recalée sur la surface
  évaluée du maillage, image par image.
- **L'image clé** d'un clip — l'instant de l'impact — est une donnée du clip. Depuis le
  [LOT-1005](../versions/v0.1.0/v0.0.2.5-passage-3d/lots/LOT-1005-squelette-et-animations.md) elle
  s'écrit, avec la durée et la boucle, dans la **description du squelette**
  (`Common/Characters/Skeletons/<silhouette>/skeleton.json`, champ `key`, en secondes) : le combat
  y accroche le touché de la cible. Les courbes sont dans le `.glb` de chaque personnage, posées
  sur ses propres articulations ; le script de liaison écrit les deux d'une même source.
- **La fiche du personnage** (`character.json`, dans son dossier) nomme son modèle et son
  squelette : c'est elle que le moteur lit pour savoir qu'un personnage est un modèle.
- Les quatre clips « pose simple » et `cast` sont **repris** au LOT-1006, qui les soumet à
  l'auteur comme la marche l'a été.

## 8. Les armes

**Un modèle se génère sans arme** (décision de l'auteur, 1er octobre 2026,
[D-42](../vision/decisions.md), qui remplace D-41) : c'est plus simple à animer. Des bras en pose
neutre et des mains vides se pèsent sans les volumes d'équipement que la preuve a dû régler à la
main — la hache du brawler, soudée à ses deux mains, et les lames du scoundrel.

- Le champ `weapons` de la fiche de liaison ne sert plus qu'à ce qui est **rigide et porté** :
  une épaulière, un casque à plumet.
- Les clips se posent **mains vides** : une attaque est un geste, pas le trajet d'une lame.
- **Ce que le personnage tient n'est pas décidé.** Soit il combat à mains nues à l'écran, soit
  l'arme est un modèle à part accroché à un os de main : l'auteur le tranche, et le lot qui le
  porte s'écrit alors. D'ici là aucun lot ne produit d'arme.

## 9. Les contrôles

Ce qu'un modèle doit tenir avant de s'installer, et ce que la preuve a relevé :

| Contrôle | Seuil | Relevé au LOT-1000 |
|---|---|---|
| Structure du `.glb` : un maillage, 53 os, les clips attendus, texture incorporée, indices valides | exact | conforme, les deux |
| Poids | somme à 1, quatre os au plus | écart < 3 × 10⁻⁸ |
| Géométrie évaluée, sur huit poses par clip | aucune coordonnée non finie | 40 poses par personnage, toutes finies |
| Pénétration du sol | pas plus que la preuve : **1,3 mm** | 1,3 mm (brawler), moins de 0,1 mm (scoundrel) |
| Glissement du pied posé à la marche, mesuré sur les os | pas plus que la preuve : **0,53 px d'art** | 0,45 px (brawler), 0,53 px (scoundrel) |
| Cadrage des bandes rendues | le corps entier dans la cellule, couché compris | 40 bandes sur 40 |

Ces contrôles **ne remplacent pas** le jugement de l'auteur : les semelles, les vêtements en
mouvement et la lisibilité du visage se jugent **dans le jeu**, à 100 px par case.

## 10. Portrait, jeton, mannequin

- **Portrait et jeton restent peints** ([D-30](../vision/decisions.md)), aux tailles du
  [standard](style-3d.md#7-les-images-tolérées). Le portrait se peint **avant** l'image de
  référence : c'est lui qui fixe le visage.
- **Le mannequin** tient la place de tout humanoïde sans modèle : un maillage neutre, lié au même
  squelette (LOT-1006). La règle de repli (`hmi::FigureResolver`, propriété `silhouette`) désigne
  un squelette.

## 11. Constats ouverts de la preuve

Présentés à l'auteur le 1er octobre 2026, acceptés pour le brawler, à lever sur les suivants :

| Constat | Ce que le standard en fait | Levé au |
|---|---|---|
| Buste vrillé d'environ 45° sur les pieds | image de référence de face, en pose neutre ([§3](#3-limage-de-référence)) | LOT-1006 |
| Visage peu lisible sous la caméra du jeu | même règle : un visage généré de face ; à juger à 100 px par case | LOT-1006 |
| Texture plus pâle que la figurine peinte | la texture se compare au portrait avant la liaison ; une texture trop pâle se **régénère** | LOT-1006 |
| Quatre clips en poses simples, `cast` absent | repris et soumis à l'auteur ([§7](#7-les-clips)) | LOT-1006 |
