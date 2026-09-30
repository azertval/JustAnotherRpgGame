+++
id = "LOT-143"
titre = "Éditeur — des zones de combat pour un groupe"
version = "0.0.2"
filiere = "editeur"
statut = "livre"
taille = "S"
resume = "En dessinant une zone de combat, on voit si quatre personnages et la rencontre entière y tiennent, et ce que pèse la rencontre."
prerequis = ["LOT-139"]
livrables = [
  "Le verdict tactique d'une zone de combat compte le **groupe de quatre** et tous les adversaires de la rencontre : places d'entrée, formation, cases libres autour.",
  "Le **budget de difficulté** de la rencontre (LOT-139) affiché à côté de l'entité `encounter`, pour un groupe de niveau donné.",
  "`--check` : une rencontre dont le groupe ne peut pas se déployer est une erreur.",
]
criteres = [
  "La zone de combat de l'Arena of Fate porte un quatre contre quatre ; réduite de moitié au canevas, son verdict passe au rouge pendant qu'on la tire.",
  "Un scénario `--apply` couvre le cas ; aucun code par famille n'est ajouté à l'éditeur.",
]
+++

## Pourquoi

Le verdict d'une zone de combat (`LOT-EDITOR-05`) a été écrit pour un duel : il vérifie que la
formation **adverse** tient sur le terrain. Avec le [LOT-139](LOT-139-combat-de-groupe.md), c'est
le groupe du joueur qu'il faut aussi placer. Toutes les zones de combat des versions suivantes en
dépendent.

## Décisions de réalisation

Livré le 28 septembre 2026 (exigence `EX-EDIT-101`).

1. **Le verdict du groupe est une fonction du `Core`**, `core::analyzePartyDeployment`
   (`Core/Combat/PartyDeployment.h`), pure, sur une grille et des entités comme
   `core::analyzeCombatZones` : l'éditeur la recalcule sur l'aperçu à chaque mouvement de souris.
2. **La zone d'une rencontre** est celle qui contient son marqueur, comme au jeu
   (`core::prepareMapEncounter`) ; à défaut, **la plus proche** — celle où se tiendra le groupe qui
   l'engage. C'est ce qui fait rougir le sable qu'on réduit : la rencontre reste la sienne, et son
   marqueur comme ses adversaires sont dits hors de la zone. Une carte qui a une rencontre et
   aucune zone est refusée (« no combat zone to fight it in »).
3. **La formation se pose sur la zone seule**, par la `core::BattleGrid` de la collision
   découpée : c'est la grille que `cropLevelToZone` donne au combat. Un adversaire hors de la zone
   est un défaut ; un mur ou un chevauchement reste dit par `core::analyzeEncounterTerrain`, une
   seule fois.
4. **Les places du groupe, décision nommée.** Au jeu, le groupe entre là où il marche
   (`LOT-139`) : l'éditeur ne peut pas le savoir. Le verdict lui cherche quatre places hors de la
   formation et **reliées** à elle sur la zone (un groupe muré loin des adversaires ne combat pas) :
   les entrées d'arène **alliées** de la zone d'abord, par rang, puis **le front opposé** au
   marqueur — la case la plus lointaine, et ses plus proches voisines. Un groupe qui y tient tient
   ailleurs sur la zone.
5. **Les cases libres autour** : les cases de la zone reliées au marqueur doivent loger
   `(adversaires + 4) × TACTICAL_CELLS_PER_COMBATANT`, le seuil du `LOT-11`, borné cette fois à la
   zone et non à un rayon sur la carte.
6. **Aucun code par famille dans l'éditeur** : `hmi::entityVerdicts`
   (`Editor/Logic/EntityVerdicts.h`) rend, par entité, des lignes, des cases à **rôle** (libre,
   pleine, entrée, adversaire, place du groupe) et un « bon ou pas ». Le canevas peint la première
   ligne à côté de chaque entité et tout pour la sélectionnée, l'inspecteur montre les lignes,
   `--apply` les verse au compte rendu par le nouvel outil `inspect` — aucun des trois ne lit un
   type. L'ancien peintre du verdict de zone est remplacé.
7. **Le budget** (`core::rateEncounter`, `LOT-139`) se lit dans `Rpg/rules/encounter-difficulty.json`,
   chargé avec les autres catalogues de l'éditeur, pour **quatre** personnages du niveau que règle
   « Party level » dans le panneau des entités (gardé d'une session à l'autre, le même pour tous
   les onglets) ; `partyLevel` dans un fichier de gestes. Les catégories se nomment comme la donnée
   (« difficile »). Un budget ne rend jamais une entité rouge : c'est une information d'auteur.
8. **`--check`** : les défauts du déploiement sont des **erreurs** du contrôle du contenu.

**Écart au critère** : la fiche parlait d'un quatre contre quatre ; depuis le `LOT-139`, la
rencontre du sable est de six bandits. Le critère se vérifie sur quatre contre six (244 cases
reliées pour 40 exigées), et réduit de moitié par sa poignée sud-est, le sable passe au rouge :
marqueur et six bandits hors de la zone.

Tests : `test_party_deployment.cpp` (zone dégagée, entrées alliées, formation dehors, zone trop
étroite, mur qui coupe le groupe, rencontre sans zone), `test_gesture_script.cpp`
(`LeSableDeLArenaOfFatePorteLeGroupeEtRougitReduit`, scénario `Fixtures/Gestures/combat-zone.json`
rejoué sur la carte livrée), `test_content_check.cpp` (`UnGroupeQuiNeSeDeploiePasFaitEchouerLaCi`).

**Reste à la main de l'auteur** : tirer le sable à la souris dans la fenêtre et voir le verdict
rougir, régler « Party level ».
