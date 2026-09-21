"""Atelier des monstres (LOT-93), étape I : intègre une créature finie dans le projet et la coche dans l'epic.
Usage : py -3.13 integre.py <slug> [<slug> ...]
Pour chaque créature :
  - TRAVAIL/<slug>/final/bandes/*.png (sortie de compose.py, sans erreur) -> Source/Elements/Assets/Monsters/<slug>/ ;
  - un .anim.json par bande, aux dimensions de son gabarit (epic, « Les deux gabarits ») ;
  - monstres/<slug>/portrait.png -> Assets/Monsters/<slug>/portrait.png ;
  - l'entrée de la créature dans Assets/Monsters/manifest.json : gabarit, créature du catalogue (ou le lot
    qu'elle attend), et ses animations — sans « cast » pour une créature sans sort ;
  - la case de la créature cochée dans epic.md.
Pas de vérification en jeu : le contrôle visuel se fait dans la galerie des assets (--screen=AssetGallery)."""
import json
import re
import shutil
import sys
from PIL import Image
from prompts import DEPOT, MONSTRES, TRAVAIL, fiche

PROJET = DEPOT.parents[3]
ASSETS = PROJET / "Source" / "Elements" / "Assets" / "Monsters"
EPIC = DEPOT.parent / "epic.md"
# animation : (images, durée par image, boucle, cellule large ?)
JEU = {"idle": (6, 0.15, True, False), "walk": (8, 0.10, True, False), "hit": (4, 0.08, False, False),
       "death": (6, 0.12, False, None), "attack": (8, 0.08, False, True), "cast": (8, 0.10, False, True)}
# le gabarit de l'atelier (normalise.py --gabarit) et son nom dans le manifeste, en anglais comme les tailles du catalogue
MODELES = {"moyen": "medium", "grand": "large"}


def cellule(manifeste, gabarit, anim):
    """(largeur, hauteur) de la cellule de bande : lue dans le manifeste, qui porte les gabarits."""
    g = manifeste["templates"][MODELES[gabarit]]
    large = JEU[anim][3]
    if large is None:   # la mort : large au gabarit Moyen (couché, deux fois la hauteur), de base au Grand
        large = g["deathWide"]
    return tuple(g["wideFrame"] if large else g["frame"])


manifeste = json.loads((ASSETS / "manifest.json").read_text(encoding="utf-8"))
for slug in sys.argv[1:]:
    f = fiche(slug)
    final = TRAVAIL / slug / "final"
    rapport = json.loads((final / "planche.json").read_text(encoding="utf-8"))
    if rapport["erreurs"]:
        raise SystemExit(f"{slug} : final/planche.json porte des erreurs, compose.py doit rendre 0 d'abord")
    if rapport.get("gabarit", "moyen") != f["gabarit"]:
        raise SystemExit(f"{slug} : planche normalisée au gabarit {rapport.get('gabarit', 'moyen')}, la fiche dit {f['gabarit']}")
    portrait = MONSTRES / slug / "portrait.png"
    if not portrait.exists():
        raise SystemExit(f"{slug} : {portrait} absent")
    animations = [a for a in JEU if a != "cast" or f["cast"]]
    dossier = ASSETS / slug
    if dossier.exists(): shutil.rmtree(dossier)
    dossier.mkdir(parents=True)
    for anim in animations:
        n, duree, boucle, _ = JEU[anim]
        largeur, hauteur = cellule(manifeste, f["gabarit"], anim)
        bande = final / "bandes" / f"{anim}.png"
        if not bande.exists() or Image.open(bande).size != (n * largeur, hauteur):
            taille = Image.open(bande).size if bande.exists() else "absente"
            raise SystemExit(f"{slug} : {bande.name} : {taille}, attendu {(n * largeur, hauteur)}")
        shutil.copy(bande, dossier / bande.name)
        clip = {"version": 1, "frameWidth": largeur, "frameHeight": hauteur,
                "clips": {anim: {"frames": list(range(n)), "frameDuration": duree, "loop": boucle}}}
        (dossier / f"{anim}.anim.json").write_text(json.dumps(clip, indent=2) + "\n", encoding="utf-8", newline="\n")
    shutil.copy(portrait, dossier / "portrait.png")
    entree = {"slug": slug, "template": MODELES[f["gabarit"]], "creature": f["creature"]}
    if f["creature"] is None:
        entree["awaiting"] = f["attend"]
    entree["animations"] = animations
    manifeste["monsters"] = [m for m in manifeste["monsters"] if m["slug"] != slug] + [entree]
    manifeste["monsters"].sort(key=lambda m: m["slug"])
    (ASSETS / "manifest.json").write_text(json.dumps(manifeste, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    texte = EPIC.read_text(encoding="utf-8")
    coche, n = re.subn(rf"^- \[ \] (.*`{re.escape(slug)}`)", r"- [x] \1", texte, flags=re.M)
    if n == 0 and not re.search(rf"^- \[x\] .*`{re.escape(slug)}`", texte, re.M):
        print(f"{slug} : AVERTISSEMENT, pas de ligne « - [ ] … `{slug}` » dans epic.md")
    EPIC.write_text(coche, encoding="utf-8", newline="\n")
    print(f"{slug} : intégré dans {dossier.relative_to(PROJET)} ({', '.join(animations)})")
