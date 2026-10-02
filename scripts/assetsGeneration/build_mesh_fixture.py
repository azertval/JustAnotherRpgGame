#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Engendre les données d'essai en maillages du moteur (LOT-1003).

Le LOT-1003 apprend au moteur à dessiner des volumes, sur des données d'essai : aucun asset livré
ne change avant le LOT-1004. Ce script écrit ces données, sous `Source/Test/Fixtures/Meshes/` :

- trois maillages `.glb` -- un **sol**, un **mur**, un **toit** --, chacun avec sa texture
  incorporée, au format que le standard 3D fixe (`Planning/standards/style-3d.md`, §1 et §2) :
  le mètre, la hauteur vers +Y, l'origine au sol sous le centre de l'emprise, un maillage, une
  primitive, une matière, `POSITION` / `NORMAL` / `TEXCOORD_0` ;
- une dalle de sol **en image**, pour qu'une même carte mêle les deux formes ;
- le manifeste du lieu `ilot`, dont trois clés citent un maillage (`"mesh"`) et une une image ;
- la carte `ilot` : une cour dallée, un îlot de murs en anneau, son toit à l'étage ;
- une **image témoin** de 1,80 m, d'une teinte que rien d'autre ne porte, rangée comme un effet
  (`Common/Fx/temoin`, la seule sorte de bande qui reste depuis le LOT-1006) : ce que le tampon de
  profondeur laisse voir d'une image dressée se compte ;
- un **pantin** (LOT-1005) : un modèle de 1,80 m lié à un squelette de trois os, ses six clips, sa
  fiche (`character.json`) et la description de son squelette (`skeleton.json`). Chaque sommet
  suit un seul os et chaque clip tient en trois clés : ce qu'un test attend d'une pose se calcule
  de tête.

Rien ne s'y retouche à la main, et rien n'y dépend d'une bibliothèque : le `.glb` et le PNG sont
écrits octet par octet, sans compression, pour que deux postes produisent les mêmes fichiers.

Usage :
    python scripts/assetsGeneration/build_mesh_fixture.py           # écrit les données d'essai
    python scripts/assetsGeneration/build_mesh_fixture.py --check   # vérifie qu'elles sont à jour
