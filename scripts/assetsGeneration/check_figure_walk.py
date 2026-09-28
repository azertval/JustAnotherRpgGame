#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Contrôle une marche normalisée, produit les preuves et refuse une validation incomplète.

Ne génère ni ne modifie les assets. Entrée : dossier de bandes installables, avec
walk-{se,sw,ne,nw}.png et .anim.json. Sortie : rapport JSON, galerie autonome,
gabarit de revue et consigne de reprise. Codes : 0 validé, 1 refusé, 2 à examiner.
"""
from __future__ import annotations

import argparse
import base64
import hashlib
import html
import json
import math
from pathlib import Path

import numpy as np
from PIL import Image

FACINGS = {"se": (1, 1), "sw": (-1, 1), "ne": (1, -1), "nw": (-1, -1)}
VISUAL_CHECKS = ("identity", "facing", "anatomy_equipment", "weight_transfer", "alternating_support",
                 "loop_continuity", "in_game_motion")
POLICY = {"version": 2, "speed": 2.0, "cycle_cells_tolerance": 0.03,
          "max_contact_slip_art_px": 4.0, "minimum_stance_transitions_per_foot": 2}


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def fingerprint(paths):
    digest = hashlib.sha256(json.dumps(POLICY, sort_keys=True).encode())
    for path in paths:
        data = path.read_bytes()
        digest.update(len(data).to_bytes(8, "big"))
        digest.update(data)
    return digest.hexdigest()


def number(value):
    return type(value) in (int, float) and math.isfinite(value)


def contact_slip(start, end, facing, duration, tile, speed=2.0):
    """Déplacement du même point d'appui dans le monde entre deux débuts d'image.

