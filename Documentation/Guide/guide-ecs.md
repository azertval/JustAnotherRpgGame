# ECS : entités, composants, systèmes

Cette page explique le patron d'architecture **Entity-Component-System**
([ECS](https://en.wikipedia.org/wiki/Entity_component_system) ⧉) depuis ses principes, puis détaille
l'implémentation maison de `Source/Core/Ecs`.

## Le problème que l'ECS résout

Dans un moteur orienté objet « classique », on modéliserait naturellement un personnage par une
classe `Character` héritant de `GameObject`, avec des méthodes comme `update()`, `render()`,
`takeDamage()`. Très vite, ce modèle par héritage devient un obstacle dans un jeu :

- un PNJ a besoin d'une position, d'un visuel, d'une fiche de personnage et d'être sollicitable ;
  un coffre a besoin d'une position, d'un visuel et d'être sollicitable, mais pas de fiche ; un
  décor animé n'a besoin que d'une position et d'un visuel. L'héritage simple ne capture pas ces
  combinaisons : on finit avec une hiérarchie de classes profonde, ou des interfaces vides à
  implémenter « pour la forme » ;
- ajouter un comportement à une seule sorte d'objet (par exemple, rendre les coffres
  destructibles) oblige à modifier une classe existante ou à multiplier les sous-classes ;
- itérer sur « tous les objets sollicitables » demande de parcourir des objets hétérogènes en
  testant leur type dynamiquement, ce qui est lent et fragile.

L'ECS répond en **séparant radicalement** trois notions que l'orienté objet mélange dans une seule
classe :

- **Entité** : juste un **identifiant**. Aucune donnée, aucun comportement — un numéro qui désigne
  « une chose qui existe dans le monde ».
- **Composant** : une **donnée pure**, sans aucune méthode de logique (`EX-ARCH-011`) — par exemple
  une position (`core::Transform`), un visuel (`core::Sprite`), un état d'animation
  (`core::Animation`), une cible d'interaction (`core::Interactable`) ou le lien vers une fiche de
  personnage (`core::RpgActor`). Un composant répond à la question « **quelle donnée** ? », jamais
  « que fait cette donnée ? ».
- **Système** : la **logique**, qui parcourt toutes les entités possédant un certain ensemble de
  composants et les fait évoluer — par exemple une logique qui désigne, parmi les entités
  `Interactable`, celle que le personnage vise.

Un PNJ n'est alors qu'une entité qui **possède** les composants `Transform`, `Sprite`,
`Interactable` et `RpgActor` — une **combinaison** de données, pas une classe dédiée. Un coffre est
une entité avec `Transform`, `Sprite` et `Interactable`, mais sans `RpgActor`. Ajouter un
comportement à un sous-ensemble d'entités revient à écrire un nouveau système qui parcourt les
composants pertinents, sans toucher au reste. La règle d'or à retenir : **les données vivent dans
les composants, la logique vit dans les systèmes** ; un composant ne contient jamais de
comportement, un système ne stocke jamais d'état de jeu à demeure (il le lit/écrit dans les
composants, qui restent la seule source de vérité).

## L'entité : `core::Entity`

Une entité (`core::Entity`) est un **handle générationnel** : une paire `{index, generation}`.

- `index` est la position de l'entité dans les tableaux internes — c'est ce qui permet un accès en
  temps constant.
- `generation` est un compteur qui protège contre un piège classique : quand une entité est
  détruite, son `index` est **recyclé** (réutilisé pour une future entité, plutôt que de croître
  indéfiniment). Sans précaution, un ancien handle vers l'entité détruite désignerait alors, par
  accident, la **nouvelle** entité qui a récupéré le même `index` — un bug silencieux et difficile à
  diagnostiquer, où du code croit encore manipuler l'ancien personnage. En incrémentant
  `generation` à chaque recyclage d'`index`, un handle périmé (ancienne génération) ne correspond
  **plus** à l'entité courante (nouvelle génération) : la comparaison `operator==` compare les deux
  champs, donc `has()`/`getComponent()` rejettent proprement un handle obsolète au lieu de renvoyer
  les données d'une entité sans rapport.

Une entité, à elle seule, ne « fait » rien : elle ne devient un personnage, un décor ou un
coffre que par les composants qu'on lui attache.