"""

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "Source" / "Test" / "Fixtures" / "Meshes"
PLACE = "ilot"

# --- Le standard -------------------------------------------------------------------------------
TILE_METRES = 1.5                    # une case de la grille du Manuel
DIAMOND_RATIO = 0.62                 # le sinus de l'élévation de la caméra (38,3°)
ART_TILE = (64, 40)                  # le losange de l'art du lieu d'essai : petit, c'est une fixture
STOREY_ART = 56                      # la hauteur d'un étage, en pixels d'art : 0,875 largeur de case

# Ce qu'un étage de `STOREY_ART` pixels fait en mètres sous la caméra du jeu : la largeur du
# losange est la diagonale d'une case, et une hauteur se voit raccourcie du cosinus de l'élévation.
STOREY_METRES = (STOREY_ART / ART_TILE[0]) * TILE_METRES * math.sqrt(2.0) / math.sqrt(
    1.0 - DIAMOND_RATIO * DIAMOND_RATIO)
ROOF_RISE_METRES = 1.4
ROOF_FOOTPRINT = 3

# La palette de l'Empire central (standard 3D, §4).
IVORY = (0xEF, 0xE6, 0xD2)
SAND = (0xD9, 0xC7, 0xA3)
WARM_GREY = (0x9C, 0x94, 0x8A)
BURGUNDY = (0x8E, 0x23, 0x35)
BRONZE = (0x5C, 0x4A, 0x2A)
TEAL = (0x2F, 0x7F, 0x86)
LEAF = (0x3F, 0x6B, 0x34)

TEXTURE_SIDE = 64

# La figurine témoin : sa cellule, sa ligne de sol, et la hauteur d'un corps de 1,80 m en pixels
# d'art -- 170 px au losange de 256 du standard, ramenés au losange du lieu d'essai.
FIGURE_CELL = (32, 48)
FIGURE_GROUND = 46
FIGURE_BODY = round(170 * ART_TILE[0] / 256)

# --- La carte ----------------------------------------------------------------------------------
MAP_COLUMNS, MAP_ROWS = 10, 8
COURT = (3, 1, 7, 5)                 # la cour en maillage : colonnes 3 à 7, lignes 1 à 5
BLOCK = (4, 2, 6, 4)                 # l'îlot : un anneau de murs autour de la case (5, 3)


# --- PNG, sans compression ---------------------------------------------------------------------
def _stored_zlib(data: bytes) -> bytes:
    """Un flux zlib en blocs « stockés » : sans compresseur, donc identique d'un poste à l'autre."""
    out = bytearray(b"\x78\x01")
    for start in range(0, len(data), 65535):
        block = data[start:start + 65535]
        final = 1 if start + 65535 >= len(data) else 0
        out += struct.pack("<BHH", final, len(block), len(block) ^ 0xFFFF)
        out += block
    out += struct.pack(">I", zlib.adler32(data))
    return bytes(out)


def _chunk(kind: bytes, payload: bytes) -> bytes:
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(
        ">I", zlib.crc32(kind + payload))


def encode_png(width: int, height: int, pixels: list[tuple[int, int, int, int]]) -> bytes:
    """Un PNG RGBA 8 bits de `pixels`, ligne par ligne depuis le haut."""
    raw = bytearray()
    for row in range(height):
        raw.append(0)  # filtre « aucun »
        for red, green, blue, alpha in pixels[row * width:(row + 1) * width]:
            raw += bytes((red, green, blue, alpha))
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + _chunk(b"IHDR", header)
            + _chunk(b"IDAT", _stored_zlib(bytes(raw))) + _chunk(b"IEND", b""))


def _shade(color: tuple[int, int, int], factor: float) -> tuple[int, int, int, int]:
    return (*(max(0, min(255, round(channel * factor))) for channel in color), 255)


def paving_texture() -> list[tuple[int, int, int, int]]:
    """Des dalles de sable, quatre par côté, joints gris chaud."""
    side = TEXTURE_SIDE
    return [(_shade(WARM_GREY, 1.0) if x % 16 == 0 or y % 16 == 0
             else _shade(SAND, 0.94 + 0.06 * (((x // 16) + (y // 16)) % 2)))
            for y in range(side) for x in range(side)]


def stone_texture() -> list[tuple[int, int, int, int]]:
    """Des assises de pierre ivoire, en quinconce, joints gris chaud."""
    side = TEXTURE_SIDE
    pixels = []
    for y in range(side):
        course = y // 16
        for x in range(side):
            joint = y % 16 == 0 or (x + 16 * (course % 2)) % 32 == 0
            pixels.append(_shade(WARM_GREY, 1.0) if joint
                          else _shade(IVORY, 0.92 + 0.04 * (course % 2)))
    return pixels


def tile_texture() -> list[tuple[int, int, int, int]]:
    """Des rangs de tuiles bourgogne, soulignés de bronze."""
    side = TEXTURE_SIDE
    return [(_shade(BRONZE, 1.0) if y % 8 == 0
             else _shade(BURGUNDY, 0.9 + 0.1 * ((x // 8 + y // 8) % 2)))
            for y in range(side) for x in range(side)]


def paving_image() -> bytes:
    """La dalle de sol en image : le losange de l'art, plein, bords nets."""
    width, height = ART_TILE
    pixels = []
    for y in range(height):
        for x in range(width):
            # Le centre du pixel est-il dans le losange ?
            inside = (abs((x + 0.5) - width / 2) / (width / 2)
                      + abs((y + 0.5) - height / 2) / (height / 2)) <= 1.0
            if not inside:
                pixels.append((0, 0, 0, 0))
            else:
                pixels.append(_shade(SAND, 0.82 + 0.05 * ((x // 8 + y // 5) % 2)))
    return encode_png(width, height, pixels)


# --- Géométrie ---------------------------------------------------------------------------------
class Mesh:
    """Des faces planes, chacune avec ses propres sommets : la normale est celle de la face."""

    def __init__(self) -> None:
        self.positions: list[tuple[float, float, float]] = []
        self.normals: list[tuple[float, float, float]] = []
        self.uvs: list[tuple[float, float]] = []
        self.indices: list[int] = []

    def face(self, corners: list[tuple[float, float, float]],
             uvs: list[tuple[float, float]]) -> None:
        """Une face convexe, ses coins dans le sens trigonométrique vue de l'extérieur."""
        (ax, ay, az), (bx, by, bz), (cx, cy, cz) = corners[0], corners[1], corners[2]
        ux, uy, uz = bx - ax, by - ay, bz - az
        vx, vy, vz = cx - ax, cy - ay, cz - az
        nx, ny, nz = uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx
        length = math.sqrt(nx * nx + ny * ny + nz * nz)
        base = len(self.positions)
        for corner, uv in zip(corners, uvs):
            self.positions.append(corner)
            self.normals.append((nx / length, ny / length, nz / length))
            self.uvs.append(uv)
        for index in range(1, len(corners) - 1):
            self.indices += [base, base + index, base + index + 1]


def floor_mesh() -> Mesh:
    """Une dalle d'une case, à plat, à la hauteur zéro."""
    half = TILE_METRES / 2
    mesh = Mesh()
    mesh.face([(-half, 0.0, -half), (-half, 0.0, half), (half, 0.0, half), (half, 0.0, -half)],
              [(0.0, 0.0), (0.0, 1.0), (1.0, 1.0), (1.0, 0.0)])
    return mesh


def wall_mesh() -> Mesh:
    """Un bloc de mur d'une case, haut d'un étage : quatre flancs et le dessus."""
    half = TILE_METRES / 2
    top = STOREY_METRES
    rows = top / TILE_METRES  # la texture se répète d'une case en une case de haut
    mesh = Mesh()
    flanks = [  # (coin gauche, coin droit), vus de l'extérieur
        ((-half, half), (half, half)),     # flanc +Z : vers les lignes croissantes
        ((half, half), (half, -half)),     # flanc +X : vers les colonnes croissantes
        ((half, -half), (-half, -half)),   # flanc -Z
        ((-half, -half), (-half, half)),   # flanc -X
    ]
    for (lx, lz), (rx, rz) in flanks:
        mesh.face([(lx, 0.0, lz), (rx, 0.0, rz), (rx, top, rz), (lx, top, lz)],
                  [(0.0, rows), (1.0, rows), (1.0, 0.0), (0.0, 0.0)])
    mesh.face([(-half, top, -half), (-half, top, half), (half, top, half), (half, top, -half)],
              [(0.0, 0.0), (0.0, 1.0), (1.0, 1.0), (1.0, 0.0)])
    return mesh


def roof_mesh() -> Mesh:
    """Un toit à quatre pans sur une emprise de trois cases de côté, posé à la hauteur zéro."""
    half = ROOF_FOOTPRINT * TILE_METRES / 2
    apex = (0.0, ROOF_RISE_METRES, 0.0)
    corners = [(-half, 0.0, half), (half, 0.0, half), (half, 0.0, -half), (-half, 0.0, -half)]
    mesh = Mesh()
    for index in range(4):
        left, right = corners[index], corners[(index + 1) % 4]
        mesh.face([left, right, apex], [(0.0, 2.0), (float(ROOF_FOOTPRINT), 2.0),
                                        (ROOF_FOOTPRINT / 2, 0.0)])
    return mesh


# --- glTF binaire ------------------------------------------------------------------------------
def _pad(data: bytes, filler: bytes) -> bytes:
    return data + filler * ((4 - len(data) % 4) % 4)


def encode_glb(name: str, mesh: Mesh, texture: bytes) -> bytes:
    """Un `.glb` autonome : un maillage, une primitive, une matière, sa texture incorporée."""
    blobs = [
        b"".join(struct.pack("<3f", *position) for position in mesh.positions),
        b"".join(struct.pack("<3f", *normal) for normal in mesh.normals),
        b"".join(struct.pack("<2f", *uv) for uv in mesh.uvs),
        b"".join(struct.pack("<H", index) for index in mesh.indices),
        texture,
    ]
    views = []
    binary = bytearray()
    for blob in blobs:
        views.append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(blob)})
        binary += _pad(blob, b"\x00")
    count = len(mesh.positions)
    # Les bornes sont celles des flottants 32 bits écrits, pas des nombres Python qui les ont donnés.
    stored = [struct.unpack("<3f", struct.pack("<3f", *position)) for position in mesh.positions]
    minimum = [min(position[axis] for position in stored) for axis in range(3)]
    maximum = [max(position[axis] for position in stored) for axis in range(3)]
    document = {
        "asset": {"version": "2.0", "generator": "build_mesh_fixture.py (LOT-1003)"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"name": name, "mesh": 0}],
        "meshes": [{"name": name, "primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
            "indices": 3, "material": 0, "mode": 4}]}],
        "materials": [{"name": name, "pbrMetallicRoughness": {
            "baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": 1.0}}],
        "textures": [{"sampler": 0, "source": 0}],
        "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
        "images": [{"bufferView": 4, "mimeType": "image/png"}],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": count, "type": "VEC3",
             "min": minimum, "max": maximum},
            {"bufferView": 1, "componentType": 5126, "count": count, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": count, "type": "VEC2"},
            {"bufferView": 3, "componentType": 5123, "count": len(mesh.indices),
             "type": "SCALAR"},
        ],
        "bufferViews": views,
        "buffers": [{"byteLength": len(binary)}],
    }
    text = _pad(json.dumps(document, separators=(",", ":")).encode("utf-8"), b" ")
    body = (struct.pack("<I4s", len(text), b"JSON") + text
            + struct.pack("<I4s", len(binary), b"BIN\x00") + bytes(binary))
    return struct.pack("<4sII", b"glTF", 2, 12 + len(body)) + body


