#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Ramène un modèle reçu de Meshy au standard 3D : au sol, une matière, 100 000 triangles au plus.

Un `.glb` sort de Meshy centré sur son origine, avec trois images (couleur, métal-rugosité, relief)
et, pour un maître, un à deux millions et demi de triangles. Le standard 3D
(`Planning/standards/style-3d.md`, §1 à §4) veut l'origine au sol sous le centre de l'emprise, la
couleur de base seule, et pas plus de triangles que la preuve du LOT-1000 n'en a mesuré. Ce script
fait les trois, par Blender sans fenêtre, sans une retouche à la main :

- **pose** : le centre de la boîte sur l'axe, le point le plus bas à la hauteur zéro ;
- **matière** : une seule, la couleur de base ; les images de métal, de rugosité et de relief
  partent ;
- **réduction** : décimation par effondrement d'arêtes vers `--triangles` (100 000), seulement si le
  modèle en a davantage ;
- **texture** : les octets de l'image de couleur du fichier reçu sont **remis tels quels** dans le
  fichier écrit — Blender la réencoderait, et un JPEG réencodé se dégrade.

Le maître n'est jamais modifié : la copie s'écrit ailleurs, avec un relevé (`releve.json`) de ce
qui a été mesuré avant et après. Décision de l'auteur du 2 octobre 2026, sur une planche comparant
le maître et la copie de trois modèles sous la caméra du jeu : la réduction se fait par ce script.

Usage :
    python scripts/assetsGeneration/reduce_model.py SOURCE SORTIE [--triangles 100000]
                                                    [--blender CHEMIN] [--force]

`SOURCE` est un `.glb` ou un dossier parcouru en profondeur ; `SORTIE` reçoit les copies, rangées
comme la source. Blender se trouve par `--blender`, la variable `BLENDER`, ou son emplacement par
défaut sur le poste. Dépendance : Blender 5.2 (outil de production, pas de CI).
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

BLENDER_DEFAUT = Path("D:/Blender Foundation/Blender 5.2/blender.exe")
TRIANGLES_MAX = 100_000
# Ce que la preuve du LOT-1000 a mesuré, et qu'un modèle ne dépasse pas (standard 3D, §3).
TRIANGLES_STANDARD = 103_000

GLB_MAGIC = b"glTF"
CHUNK_JSON = b"JSON"
CHUNK_BIN = b"BIN\x00"


# --- Le conteneur .glb, sans Blender ---------------------------------------------------------------
def read_glb(data: bytes) -> tuple[dict, bytes]:
    """Le document JSON et le bloc binaire d'un `.glb` 2.0."""
    magic, version, length = struct.unpack_from("<4sII", data, 0)
    if magic != GLB_MAGIC or version != 2 or length > len(data):
        raise ValueError("pas un fichier .glb 2.0")
    json_length, json_kind = struct.unpack_from("<I4s", data, 12)
    if json_kind != CHUNK_JSON:
        raise ValueError("bloc JSON absent")
    document = json.loads(data[20:20 + json_length])
    binary = b""
    offset = 20 + json_length
    if offset + 8 <= length:
        binary_length, binary_kind = struct.unpack_from("<I4s", data, offset)
        if binary_kind == CHUNK_BIN:
            binary = data[offset + 8:offset + 8 + binary_length]
    return document, binary


def write_glb(document: dict, binary: bytes) -> bytes:
    """Un `.glb` de `document` et de `binary`, chaque bloc complété à quatre octets."""
    text = json.dumps(document, separators=(",", ":")).encode("utf-8")
    text += b" " * ((4 - len(text) % 4) % 4)
    binary += b"\x00" * ((4 - len(binary) % 4) % 4)
    body = (struct.pack("<I4s", len(text), CHUNK_JSON) + text
            + struct.pack("<I4s", len(binary), CHUNK_BIN) + binary)
    return struct.pack("<4sII", GLB_MAGIC, 2, 12 + len(body)) + body


