#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Atelier des textures (LOT-92, T3) : les planches de scène d'un lieu, déclarées par sa disposition.

`extract_coliseum_atlas.py` (LOT-50) découpait UNE planche à des coordonnées écrites à la main,
relevées sur l'image après coup. Ici la disposition vient AVANT l'image : un JSON
(`atelier/dispositions/<id>.json`) déclare la taille de ses planches et leur pas de pixel, puis
chaque cellule -- son nom, sa classe (sol, pièce haute, grande pièce), son emprise en cases et la
hauteur qu'elle prend au-dessus du sol -- et tout le reste s'en déduit : la répartition des cellules
sur une ou plusieurs planches, le gabarit de chaque planche, le bloc C du prompt, la découpe et la
clé de chaque texture. Aucune coordonnée n'est écrite à la main. Un lieu sans cellules propres
nomme un modèle (`dispositions/modeles/`) : sa planche se commande depuis sa seule fiche d'atlas.

La géométrie est celle d'`IsoProjection` : un losange de 68 × 42 pixels d'art (rapport 0,62). Une
emprise de a × b cases est un parallélogramme de (a + b) · 34 × (a + b) · 21 pixels d'art ; son
sommet haut est le coin (0, 0) de la grille, un pas en x descend de (34, 21), un pas en y de
(−34, 21). Une pièce haute ajoute sa hauteur au-dessus. Le pas (`sheet.scale`, pixels d'écran par
pixel d'art) ne change pas la texture installée, qui est en pixels d'art : il donne au générateur
plus de pixels pour dessiner chacun d'eux, et la réduction en est plus propre.

La taille d'une planche suit les contraintes du générateur (`gpt-image-2` et suivants) : côtés
multiples de 16, au plus 3840 px, rapport au plus 3 : 1, entre 655 360 et 8 294 400 pixels ; au-delà
de 2560 × 1440 (3 686 400 pixels), le rendu est donné pour expérimental.

Commandes :
    py -3.13 scripts/extract_texture_sheet.py commande <id> <tour>
        prépare l'envoi à la main de chaque planche : prompt (blocs A, B, C), maquette et gabarit,
        sous <TEXTURE_ATELIER>/chatgpt/<id>-tour<K>/planche-<N>/ ;
    py -3.13 scripts/extract_texture_sheet.py decoupe <id> <planche-1.png> [<planche-2.png> ...]
        met les planches reçues au format, les découpe et les installe sous `installRoot` ;
    py -3.13 scripts/extract_texture_sheet.py --check <id>
        refait la découpe en mémoire depuis les planches installées et compare, SANS rien écrire ;
    py -3.13 scripts/extract_texture_sheet.py valider
        contrôle toutes les dispositions (sans Pillow : c'est ce que `check_assets_brief.py` appelle).

Dépendances : aucune pour `valider` ; Pillow et numpy pour le reste (outil de production).
"""

from __future__ import annotations

import hashlib
import io
import json
import os
import re
import shutil
import sys
from pathlib import Path

RACINE = Path(__file__).resolve().parent.parent
ATELIER = RACINE / "Documentation" / "Lot" / "LOT-92-atelier-textures" / "atelier"
DISPOSITIONS = ATELIER / "dispositions"
MODELES = DISPOSITIONS / "modeles"
STYLE = ATELIER / "prompts" / "style.txt"
MAQUETTE = ATELIER / "ancres" / "maquette.png"
PAS_MAQUETTE = 2
LIEUX = RACINE / "Source" / "Elements" / "World" / "locations"
TRAVAIL = Path(os.environ.get("TEXTURE_ATELIER", r"D:/JustAnotherDnDGame-textures"))

DEMI_L, DEMI_H = 34, 21     # demi-losange, pixels d'art (68 × 42, IsoProjection 0,62)
MARGE = 4                   # pixels d'art autour de chaque cellule
COULEURS = 64               # palette commune à toutes les planches d'un lieu
# Part minimale du losange qu'un sol doit remplir. Au tour 1 du Colisée, quinze sols pleins à l'œil
# couvraient de 96,9 % à 99,8 % : l'arête d'un losange de 68 × 42 perd ses pixels à l'arrondi.
COUVERTURE_SOL = 0.95
ARETES = {"left": ((3, 0),), "right": ((0, 1),), "both": ((3, 0), (0, 1))}   # indices de sommets
ALPHA = 127                 # seuil de binarisation, comme au LOT-91

# Le générateur (documentation d'OpenAI, gpt-image-2 et suivants).
COTE_MULTIPLE, COTE_MAX, RAPPORT_MAX = 16, 3840, 3
PIXELS_MIN, PIXELS_MAX, PIXELS_FIABLES = 655_360, 8_294_400, 3_686_400

CLE_RE = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*(/[a-z0-9]+(-[a-z0-9]+)*)+$")   # LOT-39
NOM_RE = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")


class DispositionError(ValueError):
    """Une disposition qui ne se lit pas comme ce module l'attend."""


# -- La disposition, sans image ------------------------------------------------------------------

def charger(identifiant: str) -> dict:
    chemin = DISPOSITIONS / f"{identifiant}.json"
    if not chemin.is_file():
        raise DispositionError(f"disposition « {identifiant} » introuvable ({chemin}).")
    return resoudre(json.loads(chemin.read_text(encoding="utf-8")))


def resoudre(disposition: dict) -> dict:
    """La disposition complète : celle du fichier, sur son modèle (`model`) s'il en nomme un.

    Un lieu qui n'a rien de plus que sa fiche d'atlas -- Martpart, au T5 -- ne se rédige pas : sa
    disposition nomme le lieu et un modèle (`dispositions/modeles/<nom>.json`), qui porte la taille
    des planches, les classes, le sujet et des cellules neutres (« un objet typique du lieu décrit
    plus haut ») ; le bloc B, la fiche entière, dit au générateur ce qui rend le lieu reconnaissable.
    Le titre vient du nom de la fiche, la clé et le dossier d'installation de l'identifiant. Tout
    champ écrit dans le fichier l'emporte sur le modèle et sur ces valeurs déduites.
    """
    nom = disposition.get("model")
    if nom is None:
        return disposition
    chemin = MODELES / f"{nom}.json"
    if not NOM_RE.match(str(nom)) or not chemin.is_file():
        raise DispositionError(f"{disposition.get('id', '?')} : modèle « {nom} » introuvable ({chemin}).")
    modele = json.loads(chemin.read_text(encoding="utf-8"))
    ident = disposition.get("id", "")
    deduits: dict = {"keyPrefix": f"scene/{ident}", "installRoot": f"Source/Elements/Assets/Scene/{ident}"}
    fiche = LIEUX / f"{disposition.get('location', '')}.json"
    if fiche.is_file():
        deduits["title"] = json.loads(fiche.read_text(encoding="utf-8"))["name"]
    return {**modele, **deduits, **disposition}


def taille_planche(disposition: dict) -> tuple[int, int]:
    w, h = disposition["sheet"]["size"]
    return int(w), int(h)


def pas(disposition: dict) -> int:
    return int(disposition["sheet"]["scale"])


def source_du_miroir(disposition: dict, cellule: dict) -> dict | None:
    """La cellule dessinée dont `cellule` est le miroir, ou None."""
    nom = cellule.get("mirrorOf")
    return next((c for c in disposition["cells"] if c.get("name") == nom), None) if nom else None


def dessinees(disposition: dict) -> list[dict]:
    """Les cellules que le générateur dessine : toutes, sauf les miroirs (`mirrorOf`).

    Aux tours 1 et 2 du Colisée, la seconde pièce d'une paire orientée (« the same … ») a été
    recopiée dans le sens de la première au lieu d'être mise en miroir, une fois sur deux. Décision
    de l'auteur : une seule orientation se dessine, l'autre est son retournement horizontal, au
    prix d'une lumière venue d'en haut à droite sur la pièce retournée.
    """
    return [c for c in disposition["cells"] if "mirrorOf" not in c]


def emprise(disposition: dict, cellule: dict) -> tuple[int, int]:
    source = source_du_miroir(disposition, cellule)
    if source is not None:
        a, b = emprise(disposition, source)
        return b, a
    a, b = cellule.get("footprint", disposition["classes"][cellule["class"]]["footprint"])
    return int(a), int(b)


def canevas(disposition: dict, cellule: dict) -> tuple[int, int]:
    """Taille de la texture, en pixels d'art : l'emprise, et la hauteur au-dessus."""
    a, b = emprise(disposition, cellule)
    hausse = disposition["classes"][cellule["class"]]["rise"]
    return (a + b) * DEMI_L, (a + b) * DEMI_H + hausse


def sommets(disposition: dict, cellule: dict) -> list[tuple[int, int]]:
    """Haut, droite, bas, gauche de l'emprise, en pixels d'art dans le canevas."""
    a, b = emprise(disposition, cellule)
    hausse = disposition["classes"][cellule["class"]]["rise"]
    haut = (b * DEMI_L, hausse)
    droite = (haut[0] + a * DEMI_L, hausse + a * DEMI_H)
    gauche = (haut[0] - b * DEMI_L, hausse + b * DEMI_H)
    bas = (haut[0] + (a - b) * DEMI_L, hausse + (a + b) * DEMI_H)
    return [haut, droite, bas, gauche]


def grille(disposition: dict) -> list[dict]:
    """Les cellules posées sur les planches, dans l'ordre de la disposition.

    Rangées remplies de gauche à droite ; une rangée prend la hauteur de sa plus haute cellule ; une
    rangée qui ne tient plus sur la planche ouvre la suivante. Chaque entrée : la cellule,
    `planche` (numéro à partir de 1), `boite` (x0, y0, x1, y1) marge comprise et `origine`, le coin
    haut-gauche du canevas, en pixels d'écran ; le canevas est posé en bas de sa boîte.
    """
    largeur, hauteur = taille_planche(disposition)
    s = pas(disposition)
    poses: list[dict] = []
    numero, x, y, h_rangee, rangee = 1, 0, 0, 0, []

    def fermer():
        nonlocal numero, y
        if not rangee:
            return
        if y + h_rangee > hauteur:
            numero, y = numero + 1, 0
        for x0, w, cellule in rangee:
            _, ch = canevas(disposition, cellule)
            poses.append({
                "cellule": cellule,
                "planche": numero,
                "boite": (x0, y, x0 + w, y + h_rangee),
                "origine": (x0 + MARGE * s, y + h_rangee - MARGE * s - ch * s),
            })
        y += h_rangee

    for cellule in dessinees(disposition):
        cw, ch = canevas(disposition, cellule)
        w, h = (cw + 2 * MARGE) * s, (ch + 2 * MARGE) * s
        if w > largeur or h > hauteur:
            raise DispositionError(f"{disposition['id']} : la cellule « {cellule['name']} » "
                                   f"({w} × {h} px) ne tient pas dans une planche de "
                                   f"{largeur} × {hauteur} au pas {s}.")
        if x + w > largeur:
            fermer()
            x, h_rangee, rangee = 0, 0, []
        rangee.append((x, w, cellule))
        x += w
        h_rangee = max(h_rangee, h)
    fermer()
    return poses


def planches(disposition: dict) -> int:
    return max(p["planche"] for p in grille(disposition))


def cle(disposition: dict, cellule: dict) -> str:
    return f"{disposition['keyPrefix']}/{cellule['name']}"


def fautes_de_taille(largeur: int, hauteur: int) -> list[str]:
    fautes = []
    if largeur % COTE_MULTIPLE or hauteur % COTE_MULTIPLE:
        fautes.append(f"côtés multiples de {COTE_MULTIPLE} attendus")
    if max(largeur, hauteur) > COTE_MAX:
        fautes.append(f"côté au plus {COTE_MAX}")
    if max(largeur, hauteur) > RAPPORT_MAX * min(largeur, hauteur):
        fautes.append(f"rapport au plus {RAPPORT_MAX} : 1")
    if not PIXELS_MIN <= largeur * hauteur <= PIXELS_MAX:
        fautes.append(f"entre {PIXELS_MIN} et {PIXELS_MAX} pixels")
    return fautes


def valider(disposition: dict) -> list[str]:
    """Les fautes d'une disposition ; vide si elle est conforme."""
    fautes: list[str] = []
    ident = disposition.get("id", "?")
    if disposition.get("version") != 1:
        fautes.append(f"{ident} : version {disposition.get('version')!r}, 1 attendue.")
    for champ in ("id", "title", "location", "subject", "keyPrefix", "installRoot", "sheet",
                  "classes", "cells"):
        if champ not in disposition:
            fautes.append(f"{ident} : champ « {champ} » absent.")
    if fautes:
        return fautes
    try:
        largeur, hauteur = taille_planche(disposition)
        if pas(disposition) < 1:
            fautes.append(f"{ident} : pas de pixel {pas(disposition)}, au moins 1.")
        for f in fautes_de_taille(largeur, hauteur):
            fautes.append(f"{ident} : planche {largeur} × {hauteur} refusée par le générateur ({f}).")
    except (KeyError, TypeError, ValueError):
        fautes.append(f"{ident} : « sheet » attend « size » [largeur, hauteur] et « scale ».")
        return fautes
    if not CLE_RE.match(disposition["keyPrefix"] + "/x"):
        fautes.append(f"{ident} : préfixe de clé « {disposition['keyPrefix']} » invalide (LOT-39).")
    if not disposition["keyPrefix"].startswith("scene/"):
        fautes.append(f"{ident} : une texture de scène a une clé « scene/… ».")
    fiche = LIEUX / f"{disposition['location']}.json"
    if not fiche.is_file():
        fautes.append(f"{ident} : lieu « {disposition['location']} » absent de l'atlas : le bloc B "
                      "se rédige depuis sa fiche.")
    else:
        description = json.loads(fiche.read_text(encoding="utf-8"))["description"]
        extraits = disposition.get("locationExcerpt", [])
        if not isinstance(extraits, list) or not all(isinstance(e, str) and e for e in extraits):
            fautes.append(f"{ident} : « locationExcerpt » attend une liste de phrases.")
        else:
            for extrait in extraits:
                if extrait not in description:
                    fautes.append(f"{ident} : extrait absent de la fiche « {disposition['location']} » "
                                  f"(le bloc B cite le livre, il ne le récrit pas) : « {extrait[:60]}… »")
    for nom, classe in disposition["classes"].items():
        a, b = classe.get("footprint", (0, 0))
        if a < 1 or b < 1 or classe.get("rise", -1) < 0:
            fautes.append(f"{ident} : classe « {nom} » : emprise ou hauteur invalide.")
    vus: set[str] = set()
    for cellule in disposition["cells"]:
        nom = cellule.get("name", "")
        if not NOM_RE.match(nom):
            fautes.append(f"{ident} : nom de cellule « {nom} » invalide.")
        if nom in vus:
            fautes.append(f"{ident} : cellule « {nom} » déclarée deux fois.")
        vus.add(nom)
        if cellule.get("class") not in disposition["classes"]:
            fautes.append(f"{ident} : cellule « {nom} » : classe « {cellule.get('class')} » inconnue.")
        if "mirrorOf" in cellule:
            source = source_du_miroir(disposition, cellule)
            if source is None or "mirrorOf" in source:
                fautes.append(f"{ident} : cellule « {nom} » : « mirrorOf » doit nommer une cellule "
                              "dessinée.")
            elif source.get("class") != cellule.get("class"):
                fautes.append(f"{ident} : cellule « {nom} » : classe différente de sa source.")
            continue
        if cellule.get("stance", "along") not in ("along", "toward"):
            fautes.append(f"{ident} : cellule « {nom} » : « stance » vaut along ou toward.")
        if not cellule.get("prompt", "").strip():
            fautes.append(f"{ident} : cellule « {nom} » sans description pour le générateur.")
        if "edge" in cellule and cellule["edge"] not in ARETES:
            fautes.append(f"{ident} : cellule « {nom} » : arête « {cellule['edge']} » inconnue "
                          f"({', '.join(ARETES)}).")
        if "footprint" in cellule and min(cellule["footprint"]) < 1:
            fautes.append(f"{ident} : cellule « {nom} » : emprise invalide.")
    if not fautes:
        try:
            grille(disposition)
        except DispositionError as erreur:
            fautes.append(str(erreur))
    return fautes


def valider_tout() -> tuple[int, list[str]]:
    """Toutes les dispositions de l'atelier ; clés uniques entre elles."""
    fautes: list[str] = []
    cles: dict[str, str] = {}
    for fichier in sorted(DISPOSITIONS.glob("*.json")):
        try:
            disposition = charger(fichier.stem)
        except DispositionError as erreur:
            fautes.append(str(erreur))
            continue
        if disposition.get("id") != fichier.stem:
            fautes.append(f"{fichier.name} : id « {disposition.get('id')} », le nom du fichier attendu.")
        fautes += valider(disposition)
        for cellule in disposition.get("cells", []):
            if "keyPrefix" in disposition and "name" in cellule:
                k = cle(disposition, cellule)
                if k in cles:
                    fautes.append(f"clé « {k} » déclarée par {cles[k]} et {fichier.name}.")
                cles[k] = fichier.name
    return len(cles), fautes


def bloc_a(disposition: dict) -> str:
    s = pas(disposition)
    lignes = [ligne for ligne in STYLE.read_text(encoding="utf-8").splitlines()
              if not ligne.startswith("#")]
    return ("\n".join(lignes).strip()
            .replace("{PAS}", str(s))
            .replace("{LOSANGE_L}", str(2 * DEMI_L * s))
            .replace("{LOSANGE_H}", str(2 * DEMI_H * s)))


def bloc_b(disposition: dict) -> str:
    """Le lieu, cité du livre : la fiche d'atlas entière, ou les seules phrases de `locationExcerpt`.

    Une planche peut ne montrer qu'une partie de son lieu -- l'Arène du Destin n'est qu'une phrase de
    la fiche d'Arenarea. Recopier toute la fiche ferait dessiner au générateur les casinos et les
    fontaines du quartier ; les extraits restent des citations, vérifiées mot pour mot par `valider`.
    """
    fiche = json.loads((LIEUX / f"{disposition['location']}.json").read_text(encoding="utf-8"))
    extraits = disposition.get("locationExcerpt")
    texte = " ".join(extraits) if extraits else fiche["description"]
    return (f"PLACE: {fiche['name']}, as the world atlas describes it: {texte}\n\n"
            f"SUBJECT OF THIS SHEET: {disposition['subject']}.")


def bloc_c(disposition: dict, numero: int = 1) -> str:
    largeur, hauteur = taille_planche(disposition)
    s = pas(disposition)
    poses = [p for p in grille(disposition) if p["planche"] == numero]
    lignes = [
        f"LAYOUT — ONE TEXTURE SHEET: one {largeur}x{hauteur} image, fully transparent "
        "background. Reference image 2 is the layout template: it shows "
        f"{len(poses)} cells as faint grey boxes, each with the blue outline of the floor "
        f"footprint its piece stands on (diamonds {2 * DEMI_L * s}x{2 * DEMI_H * s} px). Draw "
        "exactly one piece in each cell, standing exactly on its footprint outline, never crossing "
        "its box. Do not draw the boxes, the outlines or anything between the cells.",
        f"Reference image 1 is drawn at {PAS_MAQUETTE} screen pixels per art pixel; this sheet is "
        f"drawn at {s}: keep its style, not its pixel size.",
        "A thick red segment on a footprint outline marks the edge an oriented piece is built "
        "against, seen exactly as in the template: walls, arches, torches, banners, gates and "
        "boxes stand along that edge; stands and stairs rise toward it. Never draw the red line.",
        "Floor pieces fill their diamond exactly, edge to edge, and nothing outside it. Standing "
        "pieces rest on their footprint and include no floor under them.",
        "Cells, read left to right, then top to bottom:",
    ]
    for rang, pose in enumerate(poses, 1):
        cellule = pose["cellule"]
        classe = disposition["classes"][cellule["class"]]
        hausse = classe["rise"] * s
        # aux tours 2 et 3 de Martpart, des losanges de 270 × 154 pour 272 × 168 : la taille dite une
        # fois en tête du bloc ne suffit pas, chaque sol la répète
        detail = (f"at most {hausse} px above its footprint" if hausse else
                  f"flat, exactly {2 * DEMI_L * s} px wide and {2 * DEMI_H * s} px tall from its top "
                  "corner to its bottom corner, never flatter")
        lignes.append(f"{rang}. {cellule['prompt']} ({classe['description']}; {detail}).")
    return "\n".join(lignes)


def prompt(disposition: dict, numero: int = 1) -> str:
    return "\n\n".join([bloc_a(disposition), bloc_b(disposition), bloc_c(disposition, numero)])


# -- Les images ----------------------------------------------------------------------------------

def _pil():
    import numpy as np
    from PIL import Image, ImageDraw
    return np, Image, ImageDraw


def gabarit(disposition: dict, numero: int = 1):
    """Le gabarit d'une planche : boîtes grises, emprises en contour bleu, fond transparent."""
    _, Image, ImageDraw = _pil()
    s = pas(disposition)
    image = Image.new("RGBA", taille_planche(disposition), (0, 0, 0, 0))
    trait = ImageDraw.Draw(image)
    for pose in grille(disposition):
        if pose["planche"] != numero:
            continue
        x0, y0, x1, y1 = pose["boite"]
        trait.rectangle([x0, y0, x1 - 1, y1 - 1], outline=(150, 150, 150, 255))
        ox, oy = pose["origine"]
        points = [(ox + x * s, oy + y * s) for x, y in sommets(disposition, pose["cellule"])]
        trait.polygon(points, outline=(60, 90, 200, 255))
        # l'arête contre laquelle la pièce se dresse : au tour 1 du Colisée, « upper-left edge »
        # écrit en toutes lettres a donné neuf pièces sur dix-neuf dans le mauvais sens
        for i, j in ARETES.get(pose["cellule"].get("edge"), ()):
            trait.line([points[i], points[j]], fill=(220, 40, 40, 255), width=max(3, s))
    return image


def mettre_au_format(disposition: dict, candidat):
    """Une planche reçue, en RGBA à alpha binaire, à la taille où elle a été rendue.

    La taille n'est pas ramenée à celle de la disposition : l'interface du générateur ne la tient
    pas (1672 × 941 rendus pour 2560 × 1440 demandés au tour 1 du Colisée), et la découpe mesure
    chaque pièce là où elle est.
    """
    np, Image, _ = _pil()
    pixels = np.asarray(candidat.convert("RGBA")).copy()
    alpha = pixels[..., 3]
    if alpha.min() >= 250:
        raise DispositionError("une planche reçue n'a pas de fond transparent : le prompt le "
                               "demande, la génération est à refaire.")
    opaque = alpha > ALPHA
    pixels[..., 3] = np.where(opaque, 255, 0)
    pixels[~opaque] = 0
    return Image.fromarray(pixels)


def _masque_emprise(disposition: dict, cellule: dict):
    np, Image, ImageDraw = _pil()
    masque = Image.new("L", canevas(disposition, cellule), 0)
    ImageDraw.Draw(masque).polygon(sommets(disposition, cellule), fill=255, outline=255)
    return np.asarray(masque) > 0


def _etiquettes(masque, grain: int):
    """Composantes 4-connexes d'un masque lu par blocs de `grain` px : (étiquettes, nombre)."""
    np, _, _ = _pil()
    h, w = masque.shape[0] // grain, masque.shape[1] // grain
    petit = masque[:h * grain, :w * grain].reshape(h, grain, w, grain).any(axis=(1, 3))
    etiquettes = np.zeros((h, w), dtype=np.int32)
    nombre = 0
    for y, x in zip(*np.nonzero(petit)):
        if etiquettes[y, x]:
            continue
        nombre += 1
        etiquettes[y, x] = nombre
        pile = [(y, x)]
        while pile:
            cy, cx = pile.pop()
            for ny, nx in ((cy + 1, cx), (cy - 1, cx), (cy, cx + 1), (cy, cx - 1)):
                if 0 <= ny < h and 0 <= nx < w and petit[ny, nx] and not etiquettes[ny, nx]:
                    etiquettes[ny, nx] = nombre
                    pile.append((ny, nx))
    return etiquettes, nombre


def pieces(planche) -> list[dict]:
    """Les pièces d'une planche, dans l'ordre de lecture : {boite, masque}.

    Le générateur ne pose pas ses pièces dans les cellules du gabarit (tour 1 du Colisée) : il en
    tient le nombre et l'ordre, pas la place. Une pièce est donc une composante connexe ; un éclat
    (une flamme détachée de sa torche) rejoint la pièce la plus proche s'il la touche presque, et
    s'efface sinon ; les rangées se forment par
    recouvrement vertical -- une pièce rejoint la rangée dont l'étendue couvre la moitié de sa
    hauteur -- et se lisent de gauche à droite. Le masque d'une pièce est sa composante, pas sa
    boîte : sur la planche 2 du tour 1, la boîte d'une loge mord sur la porte voisine.
    """
    np, _, _ = _pil()
    grain = 2
    etiquettes, nombre = _etiquettes(np.asarray(planche)[..., 3] > 0, grain)
    composantes = []
    for i in range(1, nombre + 1):
        ys, xs = np.nonzero(etiquettes == i)
        composantes.append({"ids": [i], "aire": len(ys),
                            "boite": [int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1]})
    if not composantes:
        return []
    seuil = 0.02 * float(np.median([c["aire"] for c in composantes]))
    grandes = [c for c in composantes if c["aire"] >= seuil]
    for eclat in (c for c in composantes if c["aire"] < seuil):
        def distance(c, e=eclat):
            (ax0, ay0, ax1, ay1), (bx0, by0, bx1, by1) = c["boite"], e["boite"]
            return max(ax0 - bx1, bx0 - ax1, 0) + max(ay0 - by1, by0 - ay1, 0)
        cible = min(grandes, key=distance)
        x0, y0, x1, y1 = cible["boite"]
        if distance(cible) > 0.25 * min(x1 - x0, y1 - y0):
            continue   # trop loin de toute pièce : du bruit, qui fausserait la boîte et l'échelle
        cible["ids"].extend(eclat["ids"])
        b, e = cible["boite"], eclat["boite"]
        cible["boite"] = [min(b[0], e[0]), min(b[1], e[1]), max(b[2], e[2]), max(b[3], e[3])]

    rangees: list[dict] = []
    for piece in sorted(grandes, key=lambda c: c["boite"][1]):
        _, y0, _, y1 = piece["boite"]
        for rangee in rangees:
            if min(y1, rangee["y1"]) - max(y0, rangee["y0"]) >= 0.5 * (y1 - y0):
                rangee["pieces"].append(piece)
                rangee["y0"], rangee["y1"] = min(y0, rangee["y0"]), max(y1, rangee["y1"])
                break
        else:
            rangees.append({"y0": y0, "y1": y1, "pieces": [piece]})
    ordre = [p for r in sorted(rangees, key=lambda r: r["y0"])
             for p in sorted(r["pieces"], key=lambda p: p["boite"][0])]

    sortie = []
    for piece in ordre:
        x0, y0, x1, y1 = piece["boite"]
        dedans = np.isin(etiquettes[y0:y1, x0:x1], piece["ids"])
        sortie.append({"boite": (x0 * grain, y0 * grain, x1 * grain, y1 * grain),
                       "masque": np.repeat(np.repeat(dedans, grain, axis=0), grain, axis=1)})
    return sortie


def _reduire(planche, piece: dict, largeur: int, hauteur: int):
    """La pièce ramenée à largeur × hauteur px d'art : moyenne des pixels opaques, alpha binaire."""
    np, Image, _ = _pil()
    x0, y0, x1, y1 = piece["boite"]
    zone = np.asarray(planche)[y0:y1, x0:x1].astype(np.float32)
    masque = piece["masque"][:zone.shape[0], :zone.shape[1]] & (zone[..., 3] > 0)
    poids = masque.astype(np.float32)
    taille = (largeur, hauteur)
    somme = np.dstack([np.asarray(Image.fromarray(zone[..., c] * poids).resize(taille, Image.BOX))
                       for c in range(3)])
    couverture = np.asarray(Image.fromarray(poids).resize(taille, Image.BOX))
    art = np.zeros((hauteur, largeur, 4), dtype=np.uint8)
    pleins = couverture >= 0.5
    moyenne = np.clip(somme / np.maximum(couverture, 1e-6)[..., None], 0, 255)
    art[..., :3] = np.where(pleins[..., None], moyenne, 0)
    art[..., 3] = np.where(pleins, 255, 0)
    return art


def _poser(disposition: dict, cellule: dict, art, avertissements: list[str]):
    """La pièce réduite posée dans son canevas : (texture, sommet haut de l'emprise)."""
    np, _, _ = _pil()
    k = cle(disposition, cellule)
    cw, ch = canevas(disposition, cellule)
    a, b = emprise(disposition, cellule)
    h_art, w_art = art.shape[:2]
    if disposition["classes"][cellule["class"]]["rise"] == 0:
        # un sol : sommet haut du losange en haut de la pièce ; l'épaisseur que le générateur
        # dessine sous le losange tombe hors du masque
        texture = np.zeros((ch, cw, 4), dtype=np.uint8)
        texture[:min(ch, h_art), :min(cw, w_art)] = art[:ch, :cw]
        texture[~_masque_emprise(disposition, cellule)] = 0
        return texture, sommets(disposition, cellule)[0]
    h = max(ch, h_art)
    if h_art > ch:
        avertissements.append(f"{k} : {h_art} px d'art de haut, la classe en prévoit {ch} ; "
                              "canevas agrandi.")
    if cellule.get("fill", True):
        if w_art > cw:
            avertissements.append(f"{k} : {w_art} px d'art de large pour une emprise de {cw}, rognée.")
        texture = np.zeros((h, cw, 4), dtype=np.uint8)
        zone = art[:, :cw]
        texture[h - h_art:, :zone.shape[1]] = zone
        return texture, (b * DEMI_L, h - (a + b) * DEMI_H)
    # Une pièce libre est centrée sur le milieu de son emprise -- le sommet bas pour une case, pas
    # pour une emprise allongée (tour 2 de Martpart : 17 px d'art de décalage). Plus large, elle n'est
    # PAS rognée : le canevas s'élargit de part et d'autre et l'ancre suit (tour 1 de Martpart : des
    # étals de 170 px d'art pour une emprise de 102 perdaient leur côté droit). Elle déborde alors
    # sur les cases voisines, comme tout objet isométrique plus large que sa case. Décision de
    # l'auteur, 16 septembre 2026.
    debut = cw // 2 - w_art // 2
    deborde_g, deborde_d = max(0, -debut), max(0, debut + w_art - cw)
    if deborde_g or deborde_d:
        avertissements.append(f"{k} : {w_art} px d'art de large pour une emprise de {cw} ; "
                              f"canevas élargi ({deborde_g} à gauche, {deborde_d} à droite).")
        debut = max(0, debut)
    else:
        debut = max(0, min(cw - w_art, debut))
    texture = np.zeros((h, cw + deborde_g + deborde_d, 4), dtype=np.uint8)
    texture[h - h_art:, debut:debut + w_art] = art
    return texture, (b * DEMI_L + deborde_g, h - (a + b) * DEMI_H)


def _orienter(disposition: dict, cellule: dict, texture, avertissements: list[str]):
    """Contrôle l'orientation d'une pièce dressée le long de son arête, et la retourne si besoin.

    Bâtie le long de l'arête gauche (du sommet gauche au sommet haut), une pièce descend vers la
    gauche : son point le plus bas est à gauche de sa largeur ; le long de l'arête droite, à droite.
    Au tour 2 du Colisée, cet indice (x moyen des trois rangées les plus basses, rapporté à la
    largeur) valait de 0,16 à 0,26 ou de 0,74 à 0,85, et concordait avec l'œil. Une pièce dessinée
    contre l'autre arête est une orientation juste de l'autre côté : elle est retournée.

    Une pièce qui MONTE vers son arête (`"stance": "toward"` : gradins, escaliers) a son point bas
    au milieu ; c'est sa moitié haute qui penche du côté de l'arête. L'indice est alors le x moyen
    de cette moitié : 0,41 et 0,45 au tour 1, 0,58 et 0,60 au tour 2 (montés vers la droite, à l'œil
    aussi). Les sols et les pièces sans arête ne se mesurent pas.
    """
    np, _, _ = _pil()
    arete = cellule.get("edge")
    if arete not in ("left", "right") or disposition["classes"][cellule["class"]]["rise"] == 0:
        return texture
    k = cle(disposition, cellule)
    ys, xs = np.nonzero(texture[..., 3])
    if cellule.get("stance", "along") == "toward":
        milieu = (ys.min() + ys.max()) / 2
        indice, marge = xs[ys <= milieu].mean() / texture.shape[1], 0.05
    else:
        indice, marge = xs[ys >= ys.max() - 2].mean() / texture.shape[1], 0.1
    if abs(indice - 0.5) < marge:
        avertissements.append(f"{k} : orientation indécise (indice {indice:.2f}), à vérifier à l'œil.")
        return texture
    if (indice < 0.5) != (arete == "left"):
        avertissements.append(f"{k} : dessinée contre l'autre arête (indice {indice:.2f}), retournée.")
        return texture[:, ::-1].copy()
    return texture


def decouper(disposition: dict, planches_recues: list) -> tuple[dict, dict, list[str], list[str]]:
    """Les textures des planches reçues : {clé: image}, manifeste, erreurs, avertissements.

    Chaque pièce lue (voir `pieces`) va à la cellule de même rang sur sa planche, puis s'ajuste à
    son emprise : une pièce qui la remplit (`fill`, vrai par défaut) est mise à sa largeur ; une
    pièce plus petite (un brasero, un banc) prend le facteur médian des pièces pleines de sa
    planche. Un sol est découpé au losange et refusé sous 97 % de couverture ; une pièce haute est
    posée en bas de son canevas, qui grandit si elle dépasse sa classe (l'ancre suit).
    """
    np, Image, _ = _pil()
    attendu = planches(disposition)
    if len(planches_recues) != attendu:
        raise DispositionError(f"{disposition['id']} : {len(planches_recues)} planche(s) reçue(s), "
                               f"{attendu} attendue(s).")
    poses = grille(disposition)
    brutes, erreurs, avertissements = {}, [], []
    for numero, planche in enumerate(planches_recues, 1):
        cellules = [p["cellule"] for p in poses if p["planche"] == numero]
        lues = pieces(planche)
        if len(lues) != len(cellules):
            erreurs.append(f"planche {numero} : {len(lues)} pièce(s) lue(s), {len(cellules)} "
                           "attendue(s) : la génération est à refaire.")
            continue
        facteurs = {c["name"]: canevas(disposition, c)[0] / (p["boite"][2] - p["boite"][0])
                    for c, p in zip(cellules, lues) if c.get("fill", True)}
        repli = float(np.median(list(facteurs.values()))) if facteurs else None
        for cellule, piece in zip(cellules, lues):
            k = cle(disposition, cellule)
            facteur = facteurs.get(cellule["name"], repli)
            if facteur is None:
                erreurs.append(f"{k} : aucune pièce pleine sur la planche {numero} pour en tirer "
                               "l'échelle.")
                continue
            x0, y0, x1, y1 = piece["boite"]
            art = _reduire(planche, piece, max(1, round((x1 - x0) * facteur)),
                           max(1, round((y1 - y0) * facteur)))
            texture, haut = _poser(disposition, cellule, art, avertissements)
            if disposition["classes"][cellule["class"]]["rise"] == 0:
                masque = _masque_emprise(disposition, cellule)
                couverture = (texture[..., 3] > 0)[masque].mean()
                if couverture < COUVERTURE_SOL:
                    erreurs.append(f"{k} : le sol couvre {couverture:.0%} de son losange, "
                                   f"{COUVERTURE_SOL:.0%} attendus.")
            if not texture[..., 3].any():
                erreurs.append(f"{k} : pièce vide.")
            else:
                texture = _orienter(disposition, cellule, texture, avertissements)
            brutes[k] = (cellule, numero, piece["boite"], facteur, haut, texture)

    for cellule in disposition["cells"]:
        source = source_du_miroir(disposition, cellule)
        if source is None or cle(disposition, source) not in brutes:
            continue
        _, numero, boite, facteur, haut, texture = brutes[cle(disposition, source)]
        miroir = texture[:, ::-1].copy()
        brutes[cle(disposition, cellule)] = (cellule, numero, boite, facteur,
                                             (miroir.shape[1] - haut[0], haut[1]), miroir)

    # une palette commune au lieu : quantifier pièce par pièce, ou planche par planche, donnerait
    # des palettes voisines qui ne s'accordent pas une fois les tuiles posées côte à côte
    opaques = [t[t[..., 3] > 0][:, :3] for *_, t in brutes.values()]
    textures, fichiers = {}, {}
    if any(len(o) for o in opaques):
        mosaique = np.concatenate([o for o in opaques if len(o)]).reshape(-1, 1, 3)
        palette = Image.fromarray(mosaique.astype(np.uint8), "RGB").quantize(
            COULEURS, Image.Quantize.MEDIANCUT)
        for k, (cellule, numero, boite, facteur, haut, texture) in brutes.items():
            rgb = Image.fromarray(texture[..., :3], "RGB").quantize(palette=palette,
                                                                    dither=Image.Dither.NONE)
            finale = np.dstack([np.asarray(rgb.convert("RGB")), texture[..., 3]])
            finale[finale[..., 3] == 0] = 0
            textures[k] = Image.fromarray(finale.astype(np.uint8), "RGBA")
            fichiers[k] = {
                "file": f"{cellule['name']}.png",
                "class": cellule["class"],
                **({"mirrorOf": cle(disposition, source_du_miroir(disposition, cellule))}
                   if "mirrorOf" in cellule else {}),
                "footprint": list(emprise(disposition, cellule)),
                # ce que la pièce oppose à qui passe, et ses anciens noms (LOT-EDITOR-12) : un
                # miroir hérite de sa source ce qu'il ne déclare pas
                **{champ: valeur for champ in ("tactical", "aliases")
                   if (valeur := cellule.get(champ, (source_du_miroir(disposition, cellule)
                                                     or {}).get(champ))) is not None},
                "size": [int(finale.shape[1]), int(finale.shape[0])],
                # le sommet haut de l'emprise : le coin (0, 0) de la case qui porte la pièce
                "anchor": [int(haut[0]), int(haut[1])],
                "sheet": numero,
                "sourceBox": [int(v) for v in boite],
                "scale": round(float(facteur), 4),
            }
    manifeste = {
        "version": 1,
        "disposition": disposition["id"],
        "sheets": [{"file": nom_planche(n), "size": list(p.size),
                    "sha256": hashlib.sha256(_png(p)).hexdigest()}
                   for n, p in enumerate(planches_recues, 1)],
        "tile": [2 * DEMI_L, 2 * DEMI_H],
        "colors": COULEURS,
        "textures": fichiers,
    }
    return textures, manifeste, erreurs, avertissements


def nom_planche(numero: int) -> str:
    return f"planche-{numero}.png"


def _png(image) -> bytes:
    tampon = io.BytesIO()
    image.save(tampon, format="PNG", optimize=True)
    return tampon.getvalue()


def _json(donnee: dict) -> str:
    return json.dumps(donnee, ensure_ascii=False, indent=2) + "\n"


# -- Commandes -----------------------------------------------------------------------------------

def commande(identifiant: str, tour: int) -> int:
    disposition = charger(identifiant)
    fautes = valider(disposition)
    if fautes:
        print("\n".join("ERREUR " + f for f in fautes))
        return 1
    largeur, hauteur = taille_planche(disposition)
    if largeur * hauteur > PIXELS_FIABLES:
        print(f"AVERTISSEMENT {identifiant} : {largeur} × {hauteur} dépasse 2560 × 1440, rendu "
              "donné pour expérimental par le générateur.")
    racine = TRAVAIL / "chatgpt" / f"{identifiant}-tour{tour}"
    if racine.exists():
        shutil.rmtree(racine)
    for numero in range(1, planches(disposition) + 1):
        dossier = racine / f"planche-{numero}"
        dossier.mkdir(parents=True)
        texte = prompt(disposition, numero)
        (dossier / "prompt.txt").write_text(texte + "\n", encoding="utf-8")
        shutil.copy(MAQUETTE, dossier / "1_maquette.png")
        gabarit(disposition, numero).save(dossier / "2_gabarit.png")
        cible = TRAVAIL / identifiant / f"tour{tour}" / nom_planche(numero)
        cible.parent.mkdir(parents=True, exist_ok=True)
        (dossier / "A_ENREGISTRER_SOUS.txt").write_text(f"{cible}\n", encoding="utf-8")
        cellules = sum(1 for p in grille(disposition) if p["planche"] == numero)
        print(f"{dossier} : {cellules} cellules, {len(texte)} caractères -> {cible}")
    return 0


def decoupe(identifiant: str, candidats: list[Path]) -> int:
    _, Image, _ = _pil()
    disposition = charger(identifiant)
    recues = [mettre_au_format(disposition, Image.open(c)) for c in candidats]
    textures, manifeste, erreurs, avertissements = decouper(disposition, recues)
    for a in avertissements:
        print("AVERTISSEMENT " + a)
    if erreurs:
        print("\n".join("ERREUR " + e for e in erreurs))
        print(f"{identifiant} : {len(erreurs)} faute(s), rien n'est installé.")
        return 1
    racine = RACINE / disposition["installRoot"]
    racine.mkdir(parents=True, exist_ok=True)
    for ancien in racine.glob("*.png"):
        ancien.unlink()
    for numero, planche in enumerate(recues, 1):
        (racine / nom_planche(numero)).write_bytes(_png(planche))
    for k, image in textures.items():
        (racine / manifeste["textures"][k]["file"]).write_bytes(_png(image))
    (racine / "manifest.json").write_text(_json(manifeste), encoding="utf-8", newline="\n")
    print(f"{identifiant} : {len(textures)} textures installées sous {racine}.")
    return 0


def verifier(identifiant: str) -> int:
    """Refait la découpe depuis les planches installées et compare, sans rien écrire."""
    np, Image, _ = _pil()
    disposition = charger(identifiant)
    racine = RACINE / disposition["installRoot"]
    noms = [nom_planche(n) for n in range(1, planches(disposition) + 1)]
    manquantes = [n for n in noms if not (racine / n).is_file()]
    if manquantes:
        print(f"ERREUR planches absentes : {', '.join(manquantes)}.")
        return 1
    recues = [Image.open(racine / n).convert("RGBA") for n in noms]
    textures, manifeste, fautes, _ = decouper(disposition, recues)
    chemin = racine / "manifest.json"
    if not chemin.is_file() or chemin.read_text(encoding="utf-8") != _json(manifeste):
        fautes.append("manifest.json diffère de la découpe.")
    attendus = {manifeste["textures"][k]["file"] for k in textures} | set(noms)
    for nom in sorted({p.name for p in racine.glob("*.png")} - attendus):
        fautes.append(f"{nom} : installé, qu'aucune cellule ne produit.")
    for k, image in textures.items():
        fichier = racine / manifeste["textures"][k]["file"]
        if not fichier.is_file():
            fautes.append(f"{fichier.name} : absent.")
        elif not np.array_equal(np.asarray(Image.open(fichier).convert("RGBA")), np.asarray(image)):
            fautes.append(f"{fichier.name} : pixels différents de la découpe.")
    for f in fautes:
        print("ERREUR " + f)
    print(f"{identifiant} : "
          + (f"{len(fautes)} écart(s)." if fautes else f"{len(textures)} textures à jour."))
    return 1 if fautes else 0


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    try:
        if args == ["valider"]:
            nombre, fautes = valider_tout()
            for f in fautes:
                print("ERREUR " + f)
            print(f"dispositions : {nombre} clés de scène"
                  + (f", {len(fautes)} faute(s)." if fautes else ", conformes."))
            return 1 if fautes else 0
        if args[:1] == ["--check"] and len(args) == 2:
            return verifier(args[1])
        if args[:1] == ["commande"] and len(args) == 3 and args[2].isdigit():
            return commande(args[1], int(args[2]))
        if args[:1] == ["decoupe"] and len(args) >= 3:
            return decoupe(args[1], [Path(a) for a in args[2:]])
    except DispositionError as erreur:
        print("ERREUR " + str(erreur))
        return 1
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
