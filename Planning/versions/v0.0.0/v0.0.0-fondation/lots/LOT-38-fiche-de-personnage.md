+++
id = "LOT-38"
titre = "Fiche de personnage : maquette et interface"
version = "0.0.0"
filiere = "interface"
statut = "livre"
taille = "L"
resume = "La fiche de personnage, relevée sur les cinq planches du corpus, s'affiche remplie par un personnage réel dont les valeurs sont calculées par la règle."
prerequis = [
  "LOT-13",
  "LOT-68",
  "LOT-76",
]
livrables = [
  "L'écran **Fiche de personnage** rempli : les planches 1 à 4 du corpus portées par l'ossature de `hmi::rpgScreens()`.",
  "Un **neuvième écran**, la feuille d'équipe (planche 5), ajouté par une entrée de table et ses clés de traduction.",
  "`hmi::characterSheetValues`, fonction pure `identifiant → texte`, et ses tests.",
  "`Rpg/characters/demonstration-brenna.json`, son schéma `character.schema.json` et sa validation en intégration continue.",
  "Deux espaces de noms de plus dans `check_glossary.py` (`rpg.skill.`, `rpg.ability.`) : 24 clés de règle contrôlées.",
]
criteres = [
  "**Chaque champ de la maquette est affiché, ou déclaré sans source** — dans la table, à sa ligne, et non dans une liste tenue à part.",
  "**Les cinq planches sont couvertes** : quatre dans la fiche, la cinquième dans un neuvième écran qui n'a demandé de toucher à aucun des huit.",
  "**Les valeurs affichées sont calculées par la règle**, pas recopiées : modificateurs, maîtrises, points de vie, Perception passive, bonus de maîtrise.",
  "**Le personnage affiché est une donnée validée** contre son schéma, provisoire et datée de son critère de retrait.",
  "`ctest` : **1034/1034**.",
  "**L'habillage sur planche gravée a été essayé, puis écarté** — et la raison est écrite plutôt que perdue : `QSvgHandler` plafonne un chemin à 32 768 éléments, le tracé en demande seize fois plus, et le découper change ce qui est plein et ce qui est vide. La feuille reviendra en assets **unitaires**.",
]
+++

## Pourquoi

Le [LOT-68](@ref lot-68) a livré neuf écrans vides. Celui-ci en remplit un : la **fiche de
personnage**, relevée sur la maquette du corpus, et alimentée par un personnage réel.

## Conception

### La maquette : cinq planches, et ce qu'on relève dessus

La source est **`VTT/Blank Sheets RPG (1).pdf`** — les cinq feuilles Tanares **vierges**, arrivées
avec les ressources de table virtuelle. Vierges, et non remplies : un exemple rempli cache ses
propres cadres, et une mise en page se relève sur ce qui la montre. `Character_Sheets_Tanares.pdf`,
déjà au corpus, porte les mêmes feuilles remplies ; `VTT/RPG Sheets - BW print (2).pdf` en donne la
structure à plat.

Les cinq planches sont dans le périmètre :

| Planche | Où elle atterrit |
|---|---|
| 1 — identité, caractéristiques, compétences, sauvegardes, combat, attaques, personnalité | Fiche de personnage |
| 2 — apparence, équipement, langues, biographie | Fiche de personnage |
| 3 — dons, traits et pouvoirs | Fiche de personnage |
| 4 — incantation et emplacements de sorts | Fiche de personnage |
| 5 — **feuille d'équipe** : blason, quartier général, mécénat | Un **neuvième écran** |

**La cinquième planche n'est pas la fiche d'un personnage.** C'est celle de son équipe — renommée,
blason, installations, dessein caché. La ranger dans la fiche aurait mêlé deux sujets sur un même
écran ; elle a donc son écran. Et cet écran est la **preuve** de ce que le `LOT-68` affirmait : il
s'ajoute par une entrée de table et ses clés de traduction, sans qu'aucun des huit autres, ni la
feuille de style, ni le châssis, n'aient été touchés (`EX-IHM-090`).

### Un champ affiché, ou écrit comme non alimenté

Le critère d'acceptation demandait que chaque champ de la maquette soit **affiché** ou **inscrit
dans une liste explicite** de champs hors périmètre — « un champ simplement oublié n'est pas un
arbitrage ». Cette liste n'est pas un document à côté : elle est **dans la table**.

Chaque ligne de l'ossature porte un identifiant de valeur, ou une chaîne vide. Vide veut dire :
*ce champ existe sur la feuille, il est à l'écran, et rien ne l'alimente encore*. Il reste au tiret
cadratin. La colonne des identifiants vides **est** le périmètre restant, lisible d'un coup d'œil,
et impossible à laisser dériver — un champ ajouté sans source se voit à la lecture de la table.

