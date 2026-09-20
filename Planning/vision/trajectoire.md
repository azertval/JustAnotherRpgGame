# La trajectoire

## Le problème qu'elle résout

L'ancienne feuille de route appelait `0.1.0` le **monde entier** : treize régions, seize classes, le
plan pénombral. Quarante-trois lots restaient, dont la plupart disaient *quoi* sans dire *comment* :
trop de zones, pas assez de conception par zone. En démarrant le travail, l'auteur l'a jugée
titanesque. La trajectoire ci-dessous la remplace par une règle simple : **une version courte,
jouable, détaillée à fond ; les suivantes esquissées ; rien au-delà n'est promis en détail.**

## Les jalons

| Version | Ce qu'elle rend jouable | Nature |
|---|---|---|
| `0.0.1` | **Démo basique** : Martpart, Arenarea et son donjon l'Arena of Fate, en 2D HD ; une quête | détaillée, lot par lot |
| `0.0.2` | **Système de combat** : les quatre classes de base, le combat de groupe, l'interface | détaillée |
| `0.0.3` → `0.0.9` | **Une sous-version par zone** de l'Empire central : la Capitale dans ses murs, les faubourgs, les abords, la côte sud, Bak, le nord, l'ouest | détaillée à la maille de la zone |
| **`0.1.0`** | **L'Empire central**, relu d'un bloc : toutes ses zones, ses PNJ nommés, ses peuples, ses animaux, ses monstres, quatre classes | détaillée |
| **`0.2.0`** | **La compagnie**, et les dernières fonctionnalités : après elle, on n'ajoute plus que du contenu | détaillée à la maille de la fonctionnalité |
| **`0.3.0`** | **Classes et espèces, pour de vrai** : 21 espèces, 20 classes, 69 sous-classes, les sorts | détaillée à la maille du paquet |
| `0.4.0` → `0.15.0` | **Une région majeure par version**, du voisinage de l'Empire vers les confins | prévisionnelle |
| `0.16.0` | Les autres régions : Undertanares, Darkall, Mystical | prévisionnelle |
| `0.17.0` | Le plan pénombral | prévisionnelle |
| **`1.0.0`** | Le monde complet, bac à sable | prévisionnelle |

Chaque version `0.x.0` de région se segmentera en sous-versions **par zone**, comme la `0.1.0`, et
recevra au besoin des lots intermédiaires de correction ou de mécanique.

## L'ordre des régions

Par **proximité de l'Empire central**, d'après les voisinages de l'atlas
(`Source/Elements/World/regions/`) : on n'ouvre une région que si l'on peut y **marcher** depuis
une région déjà ouverte.

| Cercle | Versions | Régions |
|---|---|---|
| Voisines de l'Empire | `0.4.0` → `0.9.0` | Imperial Benênet, Seashores, Sindile Forest, Theocracy of Kepesh, Stravian Domains, Tsvetan |
| Second cercle | `0.10.0` → `0.13.0` | Republic of Freelands, Magocracy of Mage Tower, Kingdom of Kolbjörn, Taii-Maku City States |
| Confins | `0.14.0`, `0.15.0` | Storm Islands, Yama |

L'ordre **à l'intérieur** d'un cercle est une préférence, pas une contrainte : il se revoit à la
recette de la `0.3.0`. Deux arguments pèseront : la Republic of Freelands porte le siège de la
Guilde des Aventuriers et trente-trois PNJ nommés — elle mérite peut-être de passer plus tôt ;
Stravian Domains ouvre sur Undertanares.

Le détail de chaque région est dans [le référentiel](../referentiels/monde/regions.md).

## Les cinq règles

1. **Une seule version est détaillée à fond** : la prochaine. Les autres ont un périmètre et des
   critères de sortie ; leurs lots se précisent quand elles approchent.
2. **Chaque version se joue.** Pas de version « technique » : même la `0.0.2`, qui est un système,
   se juge sur un combat joué de bout en bout.
3. **Les fonctionnalités se ferment à la `0.2.0`.** Tout ce qui suit est du contenu. Une
   fonctionnalité demandée après passe par une décision écrite.
4. **L'ordre des lots se calcule**, il ne s'arbitre pas : parmi les lots dont tous les prérequis
   sont livrés, celui de la version la plus proche, puis celui qui en débloque le plus.
5. **Chaque version se clôt par un bilan** qui recale les suivantes : ce qui a coûté plus que
   prévu, ce qu'une zone pèse, ce qu'un PNJ demande.

## Ce qu'on ne sait pas encore

**Ce que coûte une zone.** Toute la trajectoire repose sur ce nombre, et il n'existe pas : aucune
zone HD n'a été produite. La `0.0.1` le donnera pour deux quartiers et un donjon, la `0.0.3` pour six de plus.
D'ici là, aucune date n'est annoncée — ce serait une extrapolation, pas une prévision. À l'échelle :
l'Empire central compte une vingtaine de zones ; le monde complet, une centaine, plus le plan
pénombral.
