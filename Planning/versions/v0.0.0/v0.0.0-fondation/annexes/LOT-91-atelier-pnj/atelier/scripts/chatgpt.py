"""Prépare les envois à la main dans l'interface ChatGPT (méthode standard, poc.md §4bis).
Usage : py -3.13 chatgpt.py <portrait|planche|marche|idle|hit|death|attack|cast> <tour> <slug> [<slug>...]
Écrit chatgpt/<étape>-tour<K>/<slug>/ : prompt.txt, les pièces jointes numérotées dans l'ordre des
références, et A_ENREGISTRER_SOUS.txt, le chemin où ranger l'image rendue."""
import shutil
import sys
from prompts import ROOT, etape

nom, k, slugs = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
SORTIE = {"portrait": "token/tour{k}/candidat1.png", "planche": "tour{k}/planche/candidat1.png", "marche": "tour{k}/marche/candidat1.png"}
SORTIE.update({a: f"tour{{k}}/{a}/candidat1.png" for a in ("idle", "hit", "death", "attack", "cast")})   # passes par animation
for slug in slugs:
    prompt, refs = etape(nom, slug)
    d = ROOT / "chatgpt" / f"{nom}-tour{k}" / slug
    if d.exists(): shutil.rmtree(d)
    d.mkdir(parents=True)
    (d / "prompt.txt").write_text(prompt + "\n", encoding="utf-8")
    for i, (libelle, p) in enumerate(refs, 1):
        shutil.copy(p, d / f"{i}_{libelle}.png")
    cible = ROOT / slug / SORTIE[nom].format(k=k)
    cible.parent.mkdir(parents=True, exist_ok=True)
    (d / "A_ENREGISTRER_SOUS.txt").write_text(str(cible) + "\n", encoding="utf-8")
    print(f"{d.relative_to(ROOT)} : {len(refs)} pièces jointes, {len(prompt)} caractères -> {cible.relative_to(ROOT)}")
