"""Atelier des PNJ (LOT-91) : chemins, prompts et références d'une étape pour un PNJ.
Partagé par portrait.py et tour.py (API) et par chatgpt.py (interface, à la main) : le prompt envoyé
est le même dans les deux modes.

Deux racines :
  DEPOT   = Planning/versions/v0.0.0/v0.0.0-fondation/annexes/LOT-91-atelier-pnj/atelier/ (versionné) : prompts/, ancres/, pnj/<slug>/
            (fiche.json, profil.txt, prompt_b.txt, marche.txt, portrait.png, palette.txt) et pnj/index.json ;
  TRAVAIL = variable NPC_ATELIER, sinon D:/JustAnotherDnDGame-npc-poc (hors dépôt, EX-CNT-023) :
            <slug>/ref/ (rendus du corpus), <slug>/token|tourK|final/, chatgpt/.

Étapes :
  portrait : prompts/portrait_style.txt + CHARACTER/LOOK/WEAPON du bloc B ;
             refs = ancre de portrait, portrait peint, figurine.
  planche  : prompts/disposition_planche.txt + style.txt + bloc B ;
             refs = ancre de planche (Anariel), échelle, portrait peint, figurine, portrait pixel art.
  marche   : prompts/disposition_marche.txt + style.txt + bloc B ; refs = planche normalisée du PNJ, puis idem.
  idle|hit|death|attack|cast : passe par animation ; prompts/disposition_reprise.txt + les lignes « Row »
             de cette animation dans disposition_planche.txt, renumérotées + style.txt + bloc B ; refs comme la marche.
{MARCHE} vient de pnj/<slug>/marche.txt s'il existe (Lizz rampe), sinon de prompts/marche_jambes.txt."""
import os
import pathlib
import re

DEPOT = pathlib.Path(__file__).resolve().parents[1]
TRAVAIL = pathlib.Path(os.environ.get("NPC_ATELIER", r"D:/JustAnotherDnDGame-npc-poc"))
ROOT = TRAVAIL   # nom historique, lu par chatgpt.py, compose.py, tour.py
PROMPTS, ANCRES, PNJ = DEPOT / "prompts", DEPOT / "ancres", DEPOT / "pnj"

# animation -> (rangées de la planche, largeur de cellule)
REPRISES = {"idle": ([1], 192), "hit": ([3], 192), "death": ([4], 256), "attack": ([5, 6], 384), "cast": ([7, 8], 384)}
TAILLES = {"portrait": "1024x1024", "planche": "1536x2048", "marche": "1536x256",
           **{a: f"1536x{256 * len(r)}" for a, (r, _) in REPRISES.items()}}


def lire(p):
    """Le texte d'un fichier de prompt, sans sa ligne de version (« STYLE v1 — … », notes en français)."""
    t = pathlib.Path(p).read_text(encoding="utf-8").strip()
    return re.sub(r"\A(STYLE|PORTRAIT|LAYOUT) v\d+ — .*\n+", "", t)


def blocs_b(slug):
    """Le bloc B découpé en rubriques : une ligne « MOT: » ouvre une rubrique, les suivantes la continuent."""
    out = []
    for ligne in lire(PNJ / slug / "prompt_b.txt").splitlines():
        m = re.match(r"([A-Z][A-Z ]+?)( \(.*?\))?:", ligne)
        if m: out.append((m.group(1), [ligne]))
        elif out: out[-1][1].append(ligne)
    return out


def planche_retenue(slug):
    """La planche normalisée du tour le plus récent : TRAVAIL/<slug>/tourK/norm/planche.png."""
    tours = sorted((TRAVAIL / slug).glob("tour*/norm/planche.png"), key=lambda p: int(re.sub(r"\D", "", p.parts[-3]) or 0))
    if not tours:
        raise SystemExit(f"{slug} : aucune planche normalisée (tourK/norm/planche.png) ; la marche et les passes viennent après la planche")
    return tours[-1]


def reprise(anim):
    """La disposition d'une passe par animation : le gabarit, puis les consignes de ses rangées reprises
    telles quelles de disposition_planche.txt (une seule source), renumérotées à partir de 1."""
    rangs, cellule = REPRISES[anim]
    planche = lire(PROMPTS / "disposition_planche.txt")
    lignes = []
    if cellule == 384:   # rangées d'effets : le paragraphe qui place les pieds et borne les effets
        bloc = planche[planche.index("ROWS 5 TO 8"):]
        lignes.append(bloc[:bloc.index("\nRow 5 -")].replace("ROWS 5 TO 8 — cells 384 px wide", "ROWS — cells 384 px wide"))
    else:
        lignes.append("Feet centred in the cell.")
    for i, r in enumerate(rangs, 1):
        ligne = next(l for l in planche.splitlines() if l.startswith(f"Row {r} - "))
        lignes.append(f"Row {i} - " + ligne.split(" - ", 1)[1].replace("of row 1", "of reference image 1"))
    return (lire(PROMPTS / "disposition_reprise.txt").replace("{TAILLE}", TAILLES[anim]).replace("{ANIM}", anim.upper())
            .replace("{NRANGS}", str(len(rangs))).replace("{CELLULE}", str(cellule)).replace("{RANGEES}", "\n".join(lignes)))


def etape(nom, slug):
    """(prompt, [(libellé, chemin)]) de l'étape pour ce PNJ."""
    ref = TRAVAIL / slug / "ref"
    peint, figurine, pixel = ref / "ref3_portrait.png", ref / "ref4_figurine.png", PNJ / slug / "portrait.png"
    for p in (peint, figurine):
        if not p.exists():
            raise SystemExit(f"{slug} : {p} absent ; faire l'étape E1 (references.py, puis recadrer la figurine)")
    if nom == "portrait":
        perso = "\n".join("\n".join(l) for k, l in blocs_b(slug) if k in ("CHARACTER", "LOOK", "WEAPON"))
        perso = perso.replace("reference images 3, 4 and 5", "reference images 2 and 3")
        prompt = lire(PROMPTS / "portrait_style.txt") + "\n\n" + perso
        return prompt, [("style_portrait_anariel", ANCRES / "portrait_anariel.png"), ("portrait_peint", peint), ("figurine", figurine)]
    if not pixel.exists():
        raise SystemExit(f"{slug} : pas de pnj/{slug}/portrait.png ; faire le portrait puis palette.py avant la {nom}")
    marche = PNJ / slug / "marche.txt"
    marche = marche if marche.exists() else PROMPTS / "marche_jambes.txt"
    if nom in REPRISES:
        disposition = reprise(nom)
    else:
        disposition = lire(PROMPTS / f"disposition_{nom}.txt").replace("{MARCHE}", lire(marche))
    prompt = "\n\n".join([disposition, lire(PROMPTS / "style.txt"), lire(PNJ / slug / "prompt_b.txt")])
    un = ("planche_anariel", ANCRES / "planche_anariel.png") if nom == "planche" else (f"planche_{slug}", planche_retenue(slug))
    return prompt, [un, ("echelle", ANCRES / "echelle.png"), ("portrait_peint", peint), ("figurine", figurine), ("portrait_pixel", pixel)]
