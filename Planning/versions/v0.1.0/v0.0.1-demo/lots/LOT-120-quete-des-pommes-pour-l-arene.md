+++
id = "LOT-120"
titre = "La quête « Des pommes pour l'arène »"
version = "0.0.1"
filiere = "quete"
statut = "livre"
taille = "M"
resume = "La quête de la démo, jouable de bout en bout par ses trois issues."
prerequis = [
  "LOT-146",
  "LOT-116",
  "LOT-117",
  "LOT-118",
  "LOT-119",
  "LOT-126",
]
livrables = [
  "La quête en données : un drapeau à cinq valeurs, huit étapes.",
  "Quatre dialogues : la mère (ouverture, clôture), le garde (choix et jet), le maître d'arène, l'enfant.",
  "La rencontre de l'arène, **équilibrée par simulation**, et la fiche du combattant (`Rpg/`), joué par le mannequin humanoïde ; sa figurine est au LOT-113.",
  "Les textes en français et en anglais.",
]
criteres = [
  "Les trois issues se jouent en test système, sans fenêtre, à graine fixée.",
  "Sur cent combats simulés avec le héros de la démo, il l'emporte entre 60 et 70 fois.",
  "Un joueur qui ne connaît pas le projet finit la démo sans aide.",
]
maquettes = ["../maquettes/quete-demo-deroule.svg"]
+++

Le déroulé, les drapeaux, les PNJ et l'équilibrage sont dans
[la fiche de la quête](../quete-demo.md).

La quête se joue sur les **cartes de principe** du [LOT-146](LOT-146-cartes-de-principe-de-la-demo.md),
où ses cinq PNJ sont déjà posés avec leurs conditions de présence ; ils y sont des mannequins
([LOT-145](LOT-145-mannequins-de-remplacement.md)) ou des jetons, jusqu'à leurs figurines de la
`0.0.3` ([D-25](../../../../vision/decisions.md)). Ce lot écrit ce qu'ils disent et ce qui se joue.

![Déroulé](../maquettes/quete-demo-deroule.svg)

## Décisions de réalisation

Livré le 25 septembre 2026, avec le [LOT-146](LOT-146-cartes-de-principe-de-la-demo.md) dans la
même branche, sur décision de l'auteur.

1. **Un drapeau à cinq valeurs, six étapes.** `quete.pommes` : `inconnue`, `acceptee`,
   `persuasion-echouee`, `condamne`, `enfant-libere`. Les étapes lisent le monde : `acceptee`,
   `condamne`, `enfant-libere` par la valeur ; `persuasion-echouee` par le fait durable du jet
   raté (`dialogue/garde/jet-persuasion/failed`, `LOT-117`), parce que le dialogue écrase la valeur
   avant qu'un pas ne la lise ; `victoire` par le fait de la rencontre gagnée, et son effet pose
   `enfant-libere` ; `rendue` par le fait que pose la mère, issue `success`. La huitième étape
   annoncée n'existe pas : une défaite finit la démo sans laisser de quête à avancer.
2. **La victoire pose `encounter/<rencontre>/won`** (`core::encounterWonFlag`, dans
   `core::endEncounter`). Une rencontre engagée par un dialogue n'avait aucune clé (`LOT-118`, D6)
   et rien ne pouvait relier sa victoire à la quête ; le fait compte parmi ceux qu'un dialogue
   pose (`core::flagsWrittenBy`), et la mère en fait sa condition pour nommer la voie de la fin.
3. **Le DD de la Persuasion est le degré « moyenne » (15).** Le format n'écrit qu'un degré nommé
   (`EX-REG-021`), et 18 n'en est pas un ; avec le Brawler (Persuasion −1), c'est une chance sur
   quatre, mesurée par le test — la voie de l'arène reste le chemin attendu, comme la fiche de la
   quête le voulait.
4. **Le combattant est une créature originale du bestiaire** (`combattant-de-l-arene`, CA 14,
   16 pv, +4, 1d6+2, mannequin humanoïde), équilibrée par simulation : sur cent combats à graines
   1 à 100, les deux camps joués par l'IA, le héros l'emporte entre 60 et 70 fois ; sur mille
   graines tirées d'une graine maîtresse, entre 600 et 700. Le test du bestiaire compte désormais
   les 94 profils du SRD et admet les créatures `original`.
5. **« Nouvelle partie » entre dans la démo.** Un plan de la Capitale **provisoire**
   (`World/cities/capital.json`, `status.provisoire`) ouvre Martpart et Arenarea, Martpart en
   départ au point d'arrivée `market-gate` ; `check_rpg_data.py` n'exige un plan complet que d'une
   ville qui ne se dit pas provisoire. Le plan complet, ses dix quartiers fermés et leurs
   sentinelles sont au `LOT-121` (`0.0.3`).
6. **Le hasard se fixe des deux côtés.** `DialogueModel.seed` fixe la graine de la prochaine
   conversation (0 : le compteur du jeu), comme `EncounterModel.seed` celle du combat : les tests
   système forcent l'issue du jet par la graine, pas par un modificateur.
7. **Trois tests, trois niveaux.** `IntegrationTests` joue la chaîne sans fenêtre par
   `hmi::WorldPlay` et `core::DialogueRunner` (trois issues, équilibrage) ; `SystemGameTests`
   (étiquette `systeme`, application Qt sans fenêtre) rejoue ce que les écrans font — « Nouvelle
   partie », les dialogues, la rencontre, l'écran de mort, l'écran de fin — de Market Gate à
   chaque fin, une graine par issue ; les probabilités se mesurent sur des graines tirées, à la
   manière d'un fuzzer.
8. **Le maître d'arène attend sur le sable**, et le condamné marche jusqu'à l'escalier : voir les
   décisions 2 et 3 du `LOT-146`.

## Livraison

La quête, ses quatre dialogues et leurs textes (fr, en), la rencontre et le combattant, le plan de
ville provisoire, les tests d'intégration, système et d'équilibrage. Reste due par l'auteur : la
lecture de la démo à l'écran (« un joueur qui ne connaît pas le projet finit la démo sans aide »),
et le choix des noms des trois PNJ du marché, que les textes désignent par leur rôle.
