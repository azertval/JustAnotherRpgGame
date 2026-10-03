+++
id = "LOT-1009"
titre = "Les personnages de la démo"
version = "0.0.2.5"
filiere = "pnj"
statut = "livre"
taille = "L"
resume = "Les quatre héros, les PNJ de la quête, les adversaires et les fauves de l'arène sont des modèles produits par la chaîne du standard : une image de référence, un maillage sans arme, une fiche de liaison — la première série, et son coût."
prerequis = ["LOT-1008"]
livrables = [
  "Les **vingt modèles** de l'inventaire ci-dessous — dix-huit humanoïdes et deux fauves —, chacun généré depuis sa vue de face en pose neutre (de trois quarts pour un fauve), **sans arme** ([D-42](../../../../vision/decisions.md)), et lié au squelette commun par sa fiche de liaison : sous `Common/Characters/Heroes/` pour les héros, à leur niveau de l'[arborescence](../../../../standards/arborescence-assets.md) pour les autres.",
  "Le squelette **`quadruped`** et ses clips (repos, marche, attaque, touché, mort), sous `Common/Characters/Skeletons/quadruped/`, construits d'après le lion et rejoués par le loup ; le [standard des personnages](../../../../standards/personnages-3d.md) complété de ce qu'ils mesurent.",
  "Leurs **images de référence**, commandées au générateur d'images d'après les descriptions des livres de référence (standard des personnages, LOT-1001) ; les envois sont dans l'atelier local, `Tools/Envois/LOT-1009/`.",
  "Le coût par personnage, écrit en fin de fiche — temps passé, crédits Meshy, nombre de régénérations : c'est le coût que les lots de PNJ de la `0.0.3` reprendront.",
  "Les kits republiés et verrouillés.",
]
criteres = [
  "Chacun des quatre héros se reconnaît à côté de son portrait ; chaque PNJ et chaque adversaire se lit pour ce qu'il est à 100 px par case (jugement de l'auteur).",
  "Tous jouent leurs animations sans maillage qui se traverse ; la robe du mage suit la marche ; le lion et le loup marchent sur quatre appuis sans glissement.",
  "Aucun n'a demandé de retouche à la main d'un maillage, d'un poids ni d'une animation : seules l'image de référence et la fiche de liaison sont propres au personnage.",
  "Plus aucun personnage de la quête ni de la série de l'arène ne s'affiche par le mannequin ou par un jeton.",
  "La liste `portraits` n'existe plus dans le manifeste `Characters/` ; `check_hd_assets.py` refuse un héros sans modèle.",
]
+++

