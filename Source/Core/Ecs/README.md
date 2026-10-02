# Core/Ecs/

Entity-Component-System (implémentation **maison**), cœur de la simulation.

- Entités : handles générationnels.
- Composants : données pures (sparse sets) — sous-dossier `Components/`.
- Systèmes : logique itérant sur les composants (`ISystem`).
- `World` : façade, orchestration au pas de temps fixe.

Implémenté :
- `Entity` — handle générationnel `{ index, generation }`, `INVALID_ENTITY`.
- `EntityManager` — `create` / `destroy` / `isAlive`, recyclage des index par liste libre.
- `ComponentPool<T>` — sparse set typé (`add` / `get` / `has` / `remove` / `removeIfPresent`), tableau dense contigu, suppression par swap-and-pop.
- `View<Components...>` — vue multi-composants itérant l'intersection (pilotée par la plus petite pool), API `for (auto [entity, ...] : view)` et `view.each(...)`.
- `ISystem` — interface d'un système (`update(World&, float fixedDelta)`).
- `World` — façade : `createEntity` / `destroyEntity` (purge toutes les pools), `addComponent` / `getComponent` / `hasComponent` / `removeComponent`, `view<...>()`, `addSystem` / `update` (exécution ordonnée au pas fixe).
- `AnimationClip` / `ClipSet` — clip d'animation en tant que donnée (images, durée par image, fin en boucle ou unique) et jeu de clips nommés.
- `Components/` — composants données pures : `Transform` (position, échelle, rotation), `Sprite` (région d'atlas, couche, teinte — lu par le rendu de `HMI`), `Animation` (jeu de clips, clip et image courants, `EX-REN-005`), `Interactable` (cible d'interaction d'une entité de carte), `RpgActor` (lien d'une entité vers sa fiche de personnage).

Réf. specs : `EX-ARCH-010`, `EX-ARCH-011`, `EX-ARCH-012`, `EX-ARCH-100`.