Les coordonnées sont locales à la cellule. Ce résidu ne mesure pas le glissement
intra-image d'une animation discrète, ni la justesse de l'annotation anatomique.
"""
    sx, sy = FACINGS[facing]
    return math.hypot(end[0] - start[0] + sx * tile[0] / 2 * speed * duration,
                      end[1] - start[1] + sy * tile[1] / 2 * speed * duration)


def check_support(support, contacts):
    """Vérifie les phases annotées, sans confondre contact inconnu et pied levé."""
    if not isinstance(support, dict):
        return [], ["Phases d'appui non renseignées : alternance non démontrée."]
    errors, pending = [], []
    for foot in ("left", "right"):
        states = support.get(foot)
        if (not isinstance(states, list) or len(states) != 8
                or any(s not in ("stance", "swing", None) for s in states)):
            errors.append(f"Phases {foot} invalides : huit valeurs stance/swing/null attendues.")
            continue
        if None in states:
            pending.append(f"Phases {foot} incomplètes : alternance non démontrée.")
            continue
        if states.count("swing") < 2 or states.count("stance") < 3:
            errors.append(f"Phases {foot} : appui et retour aérien distincts requis.")
        starts = sum(states[i] == "stance" and states[i-1] == "swing" for i in range(8))
        if starts != 1:
            errors.append(f"Phases {foot} : un seul début d'appui par cycle requis, trouvé {starts}.")
        points = contacts.get(foot) if isinstance(contacts, dict) else None
        if isinstance(points, list) and len(points) == 8:
            for i, state in enumerate(states):
                if state == "swing" and points[i] is not None:
                    errors.append(f"Phases {foot}, image {i} : contact déclaré pendant le retour aérien.")
                elif state == "stance" and points[i] is None:
                    pending.append(f"Phases {foot}, image {i} : point d'appui manquant.")
    if errors or pending:
        return errors, pending
    left, right = support["left"], support["right"]
    for i in range(8):
        if left[i] == right[i] == "swing":
            errors.append(f"Image {i} : aucun appui, phase de vol incompatible avec ce profil de marche.")
    for foot, other in ((left, right), (right, left)):
        if not any(a == "stance" and b == "swing" for a, b in zip(foot, other)):
            errors.append("Appuis non alternés : chaque pied doit porter seul le corps à son tour.")
    return errors, pending


def check_contacts(contacts, facing, duration, tile, frames):
    errors, pending, measurements = [], [], []
    width, height = frames[0].size
    if not isinstance(contacts, dict):
        return [], ["Appuis gauche/droit non renseignés."], []
    for foot in ("left", "right"):
        points = contacts.get(foot)
        if not isinstance(points, list) or len(points) != len(frames):
            pending.append(f"Appuis {foot} : huit points ou null attendus.")
            continue
        valid = True
        for i, point in enumerate(points):
            if point is None:
                continue
            if (not isinstance(point, list) or len(point) != 2
                    or not all(number(v) for v in point)
                    or not (0 <= point[0] < width and 0 <= point[1] < height)):
                errors.append(f"Appui {foot}, image {i} : coordonnées invalides.")
                valid = False
                continue
            # Autorise le contour doux, mais pas un point arbitraire dans le vide.
            x, y = map(int, point)
            patch = np.asarray(frames[i])[max(0, y-2):y+3, max(0, x-2):x+3, 3]
            if not np.any(patch > 16):
                errors.append(f"Appui {foot}, image {i} : hors silhouette.")
                valid = False
        if not valid:
            continue
        count = 0
        for i, start in enumerate(points):
            j = (i + 1) % len(points)
            if start is None or points[j] is None:
                continue
            count += 1
            slip = contact_slip(start, points[j], facing, duration, tile, POLICY["speed"])
            measurements.append({"foot": foot, "from": i, "to": j,
                                 "slip_art_px": round(slip, 3)})
            if slip > POLICY["max_contact_slip_art_px"]:
                errors.append(f"Glissement {foot} {i}→{j} : {slip:.1f} px d'art (> 4).")
        if count < POLICY["minimum_stance_transitions_per_foot"]:
            pending.append(f"Appuis {foot} insuffisants : au moins deux transitions consécutives en appui.")
    return errors, pending, measurements


def inspect_strip(folder, manifest_path, facing, review):
    stem = f"walk-{facing}"
    result = {"strip": stem, "errors": [], "pending": [], "metrics": {}}
    errors, pending = result["errors"], result["pending"]
    paths = [folder / f"{stem}.png", folder / f"{stem}.anim.json", manifest_path]
    try:
        result["fingerprint"] = fingerprint(paths)
        manifest, anim = read_json(manifest_path), read_json(paths[1])
        if not isinstance(manifest, dict) or not isinstance(anim, dict):
            raise ValueError("manifest/animation doivent être des objets")
        tile, ground = manifest["tile"], manifest["ground"]
        if (not isinstance(tile, list) or len(tile) != 2
                or not all(number(v) and v > 0 for v in tile)):
            raise ValueError("tile invalide")
        if tile != [256, 159]:
            raise ValueError("ce profil humanoïde attend un losange de 256 × 159")
        if not number(ground) or not 0 < ground < 256:
            raise ValueError("ground invalide")
        if manifest.get("frame") != [192, 256]:
            errors.append("Cellule du manifeste différente du standard 192 × 256.")
        if (anim["frameWidth"], anim["frameHeight"]) != (192, 256):
            raise ValueError("cellule de marche attendue : 192 × 256")
        clip = anim["clips"]["walk"]
        if clip["frames"] != list(range(8)) or any(type(v) is not int for v in clip["frames"]):
            raise ValueError("huit images dans l'ordre 0…7 attendues (lecture du moteur monde)")
        duration = clip["frameDuration"]
        if not number(duration) or duration <= 0:
            raise ValueError("frameDuration doit être positif et fini")
        if clip.get("loop") is not True:
            errors.append("La marche doit boucler.")
        with Image.open(paths[0]) as source:
            if source.format != "PNG" or source.mode != "RGBA":
                raise ValueError("PNG RGBA requis")
            if source.size != (192 * 8, 256):
                raise ValueError("bande attendue : 1536 × 256 (cellules installées sans pas externe)")
            sheet = source.copy()
        frames = [sheet.crop((i*192, 0, (i+1)*192, 256)) for i in range(8)]
        cycle_cells = 8 * duration * POLICY["speed"]
        result["metrics"]["cycle_cells"] = cycle_cells
        if abs(cycle_cells - 1) > POLICY["cycle_cells_tolerance"]:
            errors.append(f"Cadence : {cycle_cells:.3f} cases/cycle au lieu de 1.")
        for i, frame in enumerate(frames):
            alpha = np.asarray(frame)[..., 3]
            ys, xs = np.nonzero(alpha > 16)
            if not len(xs):
                errors.append(f"Image {i} vide.")
            elif xs.min() < 8 or xs.max() >= 184 or ys.min() == 0 or ys.max() == 255:
                errors.append(f"Image {i} : silhouette au bord ou marge latérale < 8 px.")
        # Prémultiplication : le RGB invisible ne doit pas masquer des images identiques.
        rgba = np.asarray(sheet).astype(float) / 255
        rgba[..., :3] *= rgba[..., 3:4]
        parts = np.stack(np.split(rgba, 8, axis=1))
        deltas = [float(np.abs(parts[(i+1) % 8] - parts[i]).mean()) for i in range(8)]
        result["metrics"]["adjacent_pixel_delta"] = deltas
        for i, delta in enumerate(deltas):
            if delta < 1e-7:
                errors.append(f"Images {i} et {(i+1) % 8} identiques : arrêt dans le cycle.")
        median = float(np.median(deltas))
        result["metrics"]["seam_to_median_ratio"] = deltas[-1] / median if median else None
        entry = review.get(stem, {})
        if not isinstance(entry, dict) or entry.get("fingerprint") != result["fingerprint"]:
            entry = {}
            pending.append("Revue absente ou périmée pour ces fichiers et cette politique.")
        ce, cp, measured = check_contacts(entry.get("contacts"), facing, duration, tile, frames)
        se, sp = check_support(entry.get("support"), entry.get("contacts"))
        errors.extend(se)
        pending.extend(sp)
        if isinstance(entry.get("notes"), str) and entry["notes"].strip():
            result["review_notes"] = entry["notes"]
        errors.extend(ce)
        pending.extend(cp)
        result["metrics"]["contacts"] = measured
        visual = entry.get("visual", {})
        if not isinstance(visual, dict):
            visual = {}
        if median and deltas[-1] > 2.5 * median and visual.get("loop_continuity") is not True:
            pending.append("Raccord 7→0 atypique : vérifier la continuité (heuristique pixels).")
        for check in VISUAL_CHECKS:
            if visual.get(check) is False:
                errors.append(f"Revue refusée : {check}.")
            elif visual.get(check) is not True:
                pending.append(f"Revue requise : {check}.")
        if not isinstance(entry.get("reviewer"), str) or not entry["reviewer"].strip():
            pending.append("Auteur de la revue non renseigné.")
        if not isinstance(entry.get("evidence"), str) or not entry["evidence"].strip():
            pending.append("Preuve de revue en jeu non renseignée.")
        result["preview"] = {"image": base64.b64encode(paths[0].read_bytes()).decode(),
                             "duration": duration, "tile": tile, "ground": ground,
                             "direction": FACINGS[facing]}
    except (OSError, ValueError, KeyError, TypeError, IndexError) as exc:
        errors.append(f"Entrée invalide : {exc}")
    result["status"] = "rejected" if errors else "pending" if pending else "accepted"
    return result


def gallery(results):
    cards = []
    previews = []
    for result in results:
        issues = result["errors"] + result["pending"]
        if result.get("review_notes"):
            issues.insert(0, result["review_notes"])
        cards.append(f'<section><h2>{result["strip"]} — {result["status"]}</h2>'
                     + '<ul>' + ''.join(f'<li>{html.escape(s)}</li>' for s in issues) + '</ul>'
                     + '<canvas width="640" height="400"></canvas></section>')
        previews.append(result.get("preview"))
    return """<!doctype html><html lang="fr"><meta charset="utf-8">