def _view_bytes(document: dict, binary: bytes, view_index: int) -> bytes:
    view = document["bufferViews"][view_index]
    start = view.get("byteOffset", 0)
    return binary[start:start + view["byteLength"]]


def base_color_image(document: dict, binary: bytes) -> tuple[bytes, str] | None:
    """Les octets et le type de l'image de couleur de base de la première matière, s'il y en a."""
    for material in document.get("materials", []):
        texture = material.get("pbrMetallicRoughness", {}).get("baseColorTexture")
        if texture is None:
            continue
        image = document["images"][document["textures"][texture["index"]]["source"]]
        if "bufferView" not in image:
            return None
        return _view_bytes(document, binary, image["bufferView"]), image.get("mimeType", "")
    return None


def replace_base_color_image(document: dict, binary: bytes, pixels: bytes,
                             mime: str) -> tuple[dict, bytes]:
    """Remplace l'image de couleur de base par `pixels`, et recale les vues de tampon qui suivent.

    Le bloc binaire est recomposé vue par vue, dans l'ordre de leurs décalages : la vue de l'image
    change de longueur, toutes celles d'après changent de place.
    """
    material = next(m for m in document["materials"]
                    if "baseColorTexture" in m.get("pbrMetallicRoughness", {}))
    texture = document["textures"][material["pbrMetallicRoughness"]["baseColorTexture"]["index"]]
    image = document["images"][texture["source"]]
    target = image["bufferView"]
    order = sorted(range(len(document["bufferViews"])),
                   key=lambda index: document["bufferViews"][index].get("byteOffset", 0))
    rebuilt = bytearray()
    for index in order:
        view = document["bufferViews"][index]
        content = pixels if index == target else _view_bytes(document, binary, index)
        rebuilt += b"\x00" * ((4 - len(rebuilt) % 4) % 4)
        view["byteOffset"] = len(rebuilt)
        view["byteLength"] = len(content)
        rebuilt += content
    if mime:
        image["mimeType"] = mime
    document["buffers"][0]["byteLength"] = len(rebuilt) + ((4 - len(rebuilt) % 4) % 4)
    return document, bytes(rebuilt)


def measure(data: bytes) -> dict:
    """Ce que le standard regarde d'un `.glb` : triangles, primitives, matières, images, boîte."""
    document, _ = read_glb(data)
    accessors = document.get("accessors", [])
    primitives = [p for mesh in document.get("meshes", []) for p in mesh.get("primitives", [])]
    triangles = 0
    low = [float("inf")] * 3
    high = [float("-inf")] * 3
    for primitive in primitives:
        position = accessors[primitive["attributes"]["POSITION"]]
        count = accessors[primitive["indices"]]["count"] if "indices" in primitive else position["count"]
        triangles += count // 3
        for axis in range(3):
            low[axis] = min(low[axis], position["min"][axis])
            high[axis] = max(high[axis], position["max"][axis])
    return {
        "triangles": triangles,
        "primitives": len(primitives),
        "materials": len(document.get("materials", [])),
        "images": len(document.get("images", [])),
        "attributes": sorted({name for p in primitives for name in p["attributes"]}),
        "minimum": low,
        "maximum": high,
        "bytes": len(data),
    }


def conformity(after: dict, limit: int) -> list[str]:
    """Les écarts d'une copie au standard ; vide si elle s'y tient."""
    faults = []
    if after["primitives"] != 1 or after["materials"] != 1:
        faults.append(f"{after['primitives']} primitive(s), {after['materials']} matière(s)")
    if after["images"] != 1:
        faults.append(f"{after['images']} image(s) incorporée(s)")
    if after["attributes"] != ["NORMAL", "POSITION", "TEXCOORD_0"]:
        faults.append("attributs " + ", ".join(after["attributes"]))
    if after["triangles"] > limit:
        faults.append(f"{after['triangles']} triangles")
    # glTF : la hauteur est +Y. Le sol à zéro, l'emprise centrée, au millimètre.
    if abs(after["minimum"][1]) > 1e-3:
        faults.append(f"sol à {after['minimum'][1]:.4f} m")
    for axis, name in ((0, "X"), (2, "Z")):
        if abs(after["minimum"][axis] + after["maximum"][axis]) > 2e-3:
            faults.append(f"emprise décentrée en {name}")
    return faults


