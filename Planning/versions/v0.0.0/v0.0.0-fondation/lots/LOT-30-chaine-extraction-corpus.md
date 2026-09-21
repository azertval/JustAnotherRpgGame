+++
id = "LOT-30"
titre = "Chaîne d'extraction du corpus et lexique bilingue"
version = "0.0.0"
filiere = "donnees"
statut = "livre"
taille = "M"
resume = "Les huit PDF du corpus s'extraient de façon reproductible, et le lexique bilingue des termes de règle fait autorité."
prerequis = ["LOT-77"]
livrables = [
  "`scripts/sourcebook/corpus.toml` : le manifeste des huit documents (empreinte, pages, décalage, provenance).",
  "`scripts/sourcebook/corpus.py` : lecture du manifeste, refus d'une empreinte qui ne correspond plus, page imprimée ↔ index PDF.",
  "`scripts/sourcebook/extraction.py` : texte, tableaux par coordonnée, images par rendu clippé, cache disque.",
  "`scripts/sourcebook/glossaire.py` et `Source/Elements/Localization/rpg.glossary.csv` : 2 084 entrées.",
  "`scripts/sourcebook/__main__.py` : la ligne de commande.",
  "`scripts/check_glossary.py`, auto-testé, exécuté en CI.",
]
criteres = [
  "Deux exécutions successives produisent un fichier identique (`EX-CNT-020`).",
  "Une empreinte qui ne correspond plus fait échouer l'extraction au lieu de produire des données décalées (`EX-CNT-020`).",
  "Aucun tableau n'est extrait en flux de texte ; la table du barbare, en OCR, sort juste au niveau près (`EX-CNT-021`).",
  "Les images passent par le rendu d'une région, jamais par le flux brut (`EX-CNT-022`).",
  "Le corpus et l'intermédiaire ne sont pas versionnés ; le lexique l'est (`EX-CNT-023`).",
  "`check_glossary.py` échoue si une clé de règle emploie un terme absent du lexique ou en contredit la traduction — vérifié par injection dans `fr.lang`, sur les trois formes de faute.",
  "`check_glossary.py` est exécuté en intégration continue.",
]
sources = [
  "Basic Rules, p. 32 (écoles de magie), p. 49 (propriété « spéciale »), p. 134 (épuisement)",
]
+++

## Pourquoi

Outiller l'extraction des huit PDF de `Documentation/SourceBook/` — environ 1 200 pages, 280 Mo —
puis **éprouver cet outillage** sur son gisement le plus simple, le glossaire de traduction.

Ce lot ne produit presque pas de données de jeu. Il produit ce qui les produira, et une seule
sortie : la table d'autorité des termes de règle. C'est délibéré — un outil d'extraction dont on
n'a rien extrait est un outil dont on ne sait rien.

> **Fusionné à l'audit.** Ce lot a absorbé l'ancien `LOT-31` (lexique bilingue), qui livrait deux
> fichiers, soit un ordre de grandeur sous l'étalon. Le numéro `LOT-31` est retiré.

## Périmètre

### Ce que le lot livre

**`scripts/sourcebook/corpus.toml`** — le manifeste des huit documents : empreinte SHA-256, nombre
de pages, pagination, décalage, provenance (`srd`, `tanares`, `phb-fr`). Il est versionné ; les PDF
qu'il décrit ne le sont pas (`EX-CNT-023`), d'où sa place à côté de l'outil et non à côté d'eux.

**`scripts/sourcebook/corpus.py`** — la lecture du manifeste, le refus d'un document dont
l'empreinte ne correspond plus, et la correspondance page imprimée ↔ index PDF.

**`scripts/sourcebook/extraction.py`** — texte, tableaux **par coordonnée**, images **par rendu
clippé**, cache disque indexé par empreinte, découpe en demi-pages.

**`scripts/sourcebook/glossaire.py`** et **`Source/Elements/Localization/rpg.glossary.csv`** — le
lexique, `anglais;français;catégorie`, **2 084 entrées**.

**`scripts/sourcebook/__main__.py`** — la ligne de commande : `info`, `verifier`, `texte`,
`tableau`, `image`, `regions`, `stats`, `glossaire`.

**`scripts/check_glossary.py`**, exécuté en CI.

### Ce que le lot ne fait pas

**Il n'extrait aucune donnée de jeu.** Ni créature, ni objet, ni sort : ils attendent les schémas du
`LOT-32`, qui est le contrat que ces données devront honorer. Extraire avant le contrat produirait
des JSON à reprendre entièrement.

**Il n'extrait aucune illustration.** L'outil sait proposer des régions candidates et rendre celles
qu'on lui désigne ; le tri est humain, par construction (`EX-CNT-022`), et chaque lot de catalogue
livrera ses propres images.

**Rien de tout cela ne tourne en CI.** Les PDF ne sont pas sur le runner et n'y seront pas
(`EX-CNT-023`). Seul le lexique produit, qui est versionné, y est contrôlé.

## Conception

### Ce que la réalisation a vérifié, et ce qu'elle a démenti

**La double page est confirmée, chiffre à l'appui.** La page PDF 50 du `Tanares_Sourcebook` porte
les pages imprimées 100 et 101 ; celle du `Players_Guide` aussi. Les décalages des six autres
documents ont été relevés un par un, et deux ne valent pas zéro : le `Glossaire` imprime « Page 1 »
sur sa page PDF 0, les *Basic Rules* et `Animaux` sont décalés de −1. Le `Manuel-Des-Joueurs` a
demandé un relevé statistique — son OCR est trop bruité pour qu'un numéro de page se lise à coup
sûr — recoupé sur deux repères de structure : le chapitre 1 et le début du barde.

