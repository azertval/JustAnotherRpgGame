#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Installe les assets HD d'un lot de sources : du brut du générateur à l'asset du dépôt (LOT-104).

Les sorties du générateur vivent dans `Tools/AssetsHD/`, hors du dépôt. À côté d'elles, un
**descripteur** `install.json` dit, pour chaque source, ce qu'elle devient : son nom, sa famille,
son emprise, son type tactique. La commande en tire l'asset installé, et rien d'autre ne se fait à
la main :

1. **Détourer.** L'alpha du générateur est continu mais sale : un voile d'alpha 1 à 16 autour de la
   pièce, un intérieur à 252-254 au lieu de 255, quelques points perdus. Le voile tombe à 0,
   l'intérieur monte à 255, le bord garde sa pente ; les îlots de moins de `ILOT_MIN` du total
   disparaissent.
2. **Découper.** Une source peut être une **planche** (les sols du Colisée en portent six) : chaque
   morceau d'un seul tenant est une pièce, dans l'ordre de lecture, et le descripteur les nomme
   toutes — un compte qui ne tombe pas juste est une erreur, jamais une affectation au hasard.
3. **Réduire.** À l'échelle du standard (`Planning/standards/style-2d-hd.md`), en alpha prémultiplié
   pour que le bord ne tire pas vers le noir du fond transparent. L'art est **toujours réduit,
   jamais agrandi** : une source trop petite est refusée.
