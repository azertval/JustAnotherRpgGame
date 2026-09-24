# LOT-108 — Les assets livrés sortent de l'historique Git

Tâche de **fin de lot** du [LOT-108](../versions/v0.1.0/v0.0.1-demo/lots/LOT-108-assets-hd-arenarea.md),
demandée par l'auteur le 24 septembre 2026. Le LOT-108 est le premier lot qui fait vraiment grossir
le volume d'images (102 postes pour un seul quartier) ; c'est donc à lui de changer la manière de
les stocker, avant que les zones suivantes ne répètent le problème.

## Le constat

- Le pack Git pèse **1,06 Gio** pour 76 Mio d'assets présents dans l'arbre : chaque version
  intermédiaire d'une image reste dans l'historique pour toujours, et chaque clone la télécharge.
- Un kit livré ne bouge plus. Le versionner fichier par fichier ne sert à rien : ce qui compte,
  c'est de savoir **quel kit** une version du jeu attend, et de le retrouver à l'octet près.
- Volumes au 24 septembre : `Regions/` 44 Mio (742 fichiers), `Maps/` 16 Mio (17), `UI/` 12 Mio
  (215), `Common/` 4,3 Mio (48). Treize régions et une centaine de zones restent à produire.

## La décision

**Les images d'un kit livré sont publiées en archive immuable, hors de l'historique ; Git ne garde
que leurs manifestes et un fichier de verrouillage. L'installation les télécharge d'elle-même.**

| Reste suivi par Git | Sort de Git (archives) |
|---|---|
| les manifestes : `manifest.json`, `appearance.json` de chaque `Scene/` et `Characters/`, `Maps/manifest.json`, `UI/illustrations.json` | les images (`*.png`, `*.jpg`) de `Regions/`, `Common/`, `Maps/` et `UI/` (le HUD) |
| `Fonts/`, `Entities/`, `CREDITS.md`, les `README.md` | — |
| le verrou `Source/Elements/Assets/kits.lock.json` | — |

Les manifestes restent lisibles et diffables dans une PR ; ils portent déjà l'empreinte de chaque
image (`check_map_assets.py`, `check_ui_assets.py`, `check_hd_assets.py`). Le verrou y ajoute
l'empreinte de chaque archive.

Écartés :

- **Git LFS** : garde chaque version (le problème reste entier) et son quota de bande passante
  serait consommé par la CI, qui tire tous les assets à chaque run ;
- **un sous-module d'assets** : l'historique grossit tout autant, ailleurs ;
- **un stockage cloud tiers** (DVC, rclone, bucket) : un service et des identifiants de plus, pour
  rien de mieux que les releases GitHub, déjà utilisées par la CI (tag roulant).

**On ne réécrit pas l'historique.** Le 1,06 Gio déjà présent y reste : le réduire demanderait
`git filter-repo` et un force-push, exclus. La tâche arrête la croissance, elle ne rembourse pas
la dette.

## Le format

### Le kit

Un kit est **un dossier d'assets publié d'un seul tenant**, à la granularité de l'arborescence :

- un dossier de lieu (`Regions/central-empire/capital/arenarea/`, `…/capital/Common/`,
  `Regions/central-empire/Common/`, `Common/`) — un kit par niveau, sans ses sous-lieux ;
- `Maps/` — un kit ;
- `UI/` — un kit.

Identifiant : son chemin sous `Assets/`, suivi d'un numéro de publication :
`Regions/central-empire/capital/arenarea@1`, `UI@1`, `Maps@1`.

**Une retouche est un nouveau numéro, jamais une archive remplacée.** Une archive publiée n'est
plus modifiée ni supprimée tant qu'un commit de `main` la cite.

### L'archive

- `.zip` **déterministe** : fichiers triés, dates fixées, sans attributs de poste ; deux
  publications du même contenu donnent le même SHA-256.
- Ne contient que les images du kit, à leurs chemins relatifs au dossier du kit.
- Publiée comme fichier attaché à une **GitHub Release** dont le tag porte la région :
  `assets/central-empire`, `assets/ui`, `assets/maps`. Une release admet 1 000 fichiers attachés ;
  une release par région tient le volume prévu. Pas de quota de bande passante, 2 Gio par fichier.

### Le verrou `kits.lock.json`

Une entrée par kit attendu par le commit courant :

```json
{
  "version": 1,
  "kits": [
    {
      "id": "Regions/central-empire/capital/arenarea@1",
      "path": "Regions/central-empire/capital/arenarea",
      "release": "assets/central-empire",
      "asset": "arenarea-1.zip",
      "sha256": "…",
      "bytes": 0,
      "files": 0
    }
  ]
}
```

C'est le seul fichier qui change quand un kit est publié ou retouché.

## Le travail

Ordre imposé : **on ne retire rien du suivi avant d'avoir prouvé qu'un clone neuf reconstruit
l'arbre à l'identique.**

### T1 — Publier

`scripts/release/publish_asset_kit.py <chemin>` :

1. contrôle le kit (mêmes règles que `check_hd_assets.py`, `check_map_assets.py` et
   `check_ui_assets.py` selon le dossier) ;
2. construit l'archive déterministe, calcule SHA-256, taille et nombre de fichiers ;
3. refuse si l'identifiant existe déjà avec une autre empreinte (immuabilité) ;
4. téléverse par `gh release upload` (release créée si absente, jamais en `--clobber`) ;
5. met à jour `kits.lock.json`.

