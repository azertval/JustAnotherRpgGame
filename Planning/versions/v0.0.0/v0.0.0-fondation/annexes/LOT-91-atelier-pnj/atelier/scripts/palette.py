"""Retient un portrait et en tire la palette (étape P). Usage : py -3.13 palette.py <slug> <portrait normalisé.png> [<dossier des fiches>]
Le troisième argument sert à l'atelier des monstres (LOT-93, `atelier/monstres/`) ; par défaut, pnj/.
Copie le portrait en pnj/<slug>/portrait.png, écrit pnj/<slug>/palette.txt (sept couleurs, fond exclu, puis au plus
trois accents). Les sept couleurs sont les plus étendues ; les accents sont les couleurs vives loin des
sept (lame, magie, lueur des yeux), qui font le personnage sans couvrir de surface : sans eux, la lame
cramoisie de Nakral, la magie de Xorius et les notes de Jade sortaient de « use only these »."""
import re
import sys
import shutil
import pathlib
import colorsys
import numpy as np
from PIL import Image
from prompts import PNJ
slug, cand = sys.argv[1], pathlib.Path(sys.argv[2]); D = (pathlib.Path(sys.argv[3]) if len(sys.argv) > 3 else PNJ) / slug; D.mkdir(parents=True, exist_ok=True)
if cand.resolve() != (D / "portrait.png").resolve():
    shutil.copy(cand, D / "portrait.png")
im = np.asarray(Image.open(cand).convert("RGB")).reshape(-1, 3).astype(int)
fond = np.array([0x0B, 0x14, 0x1E])
px = im[np.abs(im - fond).sum(axis=1) > 40]          # le fond navy et ses voisins
q = Image.fromarray(px.reshape(1, -1, 3).astype("uint8")).quantize(colors=7, method=Image.Quantize.FASTOCTREE, kmeans=8)   # la coupe médiane noyait le cramoisi et l'or
pal = q.getpalette(); base = [(n, pal[3*i:3*i+3]) for n, i in sorted(q.getcolors(), reverse=True)]
# accents : pixels vifs (saturation > 0,45, ni trop sombres ni trop clairs) à plus de 90 (somme des écarts) des sept
hls = np.array([colorsys.rgb_to_hls(*(c / 255)) for c in px])
loin = np.abs(px[:, None, :] - np.array([c for _, c in base])[None]).sum(axis=2).min(axis=1) > 90
vifs = px[(hls[:, 2] > .45) & (hls[:, 1] > .25) & (hls[:, 1] < .85) & loin]
SEUIL = .005 * len(px)                                # un accent couvre au moins 0,5 % du personnage
accents = []
if len(vifs) >= SEUIL:
    qa = Image.fromarray(vifs.reshape(1, -1, 3).astype("uint8")).quantize(colors=3, method=Image.Quantize.FASTOCTREE, kmeans=8)
    pa = qa.getpalette(); accents = [(n, pa[3*i:3*i+3]) for n, i in sorted(qa.getcolors(), reverse=True) if n >= SEUIL]
lignes = ["#%02X%02X%02X %d" % (*c, n) for n, c in base] + ["#%02X%02X%02X %d accent" % (*c, n) for n, c in accents]
(D/"palette.txt").write_text("\n".join(lignes) + "\n", encoding="utf-8", newline="\n")
print("\n".join(lignes))
print("couleurs distinctes du portrait :", len(np.unique(im, axis=0)))
# le bloc B prend la palette du portrait retenu
b = (D/"prompt_b.txt").read_text(encoding="utf-8").splitlines()
i = next(k for k, l in enumerate(b) if l.startswith("PALETTE"))
j = next(k for k in range(i + 1, len(b)) if b[k].startswith("ANIMATION SET"))
# le numéro de la référence est celui que la fiche écrit déjà (5 pour un PNJ, 3 pour un monstre)
ref = re.search(r"reference image (\d+)", b[i]); ref = ref.group(1) if ref else "5"
b[i:j] = [f"PALETTE (taken from reference image {ref}; use only these plus their darker/lighter tones): "
          + ", ".join(l.split()[0] for l in lignes if "accent" not in l)
          + ("; accent colours (weapon, magic, effects): " + ", ".join(l.split()[0] for l in lignes if "accent" in l) if accents else "") + "."]
(D/"prompt_b.txt").write_text("\n".join(b) + "\n", encoding="utf-8", newline="\n")
print("prompt_b.txt : ligne PALETTE remplacée")
