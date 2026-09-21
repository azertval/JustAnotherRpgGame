# Gabarit d'une fiche de lot

Une fiche de lot est un fichier `LOT-NNN-objet-en-minuscules.md`, rangé dans le dossier `lots/` de
la version qu'il sert. Elle commence par un **en-tête TOML** encadré de deux lignes `+++` — c'est lui
que lisent le lint et le site — et se poursuit en Markdown libre.

## L'en-tête

```
+++
id = "LOT-104"
titre = "Assets HD de l'Arena of Fate"
version = "0.0.1"
filiere = "assets"
statut = "a-faire"
taille = "L"
resume = "Une phrase : ce que le lot rend possible, pas ce qu'il fabrique."
prerequis = ["LOT-101", "LOT-102"]
reprend = ["LOT-92"]
livrables = [
  "Ce qui existe dans le dépôt quand le lot est livré — un fichier, un dossier, un écran.",
]
criteres = [
  "Ce qu'on vérifie pour dire « livré » — observable, sans interprétation.",
]
sources = ["Tanares Sourcebook, p. 91-93"]
maquettes = ["../maquettes/arena-of-fate-plan.svg"]
+++
```

| Champ | Obligatoire | Valeurs |
|---|---|---|
| `id` | oui | `LOT-NNN`, trois chiffres, jamais réattribué |
| `titre` | oui | l'objet du lot, en une ligne |
| `version` | oui | un identifiant de [`versions.toml`](../versions/versions.toml) |
| `filiere` | oui | `standard`, `assets`, `cartes`, `pnj`, `quete`, `moteur`, `regles`, `donnees`, `interface`, `editeur`, `version` |
| `statut` | oui | `a-faire`, `en-cours`, `livre`, `abandonne` |
| `taille` | oui | `S` (une séance), `M` (deux à trois), `L` (une semaine), `XL` (à redécouper si possible) |
| `resume` | oui | une phrase |
| `prerequis` | non | lots qui doivent être **livrés** avant de démarrer |
| `reprend` | non | lots de l'ancienne feuille de route que celui-ci absorbe ou remplace |
| `livrables`, `criteres` | oui | au moins un de chaque |
| `sources` | non | pages du corpus, en pages **de livre** |
| `maquettes` | non | chemins relatifs à la fiche |

Les états **prêt**, **prochain** et **en attente** ne s'écrivent pas : ils se **calculent** depuis
les statuts et les prérequis. Écrire `statut = "livre"` est le seul geste qui fait avancer le site.

## Le corps

Quatre rubriques, dans cet ordre, et seulement celles qui ont quelque chose à dire :

1. **Pourquoi** — ce que le lot débloque, ce qui se passerait sans lui.
2. **Périmètre** — ce qui est dedans, et, nommément, ce qui n'y est **pas**.
3. **Conception** — décisions déjà prises, inventaires (liste des pièces, des PNJ, des cases),
   maquettes.
4. **Risques et questions ouvertes** — ce qui peut faire grossir le lot.

## Cycle de vie

1. Un lot naît dans la fiche de sa version, `a-faire`.
2. Au démarrage : `en-cours`. Le lint refuse ce statut tant qu'un prérequis n'est pas livré.
3. À la livraison : `livre`, avec le numéro de PR ajouté en fin de fiche. La fiche **reste** :
   les décisions prises en route s'y écrivent, sous une rubrique « Décisions de réalisation ».
4. Un lot qui n'a plus lieu d'être passe `abandonne`, avec la raison. Son numéro n'est pas repris.