Les sources de production (`Tools/`) ne sont **jamais** archivées : l'archive part des pièces
installées sous `Source/Elements/Assets/`.

### T2 — Installer

`scripts/fetch_assets.py` (point d'entrée du poste, à la racine de `scripts/`) :

- lit le verrou, télécharge chaque archive manquante dans un cache hors de l'arbre
  (`%LOCALAPPDATA%\JadgAssets\<sha256>.zip`, surchargeable par `JADG_ASSETS_CACHE`), vérifie
  l'empreinte, extrait dans `Source/Elements/Assets/<path>/` ;
- idempotent : un kit déjà en place et intact n'est ni retéléchargé ni réécrit (un témoin par kit,
  hors de l'arbre suivi) ;
- **ne détruit pas le travail local** : une image modifiée ou ajoutée à la main dans un kit
  verrouillé est signalée, et l'extraction s'arrête sans `--force` ;
- dépôt privé : authentification par `gh auth token` sur le poste, `GITHUB_TOKEN` en CI ; message
  clair si aucune n'est disponible ;
- `--check` : vérifie sans télécharger (arbre conforme au verrou, sinon code non nul).

Branchements :

- `scripts/setup_dev.ps1` l'appelle après l'installation des outils ;
- `scripts/build.ps1` l'appelle **avant la configuration CMake** : `Source/Ui/CMakeLists.txt`
  globe `UI/*/*.png` et `Maps/*.jpg` à la configuration, un arbre incomplet donnerait un binaire
  sans HUD sans erreur visible ;
- CMake, à la configuration, vérifie la présence des kits (témoins) et échoue avec la commande à
  lancer, plutôt que de télécharger en silence.

### T3 — La CI

- Un pas `fetch_assets.py` avant la configuration, dans chaque job qui lit les assets (build,
  tests, QmlTests et références PNG, galerie `EX-CNT-042`, `check_hd_assets.py`,
  `check_map_assets.py`, `check_ui_assets.py`).
- `actions/cache` sur le répertoire de cache, clé = `hashFiles('Source/Elements/Assets/kits.lock.json')`.
- `check_binary_files.py` : ajouter la règle **aucune image suivie sous un dossier couvert par le
  verrou** (le garde-fou qui empêche le retour en arrière, en pre-commit comme en CI).
- `fetch_assets.py --check` en CI : le verrou cite des archives qui existent et dont l'empreinte
  correspond.

### T4 — Publier l'existant et prouver

1. Publier `Common@1`, `Regions/central-empire/Common@1`, `…/capital/Common@1` (kit V4 du
   LOT-105), `…/capital/arena-of-fate@1` (si le LOT-106 est livré, sinon à sa livraison),
   `…/capital/arenarea@1` (ce lot), `Maps@1`, `UI@1`.
2. **Preuve** : clone neuf dans un dossier temporaire, `setup_dev.ps1` puis `build.ps1` ; chaque
   image extraite a le même SHA-256 que le fichier suivi à `HEAD` (script de comparaison,
   résultat joint à la PR) ; la galerie et la scène d'Arenarea se rendent comme avant.

### T5 — Nettoyer le suivi

Après la preuve seulement, dans **un commit dédié** :

1. `git rm -r --cached` des images des dossiers verrouillés (les fichiers restent sur le disque) ;
2. règles `.gitignore` correspondantes, **les manifestes JSON et `README.md` exceptés** ;
3. `git ls-files` ne cite plus aucun `*.png` ni `*.jpg` sous `Regions/`, `Common/`, `Maps/`, `UI/`.

Précautions :

- **Chez les autres clones, le `git pull` de ce commit supprime les images de l'arbre de travail** :
  lancer `fetch_assets.py` juste après. Le dire dans la PR et dans le `CHANGELOG.md`.
- Une branche en cours qui modifie une de ces images entre en conflit modification/suppression.
  Faire la tâche **après la fusion** des branches d'assets ouvertes (LOT-106, LOT-110, travaux des
  agents de la [production du LOT-108](production-lot108.md)) ; la publication de leurs kits suit
  alors le nouveau circuit.
- Les worktrees sous `.claude/worktrees/` partagent le cache (`%LOCALAPPDATA%`) : un `fetch` par
  worktree ne retélécharge rien.

### T6 — La documentation

- [`arborescence-assets.md`](arborescence-assets.md) : section « Le stockage » (ce qui est suivi,
  ce qui est archivé, le verrou) ; la section « Le poids » y renvoie.
- [`definition-de-livre.md`](definition-de-livre.md) : un lot d'assets est livré quand ses kits sont
  **publiés et verrouillés**, pas seulement installés.
- [`gabarit-commande-zone.md`](gabarit-commande-zone.md) : dernière étape d'une zone = publication
  du kit.
- `Source/Elements/Assets/README.md` : comment obtenir les assets après un clone, comment publier.
- `CHANGELOG.md` sous « Non publié ».

## Critères

- Un clone neuf, suivi de `setup_dev.ps1` puis `build.ps1`, lance le jeu, la galerie et la scène
  d'Arenarea à l'identique, sans étape manuelle autre que l'authentification `gh` déjà requise.
- La CI est verte avec les assets tirés du cache, et le deuxième run ne retélécharge rien.
- Aucune image de `Regions/`, `Common/`, `Maps/` ou `UI/` n'est suivie ; `check_binary_files.py`
  refuse d'en réintroduire une.
- Chaque kit publié a une empreinte dans le verrou ; republier le même identifiant avec un autre
  contenu est refusé.
- L'historique n'est pas réécrit.
