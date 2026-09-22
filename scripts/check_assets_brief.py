#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Garde-fou du cahier des assets de la charte v2 (LOT-87, T2.4).

Le cahier dit a un generateur d'images ce qu'il faut produire : chaque cadre, plaque, bouton, fond
et icone, sa taille a 1080p, ses marges 9-patch, ses etats, sa description, son prompt, et la zone
de maquette qui le montre. Il vit en deux jumeaux :

- `assets-brief.json`, la source, que ce script valide et que le T2.6 lira pour receptionner les
  images ;
- `assets-brief.md`, la page de documentation. Ses tables sont ENGENDREES depuis le JSON, entre deux
  marqueurs : un cahier recopie a la main divergerait au premier ajout de piece, et c'est la copie
  oubliee qu'on lirait en production.

Ce que JSON Schema dit (`assets-brief.schema.json`) : la forme. Ce que ce script ajoute, parce
qu'aucun schema ne le peut :

1. chaque jeton nomme -- palette d'une matiere, `{jeton}` d'un prompt, aplat de repli -- existe dans
   `Tokens.qml`. Le prompt assemble porte la VALEUR lue dans les jetons, jamais une copie : changer
   un jeton change le prompt suivant ;
2. une cle, et chaque cle de variante `<cle>/<id>`, est unique ;
3. des marges 9-patch laissent un milieu a etirer (gauche + droite < largeur, haut + bas < hauteur) ;
4. chaque zone tient dans sa maquette, dont la taille est lue dans l'en-tete PNG ;
5. une piece qu'aucune maquette ne montre prolonge une piece existante (`derivedFrom`) ;
6. chaque maquette de `references/` est citee par une piece ou par une exclusion, et chaque ecran
   des phases 3 et 4 consomme au moins une piece : un ecran sans piece n'aurait pas ete lu ;
7. `displaySizes` ne vaut que pour une piece fixe, et une piece fixe ne s'affiche jamais plus grande
   que sa production (`EX-IHM-075` : produite a 1080p, reduite avec lissage) ;
8. la page `.md` est a jour du JSON ;
Les textures de SCENE n'ont jamais eu de piece ici, et n'en ont plus du tout : la table rase du
LOT-102 a emporte l'atelier des textures avec son art. Le controle de la chaine de production 2D HD
est au LOT-104.

Usage :
    python scripts/check_assets_brief.py                    # controle, code de sortie non nul si faute
    python scripts/check_assets_brief.py --write            # reecrit les tables de la page .md
    python scripts/check_assets_brief.py --prompt ui/button/menu/hover
                                                            # imprime le prompt tel qu'il sera envoye
    python scripts/check_assets_brief.py --prompt all       # tous les prompts, un par variante
    python scripts/check_assets_brief.py --annotate <dossier>
                                                            # dessine chaque zone sur sa maquette (Pillow)

Dependance : **jsonschema**, comme `check_rpg_data.py` ; Pillow pour `--annotate` seulement.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

RACINE = Path(__file__).resolve().parent.parent
LOT = RACINE / "Planning" / "versions" / "v0.0.0" / "v0.0.0-fondation" / "annexes" / "LOT-87-charte-v2"
CAHIER = LOT / "assets-brief.json"
SCHEMA = LOT / "assets-brief.schema.json"
PAGE = LOT / "assets-brief.md"

DEBUT = "<!-- DEBUT DES TABLES ENGENDREES : scripts/check_assets_brief.py --write -->"
FIN = "<!-- FIN DES TABLES ENGENDREES -->"

# Les ecrans que le cahier doit nourrir : la phase 3 entiere et le HUD de la phase 4.
ECRANS = ["T3.1", "T3.2", "T3.3", "T3.4", "T3.5", "T3.6", "T3.7", "T3.8", "T3.9", "T4.1"]

# L'ordre et le titre des familles dans la page. La cle porte la famille, la page la nomme.
FAMILLES = [
    ("background", "Fonds"),
    ("frame", "Cadres 9-patch"),
    ("plate", "Plaques et bandeaux"),
    ("tab", "Onglets"),
    ("button", "Boutons"),
    ("control", "Contrôles"),
    ("medallion", "Médaillons"),
    ("slot", "Emplacements"),
    ("gauge", "Jauges"),
    ("ornament", "Ornements"),
    ("icon", "Icônes"),
]

