+++
id = "LOT-1011"
titre = "Les fauves de l'arène : le squelette quadruped"
version = "0.0.2.5"
filiere = "pnj"
statut = "a-faire"
taille = "L"
resume = "Le lion et le loup du Colisée marchent sur quatre appuis : un second squelette, ses clips, et la fin des portraits d'attente."
prerequis = ["LOT-1009"]
livrables = [
  "Le squelette **`quadruped`** et ses clips (repos, marche, attaque, touché, mort), sous `Common/Characters/Skeletons/quadruped/`, construits d'après le lion et rejoués par le loup ; `rig_character.py` sait lier une silhouette à quatre pattes (estimation des articulations, poids, poses par cibles, contact des quatre appuis).",
  "Le **lion** et le **loup**, liés et installés par l'atelier des assets (`LevelEditor --apply`) dans `arena-of-fate/Characters/` ; le mannequin `quadruped` lié au même squelette ; leurs fiches de règles nomment leur silhouette.",
  "Le [standard des personnages](../../../../standards/personnages-3d.md) complété de ce que les fauves mesurent : l'image de référence de trois quarts, les articulations d'un quadrupède, la marche à quatre appuis.",
  "La **fin du portrait d'attente** : la liste `portraits` quitte les manifestes `Characters/`, et sa prise en charge quitte `check_hd_assets.py`, `core::resolveFigures`, `hmi::AssetGallery` et `check_orphans.py`.",
  "Les kits republiés et verrouillés.",
]
criteres = [
  "Le lion et le loup marchent sur quatre appuis sans glissement ni traversée, jouent leurs cinq clips, et se reconnaissent à côté de leur jeton (jugement de l'auteur).",
  "`git grep -n \"portraits\" Source scripts` ne trouve plus la liste d'attente ; `check_hd_assets.py` refuse un personnage cité sans modèle.",
  "La rencontre `colisee-fauves` se joue avec les deux modèles.",
]
+++

## Pourquoi

Le [LOT-1009](LOT-1009-les-quatre-heros.md) a installé les dix-huit humanoïdes de la démo ; les
deux fauves restent des portraits d'attente, dessinés par le mannequin humanoïde, parce que le
squelette `quadruped` n'existe pas : ni description, ni clips, ni chaîne de liaison pour quatre
pattes. La fiche du LOT-1009 le prévoyait — « s'il déborde, il devient un lot à part » — et il a
débordé. Ce lot ouvre la seconde silhouette du standard et retire le dernier repli.

## Périmètre

Dedans : le squelette, ses clips, la liaison d'un quadrupède, les deux fauves, le mannequin
quadrupède, le retrait du portrait d'attente.

Dehors, nommément : la silhouette `flying`, qui viendra avec ses créatures (D-34) ; toute règle de
créature.

## À supprimer

| Quoi | Où | Pourquoi maintenant |
|---|---|---|
| Le **portrait d'attente** : un personnage sans modèle, porté par la liste `portraits` | les manifestes `Characters/` (lion, loup) ; `check_hd_assets.py`, `check_orphans.py`, `core::resolveFigures`, `hmi::AssetGallery`, et leurs tests | les deux derniers cas reçoivent leur modèle ; le concept (`LOT-145`) n'a plus de cas |
| Le mannequin `quadruped` statique, sans fiche | `Common/Characters/Mannequins/quadruped/` : **remplacé** par le mannequin lié | il attendait son squelette depuis le LOT-1006 |

## Conception

- **Le lion fixe, le loup prouve.** Les clips se posent par cibles sur le lion ; le loup, lié par
  sa seule fiche, doit les rejouer sans retouche : c'est ce qui dit que le squelette est commun.
- **La marche à quatre appuis** garde la règle du moteur : une case de 1,5 m en 0,5 s ; les
  appuis posés reculent à 3 m/s. Le pas est à écrire — amble ou trot — et à faire approuver.
- **L'image de référence** d'un fauve est de trois quarts, sur quatre pattes séparées
  (`personnages-3d.md`, §5) ; les deux existent déjà dans l'atelier local, avec leurs maillages
  Meshy réduits (`Tools/Assets3D/Standard/Personnages/{lion,wolf}/`).
- **Le réglage dans Blender** vaut pour un quadrupède comme pour un humanoïde (D-44,
  `retouch_character.py`) : la table des os animés vient de `skeleton.json`.

## Risques et questions ouvertes

- Les os d'un quadrupède ne sont pas ceux du `game_engine` de MPFB : leurs noms et leur hiérarchie
  sont une décision de ce lot, à écrire au standard.
- Si les fauves débordent encore, la version se livre avec deux jetons : le LOT-1010 le note au
  bilan, et le lot glisse à la `0.0.3`.