4. **Ancrer.** Une dalle de sol devient exactement le losange du lieu. Une pièce debout se mesure à
   ses **deux pointes** : le point le plus à gauche de l'art et le plus bas de cette colonne sont la
   pointe ouest de son emprise, de même à droite pour la pointe est. De l'écart entre les deux se
   déduisent l'étendue du socle le long des deux axes de la grille, donc l'échelle (le socle remplit
   son emprise sur l'axe où il est le plus court par rapport à elle), puis l'ancre — le sommet nord
   de l'emprise, la convention du moteur (`core::ScenePiece`). Une pièce qui ne touche pas ses
   pointes (un lampadaire, une statue au bras tendu) se corrige dans le descripteur : `scale`,
   `anchorOffset`.
5. **Inscrire.** L'image va dans le dossier `Scene/` cible, son entrée dans le `manifest.json` de ce
   dossier ; les autres entrées ne bougent pas.

Le contrôle de ce qui est installé (fichiers cités, dimensions, poids par zone) est celui de la CI,
`scripts/check_hd_assets.py`, qui n'a pas besoin des sources.

Le descripteur (`install.json`) :

    {
      "version": 1,
      "target": "Regions/central-empire/capital/arenarea/arena-of-fate/Scene",
      "pieces": [
        {"source": "Murs/mur-U.png", "name": "wall-arcade-u", "family": "02",
         "footprint": [3, 1]},
        {"source": "Sols/source-001.png", "family": "01",
         "sheet": ["floor-sand-01", "floor-sand-02", "floor-paving-01", "floor-paving-02",
                   "floor-sand-edge-nw-01", "floor-sand-edge-ne-01"]}
      ]
    }

## Les figurines (LOT-112)

Un descripteur dont la cible est un dossier `Characters/` installe des **figurines** au lieu de
pièces. Chaque source est une **bande d'animation** du générateur : N images côte à côte, sur fond
transparent (la variante « planche d'animation » de la consigne).

1. **Détourer**, comme une pièce : voile à 0, intérieur à 255, îlots effacés.
2. **Découper** en exactement N images : par les colonnes vides qui les séparent ; si leur compte
   ne tombe pas juste (deux images qui se touchent, une lance qui dépasse), l'étendue de l'art se
   partage en N parts égales, et une part vide est une erreur. Les centres des images se
   régularisent sur un pas constant : c'est la grille que le générateur a dessinée, et ce qu'une
   image s'en écarte (une fente, un recul) est du mouvement, à garder.
3. **Réduire** toute la bande d'**une seule** échelle : celle qui donne à l'image de repos
   (`standingFrame`, la première par défaut) la hauteur du standard, 170 px — ou `scale`.
4. **Poser** chaque image dans sa cellule (192 × 256, 384 × 256 si `wide` ; `frame`, `wideFrame`
   et `ground` du manifeste s'il les déclare) : un seul décalage pour toute la bande, calculé sur
   l'image de repos — sa ligne visible la plus basse sur le sol (y = 252), le milieu de sa boîte au
   milieu de la cellule. Une image qui ne laisse pas 4 px transparents à gauche et à droite de sa
   cellule est refusée : les cellules sont jointives dans la bande (le moteur l'exige), la marge de
   8 px entre deux images du standard est donc à l'intérieur. En haut et en bas, l'image n'a qu'à
   tenir : aucune voisine n'y est, et le sol est à 4 px du bord.
5. **Inscrire** : `<nom>/<clip>-<orientation>.png` (ou `<clip>.png` sans orientation) et son
   `.anim.json`, au format du moteur (`hmi::AnimationCatalog`) ; le portrait (512 × 512) et le
   jeton (128 × 128, détouré en rond) si une source de portrait est donnée ; le nom dans `npcs` du
   manifeste, les sources et leur empreinte dans son objet `sources`.

    {
      "version": 1,
      "target": "Common/Characters",
      "figures": [
        {"name": "Heroes/brawler", "portrait": "brawler/portrait.png",
         "strips": [
           {"source": "brawler/walk-se.png", "clip": "walk", "facing": "se", "frames": 8,
            "frameDuration": 0.1, "loop": true},
           {"source": "brawler/attack-se.png", "clip": "attack", "facing": "se", "frames": 6,
            "wide": true, "loop": false, "standingFrame": 0}
         ]}
      ]
    }

Usage :
    python scripts/install_hd_asset.py Tools/AssetsHD/Colisee/install.json
    python scripts/install_hd_asset.py DESCRIPTEUR --piece wall-arcade-u   # une seule pièce
    python scripts/install_hd_asset.py DESCRIPTEUR --piece Heroes/brawler  # une seule figurine
    python scripts/install_hd_asset.py DESCRIPTEUR --check   # l'installé est-il à jour des sources ?
    python scripts/install_hd_asset.py DESCRIPTEUR --measure # mesures seules, rien n'est écrit
                                                             # (figurines : image par image, boîte,
                                                             # ligne basse, appuis au sol)

Dépendances : Pillow et numpy (outil de production, pas de CI : les sources n'y sont pas).
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import re
import sys
from collections import deque
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "Source" / "Elements" / "Assets"
SOURCES = ROOT / "Tools" / "AssetsHD"

DESCRIPTOR_VERSION = 1
MANIFEST_VERSION = 1

# --- Le détourage --------------------------------------------------------------------------------
# Sous ALPHA_BAS, le voile du générateur ; au-dessus d'ALPHA_HAUT, l'intérieur de la pièce. Entre
# les deux, la pente du bord, étirée sur 0-255.
ALPHA_BAS = 16
ALPHA_HAUT = 248
# Un morceau d'un seul tenant plus petit que cette part de l'art est un point perdu, pas une pièce.
ILOT_MIN = 0.005
# Le pas de la grille d'étiquetage : un morceau se reconnaît à cette finesse, un pont de moins de
# ETIQUETAGE px entre deux pièces les soude (les dalles d'une planche sont séparées de 30 px et plus).
ETIQUETAGE = 4
# La pente du bord bas d'un socle, pour trouver ses pointes (voir `measure_tips`) : celle du losange
# (0,62), un peu raidie. Et l'écart, en px de source, sous lequel deux points de l'enveloppe se valent.
PENTE_POINTE = 0.75
TOLERANCE_POINTE = 3.0
# La marge transparente laissée autour d'une pièce debout, en px installés : le filtrage bilinéaire
# lit un pixel au-delà du bord.
MARGE = 2

# Les dix familles du standard (§4). La famille 01 est un sol : elle devient un losange exact.
FAMILLES = {f"{n:02d}" for n in range(1, 11)}
FAMILLE_SOL = "01"
TACTIQUES = {"open", "difficult", "cover", "obstacle", "solid"}
ALIGNEMENTS = {"centre", "north"}
# Les champs d'une pièce du descripteur : une faute de frappe (`foorprint`) ne passe pas en silence.
CHAMPS = {"source", "name", "sheet", "family", "footprint", "class", "tactical", "scale",
          "anchorOffset", "align"}
# Le nom d'une pièce (arborescence, règle 4) : minuscules, chiffres, tirets.
NOM = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")

# --- Les figurines (standard 2D HD, §2 et §5) -----------------------------------------------------
# La cellule d'une image, la cellule large (attaque, sort), la ligne de sol et la hauteur d'une
# figurine humanoïde, en px installés. Le manifeste de la cible peut déclarer les trois premières.
CELLULE = (192, 256)
CELLULE_LARGE = (384, 256)
SOL = 252
HAUTEUR_FIGURINE = 170
# La marge transparente de chaque côté d'une cellule : la moitié des 8 px entre deux images.
MARGE_CELLULE = 4
# Un pixel « visible » pour les mesures d'une figurine (sol, boîte, appuis).
OPAQUE = 128
# Les rangées au-dessus de la plus basse où un pixel compte encore comme un appui au sol.
APPUI = 3
PORTRAIT = 512
JETON = 128
ORIENTATIONS = {"se", "sw", "ne", "nw"}
CHAMPS_FIGURE = {"name", "strips", "portrait", "token"}
CHAMPS_BANDE = {"source", "clip", "facing", "frames", "frameDuration", "loop", "wide", "standingFrame", "scale"}
# Un dossier de figurine : des dossiers de rangement (`Heroes`), puis le nom de la figurine.
DOSSIER = re.compile(r"^[A-Za-z0-9]+(-[a-z0-9]+)*$")
CLIP = re.compile(r"^[a-z]+$")


class DescriptorError(ValueError):
    """Un descripteur qui ne dit pas assez, ou pas juste, ce que devient une source."""


# --- Le descripteur ------------------------------------------------------------------------------


@dataclass
class PieceSpec:
    """Ce qu'une source devient, lu dans le descripteur."""

    source: str
    names: list[str]
    family: str
    footprint: tuple[int, int] = (1, 1)
    piece_class: str | None = None
    tactical: str | None = None
    scale: float | None = None
    anchor_offset: tuple[int, int] = (0, 0)
    align: str = "centre"

    @property
    def is_floor(self) -> bool:
        return self.family == FAMILLE_SOL


@dataclass
class StripSpec:
    """Une bande d'animation d'une figurine, lue dans le descripteur."""

    source: str
    clip: str
    frames: int
    facing: str | None = None
    frame_duration: float = 0.1
    loop: bool = True
    wide: bool = False
    standing_frame: int = 0
    scale: float | None = None

    @property
    def stem(self) -> str:
        """Le nom du fichier installé, sans extension : `walk-se`, ou `walk` sans orientation."""
        return self.clip if self.facing is None else f"{self.clip}-{self.facing}"


@dataclass
class FigureSpec:
    """Une figurine : ses bandes, et la source de son portrait (et de son jeton)."""

    name: str
    strips: list[StripSpec]
    portrait: str | None = None
    token: str | None = None


@dataclass
class Descriptor:
    path: Path
    target: str
    pieces: list[PieceSpec] = field(default_factory=list)
    figures: list[FigureSpec] = field(default_factory=list)
    # Un essai (la cadence du LOT-112) se mesure et s'aperçoit, il ne s'installe jamais : ses
    # figurines n'ont rien à faire dans le dépôt.
    preview_only: bool = False

    @property
    def source_dir(self) -> Path:
        return self.path.parent

    @property
    def target_dir(self) -> Path:
        return ASSETS / self.target

    @property
    def is_characters(self) -> bool:
        return self.target.endswith("/Characters") or self.target == "Characters"


def _pair(value, what: str) -> tuple[int, int]:
    if (not isinstance(value, list) or len(value) != 2
            or not all(isinstance(v, int) and not isinstance(v, bool) for v in value)):
        raise DescriptorError(f"{what} : deux entiers attendus, lu {value!r}")
    return value[0], value[1]


def read_descriptor(path: Path) -> Descriptor:
    """Lit et valide un `install.json` ; toute faute est nommée, pièce par pièce."""
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise DescriptorError(f"{path} : illisible ({error})") from error
    if data.get("version") != DESCRIPTOR_VERSION:
        raise DescriptorError(f"{path} : version {data.get('version')!r}, attendu {DESCRIPTOR_VERSION}")
    target = data.get("target")
    if not isinstance(target, str) or ".." in target:
        raise DescriptorError(f"{path} : `target` doit nommer un dossier Scene/ ou Characters/ sous les assets")
    preview_only = data.get("previewOnly", False)
    if not isinstance(preview_only, bool):
        raise DescriptorError(f"{path} : `previewOnly` est un booléen")
    descriptor = Descriptor(path=path, target=target, preview_only=preview_only)
    if descriptor.is_characters:
        if "pieces" in data:
            raise DescriptorError(f"{path} : un dossier Characters/ reçoit des `figures`, pas des `pieces`")
        descriptor.figures = read_figures(path, data.get("figures", []))
        return descriptor
    if not target.endswith("/Scene"):
        raise DescriptorError(f"{path} : `target` doit nommer un dossier Scene/ ou Characters/ sous les assets")
    if "figures" in data:
        raise DescriptorError(f"{path} : un dossier Scene/ reçoit des `pieces`, pas des `figures`")
    seen: set[str] = set()
    for index, raw in enumerate(data.get("pieces", [])):
        where = f"{path.name}, pièce {index + 1}"
        source = raw.get("source")
        if not isinstance(source, str):
            raise DescriptorError(f"{where} : `source` manquante")
        family = raw.get("family")
        if family not in FAMILLES:
            raise DescriptorError(f"{where} : famille {family!r} hors des dix du standard")
        if "sheet" in raw:
            names = raw["sheet"]
            if not isinstance(names, list) or not names or "name" in raw:
                raise DescriptorError(f"{where} : `sheet` est une liste de noms, sans `name`")
        else:
            names = [raw.get("name")]
        for name in names:
            if not isinstance(name, str) or not NOM.match(name):
                raise DescriptorError(f"{where} : nom {name!r} (minuscules, chiffres, tirets)")
            if name in seen:
                raise DescriptorError(f"{where} : `{name}` nommé deux fois")
            seen.add(name)
        spec = PieceSpec(source=source, names=names, family=family)
        if "footprint" in raw:
            spec.footprint = _pair(raw["footprint"], f"{where}, footprint")
            if min(spec.footprint) < 1:
                raise DescriptorError(f"{where} : une emprise fait au moins 1 × 1")
        if spec.is_floor and spec.footprint != (1, 1):
            raise DescriptorError(f"{where} : un sol tient une case")
        if "class" in raw:
            spec.piece_class = raw["class"]
        tactical = raw.get("tactical")
        if tactical is not None and tactical not in TACTIQUES:
            raise DescriptorError(f"{where} : type tactique {tactical!r} inconnu")
        spec.tactical = tactical
        if "scale" in raw:
            scale = raw["scale"]
            if not isinstance(scale, (int, float)) or not 0 < scale <= 1:
                raise DescriptorError(f"{where} : `scale` dans ]0, 1] (l'art n'est jamais agrandi)")
            spec.scale = float(scale)
        if "anchorOffset" in raw:
            spec.anchor_offset = _pair(raw["anchorOffset"], f"{where}, anchorOffset")
        if "align" in raw:
            if raw["align"] not in ALIGNEMENTS:
                raise DescriptorError(f"{where} : `align` vaut {' ou '.join(sorted(ALIGNEMENTS))}")
            spec.align = raw["align"]
        unknown = set(raw) - CHAMPS
        if unknown:
            raise DescriptorError(f"{where} : champ(s) inconnu(s) {sorted(unknown)}")
        descriptor.pieces.append(spec)
    if not descriptor.pieces:
        raise DescriptorError(f"{path} : aucune pièce")
    return descriptor


def _positive_int(value) -> bool:
    return isinstance(value, int) and not isinstance(value, bool) and value > 0


def read_strip(raw, where: str) -> StripSpec:
    """Une bande du descripteur ; toute faute est nommée."""
    if not isinstance(raw, dict):
        raise DescriptorError(f"{where} : une bande est un objet")
    unknown = set(raw) - CHAMPS_BANDE
    if unknown:
        raise DescriptorError(f"{where} : champ(s) inconnu(s) {sorted(unknown)}")
    source, clip, frames = raw.get("source"), raw.get("clip"), raw.get("frames")
    if not isinstance(source, str):
        raise DescriptorError(f"{where} : `source` manquante")
    if not isinstance(clip, str) or not CLIP.match(clip):
        raise DescriptorError(f"{where} : `clip` {clip!r} (minuscules : idle, walk, attack…)")
    if not _positive_int(frames):
        raise DescriptorError(f"{where} : `frames`, le nombre d'images, est un entier positif")
    spec = StripSpec(source=source, clip=clip, frames=frames)
    if "facing" in raw:
        if raw["facing"] not in ORIENTATIONS:
            raise DescriptorError(f"{where} : `facing` vaut {', '.join(sorted(ORIENTATIONS))}")
        spec.facing = raw["facing"]
    if "frameDuration" in raw:
        duration = raw["frameDuration"]
        if isinstance(duration, bool) or not isinstance(duration, (int, float)) or duration <= 0:
            raise DescriptorError(f"{where} : `frameDuration` en secondes, positive")
        spec.frame_duration = float(duration)
    for key, attribute in (("loop", "loop"), ("wide", "wide")):
        if key in raw:
            if not isinstance(raw[key], bool):
                raise DescriptorError(f"{where} : `{key}` vaut true ou false")
            setattr(spec, attribute, raw[key])
    if "standingFrame" in raw:
        standing = raw["standingFrame"]
        if isinstance(standing, bool) or not isinstance(standing, int) or not 0 <= standing < frames:
            raise DescriptorError(f"{where} : `standingFrame` est le rang d'une image, de 0 à {frames - 1}")
        spec.standing_frame = standing
    if "scale" in raw:
        scale = raw["scale"]
        if isinstance(scale, bool) or not isinstance(scale, (int, float)) or not 0 < scale <= 1:
            raise DescriptorError(f"{where} : `scale` dans ]0, 1] (l'art n'est jamais agrandi)")
        spec.scale = float(scale)
    return spec


def read_figures(path: Path, raws) -> list[FigureSpec]:
    """Les figurines d'un descripteur dont la cible est un dossier Characters/."""
    if not isinstance(raws, list) or not raws:
        raise DescriptorError(f"{path} : aucune figurine")
    figures: list[FigureSpec] = []
    names: set[str] = set()
    for index, raw in enumerate(raws):
        where = f"{path.name}, figurine {index + 1}"
        if not isinstance(raw, dict):
            raise DescriptorError(f"{where} : une figurine est un objet")
        unknown = set(raw) - CHAMPS_FIGURE
        if unknown:
            raise DescriptorError(f"{where} : champ(s) inconnu(s) {sorted(unknown)}")
        name = raw.get("name")
        parts = name.split("/") if isinstance(name, str) else []
        if not parts or not all(DOSSIER.match(p) for p in parts[:-1]) or not NOM.match(parts[-1]):
            raise DescriptorError(f"{where} : nom {name!r} (dossiers de rangement, puis minuscules et tirets)")
        if name in names:
            raise DescriptorError(f"{where} : `{name}` nommée deux fois")
        names.add(name)
        strips_raw = raw.get("strips")
        if not isinstance(strips_raw, list) or not strips_raw:
            raise DescriptorError(f"{where} : `strips`, la liste des bandes, manque")
        strips = [read_strip(s, f"{where}, bande {n + 1}") for n, s in enumerate(strips_raw)]
        stems = [s.stem for s in strips]
        doubles = sorted({s for s in stems if stems.count(s) > 1})
        if doubles:
            raise DescriptorError(f"{where} : {', '.join(doubles)} installée(s) deux fois")
        figure = FigureSpec(name=name, strips=strips)
        for key in ("portrait", "token"):
            if key in raw:
                if not isinstance(raw[key], str):
                    raise DescriptorError(f"{where} : `{key}` nomme une source")
                setattr(figure, key, raw[key])
        if figure.token is not None and figure.portrait is None:
            raise DescriptorError(f"{where} : un jeton sans portrait")
        figures.append(figure)
    return figures


# --- Détourage et découpe ------------------------------------------------------------------------


def clean_alpha(alpha: np.ndarray) -> np.ndarray:
    """Le voile à 0, l'intérieur à 255, la pente du bord étirée entre les deux (uint8)."""
    scaled = (alpha.astype(np.float32) - ALPHA_BAS) / (ALPHA_HAUT - ALPHA_BAS)
    return np.clip(np.rint(scaled * 255.0), 0, 255).astype(np.uint8)


def label(mask: np.ndarray) -> tuple[np.ndarray, int]:
    """Les morceaux d'un seul tenant d'un masque booléen (4-voisinage), numérotés à partir de 1."""
    height, width = mask.shape
    labels = np.zeros((height, width), np.int32)
    count = 0
    for y0, x0 in zip(*np.nonzero(mask)):
        if labels[y0, x0]:
            continue
        count += 1
        labels[y0, x0] = count
        queue = deque([(y0, x0)])
        while queue:
            y, x = queue.popleft()
            for yy, xx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
                if 0 <= yy < height and 0 <= xx < width and mask[yy, xx] and not labels[yy, xx]:
                    labels[yy, xx] = count
                    queue.append((yy, xx))
    return labels, count


@dataclass
class Fragment:
    """Un morceau d'art détouré : son image RGBA et sa boîte dans la source."""

    image: np.ndarray
    box: tuple[int, int, int, int]  # x0, y0, x1, y1 (x1, y1 exclus)


def fragments(rgba: np.ndarray) -> list[Fragment]:
    """Détoure une source et la découpe en morceaux d'un seul tenant, dans l'ordre de lecture.

    L'étiquetage se fait sur une grille de `ETIQUETAGE` px (un bloc est plein s'il porte un pixel
    visible) : c'est ce qui le rend assez rapide pour une planche de 1254 px sans scipy, et un
    point perdu y reste un îlot. Un morceau plus petit que `ILOT_MIN` de l'art est effacé.
    """
    alpha = clean_alpha(rgba[..., 3])
    height, width = alpha.shape
    step = ETIQUETAGE
    padded = np.zeros(((height + step - 1) // step * step, (width + step - 1) // step * step), bool)
    padded[:height, :width] = alpha > 0
    blocks = padded.reshape(padded.shape[0] // step, step, padded.shape[1] // step, step).any(axis=(1, 3))
    labels, count = label(blocks)
    if count == 0:
        return []
    full = np.repeat(np.repeat(labels, step, axis=0), step, axis=1)[:height, :width]
    full = np.where(alpha > 0, full, 0)
    areas = np.bincount(full.ravel(), minlength=count + 1)
    total = areas[1:].sum()
    kept = [n for n in range(1, count + 1) if areas[n] >= ILOT_MIN * total]

    found: list[Fragment] = []
    for n in kept:
        ys, xs = np.nonzero(full == n)
        x0, x1, y0, y1 = int(xs.min()), int(xs.max()) + 1, int(ys.min()), int(ys.max()) + 1
        piece = rgba[y0:y1, x0:x1].copy()
        piece[..., 3] = np.where(full[y0:y1, x0:x1] == n, alpha[y0:y1, x0:x1], 0)
        piece[piece[..., 3] == 0] = 0
        found.append(Fragment(image=piece, box=(x0, y0, x1, y1)))
    return reading_order(found)


def reading_order(found: list[Fragment]) -> list[Fragment]:
    """De haut en bas, puis de gauche à droite : deux morceaux dont les boîtes se chevauchent en
    hauteur de plus de moitié sont sur la même rangée."""
    remaining = sorted(found, key=lambda f: f.box[1])
    ordered: list[Fragment] = []
    while remaining:
        head = remaining[0]
        row_top, row_bottom = head.box[1], head.box[3]
        row = [f for f in remaining
               if min(f.box[3], row_bottom) - max(f.box[1], row_top) > 0.5 * min(f.box[3] - f.box[1], row_bottom - row_top)]
        row.sort(key=lambda f: f.box[0])
        ordered.extend(row)
        remaining = [f for f in remaining if all(f is not r for r in row)]
    return ordered


# --- Réduction et ancrage ------------------------------------------------------------------------


def resize_premultiplied(rgba: np.ndarray, size: tuple[int, int]) -> np.ndarray:
    """Réduit une image RGBA en alpha prémultiplié : le fond transparent (noir) ne bave pas."""
    image = Image.fromarray(rgba, "RGBA").convert("RGBa")
    return np.asarray(image.resize(size, Image.LANCZOS).convert("RGBA")).copy()


@dataclass
class Tips:
    """Les deux pointes du socle d'une pièce debout, en px de source (dans sa boîte)."""

    west: tuple[float, float]
    east: tuple[float, float]


def measure_tips(alpha: np.ndarray) -> Tips:
    """Les pointes ouest et est du socle, lues sur l'**enveloppe basse** de l'art.

    Pour chaque colonne, le pixel visible le plus bas. Le bord bas d'un socle isométrique descend
    de la pointe ouest vers le sud en pente `+m`, puis remonte vers la pointe est en pente `−m` :
    la pointe ouest est donc le point de l'enveloppe qui maximise `bas − m·x` (le plus à gauche à
    `TOLERANCE_POINTE` près), la pointe est celui qui maximise `bas + m·x`. Une corniche qui
    déborde plus loin que le socle est plus haute que lui : elle ne l'emporte pas, ce qu'une simple
    colonne extrême ne garantissait pas (l'angle rentrant du Colisée). `m` est pris un peu plus
    raide que le losange du standard, parce que le générateur dessine un peu plus plat.
    """
    visible = alpha > 127
    columns = np.nonzero(visible.any(axis=0))[0]
    if len(columns) == 0:
        raise DescriptorError("pièce vide après détourage")
    height = visible.shape[0]
    # La ligne la plus basse de chaque colonne visible (le masque retourné : argmax donne la première).
    bottom = height - np.argmax(visible[::-1, columns], axis=0)
    xs = columns.astype(float)

    def tip(score: np.ndarray, pick) -> tuple[float, float]:
        best = score.max()
        index = pick(np.nonzero(score >= best - TOLERANCE_POINTE)[0])
        return float(xs[index]), float(bottom[index])

    west = tip(bottom - PENTE_POINTE * xs, np.min)
    east = tip(bottom + PENTE_POINTE * xs, np.max)
    return Tips(west=west, east=(east[0] + 1.0, east[1]))


def base_extent(tips: Tips, tile: tuple[int, int]) -> tuple[float, float]:
    """L'étendue du socle le long des colonnes et des rangées, en cases, à l'échelle 1.

    De la pointe ouest à la pointe est d'une emprise de C colonnes et R rangées, on va de
    C × (L/2, H/2) + R × (L/2, −H/2) : `dx = (C + R) L/2`, `dy = (C − R) H/2`. On l'inverse.
    """
    half_w, half_h = tile[0] / 2.0, tile[1] / 2.0
    dx = tips.east[0] - tips.west[0]
    dy = tips.east[1] - tips.west[1]
    return (dx / half_w + dy / half_h) / 2.0, (dx / half_w - dy / half_h) / 2.0


@dataclass
class Installed:
    """Une pièce prête : son image et son entrée de manifeste."""

    name: str
    image: np.ndarray
    entry: dict
    scale: float
    note: str = ""


def install_floor(spec: PieceSpec, name: str, fragment: Fragment, tile: tuple[int, int]) -> Installed:
    """Une dalle : le losange de la source devient exactement celui du lieu."""
    height, width = fragment.image.shape[:2]
    if width < tile[0] or height < tile[1]:
        raise DescriptorError(f"{name} : losange de {width} × {height} px, plus petit que {tile[0]} × {tile[1]} "
                              "(l'art n'est jamais agrandi)")
    image = resize_premultiplied(fragment.image, tile)
    ratio = height / width
    note = f"losange source {width} × {height} (rapport {ratio:.3f})"
    return Installed(name=name, image=image, scale=tile[0] / width, note=note, entry={
        "file": f"{name}.png",
        "class": spec.piece_class or "floor",
        "family": spec.family,
        "footprint": [1, 1],
        "size": [tile[0], tile[1]],
        "anchor": [tile[0] // 2, 0],
    })


def install_standing(spec: PieceSpec, name: str, fragment: Fragment, tile: tuple[int, int]) -> Installed:
    """Une pièce debout : réduite pour que son socle remplisse son emprise, ancrée à sa pointe ouest."""
    columns, rows = spec.footprint
    tips = measure_tips(fragment.image[..., 3])
    along_columns, along_rows = base_extent(tips, tile)
    if spec.scale is not None:
        scale = spec.scale
    else:
        candidates = [c / e for c, e in ((columns, along_columns), (rows, along_rows)) if e > 0.05]
        if not candidates:
            raise DescriptorError(f"{name} : socle introuvable aux pointes ; donner `scale`")
        scale = min(candidates)
    if scale > 1.0:
        raise DescriptorError(f"{name} : il faudrait agrandir la source × {scale:.2f} ; la refaire plus grande")

    height, width = fragment.image.shape[:2]
    size = (max(1, round(width * scale)), max(1, round(height * scale)))
    reduced = resize_premultiplied(fragment.image, size)

    # Le socle mesure a × b cases. Sur un axe qu'il ne remplit pas (l'épaisseur d'un mur, le pied
    # d'un lampadaire), il est centré dans son emprise, ou collé à son bord nord (`align: north`) :
    # c'est là que les bras d'un angle rentrant sont par construction, et un mur doit les rejoindre.
    # Sa pointe ouest est au point (u0, v0 + b) de l'emprise, en cases depuis son sommet nord ; une
    # case vaut (L/2, H/2) le long des colonnes et (−L/2, H/2) le long des rangées.
    half_w, half_h = tile[0] / 2.0, tile[1] / 2.0
    a, b = along_columns * scale, along_rows * scale
    if spec.align == "north":
        u0, v0 = 0.0, 0.0
    else:
        u0, v0 = max(0.0, (columns - a) / 2.0), max(0.0, (rows - b) / 2.0)
    west_x = (u0 - v0 - b) * half_w
    west_y = (u0 + v0 + b) * half_h
    anchor_x = tips.west[0] * scale - west_x + MARGE + spec.anchor_offset[0]
    anchor_y = tips.west[1] * scale - west_y + MARGE + spec.anchor_offset[1]
    # Une pièce plus basse que son losange (un muret) : on ajoute du vide au-dessus pour que l'ancre
    # tombe dans l'image, le moteur ne connaît pas d'ancre négative.
    top = max(0, -int(np.floor(anchor_y)))
    canvas = np.zeros((size[1] + 2 * MARGE + top, size[0] + 2 * MARGE, 4), np.uint8)
    canvas[MARGE + top:MARGE + top + size[1], MARGE:MARGE + size[0]] = reduced
    anchor = [round(anchor_x), round(anchor_y) + top]

    piece_class = spec.piece_class or ("tall" if (columns, rows) == (1, 1) else "wide")
    entry = {
        "file": f"{name}.png",
        "class": piece_class,
        "family": spec.family,
        "footprint": [columns, rows],
        "size": [canvas.shape[1], canvas.shape[0]],
        "anchor": anchor,
    }
    note = f"socle {along_columns * scale:.2f} × {along_rows * scale:.2f} cases pour {columns} × {rows}"
    return Installed(name=name, image=canvas, entry=entry, scale=scale, note=note)


def build(descriptor: Descriptor, only: str | None = None) -> list[Installed]:
    """Toutes les pièces du descripteur (ou la seule `only`), prêtes à écrire."""
    tile = place_tile(descriptor)
    ready: list[Installed] = []
    for spec in descriptor.pieces:
        if only is not None and only not in spec.names:
            continue
        source = descriptor.source_dir / spec.source
        if not source.is_file():
            raise DescriptorError(f"{spec.source} : source absente de {descriptor.source_dir}")
        rgba = np.asarray(Image.open(source).convert("RGBA")).copy()
        found = fragments(rgba)
        if len(found) != len(spec.names):
            raise DescriptorError(f"{spec.source} : {len(found)} morceau(x) d'un seul tenant, "
                                  f"{len(spec.names)} nom(s) au descripteur")
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        for name, fragment in zip(spec.names, found):
            if only is not None and name != only:
                continue
            installed = (install_floor if spec.is_floor else install_standing)(spec, name, fragment, tile)
            if spec.tactical is not None:
                installed.entry["tactical"] = spec.tactical
            installed.entry["source"] = {
                "file": source.relative_to(SOURCES).as_posix() if source.is_relative_to(SOURCES) else spec.source,
                "sha256": digest,
            }
            ready.append(installed)
    if only is not None and not ready:
        raise DescriptorError(f"{only} : aucune pièce de ce nom au descripteur")
    return ready


# --- Les figurines -------------------------------------------------------------------------------


@dataclass
class FrameMeasure:
    """Une image installée, mesurée dans sa cellule (px installés, pixels visibles ≥ `OPAQUE`)."""

    box: tuple[int, int, int, int]  # x0, y0, x1, y1 (x1, y1 exclus)
    bottom: int  # la rangée visible la plus basse
    contact: tuple[int, int]  # x du premier et du dernier appui au sol


@dataclass
class InstalledStrip:
    """Une bande prête : l'image (cellules jointives), son `.anim.json`, ses mesures."""

    spec: StripSpec
    image: np.ndarray
    anim: dict
    scale: float
    cell: tuple[int, int]
    frames: list[FrameMeasure]
    split: str  # comment la bande s'est découpée : « colonnes vides » ou « parts égales »


@dataclass
class InstalledFigure:
    """Une figurine prête : ses bandes, son portrait et son jeton, ses sources."""

    name: str
    strips: list[InstalledStrip]
    portrait: np.ndarray | None = None
    token: np.ndarray | None = None
    sources: dict = field(default_factory=dict)

    def files(self) -> dict[str, np.ndarray]:
        """Les images à écrire dans le dossier de la figurine, par nom de fichier."""
        images = {f"{s.spec.stem}.png": s.image for s in self.strips}
        if self.portrait is not None:
            images["portrait.png"] = self.portrait
        if self.token is not None:
            images["token.png"] = self.token
        return images


def detoured(rgba: np.ndarray) -> np.ndarray:
    """La source entière détourée comme une pièce (voile, intérieur, îlots), à sa place."""
    canvas = np.zeros_like(rgba)
    for fragment in fragments(rgba):
        x0, y0, x1, y1 = fragment.box
        visible = fragment.image[..., 3] > 0
        canvas[y0:y1, x0:x1][visible] = fragment.image[visible]
    return canvas


def runs(columns: np.ndarray) -> list[tuple[int, int]]:
    """Les suites de colonnes pleines d'un profil booléen, en [début, fin)."""
    edges = np.diff(np.concatenate(([0], columns.astype(np.int8), [0])))
    return list(zip(np.nonzero(edges == 1)[0].tolist(), np.nonzero(edges == -1)[0].tolist()))


def split_strip(alpha: np.ndarray, count: int, where: str) -> tuple[list[int], list[float], str]:
    """Les bornes des `count` images d'une bande, les centres de leur grille, et la méthode.

    Par les colonnes vides d'abord ; à défaut, `count` parts égales de l'étendue de l'art. Les
    centres sont ceux d'une grille **régulière** : le générateur dessine à pas constant, et ce
    qu'une image s'en écarte est du mouvement.
    """
    width = alpha.shape[1]
    groups = runs(alpha.any(axis=0))
    if not groups:
        raise DescriptorError(f"{where} : bande vide après détourage")
    if len(groups) == count:
        # Des médianes, pas des moindres carrés : une image en fente tirerait toute la grille à elle.
        centres = np.array([(a + b) / 2.0 for a, b in groups])
        index = np.arange(count, dtype=float)
        pitch = float(np.median(np.diff(centres))) if count > 1 else 0.0
        origin = float(np.median(centres - pitch * index))
        bounds = [0, *[(groups[i][1] + groups[i + 1][0]) // 2 for i in range(count - 1)], width]
        return bounds, [float(origin + pitch * i) for i in range(count)], "colonnes vides"
    start, end = groups[0][0], groups[-1][1]
    step = (end - start) / count
    bounds = [start + round(step * i) for i in range(count)] + [end]
    for i in range(count):
        if not alpha[:, bounds[i]:bounds[i + 1]].any():
            raise DescriptorError(f"{where} : {len(groups)} groupe(s) de colonnes pour {count} images, et "
                                  f"le partage en {count} parts égales laisse l'image {i + 1} vide")
    return bounds, [start + step * (i + 0.5) for i in range(count)], "parts égales"


def measure_frame(cell: np.ndarray) -> FrameMeasure:
    visible = cell[..., 3] >= OPAQUE
    ys, xs = np.nonzero(visible)
    if len(ys) == 0:
        return FrameMeasure(box=(0, 0, 0, 0), bottom=-1, contact=(-1, -1))
    bottom = int(ys.max())
    feet = xs[ys >= bottom - APPUI]
    return FrameMeasure(box=(int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1),
                        bottom=bottom, contact=(int(feet.min()), int(feet.max())))


def frame_images(rgba: np.ndarray, bounds: list[int], where: str) -> list[np.ndarray]:
    """Les images d'une bande, chacune à sa place dans la source, le reste transparent.

    Chaque morceau d'un seul tenant revient **entier** à l'image où tombe son centre : la hache que
    tient une main part avec elle, même quand sa lame passe la borne de la voisine — couper aux
    bornes la tranchait, et un bout de lame s'invitait chez la voisine. Un morceau qui touche le tiers
    central de plusieurs images est fait de corps qui se touchent (la lame de l'un sur l'épaule de l'autre) : il
    se partage en faisant **croître** chaque corps depuis le milieu de son image, à travers la
    matière (`grow`) — la lame revient à la main qui la tient par son manche.
    """
    height, width = rgba.shape[:2]
    count = len(bounds) - 1
    images = [np.zeros_like(rgba) for _ in range(count)]
    step = (bounds[-1] - bounds[0]) / count
    for fragment in fragments(rgba):
        x0, y0, x1, y1 = fragment.box
        alpha = fragment.image[..., 3].astype(np.float64)
        visible = fragment.image[..., 3] > 0
        columns_used = visible.any(axis=0)
        cores = sum(1 for i in range(count)
                    if columns_used[max(0, round(bounds[i] + step / 3) - x0):
                                    max(0, round(bounds[i + 1] - step / 3) - x0)].any())
        if cores > 1:
            owners = grow(visible, [b - x0 for b in bounds])
        else:
            columns = alpha.sum(axis=0)
            centre = x0 + float((columns * np.arange(x1 - x0)).sum() / max(columns.sum(), 1.0))
            owner = min(count - 1, max(0, int(np.searchsorted(bounds, centre, side="right")) - 1))
            owners = np.full(visible.shape, owner, np.int32)
        for owner in range(count):
            mine = visible & (owners == owner)
            images[owner][y0:y1, x0:x1][mine] = fragment.image[mine]
    for i, image in enumerate(images):
        if not image[..., 3].any():
            raise DescriptorError(f"{where} : l'image {i + 1} est vide après découpe")
    return images


def grow(mask: np.ndarray, bounds: list[float]) -> np.ndarray:
    """Partage un morceau fait de plusieurs corps : l'image de chaque pixel, -1 hors du masque.

    Chaque image sème le tiers central de sa part ; les graines croissent ensemble, à travers le
    masque, sur une grille de `ETIQUETAGE` px, et un bloc revient à la première qui l'atteint — la
    plus proche **en suivant la matière**, pas à vol d'oiseau.
    """
    height, width = mask.shape
    count = len(bounds) - 1
    step = ETIQUETAGE
    padded = np.zeros(((height + step - 1) // step * step, (width + step - 1) // step * step), bool)
    padded[:height, :width] = mask
    blocks = padded.reshape(padded.shape[0] // step, step, padded.shape[1] // step, step).any(axis=(1, 3))
    rows, cols = blocks.shape
    owner = np.full(blocks.shape, -1, np.int32)
    queue: deque = deque()
    for i in range(count):
        third = (bounds[i + 1] - bounds[i]) / 3.0
        c0 = max(0, int((bounds[i] + third) // step))
        c1 = min(cols, max(0, int((bounds[i + 1] - third) // step) + 1))
        if c1 <= c0:
            continue
        for y, x in zip(*np.nonzero(blocks[:, c0:c1])):
            if owner[y, x + c0] < 0:
                owner[y, x + c0] = i
                queue.append((y, x + c0))
    while queue:
        y, x = queue.popleft()
        for yy, xx in ((y + 1, x), (y - 1, x), (y, x + 1), (y, x - 1)):
            if 0 <= yy < rows and 0 <= xx < cols and blocks[yy, xx] and owner[yy, xx] < 0:
                owner[yy, xx] = owner[y, x]
                queue.append((yy, xx))
    full = np.repeat(np.repeat(owner, step, axis=0), step, axis=1)[:height, :width]
    # Un bloc qu'aucune graine n'atteint (le masque n'y passe pas) revient à la part de sa colonne.
    columns = np.clip(np.searchsorted(bounds, np.arange(width) + 0.5, side="right") - 1, 0, count - 1)
    return np.where(full >= 0, full, np.broadcast_to(columns, (height, width)))


def install_strip(spec: StripSpec, rgba: np.ndarray, cell: tuple[int, int], ground: int,
                  where: str) -> InstalledStrip:
    """Une bande du générateur devient une bande du moteur : N cellules jointives, une échelle et
    un décalage pour toutes — le pied le plus bas de la bande sur le sol, l'image de repos au milieu
    de sa cellule."""
    clean = detoured(rgba)
    bounds, centres, split = split_strip(clean[..., 3] > 0, spec.frames, where)
    images = frame_images(rgba, bounds, where)

    standing = spec.standing_frame
    rows = np.nonzero(images[standing][..., 3].any(axis=1))[0]
    scale = spec.scale if spec.scale is not None else HAUTEUR_FIGURINE / (rows.max() + 1 - rows.min())
    if scale > 1.0:
        raise DescriptorError(f"{where} : il faudrait agrandir la bande × {scale:.2f} ; la refaire plus grande")

    # Chaque image réduite à la même échelle, posée à sa place dans la bande réduite.
    parts: list[tuple[np.ndarray, int, int]] = []
    for image in images:
        ys, xs = np.nonzero(image[..., 3] > 0)
        x0, x1, y0, y1 = int(xs.min()), int(xs.max()) + 1, int(ys.min()), int(ys.max()) + 1
        size = (max(1, round((x1 - x0) * scale)), max(1, round((y1 - y0) * scale)))
        parts.append((resize_premultiplied(image[y0:y1, x0:x1], size), round(x0 * scale), round(y0 * scale)))
    slots = [c * scale for c in centres]

    # Le décalage de toute la bande. En hauteur : le pied le plus BAS de la bande sur le sol — un
    # appui d'une autre image descend souvent de quelques pixels sous celui de l'image de repos (le
    # générateur ne tient pas sa ligne de sol au pixel), et poser l'image de repos ferait passer cet
    # appui sous le sol, hors de la cellule. Un saut ou un accroupissement restent au-dessus. En
    # largeur : le milieu de la boîte de l'image de repos au milieu de la cellule.
    lowest = []
    for part, _, top in parts:
        opaque = np.nonzero((part[..., 3] >= OPAQUE).any(axis=1))[0]
        if len(opaque):
            lowest.append(top + int(opaque.max()))
    if not lowest:
        raise DescriptorError(f"{where} : aucune image opaque après réduction")
    dy = ground - max(lowest)
    part, left, _ = parts[standing]
    xs = np.nonzero((part[..., 3] >= OPAQUE).any(axis=0))[0]
    if len(xs) == 0:
        raise DescriptorError(f"{where} : l'image de repos {standing + 1} est vide")
    box_centre = left + (xs.min() + xs.max() + 1) / 2.0

    cell_w, cell_h = cell
    strip = np.zeros((cell_h, cell_w * spec.frames, 4), np.uint8)
    measures: list[FrameMeasure] = []
    for i, (part, left, top) in enumerate(parts):
        # La colonne u de la bande réduite tombe en u + ox dans la cellule.
        ox = round(cell_w / 2.0 - box_centre + slots[standing] - slots[i])
        ys, xs = np.nonzero(part[..., 3] > 0)
        x0, x1 = left + int(xs.min()) + ox, left + int(xs.max()) + 1 + ox
        y0, y1 = top + int(ys.min()) + dy, top + int(ys.max()) + 1 + dy
        # La marge ne vaut qu'à gauche et à droite : c'est là que sont les voisines, et c'est d'elles
        # que le mipmap bave. En bas, elle contredirait le sol à 4 px du bord (le bord adouci d'un
        # pied posé sur y = 252 descend plus bas) ; le haut et le bas n'ont qu'à tenir dans la cellule.
        if x0 < MARGE_CELLULE or x1 > cell_w - MARGE_CELLULE or y0 < 0 or y1 > cell_h:
            raise DescriptorError(f"{where} : l'image {i + 1} occupe x {x0}-{x1}, y {y0}-{y1} ; elle ne tient pas "
                                  f"dans sa cellule de {cell_w} × {cell_h} avec {MARGE_CELLULE} px de marge "
                                  "à gauche et à droite")
        target = strip[:, i * cell_w:(i + 1) * cell_w]
        target[y0:y1, x0:x1] = part[int(ys.min()):int(ys.max()) + 1, int(xs.min()):int(xs.max()) + 1]
        measures.append(measure_frame(target))

    anim = {
        "version": 1,
        "frameWidth": cell_w,
        "frameHeight": cell_h,
        "clips": {spec.clip: {"frames": list(range(spec.frames)), "frameDuration": spec.frame_duration,
                              "loop": spec.loop}},
    }
    return InstalledStrip(spec=spec, image=strip, anim=anim, scale=scale, cell=cell, frames=measures, split=split)


def square(rgba: np.ndarray, side: int, where: str) -> np.ndarray:
    """Le carré central d'une image, réduit à `side` : jamais agrandi."""
    height, width = rgba.shape[:2]
    edge = min(width, height)
    if edge < side:
        raise DescriptorError(f"{where} : {width} × {height} px, trop petit pour {side} × {side} "
                              "(l'art n'est jamais agrandi)")
    x0, y0 = (width - edge) // 2, (height - edge) // 2
    return resize_premultiplied(rgba[y0:y0 + edge, x0:x0 + edge], (side, side))


def round_token(rgba: np.ndarray, where: str) -> np.ndarray:
    """Le jeton : le carré central à `JETON` px, détouré en rond, bord adouci d'un pixel."""
    token = square(rgba, JETON, where)
    centre = JETON / 2.0
    ys, xs = np.mgrid[0:JETON, 0:JETON]
    distance = np.hypot(xs + 0.5 - centre, ys + 0.5 - centre)
    coverage = np.clip(centre - 0.5 - distance + 0.5, 0.0, 1.0)
    token[..., 3] = np.rint(token[..., 3] * coverage).astype(np.uint8)
    token[token[..., 3] == 0] = 0
    return token


def figure_cells(manifest: dict) -> tuple[tuple[int, int], tuple[int, int], int]:
    """La cellule, la cellule large et la ligne de sol : celles du manifeste s'il les déclare."""
    where = "manifest.json de la cible"
    frame = _pair(manifest["frame"], f"{where}, frame") if "frame" in manifest else CELLULE
    wide = _pair(manifest["wideFrame"], f"{where}, wideFrame") if "wideFrame" in manifest else CELLULE_LARGE
    ground = manifest.get("ground", SOL)
    if isinstance(ground, bool) or not isinstance(ground, int) or not 0 < ground < frame[1]:
        raise DescriptorError(f"{where} : `ground` est une rangée de la cellule, lu {ground!r}")
    return frame, wide, ground


def source_entry(descriptor: Descriptor, relative: str) -> tuple[Path, dict]:
    source = descriptor.source_dir / relative
    if not source.is_file():
        raise DescriptorError(f"{relative} : source absente de {descriptor.source_dir}")
    return source, {
        "file": source.relative_to(SOURCES).as_posix() if source.is_relative_to(SOURCES) else relative,
        "sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
    }


def load_rgba(path: Path) -> np.ndarray:
    return np.asarray(Image.open(path).convert("RGBA")).copy()


def build_figures(descriptor: Descriptor, only: str | None = None) -> list[InstalledFigure]:
    """Toutes les figurines du descripteur (ou la seule `only`), prêtes à écrire."""
    frame, wide, ground = figure_cells(read_manifest(descriptor))
    ready: list[InstalledFigure] = []
    for figure in descriptor.figures:
        if only is not None and figure.name != only:
            continue
        installed = InstalledFigure(name=figure.name, strips=[])
        for spec in figure.strips:
            source, entry = source_entry(descriptor, spec.source)
            where = f"{figure.name}/{spec.stem}"
            installed.strips.append(install_strip(spec, load_rgba(source), wide if spec.wide else frame,
                                                  ground, where))
            installed.sources[f"{figure.name}/{spec.stem}.png"] = entry
        if figure.portrait is not None:
            source, entry = source_entry(descriptor, figure.portrait)
            portrait = load_rgba(source)
            installed.portrait = square(portrait, PORTRAIT, f"{figure.name}/portrait")
            installed.sources[f"{figure.name}/portrait.png"] = entry
            if figure.token is not None:
                source, entry = source_entry(descriptor, figure.token)
                portrait = load_rgba(source)
            installed.token = round_token(portrait, f"{figure.name}/token")
            installed.sources[f"{figure.name}/token.png"] = entry
        ready.append(installed)
    if only is not None and not ready:
        raise DescriptorError(f"{only} : aucune figurine de ce nom au descripteur")
    return ready


def anim_text(anim: dict) -> str:
    return json.dumps(anim, indent=2, ensure_ascii=False) + "\n"


def figure_manifest(descriptor: Descriptor, ready: list[InstalledFigure]) -> dict:
    """Le manifeste de la cible, avec les figurines de `ready` inscrites."""
    manifest = read_manifest(descriptor)
    npcs = manifest.get("npcs", [])
    manifest["npcs"] = sorted(set(npcs) | {figure.name for figure in ready})
    sources = dict(manifest.get("sources", {}))
    for figure in ready:
        sources.update(figure.sources)
    manifest["sources"] = dict(sorted(sources.items()))
    if str(manifest.get("comment", "")).startswith("Vide"):
        del manifest["comment"]
    return manifest


def write_figures(descriptor: Descriptor, ready: list[InstalledFigure]) -> None:
    for figure in ready:
        folder = descriptor.target_dir / figure.name
        folder.mkdir(parents=True, exist_ok=True)
        for filename, image in figure.files().items():
            (folder / filename).write_bytes(encode_png(image))
        for strip in figure.strips:
            (folder / f"{strip.spec.stem}.anim.json").write_bytes(anim_text(strip.anim).encode("utf-8"))
    manifest = figure_manifest(descriptor, ready)
    (descriptor.target_dir / "manifest.json").write_bytes(manifest_text(manifest).encode("utf-8"))


def stale_figures(descriptor: Descriptor, ready: list[InstalledFigure]) -> list[str]:
    """Ce qui, dans les figurines installées, n'est plus ce que la commande produirait."""
    manifest = read_manifest(descriptor)
    problems = []
    for figure in ready:
        if figure.name not in manifest.get("npcs", []):
            problems.append(f"{figure.name} : absente de `npcs`")
        recorded = manifest.get("sources", {})
        for key, entry in figure.sources.items():
            if recorded.get(key) != entry:
                problems.append(f"{key} : source absente de `sources`, ou d'une autre empreinte")
        folder = descriptor.target_dir / figure.name
        for filename, image in figure.files().items():
            path = folder / filename
            if not path.is_file():
                problems.append(f"{figure.name}/{filename} : absent")
            elif not np.array_equal(np.asarray(Image.open(path).convert("RGBA")), image):
                problems.append(f"{figure.name}/{filename} : diffère de ce que donne la source")
        for strip in figure.strips:
            path = folder / f"{strip.spec.stem}.anim.json"
            if not path.is_file() or path.read_text(encoding="utf-8") != anim_text(strip.anim):
                problems.append(f"{figure.name}/{path.name} : absent ou différent")
    return problems


def print_figures(ready: list[InstalledFigure], measure: bool) -> None:
    for figure in ready:
        for strip in figure.strips:
            print(f"{figure.name}/{strip.spec.stem:12} {strip.spec.frames} image(s)  cellule "
                  f"{strip.cell[0]} × {strip.cell[1]}  échelle {strip.scale:.3f}  ({strip.split})")
            if not measure:
                continue
            for i, frame in enumerate(strip.frames):
                x0, y0, x1, y1 = frame.box
                print(f"    image {i + 1:2}  boîte x {x0:3}-{x1:<3} y {y0:3}-{y1:<3}  ligne basse {frame.bottom:3}  "
                      f"appuis x {frame.contact[0]:3} … {frame.contact[1]:<3}")
        if figure.portrait is not None:
            print(f"{figure.name}/portrait     {PORTRAIT} × {PORTRAIT}, jeton {JETON} × {JETON}")


# --- Le manifeste --------------------------------------------------------------------------------


def place_tile(descriptor: Descriptor) -> tuple[int, int]:
    """Le losange du lieu, lu dans son manifeste : l'échelle de l'art est une donnée du lieu."""
    manifest = read_manifest(descriptor)
    return _pair(manifest.get("tile"), f"{descriptor.target}/manifest.json, tile")


def read_manifest(descriptor: Descriptor) -> dict:
    path = descriptor.target_dir / "manifest.json"
    if not path.is_file():
        raise DescriptorError(f"{descriptor.target} : pas de manifest.json (le dossier se crée au LOT-102 "
                              "ou au lot qui ouvre la zone)")
    return json.loads(path.read_text(encoding="utf-8"))


def key_prefix(manifest: dict, descriptor: Descriptor) -> str:
    """Le préfixe des clés d'atelier : `scene/<lieu>/`, du `disposition` du manifeste."""
    place = manifest.get("disposition") or descriptor.target_dir.parent.name
    return f"scene/{place}/"


def encode_png(image: np.ndarray) -> bytes:
    buffer = io.BytesIO()
    Image.fromarray(image, "RGBA").save(buffer, format="PNG", optimize=True)
    return buffer.getvalue()


def manifest_text(manifest: dict) -> str:
    return json.dumps(manifest, indent=2, ensure_ascii=False) + "\n"


def write(descriptor: Descriptor, ready: list[Installed]) -> None:
    manifest = read_manifest(descriptor)
    textures = manifest.setdefault("textures", {})
    prefix = key_prefix(manifest, descriptor)
    for installed in ready:
        (descriptor.target_dir / installed.entry["file"]).write_bytes(encode_png(installed.image))
        textures[prefix + installed.name] = installed.entry
    # Le commentaire d'un dossier créé vide (« Vide : … », LOT-102) ne dit plus vrai.
    if str(manifest.get("comment", "")).startswith("Vide"):
        del manifest["comment"]
    # Écrit en LF, sans BOM : ce que check_json_files.py attend.
    (descriptor.target_dir / "manifest.json").write_bytes(manifest_text(manifest).encode("utf-8"))


def stale(descriptor: Descriptor, ready: list[Installed]) -> list[str]:
    """Ce qui, dans l'installé, n'est plus ce que la commande produirait des sources."""
    manifest = read_manifest(descriptor)
    textures = manifest.get("textures", {})
    prefix = key_prefix(manifest, descriptor)
    problems = []
    for installed in ready:
        key = prefix + installed.name
        if textures.get(key) != installed.entry:
            problems.append(f"{key} : entrée de manifeste absente ou différente")
            continue
        path = descriptor.target_dir / installed.entry["file"]
        if not path.is_file():
            problems.append(f"{key} : {path.name} absent")
        elif not np.array_equal(np.asarray(Image.open(path).convert("RGBA")), installed.image):
            problems.append(f"{key} : {path.name} diffère de ce que donne la source")
    return problems


# --- Commande ------------------------------------------------------------------------------------


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("descriptor", type=Path, help="le install.json d'un dossier de sources")
    parser.add_argument("--piece", help="n'installer que cette pièce (ou cette figurine)")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="vérifier que l'installé est à jour")
    mode.add_argument("--measure", action="store_true", help="afficher les mesures sans rien écrire")
    args = parser.parse_args(argv)

    try:
        descriptor = read_descriptor(args.descriptor.resolve())
        if descriptor.is_characters:
            return main_figures(args, descriptor)
        ready = build(descriptor, args.piece)
    except DescriptorError as error:
        print(f"install_hd_asset : {error}", file=sys.stderr)
        return 1

    for installed in ready:
        entry = installed.entry
        print(f"{installed.name:28} {entry['class']:6} {entry['footprint'][0]}×{entry['footprint'][1]}  "
              f"{entry['size'][0]:4} × {entry['size'][1]:<4} ancre {entry['anchor']}  "
              f"échelle {installed.scale:.3f}  {installed.note}")
    if args.measure:
        return 0
    if args.check:
        problems = stale(descriptor, ready)
        for problem in problems:
            print(problem, file=sys.stderr)
        if problems:
            print(f"relancer : python scripts/install_hd_asset.py {args.descriptor}", file=sys.stderr)
            return 1
        print(f"{len(ready)} pièce(s) à jour dans {descriptor.target}")
        return 0
    write(descriptor, ready)
    print(f"{len(ready)} pièce(s) installée(s) dans {descriptor.target}")
    return 0


def main_figures(args: argparse.Namespace, descriptor: Descriptor) -> int:
    """La commande pour un dossier Characters/ : mêmes modes que pour les pièces."""
    ready = build_figures(descriptor, args.piece)
    print_figures(ready, args.measure)
    if args.measure:
        return 0
    if args.check:
        problems = stale_figures(descriptor, ready)
        for problem in problems:
            print(problem, file=sys.stderr)
        if problems:
            print(f"relancer : python scripts/install_hd_asset.py {args.descriptor}", file=sys.stderr)
            return 1
        print(f"{len(ready)} figurine(s) à jour dans {descriptor.target}")
        return 0
    if descriptor.preview_only:
        print(f"install_hd_asset : {args.descriptor} est un essai (`previewOnly`) : il se mesure "
              "(--measure) et s'aperçoit (preview_figure_walk.py), il ne s'installe pas", file=sys.stderr)
        return 1
    write_figures(descriptor, ready)
    print(f"{len(ready)} figurine(s) installée(s) dans {descriptor.target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
