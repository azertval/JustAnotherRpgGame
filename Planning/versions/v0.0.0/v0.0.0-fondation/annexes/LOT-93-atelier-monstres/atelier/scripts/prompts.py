"""Atelier des monstres (LOT-93) : chemins, fiches et prompts d'une étape pour une créature.
Le pendant de LOT-91-atelier-pnj/atelier/scripts/prompts.py, pour le cas SANS référence visuelle : pas
d'étape « références », pas de portrait peint ni de figurine ; la créature est décrite par le texte seul
de son bloc B, et le style est celui des PNJ (style.txt du LOT-91, gelé), pour que la bête et la figurine
qui l'affronte soient du même monde.

Deux racines :
  DEPOT   = Planning/versions/v0.0.0/v0.0.0-fondation/annexes/LOT-93-atelier-monstres/atelier/ (versionné) : prompts/, ancres/,
            monstres/<slug>/ (fiche.json, prompt_b.txt, portrait.png, palette.txt) et monstres/index.json ;
  TRAVAIL = variable MONSTRES_ATELIER, sinon D:/JustAnotherDnDGame-monstres (hors dépôt) :
            <slug>/token|tourK|final/, chatgpt/.

fiche.json : slug, nom, creature (l'identifiant du catalogue, ou null tant que le bloc attend son lot,
nommé par « attend »), taille, gabarit (moyen | grand), corps (humanoide | bete), hauteur (pixels d'art
de la pose de garde, pour normalise.py), cast (vrai si la créature lance des sorts).

Étapes :
  portrait : prompts/portrait_creature.txt + CREATURE/LOOK/SILHOUETTE du bloc B ; ref = l'ancre de
             portrait des PNJ (Anariel), pour le style seul.
  planche  : prompts/disposition_<gabarit>.txt + style + bloc B ;
             refs = la planche d'ancrage, l'échelle des héros, le portrait pixel art de la créature.
  marche   : la rangée de marche seule ; refs = la planche normalisée de la créature, puis idem.
  idle|hit|death|attack|cast : passe par animation ; prompts/disposition_reprise.txt + les lignes « Row »
             de cette animation, renumérotées ; refs comme la marche."""
import json
import os
import pathlib
import re

DEPOT = pathlib.Path(__file__).resolve().parents[1]
TRAVAIL = pathlib.Path(os.environ.get("MONSTRES_ATELIER", r"D:/JustAnotherDnDGame-monstres"))
PROMPTS, ANCRES, MONSTRES = DEPOT / "prompts", DEPOT / "ancres", DEPOT / "monstres"
# l'atelier des PNJ, dont on reprend le style gelé, les ancres et les scripts de normalisation
PNJ_ATELIER = DEPOT.parents[1] / "LOT-91-atelier-pnj" / "atelier"
PNJ_ANCRES, PNJ_SCRIPTS = PNJ_ATELIER / "ancres", PNJ_ATELIER / "scripts"

# animation -> (rangées de la planche, largeur de cellule de planche par gabarit)
REPRISES = {"idle": [1], "hit": [3], "death": [4], "attack": [5, 6], "cast": [7, 8]}
CELLULES = {"moyen": {1: 192, 2: 192, 3: 192, 4: 256, 5: 384, 6: 384, 7: 384, 8: 384},
            "grand": {1: 192, 2: 192, 3: 192, 4: 192, 5: 384, 6: 384, 7: 384, 8: 384}}
TAILLES = {"portrait": "1024x1024", "planche": "1536x2048", "marche": "1536x256",
           **{a: f"1536x{256 * len(r)}" for a, r in REPRISES.items()}}
