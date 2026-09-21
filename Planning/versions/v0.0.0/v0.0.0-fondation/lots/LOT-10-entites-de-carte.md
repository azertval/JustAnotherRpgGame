+++
id = "LOT-10"
titre = "Entités de carte et interaction"
version = "0.0.0"
filiere = "moteur"
statut = "livre"
taille = "M"
resume = "Les cartes se peuplent d'entités qui ne sont pas des tuiles — PNJ, coffres, panneaux, portails, déclencheurs — et le joueur interagit avec elles, leur état survivant au changement de carte."
prerequis = ["LOT-04", "LOT-06", "LOT-08"]
livrables = [
  "`core::WorldFlags` (`Source/Core/Gameplay/WorldFlags.{h,cpp}`) : les faits acquis d'une partie, et `keyForEntity()` qui fabrique la clé d'une entité de carte.",
  "`core::Interactable` (`Source/Core/Ecs/Components/Interactable.h`) : le composant qui fait d'une entité une cible.",
  "`core::spawnMapEntities` (`Source/Core/Gameplay/MapEntitySpawner.{h,cpp}`) : une entité ECS par objet de la couche `objects`.",
  "`core::findInteractionTarget` et `core::interact` (`Source/Core/Gameplay/Interaction.{h,cpp}`) : la désignation de la cible et sa résolution.",
  "Neuf tests, dont un par critère d'acceptation.",
]
criteres = [
  "**Ouvrir un coffre deux fois ne donne le butin qu'une fois, y compris après aller-retour de carte** — le test détruit le monde et le reconstruit depuis les mêmes données. ✔",
  "**L'interaction ne traverse pas un mur** : le même coffre est atteignable, puis ne l'est plus une fois sa case rendue pleine. ✔",
  "**À deux cibles à portée, la désignée est déterministe** : dix désignations successives donnent la même. ✔",
  "Deux cartes portant un coffre à la même case ne se marchent pas dessus. ✔",
  "`ctest` : **1001/1001** (992 avant le lot, plus 9). ✔",
]
+++

## Pourquoi

Peupler les cartes d'entités qui ne sont **pas des tuiles** — PNJ, coffres, panneaux, portails,
déclencheurs — et permettre au joueur d'interagir avec elles.

## Périmètre

### Ce que le lot livre

**`core::WorldFlags`** (`Source/Core/Gameplay/WorldFlags.{h,cpp}`) : les faits acquis d'une partie,
et `keyForEntity()` qui **fabrique** la clé d'une entité de carte.

**`core::Interactable`** (`Source/Core/Ecs/Components/Interactable.h`) : le composant qui fait
d'une entité une cible.

**`core::spawnMapEntities`** (`Source/Core/Gameplay/MapEntitySpawner.{h,cpp}`) : une entité ECS par
objet de la couche `objects`.

**`core::findInteractionTarget` et `core::interact`** (`Source/Core/Gameplay/Interaction.{h,cpp}`) :
la désignation de la cible et sa résolution.

**Neuf tests**, dont un par critère d'acceptation.

### Ce que le lot ne fait pas

**Il n'affiche pas l'invite.** Le `Core` produit la clé de localisation et la case visée ; le dessin
de l'invite est de l'interface, et `Core` n'écrit pas de français (`EX-NFR-011`).

**Il ne donne pas de butin.** Un coffre dit qu'il vient d'être consommé ; ce qu'il contient est le
[LOT-26](@ref lot-26), qui suppose les objets du [LOT-34](@ref lot-34).

**Il ne fait pas parler les PNJ**, ni ne fait franchir les portails : ce sont les
[LOT-15](@ref lot-15) et [LOT-09](@ref lot-09).

## Conception

### Le piège du lot : un coffre ouvert deux fois

Le critère est facile à énoncer et facile à rater : *ouvrir un coffre deux fois ne doit donner le
butin qu'une fois, y compris après un aller-retour de carte*. La seconde moitié de la phrase est
celle qui décide de la conception.

**Un booléen sur l'entité ne tient pas.** Quand le joueur quitte la carte et y revient
([LOT-09](@ref lot-09)), l'entité du coffre est détruite puis **recréée depuis le fichier de
niveau**, qui ne sait rien de ce qui s'est passé. Un drapeau porté par l'entité disparaîtrait avec
elle, et le coffre redonnerait son butin à chaque passage — un défaut qui ne casse rien, ne lève
aucune alerte, et se confond avec de la générosité de conception.