JETON = re.compile(r'^\s*(?:readonly\s+)?property\s+(?:color|string)\s+(\w+)\s*:\s*"([^"]*)"', re.M)
MARQUE = re.compile(r"\{(\w+)\}")

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

erreurs: list[str] = []


def fail(message: str) -> None:
    erreurs.append(message)


def lire_jetons(cahier: dict) -> dict[str, str]:
    chemin = RACINE / cahier["tokens"]
    if not chemin.is_file():
        fail("%s absent : aucune palette a imposer." % cahier["tokens"])
        return {}
    return dict(JETON.findall(chemin.read_text(encoding="utf-8")))


def taille_png(chemin: Path) -> tuple[int, int] | None:
    """Largeur et hauteur lues dans l'en-tete IHDR : aucune dependance pour un controle de CI."""
    with chemin.open("rb") as flux:
        tete = flux.read(24)
    if len(tete) < 24 or tete[:8] != b"\x89PNG\r\n\x1a\n" or tete[12:16] != b"IHDR":
        return None
    return struct.unpack(">II", tete[16:24])


def variantes(piece: dict) -> list[dict]:
    return piece.get("states") or piece.get("members") or []


def cles(piece: dict) -> list[str]:
    """Les cles d'asset que la piece engendre : une par variante, ou la sienne."""
    vs = variantes(piece)
    return ["%s/%s" % (piece["key"], v["id"]) for v in vs] if vs else [piece["key"]]


# --------------------------------------------------------------------------------------- controles


def valider_schema(cahier: dict) -> None:
    try:
        import jsonschema
    except ImportError as erreur:  # pragma: no cover - depend de l'environnement
        print("check_assets_brief : jsonschema est requis -- pip install jsonschema (%s)." % erreur)
        sys.exit(1)
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    jsonschema.Draft202012Validator.check_schema(schema)
    for faute in sorted(jsonschema.Draft202012Validator(schema).iter_errors(cahier), key=lambda e: list(e.path)):
        # Le message d'un `not`/`if` recopie toute l'instance : la regle violee suffit, le chemin situe.
        regle = faute.message if len(faute.message) < 160 else "%s %s" % (faute.validator, json.dumps(faute.validator_value))
        fail("schema : %s -- %s" % ("/".join(str(p) for p in faute.path) or "(racine)", regle))


def controler_jetons(cahier: dict, jetons: dict[str, str]) -> None:
    def exiger(nom: str, ou: str) -> None:
        if nom not in jetons:
            fail("%s : le jeton '%s' n'existe pas dans %s" % (ou, nom, cahier["tokens"]))

    for nom, matiere in cahier["materials"].items():
        for jeton in matiere["tokens"]:
            exiger(jeton, "matiere '%s'" % nom)
        for marque in MARQUE.findall(matiere["prompt"]):
            if marque not in matiere["tokens"]:
                fail("matiere '%s' : {%s} n'est pas dans sa palette declaree" % (nom, marque))
    for phrase in ("noLettering", "lettering"):
        for marque in MARQUE.findall(cahier["prompt"][phrase]):
            exiger(marque, "prompt.%s" % phrase)
    for piece in cahier["pieces"]:
        if piece["material"] not in cahier["materials"]:
            fail("%s : matiere inconnue '%s'" % (piece["key"], piece["material"]))
        for jeton in piece["fallback"]:
            exiger(jeton, "%s (repli)" % piece["key"])


