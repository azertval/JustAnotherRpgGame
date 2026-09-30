#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Rend un personnage animé (`.glb`) en bandes de figurine, au format que lit le moteur (LOT-1000).

Le modèle vient de l'atelier des personnages (un corps commun, une texture peinte, des pièces). Ses
animations sont rendues par Blender, sans fenêtre, sous la caméra du jeu :

- **orthographique**, tournée de 45° entre les deux axes de la grille et inclinée de
  asin(0,62) ≈ 38,3° : le sol se projette exactement en losange de rapport 0,62
  (`Planning/standards/style-2d-hd.md`) ;
- **à l'échelle du losange** : une case de 1,5 m fait 256 px de large, soit 120,7 px par mètre ; un
  corps de 1,80 m mesure alors 170 px, la hauteur de figurine du standard ;
- **les pieds sur la ligne de sol** (y = 252) au milieu de la cellule : la caméra vise le point qui
  tombe là, la figurine n'est jamais recentrée après coup — ni sur sa boîte, que la hache déplace, ni
  sur son bassin ;
- **la lumière du haut à gauche**, pas d'ombre portée (le sol est l'affaire du moteur).

Chaque animation × orientation donne une bande de 8 cellules jointives (192 × 256, 384 × 256 pour
`attack` et `death`). Une boucle est échantillonnée sur son cycle entier (l'image 8 précède le
retour à l'image 1) ; une animation qui s'arrête finit sur sa dernière pose. Les images se rendent
au double de leur taille puis se réduisent en alpha prémultiplié, dans une toile plus grande que la
cellule : ce qui déborde en haut ou sur les côtés est une erreur, jamais une coupe silencieuse ; ce
qui passe sous le sol allonge la cellule vers le bas par pas de 8 px, comme à l'installation.

La sortie est un dossier de sources prêt pour `install_hd_asset.py` : les bandes, et, avec
`--figure`, un descripteur `install.json` dont les bandes sont `placed` (copiées telles quelles).

    py -3.13 scripts/assetsGeneration/render_character_strips.py MODELE.glb SORTIE \\
        [--clips idle,walk] [--facings se,sw] [--figure Heroes/brawler --portrait CHEMIN] \\
        [--blender CHEMIN] [--zoom 3]

`--zoom` rend un aperçu agrandi (rien d'installable). Blender se trouve par `--blender`, la variable
`BLENDER`, ou son emplacement par défaut sur le poste.

Dépendances : Pillow et numpy ; Blender 5.2 pour le rendu (outil de production, pas de CI).
"""
from __future__ import annotations

import argparse
import json
import math
import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

CELLULE = (192, 256)
CELLULE_LARGE = (384, 256)
SOL = 252
LOSANGE = 256            # la largeur du losange de sol, en px d'art
RAPPORT = 0.62           # hauteur / largeur du losange
CASE_M = 1.5             # une case de la grille, en mètres
MARGE = 4                # px transparents à gauche et à droite de chaque cellule
SURECHANTILLON = 2       # les images se rendent au double, puis se réduisent
DEBORD = 24              # la toile de rendu dépasse la cellule d'autant, de chaque côté (px de cellule)
DESSOUS = 128            # et d'autant sous la cellule : un corps couché vers l'œil passe sous le sol
IMAGES = 8
FACINGS = ("se", "sw", "ne", "nw")
BLENDER_DEFAUT = Path("D:/Blender Foundation/Blender 5.2/blender.exe")


@dataclass(frozen=True)
class Clip:
    nom: str
    duree_image: float
    boucle: bool
    large: bool


# Les durées sont celles du moteur (et de l'atelier, qui pose les animations à ces cadences).
CLIPS = {
    "idle": Clip("idle", 0.125, True, False),
    "walk": Clip("walk", 0.0625, True, False),
    "attack": Clip("attack", 0.125, False, True),
    "hit": Clip("hit", 0.1, False, False),
    "death": Clip("death", 0.12, False, True),
}


class RenduError(RuntimeError):
    """Un rendu qui ne tient pas dans sa cellule, ou un modèle incomplet."""


# --- La caméra du jeu ----------------------------------------------------------------------------


def pixels_par_metre() -> float:
    """Le losange de 256 px est la diagonale d'une case de 1,5 m vue sous 45°."""
    return LOSANGE / (CASE_M * math.sqrt(2.0))


def elevation() -> float:
    """L'inclinaison de la caméra, en radians : le sol s'y écrase exactement de 0,62."""
    return math.asin(RAPPORT)


def repere_camera() -> dict[str, tuple[float, float, float]]:
    """Les axes de la caméra dans le monde de Blender (Z en haut).

    La colonne de la grille est +X, la rangée -Y : une colonne de plus descend à droite de l'écran
    (le sud-est), une rangée de plus descend à gauche (le sud-ouest), comme dans `IsoProjection`.
    """
    e = elevation()
    s = 1.0 / math.sqrt(2.0)
    vers_camera = (s * math.cos(e), -s * math.cos(e), math.sin(e))
    droite = (s, s, 0.0)
    haut = (-s * math.sin(e), s * math.sin(e), math.cos(e))
    return {"vue": tuple(-c for c in vers_camera), "droite": droite, "haut": haut}


def lacet_orientation(facing: str) -> float:
    """La rotation (radians, autour de Z) qui tourne un personnage modelé face à -Y vers `facing`.

    se = +colonne = +X ; sw = +rangée = -Y ; ne = +Y ; nw = -X.
    """
    return {"sw": 0.0, "se": math.pi / 2, "ne": math.pi, "nw": -math.pi / 2}[facing]


def cible_camera(cellule: tuple[int, int]) -> tuple[float, float]:
    """De combien (m, le long des axes droite et haut de la caméra) viser au-dessus du pied, pour que
    le pied tombe au milieu de la cellule sur la ligne de sol. La toile de rendu déborde de la
    cellule de `DEBORD` de chaque côté et de `DESSOUS` en plus vers le bas."""
    largeur, hauteur = cellule
    toile_h = hauteur + 2 * DEBORD + DESSOUS
    centre_y = toile_h / 2.0
    sol_y = DEBORD + SOL
    return 0.0, (sol_y - centre_y) / pixels_par_metre()


def toile(cellule: tuple[int, int]) -> tuple[int, int]:
    largeur, hauteur = cellule
    return largeur + 2 * DEBORD, hauteur + 2 * DEBORD + DESSOUS


def instants(debut: float, fin: float, boucle: bool, images: int = IMAGES) -> list[float]:
    """Les instants (en images de la scène) des `images` échantillons d'une animation."""
    pas = (fin - debut) / (images if boucle else images - 1)
    return [debut + pas * i for i in range(images)]


# --- Des pixels rendus à la bande ----------------------------------------------------------------


def reduire(rgba, facteur: int):
    """Réduit d'un facteur entier, en moyenne de blocs sur l'alpha prémultiplié."""
    import numpy as np

    if facteur == 1:
        return rgba.copy()
    h, w = rgba.shape[:2]
    if h % facteur or w % facteur:
        raise RenduError(f"image de {w} × {h} px, non divisible par {facteur}")
    f = rgba.astype(np.float64) / 255.0
    f[..., :3] *= f[..., 3:4]
    f = f.reshape(h // facteur, facteur, w // facteur, facteur, 4).mean(axis=(1, 3))
    alpha = f[..., 3:4]
    f[..., :3] = np.where(alpha > 0, f[..., :3] / np.maximum(alpha, 1e-9), 0.0)
    out = np.rint(np.clip(f, 0, 1) * 255).astype(np.uint8)
    out[out[..., 3] == 0] = 0
    return out


def cellule_depuis_toile(rendu, cellule: tuple[int, int], ou: str):
    """Découpe la cellule dans la toile rendue (à l'échelle 1) ; rend (image, rangée visible la plus basse).

    Rien ne doit dépasser en haut ni sur les côtés ; ce qui passe sous la cellule est gardé.
    """
    import numpy as np

    largeur, hauteur = cellule
    alpha = rendu[..., 3] > 0
    if not alpha.any():
        raise RenduError(f"{ou} : image vide (le modèle est-il dans le champ ?)")
    ys, xs = np.nonzero(alpha)
    if ys.min() < DEBORD:
        raise RenduError(f"{ou} : l'image dépasse le haut de la cellule de {DEBORD - ys.min()} px")
    x0, x1 = xs.min() - DEBORD, xs.max() + 1 - DEBORD
    if x0 < MARGE or x1 > largeur - MARGE:
        raise RenduError(f"{ou} : l'image occupe x {x0}-{x1} ; il faut {MARGE} px transparents à gauche et à "
                         f"droite de sa cellule de {largeur} px")
    bas = int(ys.max()) + 1 - DEBORD
    if bas > hauteur + DESSOUS:
        raise RenduError(f"{ou} : l'image passe sous la toile")
    return rendu[DEBORD:, DEBORD:DEBORD + largeur], bas


def assembler(images: list, cellule: tuple[int, int], bas: int):
    """Les cellules côte à côte ; la hauteur s'allonge par pas de 8 px pour ce qui passe sous le sol."""
    import numpy as np

    largeur, hauteur = cellule
    hauteur = max(hauteur, -(-bas // 8) * 8)
    bande = np.zeros((hauteur, largeur * len(images), 4), np.uint8)
    for i, image in enumerate(images):
        h = min(hauteur, image.shape[0])
        bande[:h, i * largeur:(i + 1) * largeur] = image[:h]
    return bande


def descripteur(figure: str, portrait: str | None, bandes: list[tuple[str, str]], cible: str) -> dict:
    """Le descripteur d'installation des bandes rendues : `placed`, copiées telles quelles."""
    strips = []
    for clip, facing in bandes:
        c = CLIPS[clip]
        strips.append({"source": f"{clip}-{facing}.png", "clip": clip, "facing": facing, "frames": IMAGES,
                       "frameDuration": c.duree_image, "loop": c.boucle, "wide": c.large, "placed": True})
    entree = {"name": figure, "strips": strips}
    if portrait is not None:
        entree["portrait"] = portrait
    return {"version": 1, "target": cible, "figures": [entree]}


# --- Côté Blender --------------------------------------------------------------------------------


def _blender_main(parametres: dict) -> None:
    """Exécuté DANS Blender : rend chaque image demandée en PNG dans `parametres['sortie']`."""
    import bpy
    from mathutils import Vector

    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj)
    scene = bpy.context.scene
    scene.render.fps = 64
    bpy.ops.import_scene.gltf(filepath=parametres["modele"])
    squelettes = [o for o in bpy.data.objects if o.type == "ARMATURE"]
    if len(squelettes) != 1:
        raise RenduError(f"le modèle porte {len(squelettes)} squelettes, il en faut un")
    squelette = squelettes[0]
    # Le personnage pivote autour de l'origine : un parent vide porte toute la hiérarchie importée.
    pivot = bpy.data.objects.new("pivot", None)
    scene.collection.objects.link(pivot)
    for obj in bpy.data.objects:
        if obj.parent is None and obj is not pivot:
            obj.parent = pivot

    scene.render.engine = "BLENDER_EEVEE"
    scene.eevee.taa_render_samples = 64
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    monde = bpy.data.worlds.new("monde")
    scene.world = monde
    monde.use_nodes = True
    monde.node_tree.nodes["Background"].inputs[0].default_value = (0.42, 0.42, 0.45, 1.0)
    monde.node_tree.nodes["Background"].inputs[1].default_value = 0.9

    axes = {k: Vector(v) for k, v in repere_camera().items()}
    # La lumière vient du haut à gauche de l'écran, un peu de face.
    for nom, direction, force in (("cle", axes["droite"] * 0.7 - axes["haut"] * 0.9 + axes["vue"] * 0.5, 3.2),
                                  ("contre", -axes["droite"] * 0.8 - axes["haut"] * 0.3 + axes["vue"] * 0.3, 0.8)):
        lumiere = bpy.data.lights.new(nom, "SUN")
        lumiere.energy = force
        lumiere.angle = math.radians(10)
        soleil = bpy.data.objects.new(nom, lumiere)
        scene.collection.objects.link(soleil)
        soleil.rotation_euler = direction.normalized().to_track_quat("-Z", "Y").to_euler()

    camera = bpy.data.objects.new("camera", bpy.data.cameras.new("camera"))
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    camera.data.sensor_fit = "HORIZONTAL"
    camera.data.clip_start = 0.1
    camera.data.clip_end = 100.0
    camera.rotation_euler = axes["vue"].to_track_quat("-Z", "Y").to_euler()

    actions = {a.name: a for a in bpy.data.actions}
    zoom = parametres["zoom"]
    for travail in parametres["travaux"]:
        clip = CLIPS[travail["clip"]]
        action = next((a for n, a in actions.items() if n == clip.nom or n.startswith(clip.nom + "_")), None)
        if action is None:
            raise RenduError(f"le modèle n'a pas d'animation « {clip.nom} » (animations : {', '.join(actions)})")
        squelette.animation_data_create()
        for piste in squelette.animation_data.nla_tracks:
            piste.mute = True
        squelette.animation_data.action = action
        cellule = CELLULE_LARGE if clip.large else CELLULE
        largeur, hauteur = toile(cellule)
        scene.render.resolution_x = largeur * SURECHANTILLON * zoom
        scene.render.resolution_y = hauteur * SURECHANTILLON * zoom
        camera.data.ortho_scale = largeur / pixels_par_metre()
        _, dy = cible_camera(cellule)
        camera.location = axes["haut"] * dy - axes["vue"] * 30.0
        pivot.rotation_euler = (0.0, 0.0, lacet_orientation(travail["facing"]))
        debut, fin = action.frame_range
        for i, instant in enumerate(instants(debut, fin, clip.boucle)):
            entier = math.floor(instant)
            scene.frame_set(entier, subframe=instant - entier)
            scene.render.filepath = os.path.join(parametres["sortie"], f"{clip.nom}-{travail['facing']}-{i}.png")
            bpy.ops.render.render(write_still=True)


# --- Côté poste ----------------------------------------------------------------------------------


def trouver_blender(option: str | None) -> Path:
    for candidat in (option, os.environ.get("BLENDER"), str(BLENDER_DEFAUT)):
        if candidat and Path(candidat).is_file():
            return Path(candidat)
    raise SystemExit("Blender introuvable : --blender CHEMIN, ou la variable BLENDER")


def rendre(modele: Path, sortie: Path, clips: list[str], facings: list[str], blender: Path, zoom: int) -> list[tuple[str, str]]:
    import numpy as np
    from PIL import Image

    travaux = [{"clip": c, "facing": f} for c in clips for f in facings]
    with tempfile.TemporaryDirectory(prefix="bandes-") as temporaire:
        parametres = {"modele": str(modele.resolve()), "sortie": temporaire, "zoom": zoom, "travaux": travaux}
        fichier = Path(temporaire) / "parametres.json"
        fichier.write_text(json.dumps(parametres), encoding="utf-8")
        commande = [str(blender), "-b", "--factory-startup", "-noaudio", "--python-exit-code", "1",
                    "--python", str(Path(__file__).resolve()), "--", "--blender-interne", str(fichier)]
        resultat = subprocess.run(commande, capture_output=True, text=True, encoding="utf-8", errors="replace")
        if resultat.returncode != 0:
            sys.stderr.write(resultat.stdout[-4000:] + resultat.stderr[-4000:])
            raise SystemExit("le rendu Blender a échoué")
        sortie.mkdir(parents=True, exist_ok=True)
        faites = []
        for travail in travaux:
            clip = CLIPS[travail["clip"]]
            cellule = CELLULE_LARGE if clip.large else CELLULE
            nom = f"{clip.nom}-{travail['facing']}"
            images, bas = [], 0
            for i in range(IMAGES):
                rendu = np.asarray(Image.open(Path(temporaire) / f"{nom}-{i}.png").convert("RGBA"))
                rendu = reduire(rendu, SURECHANTILLON)
                if zoom > 1:
                    images.append(rendu)
                    continue
                image, fond = cellule_depuis_toile(rendu, cellule, f"{nom}, image {i + 1}")
                images.append(image)
                bas = max(bas, fond)
            if zoom > 1:
                bande = np.concatenate(images, axis=1)
            else:
                bande = assembler(images, cellule, bas)
            Image.fromarray(bande, "RGBA").save(sortie / f"{nom}.png", optimize=True)
            faites.append((clip.nom, travail["facing"]))
            print(f"{nom}.png : {bande.shape[1]} × {bande.shape[0]} px")
    return faites


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("modele", type=Path, help="le personnage animé, .glb")
    parser.add_argument("sortie", type=Path, help="le dossier des bandes rendues")
    parser.add_argument("--clips", default=",".join(CLIPS), help="les animations, séparées par des virgules")
    parser.add_argument("--facings", default=",".join(FACINGS), help="les orientations")
    parser.add_argument("--figure", help="le nom de la figurine installée (Heroes/brawler) : écrit install.json")
    parser.add_argument("--portrait", help="la source du portrait, relative au dossier de sortie")
    parser.add_argument("--cible", default="Common/Characters", help="le dossier Characters/ cible")
    parser.add_argument("--blender", help="blender.exe")
    parser.add_argument("--zoom", type=int, default=1, help="aperçu agrandi (rien d'installable)")
    args = parser.parse_args(argv)

    clips = [c for c in args.clips.split(",") if c]
    facings = [f for f in args.facings.split(",") if f]
    inconnus = [c for c in clips if c not in CLIPS] + [f for f in facings if f not in FACINGS]
    if inconnus:
        parser.error(f"inconnu(s) : {', '.join(inconnus)}")
    if not args.modele.is_file():
        parser.error(f"{args.modele} : modèle absent")
    if args.figure and args.zoom != 1:
        parser.error("--figure veut des bandes installables, sans --zoom")
    faites = rendre(args.modele, args.sortie, clips, facings, trouver_blender(args.blender), args.zoom)
    if args.figure:
        texte = json.dumps(descripteur(args.figure, args.portrait, faites, args.cible), indent=2, ensure_ascii=False)
        (args.sortie / "install.json").write_text(texte + "\n", encoding="utf-8", newline="\n")
        print(f"install.json : {args.figure}, {len(faites)} bandes placées")
    return 0


if __name__ == "__main__":
    if "--blender-interne" in sys.argv:
        _blender_main(json.loads(Path(sys.argv[sys.argv.index("--blender-interne") + 1]).read_text(encoding="utf-8")))
    else:
        sys.exit(main())
