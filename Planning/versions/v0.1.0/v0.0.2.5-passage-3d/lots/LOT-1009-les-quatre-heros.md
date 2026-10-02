+++
id = "LOT-1009"
titre = "Les personnages de la démo"
version = "0.0.2.5"
filiere = "pnj"
statut = "a-faire"
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
| Le **portrait d'attente** : un héros sans figurine, porté par la liste `portraits` | `Source/Elements/Assets/Common/Characters/manifest.json` ; sa prise en charge dans `check_hd_assets.py`, `install_hd_asset.py` et `hmi::AssetGallery` | les quatre ont leur modèle ; le concept (`LOT-145`) n'a plus de cas |
| Le brawler et le scoundrel **de la preuve** | atelier local `Tools/Assets3D/Meshy-test/` ; le modèle du brawler installé au LOT-1006 | les quatre héros sont refaits au standard (décision de l'auteur) ; les données d'essai du moteur (`Source/Test/Fixtures/`) restent, elles ne sont pas des héros |
| Le repli de ces personnages sur le mannequin ou le jeton | les propriétés `figure` vides des cartes de la démo : elles nomment leur modèle | plus aucun personnage de la démo ne doit y tomber |
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

## Risques et questions ouvertes

- **Le quadrupède fait grossir le lot** : un squelette, cinq clips et une marche à quatre appuis à
  faire approuver. S'il déborde, il devient un lot à part, et les fauves restent des jetons d'ici là.

- Si un personnage ne se reconnaît pas depuis sa vue de face, il se régénère ; au-delà de trois
  essais, la décision D-39 se rouvre pour lui.
- Vingt modèles de 15 Mio font 300 Mio de kit : le budget du LOT-1005 peut imposer de les
  réduire avant l'installation.
- **Noms et apparences** de la mère, de l'enfant, du garde et du maître d'arène : les livres n'en
  disent rien (ancienne Q-05) ; la commande les décide, l'auteur les valide sur l'image.