def controler_pieces(cahier: dict) -> None:
    references = RACINE / cahier["references"]
    tailles: dict[str, tuple[int, int] | None] = {}

    def zone_valide(ou: str, fichier: str, zone: list[int]) -> None:
        if fichier not in tailles:
            chemin = references / fichier
            tailles[fichier] = taille_png(chemin) if chemin.is_file() else None
        taille = tailles[fichier]
        if taille is None:
            fail("%s : maquette introuvable ou illisible '%s'" % (ou, fichier))
            return
        x0, y0, x1, y1 = zone
        if not (x0 < x1 <= taille[0] and y0 < y1 <= taille[1]):
            fail("%s : zone %s hors de %s (%d x %d)" % (ou, zone, fichier, taille[0], taille[1]))

    vues: set[str] = set()
    toutes = {piece["key"] for piece in cahier["pieces"]}
    for piece in cahier["pieces"]:
        cle = piece["key"]
        engendrees = {cle, *cles(piece)}
        for doublon in sorted(engendrees & vues):
            fail("%s : cle en double" % doublon)
        vues |= engendrees
        ids = [v["id"] for v in variantes(piece)]
        if len(ids) != len(set(ids)):
            fail("%s : identifiant de variante en double" % cle)

        largeur, hauteur = piece["size"]
        marges = piece.get("margins")
        if marges:
            if marges["left"] + marges["right"] >= largeur or marges["top"] + marges["bottom"] >= hauteur:
                fail("%s : marges %s sans milieu a etirer dans %d x %d" % (cle, marges, largeur, hauteur))
            if not any(marges.values()):
                fail("%s : 9-patch sans aucune marge -- c'est une piece etiree entiere" % cle)
        if "displaySizes" in piece:
            if piece["display"] != "fixed":
                fail("%s : displaySizes ne vaut que pour une piece fixe" % cle)
            for l, h in piece["displaySizes"]:
                if l > largeur or h > hauteur:
                    fail("%s : affichee a %d x %d, plus grande que sa production %d x %d" % (cle, l, h, largeur, hauteur))
        if piece["display"] == "cover" and piece["size"] != cahier["designResolution"]:
            fail("%s : un fond plein ecran se produit a la definition de conception" % cle)
        if piece["display"] == "cover" and piece["transparent"]:
            fail("%s : un fond plein ecran est opaque" % cle)

        for ref in piece["mockups"]:
            zone_valide(cle, ref["file"], ref["zone"])
        for v in variantes(piece):
            if "mockup" in v:
                zone_valide("%s/%s" % (cle, v["id"]), v["mockup"]["file"], v["mockup"]["zone"])
        origine = piece.get("derivedFrom")
        if origine is not None and (origine == cle or origine not in toutes):
            fail("%s : derivedFrom '%s' ne nomme aucune autre piece du cahier" % (cle, origine))


def controler_couverture(cahier: dict) -> None:
    citees = {ref["file"] for piece in cahier["pieces"] for ref in piece["mockups"]}
    citees |= {f for exclusion in cahier["excluded"] for f in exclusion["mockups"]}
    for maquette in sorted((RACINE / cahier["references"]).glob("*.png")):
        if maquette.name not in citees:
            fail("maquette %s lue par aucune piece ni exclusion" % maquette.name)
    servis = {ecran for piece in cahier["pieces"] for ecran in piece["screens"]}
    for ecran in ECRANS:
        if ecran not in servis:
            fail("%s : aucun ecran ne consomme de piece du cahier" % ecran)
    familles = {nom for nom, _ in FAMILLES}
    for piece in cahier["pieces"]:
        if piece["key"].split("/")[1] not in familles:
            fail("%s : famille absente de la page" % piece["key"])


# ---------------------------------------------------------------------------------------- prompts


def assembler(cahier: dict, jetons: dict[str, str], piece: dict, variante: dict | None) -> str:
    """Le prompt tel qu'il part au generateur : style, palette, lettres, piece, variante, toile."""
    p = cahier["prompt"]
    valeur = lambda mo: jetons.get(mo.group(1), "?")  # noqa: E731
    morceaux = [p["style"], MARQUE.sub(valeur, cahier["materials"][piece["material"]]["prompt"])]
    morceaux.append(MARQUE.sub(valeur, p["lettering" if piece.get("lettering") else "noLettering"]))
    morceaux.append(piece["prompt"])
    if variante is not None:
        morceaux.append(("Subject: %s." if "members" in piece else "State '%s': %%s." % variante["id"])
                        % variante["prompt"])
    largeur, hauteur = piece["size"]
    fond = "transparent background (alpha)" if piece["transparent"] else "fully opaque"
    morceaux.append(p["canvas"].format(width=largeur, height=hauteur, background=fond))
    if piece["display"] == "nine-patch":
        marges = piece["margins"]
        tranche = "three-slice" if marges["top"] == marges["bottom"] == 0 else "nine-patch"
        morceaux.append(p[tranche].format(**marges))
    else:
        morceaux.append(p[piece["display"]])
    return " ".join(morceaux)