<title>Contrôle de marche</title><style>
body{background:#20262c;color:#ece8dd;font:16px system-ui;margin:24px}
section{background:#30383f;padding:16px;margin:16px 0}canvas{max-width:100%;background:#555b57}
button,select,input{font:inherit;margin:8px}li{margin:4px}header{position:sticky;top:0;background:#20262c;padding:8px}
</style><h1>Contrôle de marche sur sol fixe</h1>
<p>Prévisualisation simulée, pas une capture moteur. Lecture à 2 cases/s ; le ralenti ralentit
ensemble déplacement et animation. Le retour en début de trajet est volontaire.</p>
<header><button id="pause">Pause</button><button id="step">Image suivante</button>
<label>Lecture <select id="rate"><option value="1">normale</option><option value="0.25">×0,25</option></select></label>
<label>Case écran <select id="scale"><option value="100">100 px (1080p)</option><option value="200">200 px (2160p)</option><option value="256">256 px (art)</option></select></label>
<label><input id="moving" type="checkbox" checked>Déplacement</label></header>
""" + ''.join(cards) + """<script>
const data=PREVIEWS;
const canvases=[...document.querySelectorAll('canvas')];
const images=data.map(d=>{const im=new Image();if(d)im.src='data:image/png;base64,'+d.image;return im});
let time=0,last=null,paused=false;
document.getElementById('pause').onclick=()=>{paused=!paused;document.getElementById('pause').textContent=paused?'Lecture':'Pause'};
document.getElementById('step').onclick=()=>{paused=true;document.getElementById('pause').textContent='Lecture';time+=data.find(Boolean)?.duration||0.0625};
function draw(now){if(last!==null&&!paused)time+=(now-last)/1000*Number(document.getElementById('rate').value);last=now;
data.forEach((d,i)=>{const c=canvases[i],g=c.getContext('2d');g.clearRect(0,0,c.width,c.height);if(!d||!images[i].complete)return;
const k=Number(document.getElementById('scale').value)/d.tile[0],tw=d.tile[0]*k,th=d.tile[1]*k;
g.strokeStyle='#879087';g.lineWidth=1;
for(let a=-12;a<=12;a++){g.beginPath();g.moveTo(320+a*tw/2-12*tw/2,260+a*th/2+12*th/2);g.lineTo(320+a*tw/2+12*tw/2,260+a*th/2-12*th/2);g.stroke();g.beginPath();g.moveTo(320-a*tw/2-12*tw/2,260+a*th/2-12*th/2);g.lineTo(320-a*tw/2+12*tw/2,260+a*th/2+12*th/2);g.stroke()}
const n=Math.floor(time/d.duration)%8,dist=document.getElementById('moving').checked?(time*2)%2-1:0;
const x=320+d.direction[0]*tw/2*dist,y=260+d.direction[1]*th/2*dist;
g.strokeStyle='#ffe382';g.beginPath();g.moveTo(x-6,y);g.lineTo(x+6,y);g.moveTo(x,y-6);g.lineTo(x,y+6);g.stroke();
g.drawImage(images[i],n*192,0,192,256,x-96*k,y-d.ground*k,192*k,256*k);
g.fillStyle='white';g.fillText('Image '+n+' / 7',12,22);
});requestAnimationFrame(draw)}requestAnimationFrame(draw);
</script></html>""".replace('PREVIEWS', json.dumps(previews))


def run(folder, manifest, out, review_path=None):
    review = read_json(review_path) if review_path else {}
    if not isinstance(review, dict):
        raise ValueError("La revue doit être un objet JSON.")
    template_path = out / "review-template.json"
    outputs = [out / name for name in ("review-template.json", "report.json", "index.html", "reprises.md")]
    if review_path and review_path.resolve() in [p.resolve() for p in outputs]:
        raise ValueError("La revue doit avoir un nom distinct des fichiers produits, par exemple review.json.")
    results = [inspect_strip(folder, manifest, facing, review) for facing in FACINGS]
    out.mkdir(parents=True, exist_ok=True)
    (out / "index.html").write_text(gallery(results), encoding="utf-8")
    template = {r["strip"]: {"fingerprint": r.get("fingerprint"), "reviewer": "", "evidence": "",
                            "contacts": {"left": [None]*8, "right": [None]*8},
                            "support": {"left": [None]*8, "right": [None]*8}, "notes": "",
                            "visual": dict.fromkeys(VISUAL_CHECKS)} for r in results}
    # Le gabarit ne remplace jamais une revue renseignée.
    template_path.write_text(json.dumps(template, indent=2), encoding="utf-8")
    status = "rejected" if any(r["errors"] for r in results) else "pending" if any(r["pending"] for r in results) else "accepted"
    for r in results:
        r.pop("preview", None)
    report = {"status": status, "policy": POLICY, "folder": str(folder), "strips": results,
              "scope": "Marche uniquement ; seuils initiaux à calibrer, revue visuelle et moteur requise."}
    (out / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    lines = ["# Reprises de marche", "", "Ne régénérer que les bandes refusées après diagnostic.",
             "Les preuves manquantes demandent une revue, pas une régénération aveugle.", ""]
    for r in results:
        lines += [f'## {r["strip"]} — {r["status"]}', *[f'- {s}' for s in r["errors"] + r["pending"]], ""]
        if r.get("review_notes"):
            lines += [r["review_notes"], ""]
        if r["errors"]:
            lines += ["Prompt addition (attach the current strip and approved identity reference):",
                      "Preserve the approved character identity, equipment, painted style and facing.",
                      "Change only this walk strip. Produce eight ordered poses of one complete gait cycle:",
                      "contact, down, passing, up, opposite contact, down, passing, up.",
                      "Do not duplicate the first pose as the final frame. Keep the pelvis registration stable.",
                      "During stance, the same sole contact point moves opposite to world translation.",
                      "At the final art scale, each frame advances the root by 1/8 tile (16 px horizontally",
                      "and 9.9375 px vertically along the facing). Match that displacement during stance.",
                      "Maintain foot lift during swing and continuous weight transfer through frame 7 to 0.", ""]
    (out / "reprises.md").write_text('\n'.join(lines), encoding="utf-8")
    return {"accepted": 0, "rejected": 1, "pending": 2}[status]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("folder", type=Path)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--review", type=Path)
    args = parser.parse_args(argv)
    try:
        code = run(args.folder.resolve(), args.manifest.resolve(), args.out.resolve(), args.review)
    except (OSError, ValueError) as exc:
        print(f"check_figure_walk : {exc}")
        return 1
    print(f"Contrôle marche : {['accepté', 'refusé', 'à examiner'][code]} — {args.out / 'report.json'}")
    return code


if __name__ == "__main__":
    raise SystemExit(main())