# --- Le pantin (LOT-1005) ----------------------------------------------------------------------
PUPPET = "pantin"
PUPPET_HEIGHT = 1.8
PUPPET_HALF = 0.2                    # demi-largeur du corps, en mètres
# Les os, parents avant enfants : (nom, parent, position de l'articulation dans le maillage).
PUPPET_BONES = [
    ("Root", "", (0.0, 0.0, 0.0)),
    ("spine_01", "Root", (0.0, 0.9, 0.0)),
    ("head", "spine_01", (0.0, 1.5, 0.0)),
]


def _quaternion(axis: str, degrees: float) -> tuple[float, float, float, float]:
    """La rotation de `degrees` autour de l'axe `axis`, en quaternion (x, y, z, w)."""
    half = math.radians(degrees) / 2
    sine = math.sin(half)
    return (sine if axis == "x" else 0.0, sine if axis == "y" else 0.0,
            sine if axis == "z" else 0.0, math.cos(half))


# Les clips : (nom, durée, boucle, image clé, canaux). Un canal est (os, chemin, clés), une clé
# (instant, valeur). Le pantin regarde vers +Z : se pencher en avant tourne autour de +X.
PUPPET_CLIPS = [
    ("idle", 1.0, True, None, [
        ("head", "rotation", [(0.0, _quaternion("x", 0)), (0.5, _quaternion("x", 10)),
                              (1.0, _quaternion("x", 0))])]),
    ("walk", 0.5, True, None, [
        ("spine_01", "rotation", [(0.0, _quaternion("y", 0)), (0.125, _quaternion("y", 20)),
                                  (0.375, _quaternion("y", -20)), (0.5, _quaternion("y", 0))])]),
    ("attack", 0.8, False, 0.4, [
        ("spine_01", "rotation", [(0.0, _quaternion("x", 0)), (0.4, _quaternion("x", 60)),
                                  (0.8, _quaternion("x", 0))])]),
    ("cast", 0.6, False, 0.3, [
        ("head", "rotation", [(0.0, _quaternion("x", 0)), (0.3, _quaternion("x", -30)),
                              (0.6, _quaternion("x", 0))])]),
    ("hit", 0.4, False, None, [
        ("spine_01", "rotation", [(0.0, _quaternion("x", 0)), (0.2, _quaternion("x", -25)),
                                  (0.4, _quaternion("x", 0))])]),
    # La chute : le corps bascule en arrière autour de ses pieds et se soulève de sa demi-épaisseur,
    # pour reposer sur le sol.
    ("death", 0.8, False, None, [
        ("Root", "rotation", [(0.0, _quaternion("x", 0)), (0.8, _quaternion("x", -90))]),
        ("Root", "translation", [(0.0, (0.0, 0.0, 0.0)), (0.8, (0.0, PUPPET_HALF, 0.0))])]),
]


