+++
id = "LOT-122"
titre = "Recette et version 0.0.1"
version = "0.0.1"
filiere = "version"
statut = "livre"
taille = "M"
resume = "La démo est jouée, corrigée, empaquetée et taguée."
prerequis = ["LOT-120", "LOT-121"]
reprend = ["LOT-28 (version et tag ; l'audio part en 0.2.0)"]
livrables = [
  "Le numéro de version ramené à `0.0.1` dans `CMakeLists.txt` — il porte `0.1.0`, ce qui est un contresens.",
  "La recette : trois parties complètes par trois personnes, anomalies corrigées ou inscrites.",
  "L'installeur, les notes de version, le tag `v0.0.1`.",
  "Le bilan de la version dans `Planning/` : ce qui a coûté plus que prévu, ce que la `0.0.2` doit en retenir.",
]
criteres = [
  "Les quatre critères de sortie de la version sont tenus.",
  "L'installeur se lance sur un poste vierge (test de fumée de la release).",
]
+++

## Périmètre

Pas d'audio, pas d'effets : ils étaient au `LOT-28`, ils partent avec les dernières
fonctionnalités de la `0.2.0`.

## Décisions de réalisation

Recette faite le 25 septembre 2026 par l'auteur, sur la branche de recette (PR de publication de
la version, label `no-changelog` : elle ferme la section du CHANGELOG, elle n'y ajoute rien).

1. **La recette s'est faite à une personne et trois tests.** Le livrable disait « trois parties
   complètes par trois personnes ». L'auteur a joué l'IHM et la démo jusqu'à ses trois fins ; les
   trois `SystemGameTests` (étiquette `systeme`) rejouent chacune de « Nouvelle partie » à l'écran
   de fin, graine fixée, et `IntegrationTests` joue la chaîne sans fenêtre. Le critère « un joueur
   qui ne connaît pas le projet » n'a pas eu son joueur extérieur : le [bilan](../bilan.md) le
   passe à la `0.0.2` (`LOT-142`).
2. **Les mannequins partent à la `0.0.2`** ([D-26](../../../../vision/decisions.md)). Le `LOT-145`
   était en cours, la démo se joue avec le mannequin humanoïde déjà installé ; une version ne se
   ferme pas sur un lot dont elle n'a pas besoin. La fiche garde son numéro et déménage dans le
   dossier de la `0.0.2` ; c'est le seul rescopage de la recette.
3. **Le numéro de version se pose à la recette, et il nomme la version taguée.** `CMakeLists.txt`
   portait `0.1.0`, le référentiel de l'Empire central : ramené à `0.0.1`. `core::Engine::version()`
   et le `PROJECT_NUMBER` de la Doxygen le suivent, rien d'autre n'est à aligner.
4. **Le CHANGELOG reçoit deux sections.** `## [0.0.1] - 2026-09-25` reçoit un chapeau de jalon et
   tout ce qui suit la refonte du 20 septembre (`LOT-100` compris) ; `## [0.0.0] - 2026-09-20`,
   jamais publiée ni taguée, reçoit tout ce qui précède, comme la version « Fondation du moteur »
   du planning. Les notes de la release sont la section `0.0.1` telle quelle
   (`extract_release_notes.py v0.0.1`, 62 000 caractères, sous la limite de 125 000 de GitHub) ;
   sans ce découpage, le workflow aurait échoué au tag.
5. **Le code est audité, pas seulement joué.** Quatre relectures — `Source/Core`,
   `Source/HMI` + `App` + `Ui`, l'outillage Python et les workflows, la documentation — cherchent
   le code mort, le code sans documentation et les résidus du Colisée et du pixel art. Ce qui est
   sûr est corrigé dans cette PR ; ce qui ne l'est pas est inscrit au bilan.
6. **Le dépôt se range.** La branche `worktree-lot-146-cartes-de-principe`, dont l'arbre est celui
   d'un commit de `main` (PR #137 et #138), ne porte rien d'inédit : elle se supprime ; la release roulante `debug-latest` reprend le nom du
   projet à chaque publication (`release.yml`) ; les quatre releases `assets-*` sont les kits
   d'assets verrouillés par `kits.lock.json`, pas des releases obsolètes, et restent. Les
   vingt tags `archive/*` (sauvegardes de branches des lots `LOT-10` à `LOT-85`) sont des états
   d'avant la refonte : leur retrait est une décision de l'auteur, pas de la recette.

## Livraison

Le tag `v0.0.1` se pose sur le commit de fusion de cette PR (`git tag v0.0.1 && git push origin
v0.0.1`) : `release.yml` teste ce commit en Debug et en Release, empaquette les deux archives,
les lance, et publie la release avec la section `0.0.1` du CHANGELOG pour notes. Le
[bilan de la version](../bilan.md) est écrit ; le README du projet et le manuel décrivent la démo
telle qu'elle se joue.