CADRAGES = {
    "humanoide": "FRAMING: head and shoulders bust, three-quarter view facing the viewer's right, the head at "
                 "about 40% of the height, shoulders touching the bottom edge.",
    "bete": "FRAMING: the head and the front of the chest, three-quarter view facing the viewer's right, the eyes "
            "at about 40% of the height, the chest touching the bottom edge.",
}
VUES = {
    "humanoide": "VIEW AND SCALE: same viewpoint and camera as reference image 1 (front view, slightly from "
                 "above). The creature's right hand is on the viewer's left; the weapon is held in the hand named "
                 "in the WEAPON line. Its size is given by the SIZE line, measured against the heroes of reference "
                 "image 2 (about 90 px tall). The creature, its equipment and colours are IDENTICAL in every frame.",
    "bete": "VIEW AND SCALE: three-quarter view from the front, slightly from above, the same camera height as "
            "reference image 1, the creature facing the viewer's right. Its size is given by the SIZE line, measured "
            "against the heroes of reference image 2 (about 90 px tall). The creature, its markings and colours are "
            "IDENTICAL in every frame: same head, same body, same proportions.",
}


def lire(p):
    """Le texte d'un fichier de prompt, sans sa ligne de version (« LAYOUT v1 — … », notes en français)."""
    t = pathlib.Path(p).read_text(encoding="utf-8").strip()
    return re.sub(r"\A[A-Z]+ v\d+ — .*\n+", "", t)


def fiche(slug):
    chemin = MONSTRES / slug / "fiche.json"
    if not chemin.exists():
        raise SystemExit(f"{slug} : {chemin} absent ; écrire la fiche (étape B) d'abord")
    f = json.loads(chemin.read_text(encoding="utf-8"))
    if f["gabarit"] not in CELLULES:
        raise SystemExit(f"{slug} : gabarit {f['gabarit']!r} inconnu (moyen ou grand)")
    return f


def style(f):
    """Le style gelé des PNJ, dont le paragraphe de vue et d'échelle est remplacé : une bête n'a pas la
    taille d'un héros, et ne se voit pas de face."""
    paragraphes = lire(PNJ_ATELIER / "prompts" / "style.txt").split("\n\n")
    return "\n\n".join(VUES[f["corps"]] if p.startswith("VIEW AND SCALE") else p for p in paragraphes)


def blocs_b(slug):
    """Le bloc B découpé en rubriques : une ligne « MOT: » ouvre une rubrique, les suivantes la continuent."""
    out = []
    for ligne in lire(MONSTRES / slug / "prompt_b.txt").splitlines():
        m = re.match(r"([A-Z][A-Z ]+?)( \(.*?\))?:", ligne)
        if m: out.append((m.group(1), [ligne]))
        elif out: out[-1][1].append(ligne)
    return out


def corps(f):
    return lire(PROMPTS / f"corps_{f['corps']}.txt")


def marche(f, slug):
    """La marche propre à la créature (monstres/<slug>/marche.txt), sinon celle de son corps."""
    propre = MONSTRES / slug / "marche.txt"
    if propre.exists():
        return lire(propre)
    return lire(PROMPTS / "marche_quadrupede.txt" if f["corps"] == "bete" else PNJ_ATELIER / "prompts" / "marche_jambes.txt")


def disposition(f, slug):
    return (lire(PROMPTS / f"disposition_{f['gabarit']}.txt").replace("{CORPS}", corps(f))
            .replace("{MARCHE}", marche(f, slug)).replace("{CAST}", lire(PROMPTS / ("cast.txt" if f["cast"] else "sans_cast.txt"))))