def imprimer_prompts(cahier: dict, jetons: dict[str, str], cible: str) -> int:
    trouve = False
    for piece in cahier["pieces"]:
        vs = variantes(piece) or [None]
        for v in vs:
            cle = piece["key"] if v is None else "%s/%s" % (piece["key"], v["id"])
            if cible in ("all", cle, piece["key"]):
                print("%s\n%s\n" % (cle, assembler(cahier, jetons, piece, v)))
                trouve = True
    if not trouve:
        print("check_assets_brief : aucune piece ni variante '%s'." % cible)
        return 1
    return 0


# ------------------------------------------------------------------------------------------ page


def cellule(texte: str) -> str:
    return texte.replace("|", "/").replace("\n", " ")


def rendre(cahier: dict) -> str:
    lignes = [DEBUT, ""]
    total = sum(len(cles(p)) for p in cahier["pieces"])
    lignes.append("Le cahier compte **%d pièces**, qui engendrent **%d images** (une par état ou par membre)."
                  % (len(cahier["pieces"]), total))
    for famille, titre in FAMILLES:
        pieces = [p for p in cahier["pieces"] if p["key"].split("/")[1] == famille]
        if not pieces:
            continue
        images = sum(len(cles(p)) for p in pieces)
        lignes += ["", "### %s {#lot-87-cahier-%s}" % (titre, famille), ""]
        lignes.append("%d pièces, %d images." % (len(pieces), images))
        lignes += ["", "| Clé | Pièce | Production | Tenue | Variantes | Maquettes, zone (x0, y0, x1, y1) | Écrans |",
                   "|---|---|---|---|---|---|---|"]
        for p in pieces:
            l, h = p["size"]
            if p["display"] == "nine-patch":
                m = p["margins"]
                tenue = "9-patch %d / %d / %d / %d" % (m["left"], m["top"], m["right"], m["bottom"])
            else:
                tenue = {"fixed": "fixe", "tile": "tuile", "cover": "plein écran"}[p["display"]]
                if "displaySizes" in p:
                    tenue += ", affichée " + ", ".join("%d × %d" % tuple(s) for s in p["displaySizes"])
            production = "%d × %d%s" % (l, h, "" if p["transparent"] else ", opaque")
            vs = variantes(p)
            noms = ", ".join("`%s`" % v["id"] for v in vs) if vs else "—"
            refs = ["%s %s" % (r["file"][:2], tuple(r["zone"])) for r in p["mockups"]]
            if not refs:
                refs = ["prolonge `%s`" % p["derivedFrom"]]
            lignes.append("| `%s` | %s | %s | %s | %s | %s | %s |" % (
                p["key"], cellule(p["name"]), production, tenue, noms, "<br>".join(refs), ", ".join(p["screens"])))
        lignes.append("")
        for p in pieces:
            lignes.append("- **`%s`** — %s" % (p["key"], cellule(p["description"])))
            lignes.append("  *Prompt propre :* « %s »" % cellule(p["prompt"]))
            vs = variantes(p)
            if vs:
                detail = []
                for v in vs:
                    zone = " — %s %s" % (v["mockup"]["file"][:2], tuple(v["mockup"]["zone"])) if "mockup" in v else ""
                    detail.append("`%s` %s (« %s »%s)" % (v["id"], cellule(v["label"]), cellule(v["prompt"]), zone))
                lignes.append("  *%s :* %s." % ("Membres" if "members" in p else "États", " ; ".join(detail)))
            lignes.append("  *Repli :* %s." % ", ".join("`%s`" % t for t in p["fallback"]))
    lignes += ["", "### Ce que le cahier ne fait pas produire {#lot-87-cahier-exclusions}", "",
               "| Élément | Maquettes | Raison |", "|---|---|---|"]
    for e in cahier["excluded"]:
        lignes.append("| %s | %s | %s |" % (cellule(e["what"]), ", ".join(f[:2] for f in e["mockups"]), cellule(e["reason"])))
    lignes += ["", FIN]
    return "\n".join(lignes)


