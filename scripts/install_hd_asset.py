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

Usage :
    python scripts/install_hd_asset.py Tools/AssetsHD/Colisee/install.json
    python scripts/install_hd_asset.py DESCRIPTEUR --piece wall-arcade-u   # une seule pièce
    python scripts/install_hd_asset.py DESCRIPTEUR --check   # l'installé est-il à jour des sources ?
    python scripts/install_hd_asset.py DESCRIPTEUR --measure # mesures seules, rien n'est écrit

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
class Descriptor:
    path: Path
    target: str
    pieces: list[PieceSpec] = field(default_factory=list)

    @property
    def source_dir(self) -> Path:
        return self.path.parent

    @property
    def target_dir(self) -> Path:
        return ASSETS / self.target


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
    if not isinstance(target, str) or not target.endswith("/Scene") or ".." in target:
        raise DescriptorError(f"{path} : `target` doit nommer un dossier Scene/ sous les assets")
    descriptor = Descriptor(path=path, target=target)
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
    parser.add_argument("--piece", help="n'installer que cette pièce")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="vérifier que l'installé est à jour")
    mode.add_argument("--measure", action="store_true", help="afficher les mesures sans rien écrire")
    args = parser.parse_args(argv)

    try:
        descriptor = read_descriptor(args.descriptor.resolve())
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


if __name__ == "__main__":
    sys.exit(main())