# --- Côté Blender ----------------------------------------------------------------------------------
def _blender_main(parameters: dict) -> None:
    """Exécuté DANS Blender : pose, matière unique, décimation, export de chaque travail."""
    import bpy
    from mathutils import Matrix, Vector

    for job in parameters["jobs"]:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=job["source"])
        meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
        for other in [o for o in bpy.context.scene.objects if o.type != "MESH"]:
            bpy.data.objects.remove(other, do_unlink=True)
        bpy.ops.object.select_all(action="DESELECT")
        for mesh in meshes:
            mesh.select_set(True)
        bpy.context.view_layer.objects.active = meshes[0]
        if len(meshes) > 1:
            bpy.ops.object.join()
        model = bpy.context.view_layer.objects.active
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

        # La pose : le centre de la boîte sur l'axe, le point le plus bas au sol (Z dans Blender).
        corners = [Vector(corner) for corner in model.bound_box]
        low = Vector([min(c[axis] for c in corners) for axis in range(3)])
        high = Vector([max(c[axis] for c in corners) for axis in range(3)])
        model.data.transform(Matrix.Translation(
            Vector((-(low.x + high.x) / 2, -(low.y + high.y) / 2, -low.z))))
        model.data.update()

        # La couleur de base, cherchée avant que les matières reçues ne partent.
        image = None
        for slot in model.material_slots:
            tree = slot.material.node_tree if slot.material else None
            for node in (tree.nodes if tree else []):
                if node.type == "BSDF_PRINCIPLED" and node.inputs["Base Color"].is_linked:
                    source = node.inputs["Base Color"].links[0].from_node
                    if source.type == "TEX_IMAGE" and image is None:
                        image = source.image
        material = bpy.data.materials.new("base-color")
        material.use_nodes = True
        tree = material.node_tree
        tree.nodes.clear()
        output = tree.nodes.new("ShaderNodeOutputMaterial")
        shader = tree.nodes.new("ShaderNodeBsdfPrincipled")
        shader.inputs["Metallic"].default_value = 0.0
        shader.inputs["Roughness"].default_value = 1.0
        tree.links.new(shader.outputs["BSDF"], output.inputs["Surface"])
        if image is not None:
            texture = tree.nodes.new("ShaderNodeTexImage")
            texture.image = image
            tree.links.new(texture.outputs["Color"], shader.inputs["Base Color"])
        model.data.materials.clear()
        model.data.materials.append(material)

        model.data.calc_loop_triangles()
        triangles = len(model.data.loop_triangles)
        if triangles > job["triangles"]:
            modifier = model.modifiers.new("reduction", "DECIMATE")
            modifier.decimate_type = "COLLAPSE"
            modifier.ratio = job["triangles"] / triangles
            modifier.use_collapse_triangulate = True
            bpy.ops.object.modifier_apply(modifier=modifier.name)

        bpy.ops.export_scene.gltf(filepath=job["output"], export_format="GLB",
                                  use_selection=False, export_animations=False,
                                  export_skins=False, export_image_format="AUTO")
        print("REDUIT", job["output"], flush=True)


# --- Côté poste ------------------------------------------------------------------------------------
def find_blender(option: str | None) -> Path:
    for candidate in (option, os.environ.get("BLENDER"), str(BLENDER_DEFAUT)):
        if candidate and Path(candidate).is_file():
            return Path(candidate)
    raise SystemExit("Blender introuvable : --blender CHEMIN, ou la variable BLENDER")


def sources_of(source: Path) -> list[Path]:
    if source.is_file():
        return [source]
    return sorted(path for path in source.rglob("*.glb"))