def page_a_jour(cahier: dict, ecrire: bool) -> None:
    if not PAGE.is_file():
        fail("%s absent" % PAGE.relative_to(RACINE))
        return
    texte = PAGE.read_text(encoding="utf-8")
    debut, fin = texte.find(DEBUT), texte.find(FIN)
    if debut < 0 or fin < debut:
        fail("%s : marqueurs des tables engendrees absents" % PAGE.name)
        return
    nouveau = texte[:debut] + rendre(cahier) + texte[fin + len(FIN):]
    if nouveau == texte:
        return
    if ecrire:
        PAGE.write_text(nouveau, encoding="utf-8", newline="\n")
        print("%s reecrite." % PAGE.relative_to(RACINE))
    else:
        fail("%s n'est pas a jour du JSON : python scripts/check_assets_brief.py --write" % PAGE.name)


# -------------------------------------------------------------------------------------- annotation


def annoter(cahier: dict, dossier: Path) -> None:
    from PIL import Image, ImageDraw  # dependance de l'outil de releve, pas du controle

    dossier.mkdir(parents=True, exist_ok=True)
    par_fichier: dict[str, list[tuple[str, list[int]]]] = {}
    for p in cahier["pieces"]:
        for r in p["mockups"]:
            par_fichier.setdefault(r["file"], []).append((p["key"].split("/", 1)[1], r["zone"]))
        for v in variantes(p):
            if "mockup" in v:
                par_fichier.setdefault(v["mockup"]["file"], []).append((v["id"], v["mockup"]["zone"]))
    for fichier, zones in sorted(par_fichier.items()):
        image = Image.open(RACINE / cahier["references"] / fichier).convert("RGB")
        dessin = ImageDraw.Draw(image)
        for nom, (x0, y0, x1, y1) in zones:
            dessin.rectangle([x0, y0, x1 - 1, y1 - 1], outline=(0, 255, 255), width=2)
            dessin.text((x0 + 3, y0 + 2), nom, fill=(255, 0, 255))
        image.save(dossier / fichier)
    print("%d maquettes annotees dans %s" % (len(par_fichier), dossier))


# -------------------------------------------------------------------------------------------- main


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--write", action="store_true", help="reecrit les tables de assets-brief.md")
    parser.add_argument("--prompt", metavar="CLE", help="imprime le prompt assemble d'une cle, ou 'all'")
    parser.add_argument("--annotate", metavar="DOSSIER", type=Path, help="dessine les zones sur les maquettes")
    arguments = parser.parse_args()

    cahier = json.loads(CAHIER.read_text(encoding="utf-8"))
    valider_schema(cahier)
    if erreurs:  # la suite suppose la forme : ne pas empiler des KeyError sur une faute de schema
        for e in erreurs:
            print("ERREUR " + e)
        return 1
    jetons = lire_jetons(cahier)
    if arguments.prompt:
        return imprimer_prompts(cahier, jetons, arguments.prompt)
    if arguments.annotate:
        annoter(cahier, arguments.annotate)
        return 0

    controler_jetons(cahier, jetons)
    controler_pieces(cahier)
    controler_couverture(cahier)
    page_a_jour(cahier, arguments.write)

    for e in erreurs:
        print("ERREUR " + e)
    if erreurs:
        print("check_assets_brief : %d faute(s)." % len(erreurs))
        return 1
    images = sum(len(cles(p)) for p in cahier["pieces"])
    print("check_assets_brief : %d pieces, %d images, %d exclusions -- conforme."
          % (len(cahier["pieces"]), images, len(cahier["excluded"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