def puppet_texture() -> list[tuple[int, int, int, int]]:
    """Un aplat vert feuillage : aucune autre pièce de la carte d'essai n'a cette teinte."""
    return [_shade(LEAF, 1.0)] * (TEXTURE_SIDE * TEXTURE_SIDE)


def _box(mesh: Mesh, half: float, bottom: float, top: float) -> None:
    flanks = [((-half, half), (half, half)), ((half, half), (half, -half)),
              ((half, -half), (-half, -half)), ((-half, -half), (-half, half))]
    uvs = [(0.0, 1.0), (1.0, 1.0), (1.0, 0.0), (0.0, 0.0)]
    for (lx, lz), (rx, rz) in flanks:
        mesh.face([(lx, bottom, lz), (rx, bottom, rz), (rx, top, rz), (lx, top, lz)], uvs)
    mesh.face([(-half, top, -half), (-half, top, half), (half, top, half), (half, top, -half)], uvs)
    mesh.face([(-half, bottom, half), (-half, bottom, -half), (half, bottom, -half),
               (half, bottom, half)], uvs)


def puppet_mesh() -> tuple[Mesh, list[int]]:
    """Trois blocs empilés -- jambes, buste, tête -- et l'os que suit chaque sommet."""
    mesh = Mesh()
    bones: list[int] = []
    blocks = [(PUPPET_HALF, 0.0, 0.9), (PUPPET_HALF, 0.9, 1.5), (PUPPET_HALF * 0.75, 1.5,
                                                                PUPPET_HEIGHT)]
    for bone, (half, bottom, top) in enumerate(blocks):
        before = len(mesh.positions)
        _box(mesh, half, bottom, top)
        bones += [bone] * (len(mesh.positions) - before)
    return mesh, bones