L'état vit donc **à côté** des entités, dans `WorldFlags`, qui survit au chargement de carte et que
le [LOT-17](@ref lot-17) sérialisera telle quelle. Le test le vérifie littéralement : il ouvre le
coffre, **détruit le monde**, en construit un neuf depuis les mêmes données de carte, et vérifie
que le coffre recréé n'est plus une cible.

**La clé est fabriquée, jamais écrite à la main.** `keyForEntity()` la compose en
`"<carte>/<type>@<colonne>,<ligne>"` : deux coffres d'une même carte se distinguent par leur case,
et le nom de carte empêche que vider un coffre au village en vide un autre au donjon. Laisser
l'auteur de la carte nommer le drapeau produirait tôt ou tard deux coffres partageant la même clé,
et le second serait vide dès sa première ouverture sans que rien ne l'explique.

Corollaire assumé : **déplacer un coffre dans l'éditeur le remet à neuf** pour une partie déjà
commencée. C'est le bon compromis — l'inverse demanderait un identifiant stable écrit dans le
fichier de carte, que le [LOT-11](@ref lot-11) devrait générer et maintenir unique.

**Le drapeau est levé et sa valeur de retour dit s'il était neuf.** `WorldFlags::set()` renvoie
`false` si le fait était déjà acquis, ce qui évite au gameplay de faire un `isSet` puis un `set` —
deux appels entre lesquels un second pourrait se glisser et donner le butin deux fois.

### Un coffre vidé n'est plus une cible

Ce n'est pas la même chose que « l'interaction ne fait rien » : la désignation elle-même l'écarte.
Continuer à l'afficher comme cible promettrait au joueur quelque chose qui n'arrivera pas, et
l'invite visuelle du lot serait un mensonge.

### Trois règles de désignation, une par critère

**La cible est sur la case visée**, celle devant l'orientation. La case est la voisine dans la
**direction dominante**, jamais une diagonale : un personnage qui regarde à 30° vise la case de
droite. Viser en diagonale rendrait la cible imprévisible sur une manette analogique, alors que le
joueur doit savoir ce qu'il désigne **avant** d'appuyer. Une orientation nulle ne vise rien — rendre
une voisine arbitraire ferait ouvrir un coffre que le joueur ne regarde pas.

**L'interaction ne traverse pas un mur.** La case visée étant adjacente, il n'y a rien entre elle et
le personnage : c'est la case elle-même qui doit être traversable. Un coffre derrière une cloison se
voit et ne s'ouvre pas. Sans cette règle, on ouvre à travers un mur — ce qui se joue et ne se
diagnostique pas.

**À plusieurs candidats, le choix est déterministe** : le plus proche du **centre** de la case
visée, et à distance égale le plus petit indice. Sans départage, deux objets sur la même case
donneraient tantôt l'un tantôt l'autre selon l'ordre de parcours de l'ECS, qui n'est pas stable. La
distance se mesure au centre et non au coin, sinon deux candidats symétriques départageraient sur un
arrondi.

### Un type inconnu produit tout de même une entité

`Core` ne connaît **aucune** sémantique de `type` — c'est la règle que `core::MapEntity` pose depuis
le `LOT-04`. Une entité dont le type n'est pas dans la table des familles connues est créée avec son
`Transform`, sans `Interactable`. La refuser ferait disparaître un objet de la carte sans que son
auteur comprenne pourquoi, et `EX-NFR-040` demande de tolérer plutôt que d'effondrer.

Les familles connues sont une **table**, non un `if` par cas : le [LOT-15](@ref lot-15) ajoutera les
PNJ et le [LOT-09](@ref lot-09) les portails, et chacun devrait sinon retoucher la même fonction.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à
1001/1001, `clang-format`, les six lints, cahier de test et Doxygen verts. Le `LOT-04` apportait la
couche `objects`, le `LOT-06` l'orientation du personnage. Alimente [LOT-09](@ref lot-09),
[LOT-15](@ref lot-15), [LOT-16](@ref lot-16), [LOT-26](@ref lot-26), [LOT-27](@ref lot-27).
L'epic d'origine ne nommait aucune exigence couverte en tête ; le corps cite `EX-NFR-011` et
`EX-NFR-040`.
