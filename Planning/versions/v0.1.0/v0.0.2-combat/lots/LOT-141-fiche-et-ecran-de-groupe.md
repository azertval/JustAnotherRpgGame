+++
id = "LOT-141"
titre = "La fiche et l'écran de groupe"
version = "0.0.2"
filiere = "interface"
statut = "livre"
taille = "M"
resume = "La fiche de personnage montre la classe : capacités, sorts, progression ; l'écran de groupe montre les quatre."
prerequis = ["LOT-138", "LOT-132", "LOT-133", "LOT-134", "LOT-135"]
reprend = ["LOT-38", "LOT-74 (progression, en partie)"]
livrables = [
  "La fiche : onglet **Classe** (capacités acquises, à venir), onglet **Sorts** (lancers restants).",
  "L'écran **Groupe** : les quatre profils côte à côte, le meneur, l'ordre de marche.",
  "La montée de niveau, du niveau 1 au niveau 5.",
]
criteres = [
  "Chacune des quatre fiches préfabriquées s'affiche comme sa page du livre, valeur pour valeur.",
]
+++

## Périmètre

La montée de niveau est **donnée** (par la quête, par un bouton de débogage) : l'expérience et sa
courbe arrivent en `0.2.0`.

## Décisions de réalisation

Livré le 28 septembre 2026 (exigence `EX-IHM-109`), **PR #160**, branche empilée sur le `LOT-140`.

1. **Une seule fiche, pour tous les écrans.** Depuis le `LOT-139`, le combat lisait le registre
   du groupe (`core::PartyLedger`) et la fiche relisait le fichier pré-tiré, plein : deux
   vérités. `loadDemonstrationState(fichier)` applique désormais le registre de la partie en
   cours au personnage dont c'est la fiche — le niveau donné (`core::levelUpTo`, nouvelle
   fonction du cœur : l'expérience du seuil puis `applyClassFeatures`, ce que le support de test
   des classes faisait déjà), puis les points de vie et les lancers (`core::applyRecord`). Fiche,
   inventaire, dialogue, écran de groupe et combat lisent la même fiche ; l'écran de groupe
   relit les siennes à chaque `partyChanged`, le combat met le niveau dans la clé de son cache.
2. **Le niveau vit au registre** : `core::MemberRecord::level`, posé par `WorldModel::levelUp`
   (un personnage, ou `party` pour tout le groupe), qui monte la fiche telle que la partie l'a
   laissée et retient le niveau **et les points de vie** qui en résultent — la montée n'est pas
   un soin (`core::gainExperience`) : un Priest à 5 PV sur 12 passe à 14 sur 21. Bornée au
   maximum de la table d'expérience ; `levelUp` rend faux au-delà, ou pour un inconnu.
3. **Donnée par la quête, par le débogage.** Action de dialogue `levelUp` (`character` : une
   fiche, ou `party`), au schéma, au lecteur, au journal du runner (« niveau donne : … »), et
   `core::DialogueListener::levelUp`, sans effet par défaut comme `startEncounter` ; l'écran de
   dialogue l'applique à la partie sans rien ouvrir. Menu `F9`, section « Groupe » : un membre ou
   tout le groupe, +1. **Aucun dialogue de la démo ne la donne encore** : le quatre contre six du
   `LOT-139` a été équilibré pour quatre niveaux 1, et c'est la recette (`LOT-142`) qui décidera
   où la démo fait monter le groupe.
4. **La fiche de chacun.** `CharacterSheetModel::loadCharacter(id)` (un membre de la partie, ou
   une fiche pré-tirée du binaire ; inconnu → le personnage joué, dit au journal) et
   `loadShownCharacter()` : la partie désigne le personnage de la fiche
   (`WorldModel::shownCharacterId`, celui de l'écran de groupe — touche `F`, `Y` —, sinon le
   meneur) ; `Tab` passe au suivant. La vue-modèle publie `capacities` (avec le niveau où la
   table les donne, l'icône de leur base), `upcomingCapacities` (les **quatre** prochains
   niveaux : la page de la classe d'un coup d'œil, pas vingt niveaux) et `spells` (lancers
   restants « 1 / 2 » ou « à volonté », portée, durée, dés, soin).
5. **Trois onglets sur la colonne droite de la fiche** — Compétences, Classe, Sorts —, par
   `Page suiv.` / `Page préc.` (`RB` / `LB`), les listes défilant par `Haut` / `Bas` ; la fiche
   gagne le focus, `Échap` (`B`) la referme, comme l'écran Groupe. Le portrait est celui de la
   figurine de la classe. L'écran **Groupe** montre les **quatre profils côte à côte** dans
   l'ordre de marche (portrait, nom, espèce · classe niveau, jauge, CA · vitesse, meneur marqué,
   rang, place libre), les personnages en lignes compactes dessous.
6. **Valeur pour valeur** : `test_character_sheet_model.cpp` ouvre les quatre fiches par leur
   identifiant et compare nom, classe, niveau, points de vie, CA, capacités acquises, sorts
   connus et lancers aux pages du *Player's Guide* (registre du `LOT-130`) ; les valeurs
   dérivées restent celles de `test_premade_characters.cpp`.

Tests : `test_dialogue.cpp` (l'action `levelUp`), `test_character_sheet_model.cpp` (les quatre
pages ; la montée donnée vue par la fiche, l'écran de groupe et le combat ; la borne),
`QmlTests` (captures `CharacterSheetForm.png` et `PartyForm.png` refaites).

## Ce qui n'est pas ici

- L'expérience gagnée à la victoire et sa courbe : `0.2.0`.
- Le choix du joueur aux capacités narratives (*Ability Score Improvement*, *Experience*) : elles
  s'affichent, ne se choisissent pas (`EX-CNT-031`).
- Un repos long qui rend les lancers (`core::longRest`) : avec l'auberge.