def reprise(anim, f, slug):
    """La disposition d'une passe par animation : les consignes de ses rangées reprises de la disposition
    du gabarit (une seule source), renumérotées à partir de 1. « marche » est la rangée 2."""
    rangs = [2] if anim == "marche" else REPRISES[anim]
    cellule = CELLULES[f["gabarit"]][rangs[0]]
    planche = disposition(f, slug)
    lignes = []
    if cellule == 384:   # rangées d'effets : le paragraphe qui place les pieds et borne les effets
        bloc = planche[planche.index("ROWS 5 TO 8"):]
        lignes.append(bloc[:bloc.index("\nRow 5 -")].replace("ROWS 5 TO 8 — cells 384 px wide", "ROWS — cells 384 px wide"))
    else:
        lignes.append("Feet centred in the cell.")
    for i, r in enumerate(rangs, 1):
        ligne = next(l for l in planche.splitlines() if l.startswith(f"Row {r} - "))
        lignes.append(f"Row {i} - " + ligne.split(" - ", 1)[1].replace("of row 1", "of reference image 1"))
    return (lire(PROMPTS / "disposition_reprise.txt").replace("{TAILLE}", TAILLES[anim]).replace("{ANIM}", "WALK" if anim == "marche" else anim.upper())
            .replace("{NRANGS}", str(len(rangs))).replace("{CELLULE}", str(cellule)).replace("{CORPS}", corps(f))
            .replace("{RANGEES}", "\n".join(lignes)))


def planche_retenue(slug):
    """La planche normalisée du tour le plus récent : TRAVAIL/<slug>/tourK/norm/planche.png."""
    tours = sorted((TRAVAIL / slug).glob("tour*/norm/planche.png"), key=lambda p: int(re.sub(r"\D", "", p.parts[-3]) or 0))
    if not tours:
        raise SystemExit(f"{slug} : aucune planche normalisée (tourK/norm/planche.png) ; la marche et les passes viennent après la planche")
    return tours[-1]


def ancre_planche(f):
    """La planche d'ancrage : celle d'Anariel pour le gabarit Moyen (même grille) ; pour le Grand, la
    première planche Grande retenue une fois versée dans ancres/, et d'ici là celle d'Anariel, pour le
    style seul (la disposition Grande le dit)."""
    grande = ANCRES / "planche_grand.png"
    if f["gabarit"] == "grand" and grande.exists():
        return "planche_grand", grande
    return "planche_anariel", PNJ_ANCRES / "planche_anariel.png"


def etape(nom, slug):
    """(prompt, [(libellé, chemin)]) de l'étape pour cette créature."""
    f = fiche(slug)
    if nom == "portrait":
        perso = "\n".join("\n".join(l) for k, l in blocs_b(slug) if k in ("CREATURE", "LOOK", "SILHOUETTE"))
        prompt = lire(PROMPTS / "portrait_creature.txt").replace("{CADRAGE}", CADRAGES[f["corps"]]) + "\n\n" + perso
        return prompt, [("style_portrait_anariel", PNJ_ANCRES / "portrait_anariel.png")]
    pixel = MONSTRES / slug / "portrait.png"
    if not pixel.exists():
        raise SystemExit(f"{slug} : pas de monstres/{slug}/portrait.png ; faire le portrait puis palette.py avant la {nom}")
    if nom == "planche":
        texte = disposition(f, slug)
        un = ancre_planche(f)
    elif nom == "marche" or nom in REPRISES:
        texte = reprise(nom, f, slug)
        un = (f"planche_{slug}", planche_retenue(slug))
    else:
        raise SystemExit(f"étape inconnue : {nom}")
    prompt = "\n\n".join([texte, style(f), lire(MONSTRES / slug / "prompt_b.txt")])
    return prompt, [un, ("echelle", PNJ_ANCRES / "echelle.png"), ("portrait_pixel", pixel)]


def commande_normalise(nom, slug, candidat, sortie):
    """La ligne de normalise.py (atelier des PNJ) pour une image reçue de cette étape."""
    f = fiche(slug)
    dispo = "marche" if nom == "marche" else nom
    options = [f"--hauteur {f['hauteur']}", f"--gabarit {f['gabarit']}"]
    if dispo == "planche" and not f["cast"]:
        options.append("--vides 7,8")
    return f'py -3.13 "{PNJ_SCRIPTS / "normalise.py"}" "{candidat}" "{sortie}" {dispo} ' + " ".join(options)
