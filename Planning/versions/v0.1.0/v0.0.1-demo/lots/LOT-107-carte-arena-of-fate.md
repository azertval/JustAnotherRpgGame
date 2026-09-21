+++
id = "LOT-107"
titre = "Carte — Arena of Fate (donjon d'Arenarea)"
version = "0.0.1"
filiere = "cartes"
statut = "a-faire"
taille = "M"
resume = "L'Arena of Fate se parcourt, et figure dans l'onglet « Carte » **à l'intérieur d'Arenarea**."
prerequis = ["LOT-106", "LOT-103", "LOT-126", "LOT-127", "LOT-128"]
livrables = [
  "`Levels/central-empire/capital/arenarea/arena-of-fate.json` (le sable et ses anneaux) et `arena-of-fate/undercroft.json` (le niveau −1 : vestiaires et prison), dessinées **dans l'éditeur** et reliées par l'escalier de la porte du triomphe (décision D-21).",
  "`capital/arenarea/arena-of-fate/Map/` : l'image de la zone pour l'onglet « Carte », et son entrée dans `world-maps.json`.",
  "Portails, points d'apparition nommés, zones (combat, déclencheurs de quête).",
]
criteres = [
  "`LevelEditor --check` passe : aucune case inatteignable, aucun portail sans arrivée, aucune référence morte.",
  "La carte tient 60 images par seconde à 1080p sur le poste de référence.",
  "L'onglet « Carte » montre la zone et la position du joueur.",
  "Les 14 statues et les 4 tribunes d'honneur sont posées et nommées ; `--check` ne relève aucune référence morte vers elles.",
  "Les vestiaires et la prison sont deux chemins distincts vers l'escalier de la porte du triomphe, tous deux atteignables ; l'escalier des catacombes est posé et condamné.",
]
maquettes = ["../maquettes/plan-arena-of-fate.svg"]
+++

## Une sous-zone, pas un quartier

L'Arena of Fate est un **donjon d'Arenarea** (décision D-16) : un lieu clos, à plusieurs salles —
vestiaire A, vestiaire B, couloir, sable —, où l'on entre **depuis le quartier**, par la porte de
l'arène au bout du parvis. Elle n'a pas d'entrée sur le plan de la Capitale : l'onglet « Carte »
la montre **dans** Arenarea. Ses assets et sa carte se rangent sous `capital/arenarea/arena-of-fate/`,
et elle puise d'abord dans le kit d'Arenarea, puis dans celui de la Capitale.

## Conception

La carte comprend une **pré-carte** d'abord, montée avec les pièces déjà présentes dans `Tools/AssetsHD/Colisee/` (sols, murs, angles, gardiens) : elle éprouve la chaîne et le rendu HD avant que le reste des pièces existe. Puis la carte finale : le sable (zone de combat), l'enceinte, le vestiaire A où arrive le condamné, le vestiaire B, le couloir, la porte vers Arenarea.

### Le tracé, d'après la DA

L'arène est dessinée **à la manière du Colisée de Rome** ([DA complète au référentiel](../../../../referentiels/central-empire/capitale.md#larena-of-fate--architecture-et-iconographie-da-de-lauteur-21-sept-2026)).
La carte fait **34 × 24 cases**, mais **seuls le sable et le niveau −1 se parcourent** : les trois
anneaux qui les entourent sont du décor en hauteur, posé une fois, et ne coûtent aucune case
d'atteignabilité.

| Anneau | Cases | Parcouru ? | Ce qu'il porte |
|---|---|---|---|
| Sable | ovale de **22 × 14** | **oui** — c'est la zone de combat | dalle de fond en 3 variantes, marques au sol |
| Podium | 1 case, 2 de haut | non | mur de marbre, balustrade ; infranchissable des deux côtés |
| Coursive des dieux | 1 case | non | **14 socles à statue** répartis régulièrement, braseros entre eux |
| Gradins | 3 à 5 cases | non | **4 secteurs de peuple**, chacun percé d'une **tribune d'honneur** avec son **drapeau** devant ; enceinte à arcades et attique en fond |

Les **14 statues** se répartissent sur la coursive, régulièrement, face au sable ; les **4 tribunes**
sont dans l'anneau suivant et ne se disputent donc aucune place avec elles. L'ordre autour de
l'ovale est fixé par la DA (D-18) : la **loge impériale** sur le grand axe côté parvis — celle que
le combattant voit en levant les yeux —, les **Forces alliées** en face, **Arcanum** et **Forces de
Darkall** sur le petit axe. Ni statue d'Ungod ni loge du Culte : ils sont deux niveaux plus bas.

### Le niveau −1 : vestiaires et prison

Sous les tribunes, un second niveau se parcourt, et il tient **deux quartiers qui ne communiquent
pas entre eux** :

- les **vestiaires des gladiateurs** — ceux qui combattent de leur plein gré ; le portail vers le
  **parvis d'Arenarea** est au bout de leur couloir ;
- la **prison** — les **condamnés à mort envoyés au jeu dans l'arène** : cellules à grille, corps
  de garde, la salle où le maître d'arène vient chercher le condamné.

Les deux couloirs débouchent sur le même **escalier de la porte du triomphe**, qui monte au sable ;
la **porte des morts** est en face. Dans la quête de la démo, le condamné arrive par la prison et
son adversaire par les vestiaires : les deux chemins doivent donc être distincts et tous deux
atteignables.

Au fond de la prison, l'**escalier des catacombes** descend au niveau −2. Il est posé sur la carte
et **condamné** : c'est le crochet du `LOT-157`. Le contrôle de l'éditeur doit l'accepter comme
porte sans arrivée — sinon la carte est marquée d'un portail mort : c'est `portal.sealed`, au
[LOT-126](LOT-126-ce-que-la-quete-demande-aux-cartes.md).

### Deux cartes, pas deux étages

Le niveau −1 est **sous** les gradins, qui occupent les mêmes cases en décor. Un niveau est une carte
(décision D-21) : le lot livre **deux cartes** — `arena-of-fate.json`, le sable et ses anneaux, et
`arena-of-fate/undercroft.json`, les vestiaires et la prison. L'**escalier de la porte du triomphe** est le
portail entre elles ; la **porte des morts** en est un second. Le portail vers le parvis d'Arenarea et
l'escalier condamné des catacombes sont sur la carte du sous-sol. La taille de 34 × 24 ne vaut que
pour la carte du sable ; celle du sous-sol se fixe au tracé. L'onglet « Carte » montre les deux sous la
même sous-zone.

![Plan de principe](../maquettes/plan-arena-of-fate.svg)

Le plan ci-dessus est un **schéma de principe** : il fixe ce que la carte contient et comment on y
circule, pas son dessin.