def reduce_models(source: Path, output: Path, triangles: int, blender: Path,
                  force: bool) -> list[dict]:
    """Réduit chaque modèle de `source` dans `output` ; rend le relevé, un article par modèle."""
    root = source.parent if source.is_file() else source
    jobs = []
    for model in sources_of(source):
        target = output / model.relative_to(root)
        if target.resolve() == model.resolve():
            raise SystemExit("la sortie écraserait un maître : choisir un autre dossier")
        if force or not target.is_file() or target.stat().st_mtime < model.stat().st_mtime:
            target.parent.mkdir(parents=True, exist_ok=True)
            jobs.append({"model": model, "target": target})
    report = []
    for job in jobs:
        started = time.time()
        with tempfile.TemporaryDirectory(prefix="reduction-") as temporary:
            exported = Path(temporary) / "copie.glb"
            parameters = Path(temporary) / "parametres.json"
            parameters.write_text(json.dumps({"jobs": [{
                "source": str(job["model"].resolve()), "output": str(exported),
                "triangles": triangles}]}), encoding="utf-8")
            command = [str(blender), "-b", "--factory-startup", "-noaudio", "--python-exit-code",
                       "1", "--python", str(Path(__file__).resolve()), "--", "--blender-interne",
                       str(parameters)]
            result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8",
                                    errors="replace", check=False)
            if result.returncode != 0 or not exported.is_file():
                sys.stderr.write(result.stdout[-3000:] + result.stderr[-3000:])
                raise SystemExit(f"la réduction de {job['model'].name} a échoué")
            master = job["model"].read_bytes()
            before = measure(master)
            document, binary = read_glb(exported.read_bytes())
            # La texture du maître, telle qu'il la porte : Blender l'a réencodée à l'export.
            original = base_color_image(*read_glb(master))
            kept = False
            if original is not None and document.get("images"):
                document, binary = replace_base_color_image(document, binary, *original)
                kept = True
            copy = write_glb(document, binary)
            job["target"].write_bytes(copy)
        after = measure(copy)
        entry = {
            "model": job["target"].relative_to(output).as_posix(),
            "source": job["model"].name,
            "before": before,
            "after": after,
            "texture_kept": kept,
            "faults": conformity(after, TRIANGLES_STANDARD),
            "seconds": round(time.time() - started, 1),
        }
        report.append(entry)
        verdict = "conforme" if not entry["faults"] else "ÉCARTS : " + " ; ".join(entry["faults"])
        print(f"{entry['model']} : {before['triangles']} -> {after['triangles']} triangles, "
              f"{before['bytes'] / 1048576:.1f} -> {after['bytes'] / 1048576:.1f} Mio, "
              f"{entry['seconds']} s, {verdict}", flush=True)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("source", type=Path, help="un .glb, ou un dossier parcouru en profondeur")
    parser.add_argument("output", type=Path, help="le dossier des copies")
    parser.add_argument("--triangles", type=int, default=TRIANGLES_MAX,
                        help="le plafond de triangles d'une copie")
    parser.add_argument("--blender", help="blender.exe")
    parser.add_argument("--force", action="store_true", help="refaire les copies déjà à jour")
    arguments = parser.parse_args()
    if not arguments.source.exists():
        raise SystemExit(f"source introuvable : {arguments.source}")
    report = reduce_models(arguments.source, arguments.output, arguments.triangles,
                           find_blender(arguments.blender), arguments.force)
    if not report:
        print("toutes les copies sont à jour")
        return 0
    statement = arguments.output / "releve.json"
    previous = json.loads(statement.read_text(encoding="utf-8")) if statement.is_file() else []
    done = {entry["model"] for entry in report}
    merged = sorted([e for e in previous if e["model"] not in done] + report,
                    key=lambda entry: entry["model"])
    statement.write_text(json.dumps(merged, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    faulty = [entry["model"] for entry in report if entry["faults"]]
    print(f"{len(report)} modèle(s) réduit(s), {len(faulty)} avec écart ; relevé : {statement}")
    return 1 if faulty else 0


if __name__ == "__main__":
    if "--blender-interne" in sys.argv:
        _blender_main(json.loads(
            Path(sys.argv[sys.argv.index("--blender-interne") + 1]).read_text(encoding="utf-8")))
    else:
        sys.exit(main())