Ce que rien n'alimente aujourd'hui, et pourquoi :

| Champs | En attente de |
|---|---|
| Alignement, nom du joueur | rien ne les porte : ce sont des champs de table, pas d'état de jeu |
| Inspiration | une ressource que rien n'accorde ni ne dépense |
| Points de vie temporaires, jets contre la mort | l'agonie du `LOT-72` |
| Apparence (âge, taille, poids, yeux, peau, cheveux) | un portrait de personnage, `LOT-39` |
| Traits, idéaux, liens, défauts, biographie | le contenu narratif du `LOT-27` |
| Emplacements d'équipement, attaques | l'inventaire du `LOT-14` |
| Dons, traits et pouvoirs | le socle de classe du `LOT-47` |
| Incantation et emplacements de sorts | les sorts du `LOT-35` |
| Toute la feuille d'équipe | la Guilde du `LOT-45` et sa progression, `LOT-83` |

**Aucune de ces cases n'affiche zéro.** Un « 0 » se lirait comme un état du jeu — un personnage
sans emplacement de sort disponible, une CA nulle — et mentirait. Le tiret dit « on ne sait pas
encore », ce qui est la vérité.

### La planche gravée : une piste ouverte, puis refermée

La feuille du corpus a été **vectorisée** et essayée comme fond d'écran : l'idée était que la fiche
cesse de *ressembler* à la planche du livre pour en **être** le trait, avec les intitulés traduits
reposés dessus. Elle a fonctionné, et elle a été abandonnée. Ce qu'elle a appris mérite d'être écrit
ici, parce que le chemin est tentant et que rien n'en signale le bout avant d'y être.

#### Ce qui marchait

Le lettrage d'un tracé **se retire**. Le fichier du corpus est un calque d'encre — un unique chemin
noir de 21 906 sous-chemins, sans balise `<text>` ni calque à masquer, où une lettre est un
sous-chemin comme un filet de cadre. Ce qui les distingue est leur **étendue** : une lettre tient
dans le rectangle de son intitulé, un filet de cadre le traverse de part en part. Retirer les
sous-chemins entièrement contenus dans une liste de rectangles enlevait 2 565 lettres et gardait
19 341 traits d'ornement, proprement.

Et une **seule table** décrivait les deux moitiés — ce qui est effacé, ce qui est reposé —, si bien
qu'il devenait impossible d'effacer un intitulé sans le reposer, ou d'en reposer un sur une gravure
restée en place.

#### Ce qui l'a arrêtée

**Qt ne sait pas rendre ce tracé.** `QSvgHandler` rejette tout `<path>` de plus de 32 768 éléments —
il ne le tronque pas malgré son message, il le **jette** : `isValid()` reste vrai, la taille est
correcte, et le rendu est vide. La panne ressemble donc à un asset manquant, pas à une limite. Ce
tracé en demande 540 094, seize fois et demie la limite.

Le découper en plusieurs chemins ne marche pas davantage, et c'est le point qui n'était pas
prévisible : un remplissage — non-nul comme pair-impair — se calcule sur l'**ensemble** des contours
d'un même chemin. Le blanc d'un cartouche, le trou d'un anneau, l'intérieur du cadre de page
n'existent que parce qu'un autre contour du même chemin y annule le premier. Trois découpes ont été
essayées — dans l'ordre du fichier, par arbre quaternaire sur le centre, puis en mariant chaque
forme aux contours qu'elle contient — et les trois rendent la même image fausse. La raison est dans
la donnée : le contour du **cadre de page** fait à lui seul 24 535 points, 75 % du budget d'un
chemin, et il enveloppe toute la feuille. Aucune coupe ne peut le laisser du même côté que ce qu'il
contient.

Restait le masque d'encre en PNG, qui fonctionnait. Mais un fond monolithique n'est pas un asset :
on ne peut ni déplacer un cartouche, ni réutiliser un anneau, ni changer un ornement sans rejouer
toute la chaîne depuis le corpus. **C'est cette raison-là, et non la limite de Qt, qui a fermé la
piste** : la feuille sera reprise en assets **unitaires** — un cartouche, un anneau, un bandeau à la
fois —, et c'est à ce moment que l'écran cessera d'être une ossature pour devenir une planche.

#### Ce que l'écran est donc aujourd'hui

L'ossature en données du [LOT-68](@ref lot-68), **remplie**. C'est le rendu générique, celui des huit
autres écrans, et il porte les vraies valeurs : identité, progression, six caractéristiques avec
leur modificateur, combat, six jets de sauvegarde, dix-huit compétences. La table des écrans reste
donc la seule description de la fiche, et le jour des assets unitaires elle restera le contrat — ce
sont les *widgets* qui changeront, pas ce qu'ils affichent.