## Le `core::World`

`core::World` est le point d'entrée unique de la simulation : il possède les entités, les
composants et les systèmes. API essentielle :

- `createEntity()` / `destroyEntity(e)` — cycle de vie d'une entité ; la destruction retire
  automatiquement **tous** ses composants, dans **toutes** les pools (voir `IComponentPool`
  ci-dessous, qui existe précisément pour rendre cela possible sans connaître chaque type de
  composant à l'avance) ;
- `addComponent<T>(e, valeur)` / `getComponent<T>(e)` / `hasComponent<T>(e)` /
  `removeComponent<T>(e)` — gestion des composants d'une entité, un type à la fois ;
- `view<A, B, …>()` — une **vue** sur les entités possédant **tous** les composants listés (détail
  ci-dessous).

Chaque type de composant obtient sa propre pool, créée **à la demande** au premier
`addComponent<T>` : le `World` ne connaît pas à l'avance la liste des types de composants qui
existeront, il les découvre à l'usage (via `std::type_index` comme clé).

## Le stockage : sparse set (core::ComponentPool<T>)

Le besoin est double et a priori contradictoire :

- **itérer vite** sur tous les composants d'un type (par exemple, tous les `Transform`), ce qui
  demande un tableau **contigu** en mémoire (pas de trous) pour profiter du cache du processeur ;
- **accéder vite** au composant d'une entité précise (« quel est le `Transform` de l'entité 42 ? »),
  ce qui demande normalement un tableau indexé directement par `index`, quitte à laisser des trous
  pour les entités qui n'ont pas ce composant.

Le [sparse set](https://research.swtch.com/sparse) ⧉ concilie les deux avec **deux tableaux** :

- un tableau **dense** (`_components`) : les composants, **sans aucun trou**, dans l'ordre où ils
  ont été ajoutés — c'est lui que les systèmes parcourent ; un tableau `_entities` parallèle retient
  quelle entité possède chaque composant dense ;
- un tableau **creux** (`_sparse`), indexé directement par `Entity::index` : pour chaque entité
  potentielle, il donne la **position** de son composant dans le tableau dense (ou une valeur
  sentinelle `INVALID_POSITION` si elle n'a pas ce composant). Ce tableau peut avoir des trous — peu
  importe, on n'itère jamais dessus, on ne fait que le lire à un index précis.

`has(entity)` et `get(entity)` passent donc par le tableau creux (accès en **O(1)**, temps
constant) ; l'itération complète passe par le tableau dense (**cache-friendly**, aucune entité
absente à sauter).

### Ajout et suppression : *swap-and-pop*

`add(entity, component)` ajoute simplement en fin de tableau dense et enregistre sa position dans
le tableau creux — coût **O(1)**.

`remove(entity)` est plus subtil : retirer un élément **au milieu** d'un tableau dense en décalant
tout ce qui suit coûterait **O(n)**. La technique du ***swap-and-pop*** l'évite :

1. on repère la position `removed` du composant à retirer dans le tableau dense ;
2. on **écrase** cette position avec le **dernier** élément du tableau (`_components[removed] =
   _components[last]`), pour l'entité et le composant ;
3. on met à jour le tableau creux de l'entité **déplacée** (celle qui était en dernière position)
   pour qu'il pointe désormais vers `removed` ;
4. on retranche le dernier élément (`pop_back`) — devenu un doublon.

Résultat : le tableau dense reste **sans trou** en coût **O(1)**, au prix de changer l'**ordre**
d'itération (l'élément déplacé change de position). Comme les systèmes ne dépendent jamais de cet
ordre, ce n'est jamais un problème.

### Exemple pas à pas

Imaginons trois entités `A`, `B`, `C` ayant chacune un composant `Interactable`, dans cet ordre
d'insertion : dense = `[Ia, Ib, Ic]`, entités = `[A, B, C]`. On retire le composant de `B`
(position 1) :

1. `removed = 1`, `last = 2` ;
2. `_components[1] = _components[2]` → dense devient `[Ia, Ic, Ic]` ; `_entities[1] = C` ;
3. le tableau creux de `C` est mis à jour : il pointe maintenant vers la position 1 ;
4. `pop_back()` → dense final = `[Ia, Ic]`, entités = `[A, C]`.

