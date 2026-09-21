"""Prépare les envois à la main dans l'interface ChatGPT (atelier des monstres, LOT-93).
Usage : py -3.13 chatgpt.py <portrait|planche|marche|idle|hit|death|attack|cast> <tour> <slug> [<slug>...]
Écrit chatgpt/<étape>-tour<K>/<slug>/ : prompt.txt, les pièces jointes numérotées dans l'ordre des
références, A_ENREGISTRER_SOUS.txt (le chemin où ranger l'image rendue) et NORMALISER.txt, les commandes à lancer sur l'image reçue : normalise.py (hauteur, gabarit, rangées
vides), et pour le portrait normalise_portrait.py puis palette.py, qui le retient dans monstres/<slug>/."""
import shutil
import sys
from prompts import MONSTRES, PNJ_SCRIPTS, TRAVAIL, etape, commande_normalise

nom, k, slugs = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
SORTIE = {"portrait": "token/tour{k}/candidat1.png", "planche": "tour{k}/planche/candidat1.png", "marche": "tour{k}/marche/candidat1.png"}
SORTIE.update({a: f"tour{{k}}/{a}/candidat1.png" for a in ("idle", "hit", "death", "attack", "cast")})
for slug in slugs:
    prompt, refs = etape(nom, slug)
    d = TRAVAIL / "chatgpt" / f"{nom}-tour{k}" / slug
    if d.exists(): shutil.rmtree(d)
    d.mkdir(parents=True)
    (d / "prompt.txt").write_text(prompt + "\n", encoding="utf-8")
    for i, (libelle, p) in enumerate(refs, 1):
        shutil.copy(p, d / f"{i}_{libelle}.png")
    cible = TRAVAIL / slug / SORTIE[nom].format(k=k)
    cible.parent.mkdir(parents=True, exist_ok=True)
    (d / "A_ENREGISTRER_SOUS.txt").write_text(str(cible) + "\n", encoding="utf-8")
    if nom == "portrait":
        retenu = TRAVAIL / slug / "token" / "portrait.png"
        suite = (f'py -3.13 "{PNJ_SCRIPTS / "normalise_portrait.py"}" "{cible}" "{retenu}"\n'
                 f'py -3.13 "{PNJ_SCRIPTS / "palette.py"}" {slug} "{retenu}" "{MONSTRES}"')
    else:
        suite = commande_normalise(nom, slug, cible, TRAVAIL / slug / f"tour{k}" / "norm")
    (d / "NORMALISER.txt").write_text(suite + "\n", encoding="utf-8")
    print(f"{d.relative_to(TRAVAIL)} : {len(refs)} pièce(s) jointe(s), {len(prompt)} caractères -> {cible.relative_to(TRAVAIL)}")
