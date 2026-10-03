+++
id = "LOT-1011"
titre = "Les fauves de l'arène : le squelette quadruped"
version = "0.0.2.5"
filiere = "pnj"
statut = "livre"
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

## Décisions de réalisation

Livré le 3 octobre 2026 sur la branche `lot-1011-les-fauves-de-l-arene`, **PR #175**.

- **Le squelette `quadruped`** : 29 os, nommés comme ceux de l'humanoïde quand l'os y correspond
  (`clavicle` pour l'omoplate, `hand` et `foot` pour les canons), plus `neck_02`, `forepaw`,
  `hindpaw` et trois os de queue ; tous animés. Écrit dans `scripts/assetsGeneration/rig_quadruped.py`,
  que `rig_character.py` appelle par `--silhouette quadruped` ou d'après le champ `silhouette`
  de la fiche de liaison ; la cuisson des clips, l'écriture du `.glb`, la planche et le relevé
  sont communs aux deux silhouettes (`Silhouette`). Le standard (§5 bis, §6, §7, §9, §10) le dit.
- **Le cap du maillage reçu** : Meshy reproduit la vue de trois quarts, le lion arrive à −46°
  et le loup à −60° de l'axe. L'estimation le mesure (axe principal de l'empreinte, puis la ligne
  des appuis) et la fiche le porte (`heading`, `center_x`, `center_z`) ; le modèle écrit regarde
  vers +Z. Les deux fauves sont aussi pris **à mi-pas** : chaque patte est suivie de son côté,
  les hauteurs des articulations sont mises en commun, et `Body` garde des longueurs par côté
  pour que la pose de repos soit exactement la pose de liaison.
- **La marche est un trot** : les appuis par paires diagonales, posés 0,8 × (hanche–jarret) /
  1,5 m du cycle (lion 37 %, loup 41 %), la foulée centrée pour que le jarret et le carpe passent
  sous la hanche et l'épaule à mi-appui. **Le pas reste à approuver par l'auteur dans le jeu** —
  la fiche prévoyait amble ou trot ; le trot est le choix de ce lot, à 3 m/s c'est l'allure d'un
  fauve.
- **Le recalage au sol entre deux images** : le moteur interpole linéairement, et un roulis de
  84° (la chute) passait sous le sol de 11 mm à mi-chemin de deux images. `bake_clips` évalue
  désormais aussi le maillage à mi-chemin et remonte les deux images voisines ; pour toutes les
  silhouettes (l'humanoïde bouge de 1,2 mm au plus ; les humanoïdes installés restent ceux du
  LOT-1009). Les poses repliées de la chute donnent aux genoux et aux coudes un pôle explicite,
  pour que la résolution à deux os ne bascule pas.
- **Installés** par `LevelEditor --apply Tools/Assets3D/Fiches/{mannequin-quadruped,lion,wolf}.character.json --check`
  (22 personnages contrôlés, 0 erreur) : `Mannequins/quadruped` relié (il était statique), le lion
  et le loup dans `arena-of-fate/Characters/`, `Skeletons/quadruped/skeleton.json`. Les fiches de
  règles `lion` et `wolf` déclarent `"silhouette": "quadruped"`.
- **Les kits sont republiés et verrouillés** (`publish_asset_kit.py`) : `Common@9` (mannequin et
  squelette quadrupèdes) et `arena-of-fate@5` (lion, loup).
- `check_character_model.py` lit la silhouette au nom du squelette du `.glb` et mesure le
  glissement sur les quatre appuis ; `retouch_character.py` tire la table des os animés de la
  silhouette du squelette lu, et ouvre un quadrupède dans Blender sans `--skeleton`.
- **Fin du portrait d'attente** : la liste `portraits` et sa prise en charge sont retirées des
  manifestes, des contrôles, du moteur, de la galerie, de l'atelier et de leurs tests ;
  `check_hd_assets.py` refuse désormais la clé.
- **Le portrait du PNJ dans les dialogues** (demande de l'auteur, 3 octobre 2026) : absent depuis
  le LOT-15, où le portrait de maquette avait été retiré sans être rebranché. Le monde lit à
  l'interaction la figurine que l'entité nomme (`figure`, `silhouette`) et son dossier nommé
  (`resolveFigure(...).named`) ; `WorldModel::interlocutorPortrait`, `DialogueModel.speakerPortrait`,
  `Dialogue.qml` → `portraitSource`.

### Ce que les fauves mesurent

| Modèle | Hauteur | Triangles | `.glb` lié | Hanche–jarret | Épaule–carpe | Appui posé | Glissement | Pénétration |
|---|---|---|---|---|---|---|---|---|
| `lion` | 1,43 m | 100 000 | 9,9 Mio | 0,70 m | 0,68 m | 37 % | 0,05 px | 0,005 mm |
| `wolf` | 1,57 m | 99 999 | 10,0 Mio | 0,76 m | 0,79 m | 41 % | 0,03 px | 0,005 mm |
| `Mannequins/quadruped` | 1,36 m | 100 000 | 11,1 Mio | 0,63 m | 0,63 m | 34 % | 0,02 px | 0,005 mm |

Les trois fiches de liaison sont **estimées** sans correction ; la planche `liaison.png` et les
planches de revue (`render_character_review.py`, cinq clips) ont été relues par Claude, pas
encore par l'auteur. À juger dans le jeu : le trot, la crinière du lion (pesée par bandes entre
`spine_03`, `neck_01`, `neck_02` et `head` : elle cisaille un peu quand la tête tourne), la
pose couchée.

## Risques et questions ouvertes

- Les os d'un quadrupède ne sont pas ceux du `game_engine` de MPFB : leurs noms et leur hiérarchie
  sont une décision de ce lot, à écrire au standard.
- Si les fauves débordent encore, la version se livre avec deux jetons : le LOT-1010 le note au
  bilan, et le lot glisse à la `0.0.3`. **Non survenu.**
