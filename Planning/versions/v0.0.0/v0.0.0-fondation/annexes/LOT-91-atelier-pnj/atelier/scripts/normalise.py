"""Ramène une planche générée au format exact (étape R, préfigure T4).
Usage : py -3.13 normalise.py <candidat.png> <dossier_sortie> <planche|marche|idle|hit|death|attack|cast|corps|effets> [--hauteur 45 | --facteur-de corps.json] [--vides 3,4] [--gabarit moyen|grand]

Dispositions (voir disposition_*.md) :
  planche : 1536x2048, les 4 rangées du corps puis les 4 des effets : un seul appel, un seul personnage
           (décision du 16 septembre ; corps et effets séparés divergeaient)
  corps  : 1536x1024, 4 rangées de 256 ; idle 6, walk 8, hit 4 en cellules de 192 (bandes 48x64, pieds à x = 24),
           death 6 en cellules de 256 (bandes 96x64, pieds à x = 48)
  effets : 1536x1024, 4 x 4 cellules de 384x256, pieds au tiers gauche ; attack 1-4, 5-8, cast 1-4, 5-8
           (bandes 96x64, pieds à x = 32, les effets partant à droite)
Rangées et images sont lues par profil (le générateur ne tient pas la grille au pixel), l'alpha est
binarisé à 127, puis :
  --hauteur N      ramène la pose de garde à N pixels d'art (idle au corps, attack 8 aux effets) ;
  --facteur-de F   reprend le facteur d'un rapport précédent ;
  --vides 3,4      rangées attendues vides (Jade n'a pas de cast) ;
  --gabarit grand  créature Grande (atelier des monstres, LOT-93) : bandes 96x96, `death` comprise,
                   et 192x96 pour attack et cast (pieds à x = 64) ; la planche garde sa grille, et ses
                   cellules de mort passent à 192 px. Par défaut, moyen.
  --garde-finale hit  l'animation n'a qu'une image de moins que prévu, sa dernière pose (la garde)
                   manquante : on lit les images dessinées, et la dernière est l'image 1 d'idle, la
                   pose de garde de la même planche (soldat Ironhand, LOT-93, tour 1 : hit en 3).
Sans facteur, la planche est supposée déjà au pas 2 et on prend un pixel sur deux.
Sorties : <dossier>/<disposition>.png, la planche au format exact du prompt (pixels d'art de 2 px,
pieds à 40 px du bas de la cellule) ; <dossier>/bandes/<anim>.png, une image par cellule de bande,
pieds en bas (y = 63) ; <dossier>/<disposition>.json, le rapport.
Code 1 si un compte diffère, ou si une image sort de sa cellule de planche ou de bande."""
import sys
import json
import pathlib
import numpy as np
from PIL import Image

# rangée : (animation, images, largeur de cellule de planche, pieds dans la cellule en fraction,
#          largeur de bande en px d'art, pieds dans la bande en px d'art)
DISPOSITIONS = {
    "planche": None,   # corps + effets, rempli plus bas
    "corps": [("idle", 6, 192, 1 / 2, 48, 24), ("walk", 8, 192, 1 / 2, 48, 24), ("hit", 4, 192, 1 / 2, 48, 24),
              ("death", 6, 256, 1 / 2, 96, 48)],
    "effets": [("attack", 4, 384, 1 / 3, 96, 32), ("attack", 4, 384, 1 / 3, 96, 32),
               ("cast", 4, 384, 1 / 3, 96, 32), ("cast", 4, 384, 1 / 3, 96, 32)],
    "marche": [("walk", 8, 192, 1 / 2, 48, 24)],   # 1536x256, le rang de marche seul (tour 6 d'Anariel)
}
GABARIT = sys.argv[sys.argv.index("--gabarit") + 1] if "--gabarit" in sys.argv else "moyen"
if GABARIT == "grand":
    # Une créature Grande tient 2 x 2 cases : la cellule de bande double dans les deux sens. Couchée,
    # elle ne s'allonge pas plus que debout ne la faisait large (un fauve tombe sur le flanc) : la mort
    # garde la cellule de base, à la différence du personnage Moyen, qui debout est deux fois plus
    # haut que large.
    DISPOSITIONS["corps"] = [("idle", 6, 192, 1 / 2, 96, 48), ("walk", 8, 192, 1 / 2, 96, 48), ("hit", 4, 192, 1 / 2, 96, 48),
                             ("death", 6, 192, 1 / 2, 96, 48)]
    DISPOSITIONS["effets"] = [(a, 4, 384, 1 / 3, 192, 64) for a in ("attack", "attack", "cast", "cast")]
    DISPOSITIONS["marche"] = DISPOSITIONS["corps"][1:2]
