"""Ramène un portrait au format exact du prompt (étape R).
Usage : py -3.13 normalise_portrait.py <candidat.png> <sortie.png>
Format exact : 1024x1024, grille de 128x128 pixels d'art de 8x8, au plus 32 couleurs, fond #0B141E exact.
Écrit aussi <sortie>_128.png (un pixel d'art par pixel).
Le générateur ne tient pas la grille demandée (1254x1254 et un pas d'environ 12 px sur le premier
portrait d'Anariel) : on prend la médiane de chaque bloc de la grille 128, puis on quantifie sans
tramage (octree + k-means, qui garde l'or et les flammes que la coupe médiane délavait à 16)."""
import sys
import numpy as np
from PIL import Image
src, dst = sys.argv[1], sys.argv[2]
FOND = np.array([0x0B, 0x14, 0x1E]); COULEURS = 32
im = Image.open(src).convert("RGB")
# médiane de chaque bloc : le plus proche voisin ramassait des points gris parasites sur le visage
blocs = np.asarray(im.resize((1280, 1280), Image.NEAREST)).astype(int).reshape(128, 10, 128, 10, 3)
petit = np.median(blocs.transpose(0, 2, 1, 3, 4).reshape(128, 128, 100, 3), axis=2).astype(int)
petit[np.abs(petit - FOND).sum(axis=2) < 40] = FOND
q = Image.fromarray(petit.astype("uint8")).quantize(colors=COULEURS, method=Image.Quantize.FASTOCTREE, kmeans=4,
                                                     dither=Image.Dither.NONE).convert("RGB")
q = np.asarray(q).astype(int).copy()
q[np.abs(q - FOND).sum(axis=2) < 40] = FOND
q = Image.fromarray(q.astype("uint8"))
q.save(dst.rsplit(".", 1)[0] + "_128.png")
q.resize((1024, 1024), Image.NEAREST).save(dst)
print(f"source {im.size} ; couleurs {len(q.getcolors(1024))} ; fond exact {int((np.asarray(q) == FOND).all(axis=2).sum())} px d'art sur {128*128}")
