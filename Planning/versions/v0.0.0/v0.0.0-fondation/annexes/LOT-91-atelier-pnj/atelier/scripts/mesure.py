"""Les mesures E4 du protocole (§5). Usage : py -3.13 mesure.py <candidat.png> [attendu, ex. 6,8,8,4,6,8]"""
import sys
import numpy as np
from PIL import Image
GRILLE = "--grille" in sys.argv; argv = [a for a in sys.argv if a != "--grille"]   # --grille : planche normalisée, cellules 192 fixes
P = argv[1]; attendu = [int(x) for x in (argv[2] if len(argv) > 2 else "6,8,8,4,6,8").split(",")]
im = np.asarray(Image.open(P).convert("RGBA")).astype(int); H, W, _ = im.shape
a = im[:, :, 3]
print("1. taille", W, H, "| valeurs d'alpha", np.unique(a).size, "| semi-transparents", int(((a > 0) & (a < 255)).sum()),
      "| opaques (255)", int((a == 255).sum()), "| halo < 32", int(((a > 0) & (a < 32)).sum()))
mask = a > 127
rows = mask.sum(axis=1) > 0
bands, inr = [], False
for y, v in enumerate(rows):
    if v and not inr: y0, inr = y, True
    if not v and inr: inr = False; bands.append((y0, y))
if inr: bands.append((y0, H))
bands = [b for b in bands if b[1] - b[0] >= 8]      # miettes ignorées
print("2. rangées", len(bands), [b[1]-b[0] for b in bands])
comptes, verdict = [], True
for k, (y0, y1) in enumerate(bands, 1):
    cols = mask[y0:y1].sum(axis=0) > 0
    cells, inc, gap = [], False, 0
    for x, v in enumerate(cols):
        if v:
            if not inc: x0, inc = x, True
            gap = 0
        elif inc:
            gap += 1
            if gap >= 6: inc = False; cells.append((x0, x-gap+1))
    if inc: cells.append((x0, W))
    if GRILLE: cells = [(c * 192, c * 192 + 192) for c in range(8) if mask[y0:y1, c * 192:c * 192 + 192].any()]
    feet = [int(np.where(mask[y0:y1, c0:c1].any(axis=1))[0].max()) + y0 for c0, c1 in cells]
    comptes.append(len(cells))
    print(f"3. rangée {k}: {len(cells)} images, largeurs {[c1-c0 for c0,c1 in cells]}, pieds {feet} (écart {max(feet)-min(feet)})")
y0, y1 = bands[0]; crop = im[y0:y1, :, :3].sum(axis=2) * mask[y0:y1]
g = np.abs(np.diff(crop, axis=1)).sum(axis=0).astype(float); g -= g.mean()
ac = np.correlate(g, g, "full")[len(g)-1:]
print("4. pas de pixel (autocorrélation)", sorted(range(2, 9), key=lambda s: -ac[s])[:3])
y0, y1 = bands[0]; cols = mask[y0:y1].any(axis=0); x0 = int(np.argmax(cols))
x1 = x0
while x1 < W and mask[y0:y1, x1].any(): x1 += 1
hy = np.where(mask[y0:y1, x0:x1].any(axis=1))[0]
print("5. hauteur de la 1re image", int(hy.max() - hy.min()) + 1, "px (cible ~90 à pas 2 : 45 pixels d'art, la taille des héros du Colisée)")
ok = comptes == [n for n in attendu]
print("VERDICT comptes :", "OK" if ok else f"KO attendu {attendu}, lu {comptes}")