> **Livré le 3 octobre 2026 sur les dix-huit humanoïdes** (PR #174). Le squelette `quadruped`
> n'existait pas ; les deux fauves sont un lot à part, le [LOT-1011](LOT-1011-les-fauves-de-l-arene.md),
> comme le prévoyait la rubrique « Risques » ci-dessous. La liste `portraits` ne porte plus que le
> lion et le loup, et part avec eux.

## Pourquoi

Le LOT-1000 a prouvé la chaîne sur deux personnages, à la main. Celui-ci la fait tourner sur une
série : c'est la première mesure honnête de ce que coûte un personnage, et la démo cesse de se
jouer avec des mannequins.

> **Réécrit au `LOT-1001`** (1er octobre 2026). Un personnage n'est plus un corps commun, une
> texture et des pièces, mais **un maillage qui lui est propre**
> ([D-38](../../../../vision/decisions.md)), **généré sans arme** (D-42). L'auteur étend le lot des
> quatre héros à **tous les personnages de la démo**, et demande que les quatre héros soient
> **tous refaits** — le brawler de la preuve compris. Le nom du fichier garde l'ancien titre.

## Périmètre

Dedans : **tout** ce que la démo et la série de l'arène mettent sur une carte — humanoïdes et
fauves (décision de l'auteur, 1er octobre 2026 : « on prépare tous les assets »).

Dehors, nommément :

- les **portraits** et **jetons**, qui restent peints ([D-30](../../../../vision/decisions.md)) ;
- toute règle de classe ou de créature : une fiche de règles ne change pas.

## Inventaire

| Modèle | Qui | Armes de sa fiche (non modelées, D-42) | Fiche de règles |
|---|---|---|---|
| `brawler` | Grom Tranche-Écaille, demi-orc | hache à deux mains | `heros-brawler` (*Player's Guide*, p. 195) |
| `mage` | Faelar Trace-Carte, elfe d'automne | bâton | `heros-mage` (p. 199) |
| `priest` | Helga Pierre-Sûre, naine des collines | marteau de guerre, bouclier | `heros-priest` (p. 203) |
| `scoundrel` | Nessa Double-Vie, humaine | rapière, arc court au dos | `heros-scoundrel` (p. 207) |
| `mother` | la mère, marchande de pommes de Martpart | aucune | — |
| `child` | l'enfant, son fils | aucune | — |
| `ironhand-soldier` | le garde | épée longue, bouclier, arc long au dos | *Tanares Sourcebook*, p. 326 |
| `arena-master` | le maître d'arène | aucune | — |
| `bandit` | coupe-jarret de la Capitale | cimeterre | `bandit` |
| `bandit-archer` | bandit arbalétrier | arbalète légère | `bandit-archer` |
| `bandit-captain` | capitaine bandit | cimeterre, dague | `bandit-captain` |
| `thug` | malfrat | masse d'armes, arbalète lourde au dos | `thug` |
| `berserker` | berserker | hache à deux mains | `berserker` |
| `veteran` | vétéran des guerres de l'Empire | épée longue, épée courte, arbalète lourde au dos | `veteran` |
| `gladiator` | le champion de l'arène | lance, bouclier | `gladiator` |
| `arena-fighter` | combattant de l'arène | épée courte, bouclier | `combattant-de-l-arene` |
| `zombie` | mort du sable | aucune | `zombie` |
| `skeleton` | mort du sable | épée courte, arc court au dos | `skeleton` |
| `lion` | fauve du Colisée, silhouette `quadruped` | — | `lion` |
| `wolf` | fauve du Colisée, silhouette `quadruped` | — | `wolf` |

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Le **portrait d'attente** : un héros sans figurine, porté par la liste `portraits` | `Source/Elements/Assets/Common/Characters/manifest.json` ; sa prise en charge dans `check_hd_assets.py`, `install_hd_asset.py` et `hmi::AssetGallery` — **retiré pour les héros** (la liste a quitté le manifeste du commun, `check_hd_assets.py` refuse un héros sans modèle ; la part de l'installateur est partie au LOT-1008) ; la liste ne porte plus que le lion et le loup, et part avec eux au LOT-1011 | les quatre ont leur modèle ; le concept (`LOT-145`) n'a plus de cas |
| Le brawler et le scoundrel **de la preuve** | atelier local `Tools/Assets3D/Meshy-test/` ; le modèle du brawler installé au LOT-1006 — **fait** : le brawler installé est le modèle de production, le dossier de la preuve n'est plus dans l'atelier | les quatre héros sont refaits au standard (décision de l'auteur) ; les données d'essai du moteur (`Source/Test/Fixtures/`) restent, elles ne sont pas des héros |
| Le repli de ces personnages sur le mannequin ou le jeton | les propriétés `figure` vides des cartes de la démo : elles nomment leur modèle — **fait**, toutes nomment leur personnage ; seuls le lion et le loup tombent encore sur le mannequin (LOT-1011) | plus aucun personnage de la démo ne doit y tomber |
| Les sources 2D des quatre classes | atelier local : `Tools/AssetHd/NPC/Classes/LOT-136-v1/` | à archiver par l'auteur : les images en pose neutre les remplacent |

## Conception

- **Les livres d'abord.** L'apparence vient du *Tanares Sourcebook*, du *Player's Guide* et du
  *Manuel des Monstres* ; ce que les livres ne disent pas est une décision de la commande, écrite
  comme telle dans le `LISEZMOI.txt` des envois. Aucune image du corpus n'est jointe à un envoi.
- **Les images de référence peuvent se produire avant le lot** : elles ne demandent ni le moteur
  ni l'atelier. Leurs maillages aussi ; seule l'installation attend les LOT-1006 et LOT-1008.
- **La robe du mage** est le cas dur : elle est dans le maillage, et se pèse sur les os des
  jambes. Si elle se déchire à la marche, le mage porte une tunique courte et la fiche le dit.
- **Les tailles** : la priest (naine) et l'enfant sont les cas « petits », le brawler le cas
  massif ; le squelette ne change pas, leurs articulations si.
- **Le squelette** (mort du sable) est le cas fin : des os grêles se génèrent mal. S'il échoue,
  il porte des lambeaux d'armure qui lui donnent du volume, et la fiche le dit.

- **Les fauves ouvrent la silhouette `quadruped`** : un squelette et des clips de plus, que le
  standard nommait sans les produire. Le lion les fixe, le loup prouve qu'ils se rejouent. Leur
  image de référence est de trois quarts, sur quatre pattes séparées — rien n'en est mesuré.

## Décisions de réalisation

Livré le 3 octobre 2026, **PR #174**, sur la branche `lot-1009-personnages-de-la-demo`.

- Les **dix-huit humanoïdes** de l'inventaire — les quatre héros, tous refaits (le brawler installé
  est déjà le modèle `LOT_1009_Brawler_T` de l'atelier, pas celui de la preuve), la mère, l'enfant,
  le garde, le maître d'arène, les neuf adversaires de l'arène — sont installés par l'**atelier des
  assets** du LOT-1008 : une fiche d'atelier par personnage (`Tools/Assets3D/Fiches/<nom>.character.json`,
  atelier local), rejouée par `LevelEditor --apply Tools/Assets3D/Fiches/*.character.json --check`.
  Les modèles liés sont ceux de `Tools/Assets3D/Lies/`, produits au LOT-1006 par
  `reduce_model.py` puis `rig_character.py` depuis les maillages Meshy de production ; le
  contrôle du standard les a tous acceptés (« 19 modèle(s), 0 avec écart »). Les portraits et les
  jetons restent ceux installés au LOT-1006.
- `combattant-de-l-arene` est le nom du jeu (sa fiche de règles) ; l'atelier local l'appelle
  `arena-fighter`.
- Les kits sont republiés : `Common@8`, `central-empire/Common@3`, `capital/Common@3`,
  `arena-of-fate@4`, `martpart@3`.
- `check_hd_assets.py` refuse désormais un héros (`Heroes/…`) sans modèle.
- Les créatures `zombie` et `skeleton` n'ont pas de `silhouette` dans leur fiche de règles : elles
  sont humanoïdes par défaut, et leur modèle est lié au squelette `humanoid`.
- Dans le jeu : la série de l'arène et la quête se jouent (suites d'intégration et système vertes),
  et une capture du combat du capitaine montre héros et berserkers en modèles texturés.

### Le coût d'un personnage

Ce que la chaîne mesure, personnage par personnage (relevés de `Tools/Assets3D/Lies/*/releve.json`,
contrôle `check_character_model.py`) :

| Modèle | Taille | Triangles | `.glb` lié | Fiche de liaison |
|---|---|---|---|---|
| `brawler` | 1,80 m | 100 000 | 8,9 Mio | écrite (hauteur fixée) |
| `mage` | 1,90 m | 99 999 | 8,5 Mio | estimée |
| `priest` | 1,85 m | 100 000 | 8,6 Mio | estimée |
| `scoundrel` | 1,90 m | 100 000 | 8,3 Mio | estimée |
| `mother` | 1,90 m | 100 000 | 9,0 Mio | estimée |
| `child` | 1,25 m | 100 000 | 8,9 Mio | estimée |
| `ironhand-soldier` | 1,90 m | 100 000 | 9,4 Mio | estimée |
| `arena-master` | 1,86 m | 100 000 | 8,3 Mio | estimée |
| `bandit` | 1,90 m | 100 000 | 8,8 Mio | estimée |
| `bandit-archer` | 1,90 m | 100 000 | 8,7 Mio | estimée |
| `bandit-captain` | 1,90 m | 99 999 | 8,4 Mio | estimée |
| `thug` | 1,86 m | 100 000 | 8,2 Mio | estimée |
| `berserker` | 1,90 m | 99 999 | 10,0 Mio | estimée |
| `veteran` | 1,88 m | 100 000 | 8,6 Mio | estimée |
| `gladiator` | 1,90 m | 100 000 | 8,8 Mio | estimée |
| `combattant-de-l-arene` | 1,90 m | 100 000 | 8,7 Mio | estimée |
| `zombie` | 1,84 m | 100 000 | 9,1 Mio | estimée |
| `skeleton` | 1,90 m | 100 000 | 10,8 Mio | estimée |

Pénétration du sol : 0,74 mm au plus (vétéran, à la marche) ; glissement du pied posé : 0,17 px
d'art au plus (bandit, gladiateur), sous le seuil de 0,53 px. Dix-sept fiches sur dix-huit sont
**estimées** par le script sans correction : aucune n'a demandé de retouche de maillage, de poids
ni d'animation. Les kits pèsent 65 Mio (`Common`), 107 Mio (`arena-of-fate`), 10 Mio
(`central-empire/Common`, `martpart`) : vingt modèles de 8 à 11 Mio font bien les 300 Mio
annoncés, répartis par niveau.

Ce que le dépôt ne mesure pas et que l'auteur tient dans l'atelier : les crédits Meshy
(30 par personnage selon le standard, §4 — image vers 3D 20, texture 10 — avant toute
régénération), le temps passé par personnage et le nombre de régénérations. **À compléter par
l'auteur** sur cette fiche ; c'est ce coût que les lots de PNJ de la `0.0.3` reprendront.

## Risques et questions ouvertes

- **Le quadrupède fait grossir le lot** : un squelette, cinq clips et une marche à quatre appuis à
  faire approuver. S'il déborde, il devient un lot à part, et les fauves restent des jetons d'ici là.
  **Survenu** : le [LOT-1011](LOT-1011-les-fauves-de-l-arene.md).

- Si un personnage ne se reconnaît pas depuis sa vue de face, il se régénère ; au-delà de trois
  essais, la décision D-39 se rouvre pour lui.
- Vingt modèles de 15 Mio font 300 Mio de kit : le budget du LOT-1005 peut imposer de les
  réduire avant l'installation.
- **Noms et apparences** de la mère, de l'enfant, du garde et du maître d'arène : les livres n'en
  disent rien (ancienne Q-05) ; la commande les décide, l'auteur les valide sur l'image.