def encode_puppet_glb() -> bytes:
    """Le pantin : un maillage lié à trois os, ses clips, sa texture incorporée."""
    mesh, bones = puppet_mesh()
    names = [name for name, _, _ in PUPPET_BONES]
    joint_nodes = {name: index + 1 for index, name in enumerate(names)}  # le nœud 0 est le maillage
    blobs = [
        b"".join(struct.pack("<3f", *position) for position in mesh.positions),
        b"".join(struct.pack("<3f", *normal) for normal in mesh.normals),
        b"".join(struct.pack("<2f", *uv) for uv in mesh.uvs),
        b"".join(struct.pack("<H", index) for index in mesh.indices),
        encode_png(TEXTURE_SIDE, TEXTURE_SIDE, puppet_texture()),
        b"".join(struct.pack("<4B", bone, 0, 0, 0) for bone in bones),
        b"".join(struct.pack("<4f", 1.0, 0.0, 0.0, 0.0) for _ in bones),
        # Les matrices de liaison inverses, en colonnes : la translation opposée à l'articulation.
        b"".join(struct.pack("<16f", 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -x, -y, -z, 1)
                 for _, _, (x, y, z) in PUPPET_BONES),
    ]
    count = len(mesh.positions)
    stored = [struct.unpack("<3f", struct.pack("<3f", *position)) for position in mesh.positions]
    accessors = [
        {"bufferView": 0, "componentType": 5126, "count": count, "type": "VEC3",
         "min": [min(position[axis] for position in stored) for axis in range(3)],
         "max": [max(position[axis] for position in stored) for axis in range(3)]},
        {"bufferView": 1, "componentType": 5126, "count": count, "type": "VEC3"},
        {"bufferView": 2, "componentType": 5126, "count": count, "type": "VEC2"},
        {"bufferView": 3, "componentType": 5123, "count": len(mesh.indices), "type": "SCALAR"},
        {"bufferView": 5, "componentType": 5121, "count": count, "type": "VEC4"},
        {"bufferView": 6, "componentType": 5126, "count": count, "type": "VEC4"},
        {"bufferView": 7, "componentType": 5126, "count": len(PUPPET_BONES), "type": "MAT4"},
    ]
    animations = []
    for clip, _, _, _, channels in PUPPET_CLIPS:
        animation = {"name": clip, "channels": [], "samplers": []}
        for bone, path, keys in channels:
            times = b"".join(struct.pack("<f", instant) for instant, _ in keys)
            values = b"".join(struct.pack(f"<{len(value)}f", *value) for _, value in keys)
            for blob, kind in ((times, "SCALAR"), (values, "VEC4" if path == "rotation" else "VEC3")):
                accessor = {"bufferView": len(blobs), "componentType": 5126, "count": len(keys),
                            "type": kind}
                if kind == "SCALAR":
                    accessor |= {"min": [keys[0][0]], "max": [keys[-1][0]]}
                accessors.append(accessor)
                blobs.append(blob)
            animation["samplers"].append({"input": len(accessors) - 2, "output": len(accessors) - 1,
                                          "interpolation": "LINEAR"})
            animation["channels"].append({"sampler": len(animation["samplers"]) - 1,
                                          "target": {"node": joint_nodes[bone], "path": path}})
        animations.append(animation)
    views = []
    binary = bytearray()
    for blob in blobs:
        views.append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(blob)})
        binary += _pad(blob, b"\x00")
    nodes = [{"name": PUPPET, "mesh": 0, "skin": 0}]
    for name, parent, (x, y, z) in PUPPET_BONES:
        px, py, pz = next((position for other, _, position in PUPPET_BONES if other == parent),
                          (0.0, 0.0, 0.0))
        node = {"name": name, "translation": [x - px, y - py, z - pz]}
        children = [joint_nodes[child] for child, above, _ in PUPPET_BONES if above == name]
        if children:
            node["children"] = children
        nodes.append(node)
    document = {
        "asset": {"version": "2.0", "generator": "build_mesh_fixture.py (LOT-1005)"},
        "scene": 0,
        "scenes": [{"nodes": [0, joint_nodes["Root"]]}],
        "nodes": nodes,
        "meshes": [{"name": PUPPET, "primitives": [{
            "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2, "JOINTS_0": 4,
                           "WEIGHTS_0": 5},
            "indices": 3, "material": 0, "mode": 4}]}],
        "skins": [{"name": PUPPET, "joints": [joint_nodes[name] for name in names],
                   "inverseBindMatrices": 6, "skeleton": joint_nodes["Root"]}],
        "animations": animations,
        "materials": [{"name": PUPPET, "pbrMetallicRoughness": {
            "baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": 1.0}}],
        "textures": [{"sampler": 0, "source": 0}],
        "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
        "images": [{"bufferView": 4, "mimeType": "image/png"}],
        "accessors": accessors,
        "bufferViews": views,
        "buffers": [{"byteLength": len(binary)}],
    }
    text = _pad(json.dumps(document, separators=(",", ":")).encode("utf-8"), b" ")
    body = (struct.pack("<I4s", len(text), b"JSON") + text
            + struct.pack("<I4s", len(binary), b"BIN\x00") + bytes(binary))
    return struct.pack("<4sII", b"glTF", 2, 12 + len(body)) + body


