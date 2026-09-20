+++
id = "LOT-130"
titre = "Les quatre fiches préfabriquées, en données"
version = "0.0.2"
filiere = "regles"
statut = "a-faire"
taille = "M"
resume = "Brawler, Mage, Priest et Scoundrel de niveau 1 existent comme fiches du jeu, valeur pour valeur, avec les espèces et les historiques qu'elles demandent."
prerequis = ["LOT-122"]
reprend = ["LOT-36 (classes provisoires)"]
livrables = [
  "`Rpg/characters/` : les quatre fiches (p. 195, 199, 203, 207).",
  "Ce qui leur manque dans les catalogues : les espèces **Half-Orc**, **Autumn Elf**, **Hill Dwarf** ; les historiques **Dragon Hunter**, **Cartographer**, **Community Leader**, **Undercover**.",
  "Un test par fiche qui recalcule chaque valeur dérivée (CA, PV, initiative, sauvegardes, compétences, attaques) et la compare à la page.",
  "Le registre des **coquilles du livre** et de la valeur retenue pour chacune.",
]
criteres = [
  "Les quatre tests passent ; chaque écart entre la règle et la fiche imprimée est une décision écrite.",
  "`check_rpg_data.py` est vert.",
]
sources = ["Player's Guide to Tanares, p. 195, 199, 203, 207"]
+++

## Ce que les fiches disent — et ne disent pas

Les quatre fiches sont de **niveau 1**. Elles portent caractéristiques, compétences, sauvegardes,
PV, CA, attaques et maîtrises ; elles **ne listent ni capacités, ni sorts, ni équipement** : les
cadres « Power Symbols » et « Heroic Mark » sont vides. Le reste se déduit des règles de la classe
— voir [le référentiel des classes de base](../../../../referentiels/regles/classes-simplifiees.md).

## Coquilles à trancher

- Scoundrel : vitesse imprimée 30 ft, la règle donne **40** (*Scoundrel's Agility*) ; Perception
  passive imprimée 13, le calcul donne **14**.
- Priest : la hachette est imprimée « Piercing » ; le type normal est **tranchant**.
- Mage : *fire bolt* manque aux attaques imprimées alors que la classe le donne.
- Brawler : le texte cite une capacité *Resilient* absente de la table — la table fait foi.

Proposé : **la règle prime sur la fiche imprimée**, et chaque écart est écrit dans le registre.
