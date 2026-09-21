+++
id = "LOT-37"
titre = "Atlas des régions et graphe de cartes"
version = "0.0.0"
filiere = "donnees"
statut = "livre"
taille = "M"
resume = "Le graphe de cartes reçoit de vrais nœuds : treize régions, leurs lieux et leurs sept statistiques régionales, sans qu'aucune valeur ne soit inventée."
prerequis = [
  "LOT-32",
]
livrables = [
  "**13 régions** sous `Source/Elements/World/regions/` et **94 lieux** sous `locations/`.",
  "`scripts/sourcebook/atlas.py` et la sous-commande `python scripts/sourcebook atlas`.",
  "`region.schema.json` et `location.schema.json` (`EX-CNT-060`, `EX-CNT-061`), et l'extension de `check_rpg_data.py` au dossier `World/`, graphe compris (`EX-CNT-062`).",
  "`core::loadAtlas` (`Source/Core/World/Atlas.{h,cpp}`), `core::RegionGrade`, `core::RegionAxis` et `Atlas::unreachableFrom`.",
  "Douze tests, dont trois encarts rejoués contre des valeurs recopiées à la main du PDF.",
]
criteres = [
  "Aucune région inatteignable : le voisinage est symétrique, tout voisin existe et le graphe est connexe (`EX-CNT-062`), contrôlé à l'extraction et en intégration continue sur la donnée livrée.",
  "Les sept axes des statistiques régionales sont exploitables : chaque axe est une liste d'appréciations à note fermée, jamais du texte libre (`EX-CNT-061`).",
  "Trois encarts `Regional Statistics` sont rejoués contre des valeurs recopiées à la main du PDF.",
  "Un contrôle de cardinal par région arrête une section qui se refermerait trop tôt.",
  "`ctest` : 1021/1021.",
]
+++

## Pourquoi

Le chapitre 5 du `Tanares_Sourcebook.pdf` vers `Source/Elements/World/` : les régions, leurs lieux,
et surtout leurs **sept statistiques régionales** — sept axes déjà tabulés par les auteurs, qui
règlent le taux de rencontre, le stock d'un marchand, ses prix et la fréquence des PNJ hostiles.
Dix ambiances mécaniquement distinctes sans qu'aucune valeur ne soit inventée.

Une région est un **nœud** du graphe de cartes du [LOT-09](@ref lot-09), un lieu une **carte** à
créer. C'est ce lot qui donne au graphe de vrais nœuds à relier.

## Périmètre

**13 régions** sous `Source/Elements/World/regions/` et **94 lieux** sous `locations/`.

**`scripts/sourcebook/atlas.py`** et la sous-commande `python scripts/sourcebook atlas`, qui les
régénèrent depuis le corpus.

**`region.schema.json`** et **`location.schema.json`** (`EX-CNT-060`, `EX-CNT-061`), et l'extension
de `check_rpg_data.py` au dossier `World/`, graphe compris (`EX-CNT-062`).

**`core::loadAtlas`** (`Source/Core/World/Atlas.{h,cpp}`), les énumérations fermées
`core::RegionGrade` et `core::RegionAxis`, et `Atlas::unreachableFrom`.

**Douze tests**, dont trois encarts rejoués contre des valeurs recopiées à la main du PDF.

## Conception

### Treize régions, pas dix

La feuille de route en annonçait **dix**. Le livre en porte **treize** : treize encarts
`Regional Statistics`, treize sections `Secrets`, `Threats and Conflicts` et `Places of Interest`.
L'écart ne vient pas d'une extraction trop large — la régularité est parfaite, trois sections par
région, ni plus ni moins — mais d'un décompte fait de mémoire avant que le corpus ne soit lu.

Ce sont : l'Empire central, la République des Freelands, le Benênet impérial, le royaume de
Kolbjörn, la Magocratie de Mage Tower, les Seashores, la forêt de Sindile, les Domaines straviens,
les Storm Islands, les cités-États de Taii'Maku, la Théocratie de Kepesh, Tsvetan et Yama.

### L'encart se lit par son cadre, pas à l'estime

Un encart `Regional Statistics` occupe la colonne de gauche d'une page imprimée qui en compte deux.
Ses valeurs s'alignent, à l'ordonnée près, sur le corps de texte de la colonne voisine. Une lecture
par ligne, sans clip, donne :

> `Citizen Freedom  Very Low allows for ample military funding and a higher standard of liv-`

La valeur est **juste**. La phrase qui la suit vient d'un autre paragraphe, et rien ne le signale :
ni le schéma, qui verrait une chaîne, ni la relecture, qui ne se fait pas sur treize encarts.

Choisir une abscisse de coupe à la main marche sur onze régions et échoue sur les deux qui écrivent
une valeur longue. Le cadre, lui, est **dessiné dans le PDF** : le module retient le seul rectangle
plein, haut de plus de 50 points, qui encadre horizontalement le titre « Regional Statistics » et
commence à son ordonnée. Le clip est alors exact par construction, et le bloc d'introduction
(`Government`, `Faction`, `Population`) se lit dans la même colonne, au-dessus du titre.

### Une statistique est une liste, jamais une chaîne

Onze régions sur treize donnent une valeur uniforme. Les deux autres refusent :

| Région | Axe | Ce que le livre écrit |
|---|---|---|
| Benênet impérial | `Citizen Freedom` | `Low (south), High (north)` |
| Domaines straviens | `Crime and Violence` | `Low underground and High on the surface` |