`C` a « pris la place » de `B` dans le tableau dense — l'itération reste dense et rapide, et
`get(C)` continue de fonctionner grâce au tableau creux mis à jour.

> ⚠️ Conséquence pour qui écrit un système : une référence obtenue par `get()` (ou dans une vue)
> est invalidée par tout `add()` ou `remove()` **ultérieur sur la même pool** (le tableau dense peut
> réallouer ou déplacer ses éléments). Ne jamais conserver une telle référence au-delà de telles
> opérations.

## Les vues : core::View<Components...>

Un système a typiquement besoin d'itérer sur « toutes les entités qui ont **à la fois** tel et tel
composant » (par exemple `Transform` **et** `Interactable` pour les objets sollicitables posés sur
la carte). `World::view<A,
B, …>()` construit une `core::View` qui **joint** les pools demandées et n'expose que
l'**intersection** — les entités présentes dans **toutes**.

Pour rester efficace, la vue ne parcourt pas la pool la plus grande en testant les autres : elle
choisit comme « pilote » la **plus petite** des pools demandées (celle avec le moins de composants)
et ne teste l'appartenance aux autres pools que pour les entités de ce pilote. Le coût de
l'itération est ainsi borné par la **plus petite** population parmi les types demandés, jamais par
la plus grande — l'intersection ne peut de toute façon pas être plus grande que son plus petit
opérande.

Deux syntaxes équivalentes :

```cpp
for (auto [entity, transform, sprite] : world.view<Transform, Sprite>()) {
    transform.scale = {2.0f, 2.0f};
}

// équivalent, forme fonctionnelle :
world.view<Transform, Sprite>().each(
    [](core::Entity, core::Transform& t, core::Sprite&) { t.scale = {2.0f, 2.0f}; });
```

C'est **exactement** le motif d'un système : une vue, une lambda, la logique de mise à jour.

> ⚠️ Contrat d'itération : à l'intérieur d'un `each` ou d'une boucle sur une vue, on ne modifie que
> la **valeur** des composants obtenus. Ajouter/retirer un composant ou détruire une entité
> **pendant** l'itération invaliderait la vue (les pools sous-jacentes peuvent bouger, cf.
> *swap-and-pop* ci-dessus) — ce type de modification structurelle doit être différé après
> l'itération.

## Les systèmes et l'ordre d'exécution

`World::addSystem(std::unique_ptr<ISystem>)` enregistre un système ; `World::update(fixedDelta)`
exécute **tous** les systèmes enregistrés, **dans l'ordre d'enregistrement**, une fois par pas de
temps fixe ([Boucle de jeu et pas de temps fixe](guide-boucle.md)). Cet ordre est significatif et fait partie du contrat de
déterminisme (`EX-NFR-002`) : deux systèmes qui lisent et écrivent les mêmes composants doivent
s'exécuter dans un ordre stable pour produire toujours le même résultat.

Aucun système n'est enregistré aujourd'hui : le mécanisme est en place, mais la logique qui
existe n'en a pas eu besoin. Le monde se peuple par `core::spawnMapEntities`, qui crée **une
entité par objet** de la couche `objects` d'une carte — un `Transform` à sa case, et un
`Interactable` si son type figure dans `core::knownInteractableKinds` (coffre, panneau, PNJ…).
Une logique qui a besoin de données que l'`ISystem` générique ne transporte pas (la grille de la
carte, l'orientation du personnage, les drapeaux de monde) prend alors la forme d'une **fonction
libre** à signature dédiée, comme `core::findInteractionTarget`. Le principe reste identique : la
logique lit des composants et n'en garde aucun état.

## Voir aussi
- `core::World`, `core::Entity`, `core::EntityManager`, `core::ComponentPool`, `core::View`.
- `core::ISystem`, `core::spawnMapEntities`, `core::findInteractionTarget`.
- `core::Transform`, `core::Sprite`, `core::Animation`, `core::Interactable`, `core::RpgActor`.
- [Mathématiques du moteur](guide-maths.md) (les types de données des composants), [Niveaux : modèle, couches, entités, chargement](guide-niveaux.md) (la couche `objects`).
