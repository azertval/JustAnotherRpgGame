"""Atelier des PNJ (LOT-91), étape E : l'index et les rendus de référence, depuis le Compendium.
Usage :
  py -3.13 references.py index                 # pnj/index.json : slug, nom, page des 160 fiches (sommaire)
  py -3.13 references.py rendus <slug> [...]   # TRAVAIL/<slug>/ref/ref3_portrait.png et ref4_figurine_page.png

Le Compendium (Documentation/SourceBook/VTT/Character Compendium - High.pdf, hors dépôt, EX-CNT-023) est une
double page à pagination simple : page PDF = numéro de fiche, portrait peint à gauche, figurine dorée à droite.
Les noms viennent du sommaire (pages 1-3) ; la page 44 n'y figure pas, son titre ouvre la page.
ref4_figurine_page.png est à recadrer à la main sur la figurine seule (ref4_figurine.png). Taille, type et
description se lisent sur la page au moment de traiter le PNJ."""
import json
import re
import sys
import unicodedata
import pymupdf
from prompts import DEPOT, PNJ, TRAVAIL

PDF = DEPOT.parents[2] / "SourceBook" / "VTT" / "Character Compendium - High.pdf"
PREMIERE, DERNIERE = 4, 163


def slug_de(nom):
    base = unicodedata.normalize("NFKD", nom.split(",")[0]).encode("ascii", "ignore").decode().lower()
    return re.sub(r"[^a-z0-9]+", "_", base).strip("_")


def index(d):
    noms = {}
    for n in (1, 2, 3):
        for m in re.finditer(r"^(.+?)\s*(?:\.\s*){3,}\s*(\d{1,3})\s*$", d[n - 1].get_text(), re.M):
            noms.setdefault(int(m.group(2)), m.group(1).strip())
    fiches = []
    for page in range(PREMIERE, DERNIERE + 1):
        nom = noms.get(page)
        if nom is None:
            p = d[page - 1]; W = p.rect.width
            lignes = [l.strip() for l in p.get_text(clip=pymupdf.Rect(W / 2, 0, W, p.rect.height)).splitlines()]
            nom = " ".join(lignes[1:3]).title()
        fiches.append({"slug": slug_de(nom), "nom": nom, "page": page})
    slugs = [f["slug"] for f in fiches]
    assert len(set(slugs)) == len(slugs), "slugs en double"
    (PNJ / "index.json").write_text(json.dumps(fiches, indent=1, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    print(f"{len(fiches)} fiches")


def rendus(d, slugs):
    fiches = {f["slug"]: f for f in json.loads((PNJ / "index.json").read_text(encoding="utf-8"))}
    for slug in slugs:
        p = d[fiches[slug]["page"] - 1]; W, H = p.rect.width, p.rect.height
        out = TRAVAIL / slug / "ref"; out.mkdir(parents=True, exist_ok=True)
        p.get_pixmap(dpi=300, clip=pymupdf.Rect(0, 0, W / 2, H)).save(out / "ref3_portrait.png")
        p.get_pixmap(dpi=300, clip=pymupdf.Rect(W / 2, 0, W, H)).save(out / "ref4_figurine_page.png")
        print(f"{slug} : {out} (recadrer ref4_figurine_page.png en ref4_figurine.png)")


if __name__ == "__main__":
    doc = pymupdf.open(PDF)
    index(doc) if sys.argv[1] == "index" else rendus(doc, sys.argv[2:])