Aplatir ces deux-là sur une note unique **invente** une donnée. Les laisser en texte libre rend les
sept axes inexploitables — et c'est précisément ce que l'acceptation du lot interdit.

Chaque axe est donc une **liste d'appréciations**, chacune avec sa note fermée et sa portée
facultative (`EX-CNT-061`). Une région uniforme en porte une seule, sans portée : le cas courant
reste simple. `RegionStatistic::grade()` rend la première, pour le code qui ne modélise pas les
portées ; `appraisals` reste entier pour celui qui les modélisera.

Le degré médian du livre s'écrit **`normal`**, pas `average` — la feuille de route le citait de
mémoire, et un test le vérifie nommément.

### Le voisinage est déclaré, et c'est écrit

« Aucune région inatteignable » suppose un **graphe**, et le livre n'en écrit aucun : l'adjacence
n'existe que sur sa carte du monde. Elle est donc relevée à la main, dans une table nommée
`VOISINS` — la **seule** donnée de ce lot qui ne vienne pas d'une extraction, et c'est pour cela
qu'elle est isolée plutôt que noyée dans le flot.

Trois contrôles la tiennent : le voisinage est **symétrique**, tout voisin **existe**, et le graphe
est **connexe** (`EX-CNT-062`). Ils tournent deux fois — à l'extraction, et en intégration continue
sur la **donnée livrée**. La seconde n'est pas redondante : l'extraction exige les PDF du corpus,
absents du dépôt ; une édition à la main du JSON ne rencontrerait sinon aucun garde-fou jusqu'au
jeu. Et un test C++ coupe une arête pour vérifier que le contrôle **sait échouer** — un contrôle de
connexité qu'on n'a jamais vu refuser ne prouve rien.

### Trois irrégularités de la source, nommées plutôt que contournées

**Le « ê » de Benênet.** La police de titrage mappe l'e circonflexe sur l'apostrophe
typographique : le livre affiche `Benênet`, le PDF rend `Ben’net`. Remplacer partout serait faux —
la même apostrophe est authentique dans `Fisherman’s Wharf` et `Taii’Maku`. Un seul nom du chapitre
en souffre, et il est réparé **nommément**.

**La galerie de personnages.** Dans dix régions sur treize, `Places of Interest` se termine par une
liste de souverains, capitaines et grands prêtres. Le livre ne l'annonce par **aucun** intertitre :
ces noms portent la même graisse, le même corps et la même police que les lieux qui les précèdent.
Aucune règle typographique ne les sépare. La première entrée de chaque galerie est donc
**déclarée**, et l'extraction s'arrête dessus — dix chaînes, vérifiables d'un coup d'œil.

**Le plan de la capitale.** La République porte treize légendes numérotées — `1- The Parliament`,
`2- Arena of Future` — en `GothicUltraOT`. Ce sont les repères d'une **image**, et ils nomment des
lieux que la prose décrit quelques lignes plus bas. Les lire comme des intertitres crée treize
doublons sans description ; la police les écarte. Le même filtre écarte, dans la forêt de Sindile,
deux blocs de créature en `Lato-Black` qui ne sont pas des lieux.

Un **contrôle de cardinal** par région ferme la boucle : une section qui se refermerait trop tôt ne
lèverait aucune erreur, elle produirait simplement moins de lieux.

## Ce que le lot ne fait pas, et pourquoi

**Les clés d'espèce restent en anglais.** Le catalogue d'espèces du [LOT-36](@ref lot-36) est en
français et n'en couvre que la moitié : ni géant, ni gobelinoïde, ni kemet, ni merfolk, ni orc, ni
soulborn n'y existent. Fabriquer ici une correspondance à moitié vide donnerait un champ vrai une
fois sur deux — pire qu'un champ dont on sait qu'il ne pointe nulle part. C'est la plomberie de
clés du `LOT-39` qui la portera, en entier.

**`faction` reste en prose.** Le catalogue des factions arrive au `LOT-80`, détaché de ce lot à
l'audit ; il remplacera ces phrases par des références, et c'est à ce moment-là que « aucune faction
référencée qui n'existe pas » deviendra vérifiable.

**`Places of Interest` mélange des lieux et des particularités de lieux** — une source d'eau qui
soigne, un fléau qui frappe une ville. Le livre ne donne aucun signal pour les séparer, et ce lot
ne s'en invente pas : tout ce que la section porte est livré, et un lieu se distingue par sa
description, pas par un champ qui aurait été deviné.

## Effet de bord assumé

La borne haute des identifiants de `common.schema.json` passe de **64 à 96** caractères. Un
identifiant de lieu est **composé** — région plus nom du lieu, parce que deux régions peuvent
nommer pareillement leur capitale — et dépasse 64 dès que le livre nomme longuement. Tronquer une
clé pour tenir dans un chiffre rond la rend illisible sans rien garantir de plus.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1021/1021, `clang-format`, les six lints, cahier de test et Doxygen verts. Alimente [LOT-09](@ref lot-09), [LOT-27](@ref lot-27), `LOT-80`, `LOT-81`.

L'epic d'origine ne portait pas de section « Critères d'acceptation » : les critères de l'en-tête sont relevés dans son texte (l'acceptation qu'il cite — « aucune région inatteignable », sept axes exploitables — et les contrôles qu'il décrit).