elif GABARIT != "moyen":
    raise SystemExit(f"gabarit inconnu : {GABARIT} (moyen ou grand)")
DISPOSITIONS["planche"] = DISPOSITIONS["corps"] + DISPOSITIONS["effets"]
# passes par animation (décision de sortie du PoC) : les rangées de la planche, seules
DISPOSITIONS.update({"idle": DISPOSITIONS["corps"][0:1], "hit": DISPOSITIONS["corps"][2:3], "death": DISPOSITIONS["corps"][3:4],
                     "attack": DISPOSITIONS["effets"][0:2], "cast": DISPOSITIONS["effets"][2:4]})
CH = 256
PAS, BASE, BANDE_H = 2, 40, 96 if GABARIT == "grand" else 64
TOLERANCE = 4   # px d'art rognés au bord d'une bande : avertissement au-delà de 0, erreur au-delà de 4


def option(nom, conv=str):
    return conv(sys.argv[sys.argv.index(nom) + 1]) if nom in sys.argv else None


src, sortie, dispo = sys.argv[1], pathlib.Path(sys.argv[2]), sys.argv[3]
vides = [int(v) for v in option("--vides").split(",")] if option("--vides") else []
attendu = [(r[0], 0 if i + 1 in vides else r[1]) for i, r in enumerate(DISPOSITIONS[dispo])]

im = np.asarray(Image.open(src).convert("RGBA")).copy()
mask = im[:, :, 3] > 127
# l'interface ChatGPT ne rend pas la taille demandée (1086x1448 pour 1536x2048) : la grille du prompt
# est ramenée à la taille reçue
ECH_X, ECH_Y = im.shape[1] / 1536, im.shape[0] / (CH * len(DISPOSITIONS[dispo]))
im[:, :, 3] = np.where(mask, 255, 0); im[~mask] = 0


def plages(profil, ecart, mini):
    out, dedans, trou = [], False, 0
    for i, v in enumerate(profil):
        if v:
            if not dedans: debut, dedans = i, True
            trou = 0
        elif dedans:
            trou += 1
            if trou >= ecart: dedans = False; out.append((debut, i - trou + 1))
    if dedans: out.append((debut, len(profil)))
    return [p for p in out if p[1] - p[0] >= mini]


erreurs, avert = [], []
pleines = [i for i, (_, n) in enumerate(attendu) if n]
rangees = plages(mask.any(axis=1), 1, 8)
if len(rangees) != len(pleines):
    # des éclats flottent entre deux rangées : on revient à la grille de 256 px du prompt
    bornes = [round(r * CH * ECH_Y) for r in range(len(DISPOSITIONS[dispo]) + 1)]
    grille = [(y0, y1) for y0, y1 in zip(bornes, bornes[1:]) if mask[y0:y1].any()]
    avert.append(f"{len(rangees)} rangées lues par profil ; lecture par la grille ({len(grille)} rangées)")
    rangees = grille
if len(rangees) != len(pleines):
    erreurs.append(f"{len(rangees)} rangées lues, {len(pleines)} attendues")


def cadrer(y0, y1, x0, x1):
    ys = np.where(mask[y0:y1, x0:x1].any(axis=1))[0]
    return im[y0 + ys.min(): y0 + ys.max() + 1, x0:x1]