def puppet_files() -> dict[str, bytes]:
    """Le pantin, sa fiche et la description de son squelette."""
    clips = []
    for name, duration, loop, key, _ in PUPPET_CLIPS:
        clip = {"name": name, "duration": duration, "loop": loop}
        if key is not None:
            clip["key"] = key
        clips.append(clip)
    skeleton = {
        "version": 1,
        "silhouette": PUPPET,
        "comment": "Le squelette du pantin d'essai (LOT-1005), écrit par scripts/assetsGeneration/"
                   "build_mesh_fixture.py, jamais à la main.",
        "bones": [{"name": name, "parent": parent} for name, parent, _ in PUPPET_BONES],
        "clips": clips,
    }
    sheet = {"version": 1, "model": f"{PUPPET}.glb", "skeleton": PUPPET}

    def text(document: dict) -> bytes:
        return (json.dumps(document, indent=2, ensure_ascii=False) + "\n").encode("utf-8")

    return {
        f"Assets/Npc/{PUPPET}/{PUPPET}.glb": encode_puppet_glb(),
        f"Assets/Npc/{PUPPET}/character.json": text(sheet),
        f"Assets/Common/Characters/Skeletons/{PUPPET}/skeleton.json": text(skeleton),
    }


# --- Le lieu et la carte -----------------------------------------------------------------------
def manifest() -> str:
    width, height = ART_TILE
    document = {
        "version": 1,
        "disposition": PLACE,
        "comment": "Le lieu d'essai en maillages du LOT-1003 : trois clés citent un maillage "
                   "(\"mesh\"), une une image. Écrit par scripts/assetsGeneration/"
                   "build_mesh_fixture.py, jamais à la main.",
        "tile": [width, height],
        "storey": STOREY_ART,
        "textures": {
            f"scene/{PLACE}/floor": {"mesh": "floor.glb", "class": "floor", "footprint": [1, 1]},
            f"scene/{PLACE}/paving": {"file": "paving.png", "class": "floor",
                                      "footprint": [1, 1], "size": [width, height],
                                      "anchor": [width // 2, 0]},
            f"scene/{PLACE}/wall": {"mesh": "wall.glb", "class": "tall", "footprint": [1, 1],
                                    "tactical": "solid"},
            f"scene/{PLACE}/roof": {"mesh": "roof.glb", "class": "wide",
                                    "footprint": [ROOF_FOOTPRINT, ROOF_FOOTPRINT],
                                    "tactical": "open"},
        },
    }
    return json.dumps(document, indent=2, ensure_ascii=False) + "\n"


def _in(box: tuple[int, int, int, int], column: int, row: int) -> bool:
    return box[0] <= column <= box[2] and box[1] <= row <= box[3]


def _is_wall(column: int, row: int) -> bool:
    centre = ((BLOCK[0] + BLOCK[2]) // 2, (BLOCK[1] + BLOCK[3]) // 2)
    return _in(BLOCK, column, row) and (column, row) != centre


def level() -> str:
    cells = [(column, row) for row in range(MAP_ROWS) for column in range(MAP_COLUMNS)]
    walls = [cell for cell in cells if _is_wall(*cell)]

    def line(entry: dict) -> str:
        return "        " + json.dumps(entry, ensure_ascii=False)

    ground = ",\n".join(line({"x": column, "y": row, "type": "pavement",
                              "piece": "floor" if _in(COURT, column, row) else "paving"})
                        for column, row in cells)
    relief = ",\n".join(line({"x": column, "y": row, "type": "wall", "piece": "wall"})
                        for column, row in walls)
    collision = ",\n".join("    " + json.dumps({"x": column, "y": row, "type": "wall"})
                           for column, row in walls)
    roof = line({"x": BLOCK[0], "y": BLOCK[1], "type": "wall", "piece": "roof"})
    # Les entités, une propriété par ligne : la forme canonique de l'éditeur, sans quoi
    # `LevelEditor --check` dirait la carte à migrer.
    entities = ",\n".join(
        "\n".join("    " + text for text in json.dumps(
            {"id": identifier, "type": "npc", "x": column, "y": row, "figure": PUPPET},
            indent=2).splitlines())
        for identifier, column, row in (("e1", 5, 6), ("e2", 5, 0)))
    return f"""{{
  "version": 4,
  "name": "ilot",
  "width": {MAP_COLUMNS},
  "height": {MAP_ROWS},
  "nextEntityId": 3,
  "tiles": [
{collision},
    {json.dumps({"x": 0, "y": MAP_ROWS - 1, "type": "entry"})}
  ],
  "layers": [
    {{
      "name": "sol",
      "kind": "ground",
      "scene": "{PLACE}",
      "tiles": [
{ground}
      ]
    }},
    {{
      "name": "relief",
      "kind": "decor",
      "tiles": [
{relief}
      ]
    }},
    {{
      "name": "toit",
      "kind": "decor",
      "floor": 1,
      "tiles": [
{roof}
      ]
    }}
  ],
  "entities": [
{entities}
  ]
}}
"""


# --- La figurine témoin ------------------------------------------------------------------------
def figure_image() -> bytes:
    """Une silhouette de 1,80 m dans sa cellule : un corps d'eau sourde, une tête de sable.

    Aucune autre pièce de la carte n'a cette teinte : un test compte ses pixels pour savoir ce que
    le tampon de profondeur en a laissé voir.
    """
    width, height = FIGURE_CELL
    top = FIGURE_GROUND - FIGURE_BODY
    pixels = []
    for y in range(height):
        for x in range(width):
            head = (x + 0.5 - width / 2) ** 2 + (y + 0.5 - (top + 5)) ** 2 <= 25
            body = 10 <= x < 22 and top + 10 <= y < FIGURE_GROUND
            pixels.append(_shade(SAND, 1.0) if head else _shade(TEAL, 1.0) if body
                          else (0, 0, 0, 0))
    return encode_png(width, height, pixels)


def figure_files() -> dict[str, bytes]:
    """L'image témoin, rangée comme un effet : une bande d'une image, et le manifeste des effets.

    L'atelier à plat des PNJ (`Npc/`) ne déclare plus que le pantin, qui est un modèle.
    """
    width, height = FIGURE_CELL
    effects = {
        "version": 1,
        "disposition": "common-fx",
        "comment": "L'image témoin de la carte d'essai en maillages (LOT-1003), rangée comme un "
                   "effet depuis le LOT-1006, écrite par scripts/assetsGeneration/"
                   "build_mesh_fixture.py.",
        "tile": list(ART_TILE),
        "ground": FIGURE_GROUND,
        "textures": {"fx/temoin": {"file": "temoin.png", "class": "tall", "footprint": [1, 1],
                                   "size": [width, height]}},
    }
    workshop = {
        "version": 1,
        "comment": "Les PNJ de la carte d'essai : le pantin, un modèle (LOT-1005).",
        "tile": list(ART_TILE),
        "npcs": [PUPPET],
    }
    description = {"version": 1, "frameWidth": width, "frameHeight": height,
                   "clips": {"temoin": {"frames": [0], "frameDuration": 0.5, "loop": True}}}

    def text(document: dict) -> bytes:
        return (json.dumps(document, indent=2, ensure_ascii=False) + "\n").encode("utf-8")

    return {
        "Assets/Npc/manifest.json": text(workshop),
        "Assets/Common/Fx/manifest.json": text(effects),
        "Assets/Common/Fx/temoin.png": figure_image(),
        "Assets/Common/Fx/temoin.anim.json": text(description),
    }


def files() -> dict[str, bytes]:
    """Chaque fichier des données d'essai, par son chemin relatif à la fixture."""
    side = TEXTURE_SIDE
    scene = f"Assets/Scene/{PLACE}"
    return figure_files() | puppet_files() | {
        f"{scene}/floor.glb": encode_glb("floor", floor_mesh(),
                                         encode_png(side, side, paving_texture())),
        f"{scene}/wall.glb": encode_glb("wall", wall_mesh(),
                                        encode_png(side, side, stone_texture())),
        f"{scene}/roof.glb": encode_glb("roof", roof_mesh(),
                                        encode_png(side, side, tile_texture())),
        f"{scene}/paving.png": paving_image(),
        f"{scene}/manifest.json": manifest().encode("utf-8"),
        f"Levels/{PLACE}.json": level().encode("utf-8"),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="vérifie que les données d'essai sont à jour, sans rien écrire")
    arguments = parser.parse_args()

    stale = []
    for relative, content in files().items():
        path = FIXTURE / relative
        if arguments.check:
            if not path.is_file() or path.read_bytes() != content:
                stale.append(relative)
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        print(f"écrit {path.relative_to(ROOT).as_posix()} ({len(content)} octets)")
    if stale:
        print("données d'essai périmées : " + ", ".join(stale), file=sys.stderr)
        print("relancer scripts/assetsGeneration/build_mesh_fixture.py", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