### Ce que la fiche calcule, et où

`hmi::characterSheetValues` est une fonction **pure** : d'une `core::CharacterSheet` et des
catalogues, elle rend une table `identifiant → texte`. Aucun widget, aucun fichier. C'est ce qui
permet de vérifier par test qu'un modificateur s'affiche `+3` et non `3`, qu'une sauvegarde
maîtrisée compte son bonus, et que la Perception passive vaut 10 + le modificateur — sans ouvrir de
fenêtre.

Trois décisions d'affichage y sont prises, et chacune répond à une ambiguïté de lecture :

- **le signe des modificateurs** : `3` nu se lit comme une valeur de caractéristique, et les deux se
  côtoient sur la même ligne ;
- **les points de vie contre leur maximum** (`27 / 32`) : `27` seul ne dit pas si le personnage va
  bien ;
- **la maîtrise se voit** (`+4 •`) : c'est ce que porte la pastille cochée de la feuille, et le seul
  moyen de la rendre dans une ligne de texte.

Le lien entre les deux côtés — ce que la fonction produit, ce que l'ossature attend — est le seul
contrat, et un test le vérifie dans les deux sens. Une faute de frappe d'un côté ne se verrait
sinon qu'à l'écran, sous la forme d'un champ resté au tiret au milieu de champs remplis,
c'est-à-dire pas du tout.

### Le personnage affiché : une donnée, et un échafaudage assumé

Un écran de fiche qui n'affiche aucune fiche ne se relit pas. Il n'y a pourtant ni groupe
([LOT-29](@ref lot-29)) ni sauvegarde ([LOT-17](@ref lot-17)) d'où tirer un personnage réel.

Ce lot livre donc **`Rpg/characters/demonstration-brenna.json`** : une demi-elfe de niveau 3,
cartographe, avec son schéma (`character.schema.json`) et sa validation en intégration continue.

Le fichier ne porte que des **choix** — espèce, classe, historique, valeurs de caractéristique
avant augmentation, niveau. Rien de dérivé : ni points de vie, ni classe d'armure, ni valeurs
finales. Le moteur les calcule (`core::buildCharacterSheet`, `LOT-13`), et le niveau s'atteint par
**gain d'expérience**, le chemin qu'une partie empruntera. Écrire ces valeurs dans le fichier en
aurait fait une seconde source, qui aurait différé de la première au premier ajustement de règle —
et personne n'aurait su laquelle croire.

Le personnage est **déclaré provisoire**, avec son critère de retrait (`EX-CNT-032`) : il disparaît
le jour où une partie en fournit un vrai. C'est aussi ce qui l'autorise à référencer l'une des
quatre classes provisoires du [LOT-36](@ref lot-36) — les seules qui existent — sans casser le
garde-fou qui interdit aux données **définitives** de le faire. Ce garde-fou a été précisé, pas
contourné : une donnée provisoire porte son propre critère de retrait, et disparaît donc avec ce
qu'elle référence.

### Le lexique tient maintenant les compétences et les caractéristiques

Les dix-huit compétences et les six caractéristiques s'affichent, donc s'écrivent dans le catalogue
de traduction. Or ce sont des **termes de règle** — « Escamotage », pas « Tour de main » — et le
lexique du [LOT-30](@ref lot-30) les porte déjà. Les réécrire sans lien avec lui était exactement le
défaut que `check_glossary.py` existe pour empêcher : ses deux nouveaux espaces de noms
(`rpg.skill.`, `rpg.ability.`) portent le contrôle de 0 à **24 clés de règle**.

## Ce que le lot ne fait pas

**Il n'édite rien.** La fiche se lit ; créer ou modifier un personnage est un autre sujet, et il
n'a pas de lot à ce jour.

**Il ne traduit pas les catalogues.** Espèce, classe et historique s'affichent avec le nom que
porte leur donnée — « Brawler », « Cartographer » — parce que les catalogues Tanares sont en
anglais. C'est le sujet du lexique et des lots de contenu, pas de l'écran qui les affiche.

**Il ne remplit pas les huit autres écrans.** Ils gardent leurs tirets, et leurs lots sont nommés
ci-dessus.

## Bilan

Statut : **fait**. Vérification automatisée : build `/W4 /WX` sans avertissement, `ctest` à 1034/1034, `clang-format`, les huit lints, cahier de test et Doxygen verts. Vérification manuelle : la fiche ouverte, remplie et parcourue dans l'application. Tous les critères d'acceptation sont cochés dans l'epic d'origine.

Aucune exigence ajoutée : `EX-IHM-090` et `EX-IHM-091` du [LOT-68](@ref lot-68) couvrent ce lot, et c'est leur premier vrai emploi.