def morceaux(y0, y1, n):
    """Images d'une rangée.
    1. Plages de colonnes séparées par au moins 6 px vides ; un morceau sans pixel au sol de la
       rangée (étincelles, éclats) rejoint son voisin le plus proche.
    2. Si le compte n'y est pas (un rayon touche la figure voisine), on repart des pieds : les
       groupes de colonnes qui ont des pixels au sol, fondus deux à deux au plus proche jusqu'à n
       (une pointe d'épée au sol fait un groupe de trop), et l'on coupe entre deux pieds au milieu
       du plus long vide, ou à défaut à la colonne la moins remplie."""
    ys = np.where(mask[y0:y1].any(axis=1))[0]
    sol = y0 + ys.max() + 1
    pied = max(4, (ys.max() - ys.min()) // 10)
    ps = plages(mask[y0:y1].any(axis=0), 6, 4)
    au_sol = [mask[sol - pied:sol, x0:x1].any() for x0, x1 in ps]
    out = [list(p) for p, s in zip(ps, au_sol) if s] or [list(p) for p in ps]
    for (x0, x1), s in zip(ps, au_sol):
        if s or not any(au_sol): continue
        v = min(out, key=lambda o: min(abs(x0 - o[1]), abs(o[0] - x1)))
        v[0], v[1] = min(v[0], x0), max(v[1], x1)
    if len(out) == n or n == 0:
        return [tuple(o) for o in out]
    pieds = [list(g) for g in plages(mask[sol - pied:sol].any(axis=0), 6, 4)]
    while len(pieds) > n:
        i = min(range(len(pieds) - 1), key=lambda k: pieds[k + 1][0] - pieds[k][1])
        pieds[i][1] = pieds.pop(i + 1)[1]
    if len(pieds) != n:
        return [tuple(o) for o in out]
    remplissage = mask[y0:y1].sum(axis=0)
    coupes = [0]
    for (_, a1), (b0, _) in zip(pieds, pieds[1:]):
        vides = plages(remplissage[a1:b0] == 0, 1, 1)
        if vides:
            v0, v1 = max(vides, key=lambda v: v[1] - v[0]); coupes.append(a1 + (v0 + v1) // 2)
        else:
            coupes.append(a1 + int(np.argmin(remplissage[a1:b0])))
    coupes.append(mask.shape[1])
    res = []
    for c0, c1 in zip(coupes, coupes[1:]):
        xs = np.where(remplissage[c0:c1] > 0)[0]
        res.append((c0 + xs.min(), c0 + xs.max() + 1))
    # les deux pieds écartés d'une même figure font deux groupes : la coupe fabrique alors une image
    # (Nakral et Jade, tour 2 : 3 images de hit rendues, 4 lues, dont une de 7 à 15 px d'art). Un morceau
    # de moins de la moitié de la largeur médiane n'est pas une image : on rend la lecture brute, fausse.
    largeurs = [b - a for a, b in res]
    if min(largeurs) < np.median(largeurs) / 2:
        return [tuple(o) for o in out]
    return res


def scinder(lus, y0, y1, n):
    """4. Une image de moins que prévu, et la plus large fait près de deux fois la médiane : deux
    figures collées (le lion couché du LOT-93, death 5 et 6). On la coupe à la colonne la moins
    remplie de son tiers central."""
    if len(lus) != n - 1 or n < 3:
        return None
    largeurs = [b - a for a, b in lus]
    i = int(np.argmax(largeurs))
    if largeurs[i] < 1.6 * np.median(largeurs):
        return None
    a, b = lus[i]
    remplissage = mask[y0:y1, a:b].sum(axis=0)
    t0, t1 = (b - a) // 3, 2 * (b - a) // 3
    c = a + t0 + int(np.argmin(remplissage[t0:t1]))
    gauche = np.where(mask[y0:y1, a:c].any(axis=0))[0]
    droite = np.where(mask[y0:y1, c:b].any(axis=0))[0]
    return lus[:i] + [(a, a + gauche.max() + 1), (c + droite.min(), b)] + lus[i + 1:]


def par_la_grille(y0, y1, n, cw):
    """3. Dernier recours, quand les figures se touchent (le lion Grand du LOT-93, tour 1 : huit images
    de marche collées, sur la grille) : on coupe près des bornes de cellule du prompt, ramenées à la
    taille reçue, à la colonne la moins remplie à ±20 % d'une cellule. Retenu seulement si chaque
    cellule a son dessin et qu'aucune coupe ne tranche une colonne pleine ; sinon, rien."""
    remplissage = mask[y0:y1].sum(axis=0)
    pas = cw * ECH_X
    fenetre = max(1, int(pas * 0.2))
    coupes = [0]
    for k in range(1, n):
        b = int(round(k * pas))
        a0, a1 = max(coupes[-1] + 1, b - fenetre), min(mask.shape[1] - 1, b + fenetre)
        if a0 >= a1:
            return None
        c = a0 + int(np.argmin(remplissage[a0:a1]))
        if remplissage[c] > 0.5 * (y1 - y0):
            return None
        coupes.append(c)
    coupes.append(min(mask.shape[1], int(round(n * pas + fenetre))))
    res = []
    for c0, c1 in zip(coupes, coupes[1:]):
        xs = np.where(remplissage[c0:c1] > 0)[0]
        if xs.size == 0:
            return None
        res.append((c0 + xs.min(), c0 + xs.max() + 1))
    return res


images = {}   # index de rangée attendue -> [(bloc écran, x0 dans la source)]
garde_finale = option("--garde-finale")
for r, (y0, y1) in zip(pleines, rangees):
    n = attendu[r][1] - (1 if attendu[r][0] == garde_finale else 0)
    lus = morceaux(y0, y1, n)
    if n and len(lus) != n:
        coupe = scinder(lus, y0, y1, n)
        if coupe:
            avert.append(f"rangée {r+1} ({attendu[r][0]}) : deux figures collées, coupées au plus creux")
            lus = coupe
    if n and len(lus) != n:
        grille = par_la_grille(y0, y1, n, DISPOSITIONS[dispo][r][2])
        if grille:
            avert.append(f"rangée {r+1} ({attendu[r][0]}) : {len(lus)} images séparées, {len(grille)} lues par la grille")
            lus = grille
    images[r] = [(cadrer(y0, y1, x0, x1), x0) for x0, x1 in lus]
if garde_finale:
    rang = next((i for i, (a, _) in enumerate(attendu) if a == garde_finale), None)
    if rang is None or rang not in images or 0 not in images:
        raise SystemExit(f"--garde-finale {garde_finale} : pas de rangée {garde_finale} ou pas d'idle lue")
    # la pose de garde telle quelle ; son x source ne sert qu'à la mort, jamais à cette rangée
    images[rang].append(images[0][0])
    avert.append(f"{garde_finale} {len(images[rang])} : la pose de garde d'idle 1 (--garde-finale)")

facteur = None
if option("--facteur-de"):
    facteur = json.load(open(option("--facteur-de"), encoding="utf-8"))["facteur"]
elif option("--hauteur", int) and images:
    # repère : la pose de garde. Corps : médiane de la 1re rangée (idle). Effets : attack 8, que le
    # prompt demande presque identique à idle. Chaque planche a son facteur : le générateur ne rend
    # pas le personnage à la même taille d'une planche à l'autre.
    if dispo == "death":
        # la mort seule : l'image 1, encore debout (la médiane tomberait sur les corps couchés)
        h = float(images[min(images)][0][0].shape[0])
    elif dispo in ("effets", "attack"):
        lus = images.get(1, [])
        h = float(lus[3][0].shape[0]) if len(lus) >= 4 else float(np.median([b.shape[0] for b, _ in images[min(images)]]))
    else:
        h = float(np.median([b.shape[0] for b, _ in images[min(images)]]))
    facteur = option("--hauteur", int) / h
    print(f"hauteur de la pose de garde {h:.0f} px écran -> {option('--hauteur', int)} px d'art (facteur {facteur:.3f})")


def vers_art(bloc):
    if facteur:
        taille = (max(1, round(bloc.shape[1] * facteur)), max(1, round(bloc.shape[0] * facteur)))
        return np.asarray(Image.fromarray(bloc).resize(taille, Image.NEAREST))
    return bloc[::PAS, ::PAS]


planche = np.zeros((CH * len(DISPOSITIONS[dispo]), 1536, 4), dtype=np.uint8)
bandes, rapport = {}, []
centre_mort = 0.0   # centre, dans la bande, de la dernière image de mort placée
for r, (anim, n) in enumerate(attendu):
    _, _, CW, fx, BW, AX = DISPOSITIONS[dispo][r]
    lus = images.get(r, [])
    if len(lus) != n:
        erreurs.append(f"rangée {r+1} ({anim}) : {len(lus)} images lues, {n} attendues")
    for c, (bloc, x0) in enumerate(lus[:n]):
        art = vers_art(bloc); h, w = art.shape[:2]
        # ancre aux pieds : centre des pixels du bas, pas de la boîte, qu'un effet décale
        pied = np.where(art[-max(2, h // 8):, :, 3] > 0)[1]
        dx = int(round(pied.mean())) if pied.size else w // 2
        if anim == "death":
            # couchée, plus de pieds lisibles : on suit la position dans la cellule du générateur,
            # recalée pour que l'image 1, debout, ait ses pieds où idle les a (ni saut à l'entrée, ni dans la chute)
            grille = int(round(((c * CW + CW * fx) * ECH_X - x0) * (facteur or 1 / PAS)))
            if c == 0: recalage = grille - dx
            dx = grille - recalage
            # le générateur a pu dériver hors de sa cellule (lion du LOT-93, death 6) : l'image qui
            # sortirait de la bande garde le centre de la précédente, couchée au même endroit
            if c > 0 and (AX - dx < 0 or AX - dx + w > BW):
                dx = int(round(AX + w / 2 - centre_mort))
                avert.append(f"death {c + 1} : hors de sa cellule, calée sur le centre de la précédente")
            centre_mort = AX - dx + w / 2
        k = len(bandes.get(anim, [])) + 1
        # planche au format du prompt : pixels d'art de 2 px
        ecran = np.repeat(np.repeat(art, PAS, axis=0), PAS, axis=1)
        x, y = c * CW + int(CW * fx) - dx * PAS, r * CH + CH - BASE - h * PAS
        if x < c * CW or x + w * PAS > (c + 1) * CW or y < r * CH:
            erreurs.append(f"{anim} {k} : {w}x{h} px d'art, ancre à {dx}, sort de la cellule de planche {CW}x{CH}")
        else:
            z = planche[y:y + h * PAS, x:x + w * PAS]; z[ecran[:, :, 3] > 0] = ecran[ecran[:, :, 3] > 0]
        # bande : pixels d'art, pieds en bas, à AX
        bx, by = AX - dx, BANDE_H - h
        rogne = max(0, -bx, bx + w - BW, -by)
        if rogne > TOLERANCE:
            erreurs.append(f"{anim} {k} : {w}x{h} px d'art, ancre à {dx}, sort de la bande {BW}x{BANDE_H} (ancre {AX}) de {rogne} px")
        elif rogne:
            avert.append(f"{anim} {k} : {rogne} px rognés au bord de la bande")
        cel = np.zeros((BANDE_H, BW, 4), dtype=np.uint8)
        sx0, sy0, dx0, dy0 = max(0, -bx), max(0, -by), max(0, bx), max(0, by)
        part = art[sy0:, sx0:][:BANDE_H - dy0, :BW - dx0]
        cible = cel[dy0:dy0 + part.shape[0], dx0:dx0 + part.shape[1]]
        cible[part[:, :, 3] > 0] = part[part[:, :, 3] > 0]
        bandes.setdefault(anim, []).append(cel)
        rapport.append({"rangee": r + 1, "animation": anim, "image": k, "largeur_art": w, "hauteur_art": h, "ancre_x": dx})


def mouvement(cels):
    """Écart moyen d'alpha entre deux images consécutives (boucle comprise)."""
    a = [c[:, :, 3] > 0 for c in cels]
    return float(np.mean([(a[k] ^ a[(k + 1) % len(a)]).sum() for k in range(len(a))]))


# la marche bouge-t-elle ? rapport walk / idle ; calibré sur Anariel : figée 2,1 (tour 4), à moitié lue 3,1
# (tour 5), retenue 4,2 (tour 6). Sous 4, l'étape M (marche en second appel) est à faire.
marche = None
if len(bandes.get("walk", [])) == 8 and len(bandes.get("idle", [])) == 6:
    marche = round(mouvement(bandes["walk"]) / max(1.0, mouvement(bandes["idle"])), 1)
    if marche < 4:
        avert.append(f"walk bouge peu (rapport walk/idle {marche} < 4) : étape M à faire")
sortie.mkdir(parents=True, exist_ok=True); (sortie / "bandes").mkdir(exist_ok=True)
Image.fromarray(planche).save(sortie / f"{dispo}.png")
for anim, cels in bandes.items():
    Image.fromarray(np.concatenate(cels, axis=1)).save(sortie / "bandes" / f"{anim}.png")
json.dump({"source": src, "disposition": dispo, "gabarit": GABARIT, "facteur": facteur, "attendu": attendu, "erreurs": erreurs, "avertissements": avert, "rapport_walk_idle": marche, "images": rapport},
          open(sortie / f"{dispo}.json", "w", encoding="utf-8"), indent=2, ensure_ascii=False)
print(f"{len(rapport)} images ; bandes " + ", ".join(f"{a} {len(c)}x{c[0].shape[1]}" for a, c in bandes.items()))
for e in avert: print("AVERTISSEMENT", e)
for e in erreurs: print("ERREUR", e)
sys.exit(1 if erreurs else 0)