**Le tableau par coordonnée tient, y compris sur un scan.** La table des armes des *Basic Rules*
sort en cinq colonnes alignées, chaque valeur en face de son arme. Surtout, la table de progression
du barbare du `Manuel-Des-Joueurs` — un OCR — donne bien « Attaque supplémentaire » au **niveau 5**,
là où un rendu en flux de texte décale toute la colonne d'un cran. C'était l'affirmation la plus
risquée de l'analyse du corpus ; elle est vérifiée.

**Une limite assumée, et son échappatoire.** Le regroupement en colonnes par profil de projection
échoue quand l'entrée la plus longue d'une colonne mord sur la suivante : sur la table des armes,
« Épée à deux mains » ferme l'écart entre le nom et le dégât, et les deux colonnes fusionnent. La
valeur reste juste, mais dans la mauvaise case. D'où `--bornes`, qui impose les séparations. C'est
la seule ambiguïté qu'un humain doive lever, et elle se voit sur la première ligne.

**Le glossaire porte 2 084 entrées, pas « environ 1 200 ».** La feuille de route annonçait ce
dernier chiffre depuis l'analyse du corpus ; il était sous-évalué de 74 %. La page est corrigée.

**Le glossaire porte de vrais homonymes**, et c'est ce qui a fait changer la clé d'unicité. `light`
vaut « légère » comme propriété d'arme et « Lumière » comme sort ; `bane` vaut « Fléau /
Imprécation » comme sort et « Baine » comme divinité ; `piercer` a deux acceptions. Dédupliquer sur
l'anglais seul en écrasait un des deux **en silence** — et pour `light`, faisait traduire un sort
par un adjectif d'arme. La clé est donc le couple **(anglais, catégorie)**, et deux entrées de même
couple qui divergeraient font échouer la construction.

**Cent cinquante-trois lignes sur 2 231 sont des continuations.** Le PDF coupe à la largeur de
colonne ; une ligne sans `=` prolonge la précédente. Les ignorer aurait amputé autant de catégories
sans rien signaler. Cinq entrées étaient en outre coupées **après un trait d'union** — « demi- orc »,
« outre- monde », « anti- détection » — et se recollent sans espace.

### Le complément des *Basic Rules* est attesté, pas saisi

Trois manques du glossaire portent sur des ensembles **fermés** que le moteur énumérera :

- les **huit écoles de magie** n'y figurent que sous leur forme « voie de magicien »
  (`School of Abjuration = École d'abjuration`), jamais nues ;
- `exhaustion = épuisement` y est **sans catégorie**, alors que c'est la quinzième condition ;
- la propriété d'arme `special` manque entièrement.

Chaque complément **déclare la page où il est attesté**, et le module vérifie que le terme français
s'y trouve avant de l'écrire — page 32 pour les écoles, 49 pour `spéciale`, 134 pour `épuisement`.
Un complément saisi de mémoire est une donnée inventée qui a l'apparence d'une donnée extraite ;
c'est ce que le projet refuse, et une assertion vérifiable coûte moins cher qu'une relecture.

Le lexique fige au passage un faux ami que rien d'autre n'aurait attrapé : l'école `conjuration` se
dit **« invocation »** en français, pas « conjuration ».

### Ce que le contrôle vérifie, et pourquoi il s'auto-teste

`scripts/check_glossary.py` vérifie que le lexique est bien formé — pas de doublon de couple
(anglais, catégorie), pas de terme vide, et les trois ensembles fermés au complet : 8 écoles,
15 conditions, 13 types de dégâts. Ces nombres sont fixés par les règles du jeu, pas par le corpus ;
s'ils bougent, c'est l'extraction qui a régressé.

Il vérifie ensuite que toute **clé de règle** de `fr.lang` / `en.lang` — celles des espaces de noms
`condition.`, `damage.`, `school.`, `weapon_property.`, `ability.`, `skill.` — porte un terme du
lexique, traduit comme le lexique le dit. La comparaison ignore la casse mais **pas les accents** :
une table d'autorité française qui accepte « etourdi » pour « étourdi » n'impose plus rien.

**Aucune clé de règle n'existe encore** : les lots qui les créeront ne sont pas faits. Le contrôle
serait donc vert par vacuité, et personne ne saurait s'il fonctionne — c'est exactement la panne du
[LOT-78](LOT-78-numeros-herites.md), où une règle de lint contenait un caractère invisible qui l'empêchait de
jamais correspondre. Il s'**auto-teste** donc à chaque appel, sur six catalogues fictifs (deux
conformes, quatre fautifs) qu'il doit classer correctement avant d'avoir le droit de se prononcer
sur les vrais. Si l'auto-test échoue, le script s'arrête sans rendre de verdict.

## Vérification

Ce que l'epic relevait en face de chaque critère, dans l'ordre des critères de l'en-tête :

1. ✔
2. ✔
3. ✔
4. ✔
5. ✔
6. ✔
7. ✔

## Bilan

Statut : **fait**. Vérification automatisée : `check_glossary`, `lint_lots`, `lint_exigences`, `check_qt_version_pin`, `check_design_tokens`, cahier de test et Doxygen verts ; deux exécutions successives de l'extraction produisent un fichier identique.

Prérequis de tous les autres lots de la filière contenu.

Exigences couvertes : [`EX-CNT-020`](../../../../../Documentation/Specification/contenu.md#EX-CNT-020), [`EX-CNT-021`](../../../../../Documentation/Specification/contenu.md#EX-CNT-021), [`EX-CNT-022`](../../../../../Documentation/Specification/contenu.md#EX-CNT-022), [`EX-CNT-023`](../../../../../Documentation/Specification/contenu.md#EX-CNT-023).
