# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Fabrique locale : mémoire, commandes de tours et réception contrôlée, sans API payante.

La génération et la revue visuelle appartiennent au chat ; aucune commande ne simule
leur succès. Les images du corpus ne sont jamais copiées dans la mémoire versionnée.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MEMORY = ROOT / "Tools/AssetFactory"
OLD = ROOT / "Documentation/Lot/LOT-91-atelier-pnj/atelier"
NPC = ROOT / "Source/Elements/Assets/Npc"
PAGES = {"anariel": 10, "jade": 68, "lizz": 83, "nakral": 92, "xorius": 148}
ANIMATIONS = {
    "idle": (6, 48, .15, True), "walk": (8, 48, .10, True),
    "hit": (4, 48, .08, False), "death": (6, 96, .12, False),
    "attack": (8, 96, .08, False), "cast": (8, 96, .10, False),
}
ROWS = [("idle", 6), ("walk", 8), ("hit", 4), ("death", 6),
        ("attack", 4), ("attack", 4), ("cast", 4), ("cast", 4)]


def digest(path):
    path = Path(path)
    data = path.read_bytes()
    # Git normalise les textes en LF : une reprise doit survivre à un checkout.
    if path.suffix in (".json", ".txt", ".py"):
        data = data.replace(b"\r\n", b"\n")
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))


def write(path, value):
    """Remplacement atomique d'un journal ; les fichiers temporaires restent voisins."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(path)


def asset(slug):
    if slug not in PAGES:
        raise ValueError("Ce pilote accepte seulement les cinq PNJ déclarés")
    return MEMORY / "assets/Npc" / slug


def fingerprint(slug):
    paths = [asset(slug) / name for name in ("fiche.json", "prompt_b.txt", "palette.txt")]
    paths += [MEMORY / "profile.json"]
    return {str(p.relative_to(ROOT)): digest(p) for p in paths}


def init():
    """Importe les décisions artistiques du PoC, pas les illustrations du corpus."""
    corpus = ROOT / "Documentation/SourceBook/VTT/Character Compendium - High.pdf"
    source_hash = digest(corpus)
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    regions = {"anariel": "Aurindunnum", "jade": "Itinérante, Tanares/Pénombre",
               "lizz": "Près de Heroes Rise", "nakral": "Itinérant, chasse Shadow Wing",
               "xorius": "Commandement kemet, localisation actuelle non précisée"}
    profile = MEMORY / "profile.json"
    if not profile.exists():
        write(profile, {"version": 1, "height": 45, "ground": 63, "colors": 32,
                        "alpha_threshold": 127, "max_height_drift": 4,
                        "max_ground_drift": 2, "max_loop_mask_distance": .35,
                        "min_walk_distinct_frames": 8,
                        "calibration": "provisional: blocks promotion until calibrated"})
    for slug, page in PAGES.items():
        directory = asset(slug)
        directory.mkdir(parents=True, exist_ok=True)
        if not (directory / "fiche.json").exists():
            block = (OLD / "pnj" / slug / "prompt_b.txt").read_text(encoding="utf-8")
            block = re.sub(r" \(match reference images 3, 4 and 5\)", "", block)
            block = block.replace("reference image 5", "the retained project portrait")
            block = block.replace("reference image 1", "the project style sheet")
            (directory / "prompt_b.txt").write_text(block, encoding="utf-8")
            shutil.copyfile(OLD / "pnj" / slug / "palette.txt", directory / "palette.txt")
            write(directory / "fiche.json", {
                "slug": slug, "family": "Npc", "size": "Medium", "region": regions[slug],
                "source": {"path": str(corpus.relative_to(ROOT)), "pdf_page": page,
                           "sha256": source_hash},
                "appearance": "Décisions visuelles héritées de LOT-91, détaillées dans prompt_b.txt",
                "equipment": block.split("WEAPON:")[1].split("PALETTE")[0].strip(),
                "distinctions": "Couleurs/poses/effets hérités du projet, pas des faits textuels garantis. Lizz sans arme est une adaptation : la fiche décrit épée courte et arc.",
                "anchors": [str(p.relative_to(ROOT)) for p in (OLD / "ancres").glob("*.png")],
            })
            write(directory / "baseline.json", {"commit": commit, "files": {
                p.name: digest(p) for p in (NPC / slug).glob("*") if p.is_file()}})
        baseline = ROOT / ".cache/asset-factory/baseline" / slug
        if not baseline.exists():
            shutil.copytree(NPC / slug, baseline)
    status()


def request(slug, stage, correction=""):
    directory = asset(slug)
    number = len(list((directory / "tours").glob(stage + "-*"))) + 1
    tour = directory / "tours" / f"{stage}-{number:03}"
    tour.mkdir(parents=True)
    block = (directory / "prompt_b.txt").read_text(encoding="utf-8")
    if stage == "portrait":
        block = block.split("ANIMATION SET:")[0]
    references = [OLD / "ancres" / ("portrait_anariel.png" if stage == "portrait" else "planche_anariel.png")]
    if stage == "portrait":
        references.append(OLD / "pnj" / slug / "portrait.png")
        style = "16-bit pixel art, coarse 128x128 art-pixel grid enlarged to 1024x1024. Crisp square pixels, dark navy outline, flat three-tone shading, top-left light. Reference 1: project style only. Reference 2: this character's identity."
        layout = "One NEW head-and-shoulders portrait 1024x1024 RGBA, three-quarter view facing viewer right. True transparent background outside silhouette, no solid fill, no checkerboard. Maximum32 opaque colours. No text, border, swatches or watermark."
    else:
        portrait = directory / "candidate/portrait.png"
        if not portrait.exists():
            raise ValueError("Recevoir et contrôler le portrait avant la planche")
        references += [OLD / "ancres/echelle.png", portrait]
        style = "16-bit pixel art, crisp square pixels, navy outlines, flat three-tone shading, top-left light. Image1: style/layout reference, image2: size reference, image3: this character identity. Match the retained portrait. Transparent background, no shadows, no text, no labels, no grid, no blur. Single weapon, same hand and outfit throughout. Standing body 90px high on a 1536px-wide sheet, feet grounded."
        walk = "WALK exactly8 distinct poses, alternating contact/down/passing/up and opposite leg, fixed body scale, locomotion in place, last pose connects to first."
        if slug == "lizz":
            walk = "WALK exactly8 distinct slithering poses: snake tail undulates, NO legs or feet, grounded tail coil."
        if stage == "sheet":
            layout = (OLD / "prompts/disposition_planche.txt").read_text(encoding="utf-8").replace("{MARCHE}", walk)
        else:
            n = ANIMATIONS[stage][0]
            from PIL import Image
            baseline = ROOT / ".cache/asset-factory/baseline" / slug / f"{stage}.png"
            strip = Image.open(baseline).convert("RGBA")
            width = ANIMATIONS[stage][1]
            columns = 2 if stage in ("attack", "cast") else n // 2
            template = Image.new("RGBA", (columns * 384, (n // columns) * 256))
            for index in range(n):
                frame = strip.crop((index * width, 0, (index + 1) * width, 64))
                frame = frame.resize((width * 2, 128), Image.Resampling.NEAREST)
                template.paste(frame, ((index % columns) * 384 + 96, (index // columns) * 256 + 64))
            template.save(tour / "pose-reference.png")
            references[0] = tour / "pose-reference.png"
            block = block.replace("ANIMATION SET: all eight rows.", "").replace("ANIMATION SET: all eight rows", "Character-specific magic")
            layout = f"ONLY {stage.upper()}, exactly {n} complete figures, TWO rows of {n // 2} frames. Square image1024x1024 with completely transparent alpha-zero background. Frames separated by ample empty transparent space, no diffuse glow or background shading. Each standing body is180px high, same scale in every frame; ground at80 percent of each half-image. "
            if stage in ("attack", "cast"):
                layout = layout.replace("TWO rows of 4 frames", "FOUR rows of 2 frames").replace("Each standing body is180px", "Each standing body is120px").replace("each half-image", "each row")
            layout += {"idle": "Breathing loop, almost same first/last pose.", "walk": walk,
                       "hit": "1 flinch, 2 recoil, 3 recover, 4 guard. Four newly drawn poses.",
                       "death": "1-3 stagger,4 fall,5-6 fully lying flat on the baseline.",
                       "attack": "1-3 wind-up,4-5 strike,6-8 recover. One weapon only, correct attack per identity.",
                       "cast": "1-4 gather,5-6 release,7-8 recover. Effect per identity, inside cell."}[stage]
            layout += " Image1 is the SAME character's SAME animation from the manual baseline, arranged in the requested grid. Match its small figure size, spacing, pixel density and body movement; draw NEW frames matching the retained portrait. Do not change weapon type, duplicate it or import any poses from other animations."
    prompt = f"A — STYLE\n{style}\n\nB — IDENTITY\n{block}\n\nC — FORMAT\n{layout}\n{correction}\n"
    (tour / "prompt.txt").write_text(prompt, encoding="utf-8")
    reference_records = []
    for index, path in enumerate(references):
        frozen = tour / f"reference-{index + 1}.png"
        shutil.copyfile(path, frozen)
        reference_records.append({"path": str(path.relative_to(ROOT)), "sha256": digest(path),
                                  "snapshot": str(frozen.relative_to(ROOT))})
    write(tour / "request.json", {"stage": stage, "state": "requested", "at": datetime.now(timezone.utc).isoformat(),
                                  "inputs": fingerprint(slug), "prompt_sha256": digest(tour / "prompt.txt"),
                                  "references": reference_records,
                                  "provider": "local-chat-imagegen", "usage": None, "cost": None})
    print(tour.relative_to(ROOT))
    status()
    return tour


def runs(values, minimum_gap=1):
    """Plages non vides ; aucune fusion pour obtenir artificiellement un compte."""
    result, start, last = [], None, 0
    for i, occupied in enumerate(values):
        if occupied:
            if start is None:
                start = i
            last = i
        elif start is not None and i - last >= minimum_gap:
            result.append((start, last + 1))
            start = None
    if start is not None:
        result.append((start, last + 1))
    return result


def normalize_portrait(image):
    from PIL import Image
    import numpy as np
    raw = np.array(image.convert("RGBA"))
    if not (raw[:, :, 3] == 0).any():
        raise ValueError("Portrait sans transparence réelle : régénérer")
    small = image.convert("RGBA").resize((128, 128), Image.Resampling.NEAREST)
    pixels = np.array(small)
    mask = pixels[:, :, 3] > 127
    if not mask.any():
        raise ValueError("Portrait vide")
    # La palette est calculée sur les pixels visibles uniquement.
    visible = Image.fromarray(pixels[:, :, :3][mask].reshape(1, -1, 3)).quantize(colors=32)
    palette = visible.getpalette()[:96]
    palette_image = Image.new("P", (1, 1))
    palette_image.putpalette(palette + palette[:3] * ((768 - len(palette)) // 3))
    quantized = np.array(small.convert("RGB").quantize(palette=palette_image, dither=Image.Dither.NONE).convert("RGB"))
    pixels[:, :, :3] = quantized
    pixels[:, :, 3] = mask * 255
    pixels[~mask] = 0
    return Image.fromarray(pixels).resize((1024, 1024), Image.Resampling.NEAREST)


def normalize_sheet(image, stage):
    """Découpe par espaces transparents. Échec sur compte ambigu ou rognage."""
    import numpy as np
    from PIL import Image
    pixels = np.array(image.convert("RGBA"))
    mask = pixels[:, :, 3] > 127
    if mask.mean() > .65 or not mask.any():
        raise ValueError("Fond opaque ou image vide")
    pixels[:, :, 3] = mask * 255
    pixels[~mask] = 0
    horizontal = runs(mask.any(axis=1), max(2, image.height // 200))
    if stage == "sheet":
        rows = ROWS
    else:
        count = ANIMATIONS[stage][0]
        if not horizontal or count % len(horizontal):
            raise ValueError("Disposition indivisible : rangées ambiguës")
        rows = [(stage, count // len(horizontal))] * len(horizontal)
    if len(horizontal) != len(rows):
        raise ValueError(f"{len(horizontal)} rangées détectées, {len(rows)} attendues")
    crops, boxes, errors, source_rows = {}, {}, {}, {}
    for row_index, ((name, count), (top, bottom)) in enumerate(zip(rows, horizontal)):
        # Les particules proches appartiennent à la même figure ; le seuil est une
        # fraction fixe de cellule, indépendante du nombre finalement détecté.
        columns = runs(mask[top:bottom].any(axis=0), max(3, image.width // 160))
        if name in ("attack", "cast") and len(columns) > count:
            grouped = runs(mask[top:bottom].any(axis=0), max(3, round(image.width / count * .12)))
            if len(grouped) == count:
                columns = grouped
        if len(columns) != count:
            errors[name] = f"{len(columns)} figures détectées, {count} attendues ; aucune substitution"
            continue
        for left, right in columns:
            crop = Image.fromarray(pixels[top:bottom, left:right])
            bbox = crop.getbbox()
            crops.setdefault(name, []).append(crop.crop(bbox))
            boxes.setdefault(name, []).append([left + bbox[0], top + bbox[1], left + bbox[2], top + bbox[3]])
            source_rows.setdefault(name, []).append(row_index)
    # Une seule échelle pour le tour, jamais une remise à taille frame par frame.
    if not crops:
        raise ValueError(str(errors))
    reference = crops.get("idle", next(iter(crops.values())))[0]
    factor = 45 / reference.height
    bands = {}
    for name, frames in crops.items():
        if name in errors:
            continue
        count, width, _, _ = ANIMATIONS[name]
        band = Image.new("RGBA", (count * width, 64))
        for index, frame in enumerate(frames):
            size = (max(1, round(frame.width * factor)), max(1, round(frame.height * factor)))
            if size[0] >= width or size[1] > 64:
                errors[name] = f"frame {index}: rognage interdit {size} dans {width}x64"
                break
            frame = frame.resize(size, Image.Resampling.NEAREST)
            # L'origine suit les points de contact au sol, pas le centre des effets.
            anchor = 32 if name in ("attack", "cast") else width // 2
            foot_mask = np.array(frame)[:, :, 3] > 0
            contact = np.where(foot_mask[-max(1, round(size[1] * .08)):].any(axis=0))[0]
            foot_x = round(float(np.median(contact))) if len(contact) else size[0] // 2
            x = anchor - foot_x
            if x < 0 or x + size[0] > width:
                errors[name] = f"frame {index}: dépassement ancre"
                break
            band.paste(frame, (index * width + x, 64 - size[1]))
        if name not in errors:
            bands[name] = band
    return bands, {"source_boxes": boxes, "source_rows": source_rows, "scale": factor, "errors": errors,
                   "note": "Ancrage par boîte et sol normalisés : revue visuelle obligatoire, dérives brutes conservées."}


def receive(slug, tour_name, source):
    from PIL import Image
    if not re.fullmatch(r"(?:portrait|sheet|idle|walk|hit|death|attack|cast)-[0-9]{3,}", tour_name):
        raise ValueError("Identifiant de tour invalide")
    tour = asset(slug) / "tours" / tour_name
    request_data = read(tour / "request.json")
    if (tour / "response.json").exists():
        raise ValueError("Tour déjà reçu : créer un nouveau tour")
    if request_data["inputs"] != fingerprint(slug):
        raise ValueError("Entrées modifiées depuis la demande")
    if digest(tour / "prompt.txt") != request_data["prompt_sha256"]:
        raise ValueError("Prompt modifié depuis la demande")
    for reference in request_data["references"]:
        if digest(ROOT / reference.get("snapshot", reference["path"])) != reference["sha256"]:
            raise ValueError("Référence modifiée depuis la demande")
    if Path(source).resolve() != (tour / "response.png").resolve():
        shutil.copyfile(source, tour / "response.png")
    response = {"source_sha256": digest(tour / "response.png"), "size": None, "mode": None,
                "state": "generated", "usage": None, "cost": None, "files": {},
                "processor_sha256": digest(__file__)}
    stage = request_data["stage"]
    candidate = asset(slug) / "candidate"
    candidate.mkdir(exist_ok=True)
    provenance_file = asset(slug) / "provenance.json"
    provenance = read(provenance_file) if provenance_file.exists() else {}
    affected = ["portrait", *ANIMATIONS] if stage == "portrait" else list(ANIMATIONS) if stage == "sheet" else [stage]
    # Un échec récent ne doit jamais laisser une ancienne bande candidate passer pour
    # la nouvelle. Les versions historiques demeurent dans leurs tours immuables.
    for name in affected:
        provenance.pop(name, None)
        (candidate / f"{name}.png").unlink(missing_ok=True)
    (asset(slug) / "review.json").unlink(missing_ok=True)
    try:
        image = Image.open(tour / "response.png")
        image.load()
        response.update(size=list(image.size), mode=image.mode)
        if image.format != "PNG":
            raise ValueError("La réponse doit être un PNG")
        if stage == "portrait":
            outputs = {"portrait": normalize_portrait(image)}
        else:
            outputs, metrics = normalize_sheet(image, stage)
            response["normalization"] = metrics
            if metrics["errors"]:
                response.update(state="rejected", reason=str(metrics["errors"]))
            # Toutes les bandes utilisent les mêmes couleurs opaques que le portrait retenu.
            import numpy as np
            portrait = np.array(Image.open(candidate / "portrait.png").convert("RGBA"))
            colors = np.unique(portrait[:, :, :3][portrait[:, :, 3] > 0], axis=0)
            palette_image = Image.new("P", (1, 1))
            palette = colors.flatten().tolist()
            palette_image.putpalette(palette + palette[:3] * ((768 - len(palette)) // 3))
            for name, output in outputs.items():
                alpha = output.getchannel("A")
                output = output.convert("RGB").quantize(palette=palette_image, dither=Image.Dither.NONE).convert("RGBA")
                output.putalpha(alpha)
                outputs[name] = output
        for name, output in outputs.items():
            output.save(tour / f"{name}.png")
            shutil.copyfile(tour / f"{name}.png", candidate / f"{name}.png")
            response["files"][name] = digest(candidate / f"{name}.png")
            provenance[name] = {"tour": tour_name, "sha256": response["files"][name],
                                "inputs": request_data["inputs"], "processor_sha256": digest(__file__)}
        # Toute réception invalide la revue et l'intégration précédentes.
        (asset(slug) / "review.json").unlink(missing_ok=True)
    except (ValueError, OSError) as error:
        response.update(state="rejected", reason=str(error))
    write(tour / "response.json", response)
    write(provenance_file, provenance)
    print(json.dumps(response, ensure_ascii=False))
    status()
    return response


def reprocess(slug, tour_name):
    """Rejoue le traitement, jamais la génération ; garde l'ancien verdict intact."""
    if not re.fullmatch(r"(?:portrait|sheet|idle|walk|hit|death|attack|cast)-[0-9]{3,}", tour_name):
        raise ValueError("Identifiant de tour invalide")
    tour = asset(slug) / "tours" / tour_name
    previous = read(tour / "response.json")
    number = len(list(tour.glob("processing-*.json"))) + 1
    write(tour / f"processing-{number:03}.json", previous)
    (tour / "response.json").unlink()
    try:
        return receive(slug, tour_name, tour / "response.png")
    except Exception:
        write(tour / "response.json", previous)
        raise


def freeze_references():
    """Migration des premiers tours du PoC ; ne copie que les références projet connues."""
    for path in MEMORY.glob("assets/Npc/*/tours/*/request.json"):
        value = read(path)
        for index, reference in enumerate(value["references"]):
            if "snapshot" in reference:
                continue
            original = (ROOT / reference["path"]).resolve()
            allowed = [OLD / "ancres", OLD / "pnj", MEMORY / "assets"]
            if not any(original.is_relative_to(parent.resolve()) for parent in allowed):
                raise ValueError("Référence du corpus ou externe : copie versionnée interdite")
            if digest(original) != reference["sha256"]:
                raise ValueError(f"Référence déjà modifiée : {original}")
            target = path.parent / f"reference-{index + 1}.png"
            shutil.copyfile(original, target)
            reference["snapshot"] = str(target.relative_to(ROOT))
        write(path, value)


def status():
    index = {}
    for slug in PAGES:
        directory = asset(slug)
        received = list((directory / "tours").glob("*/response.json"))
        index[slug] = {"state": "generated" if received else "todo", "tours": len(received),
                       "pending": [p.parent.name for p in (directory / "tours").glob("*/request.json") if not (p.parent / "response.json").exists()]}
        if received and all(read(p)["state"] == "rejected" for p in received):
            index[slug]["state"] = "rejected"
        qc_file = directory / "qc.json"
        review_file = directory / "review.json"
        if qc_file.exists():
            qc = read(qc_file)
            if qc.get("snapshot") == snapshot(slug):
                index[slug]["state"] = "rejected" if qc["errors"] else "generated"
                if review_file.exists():
                    review_data = read(review_file)
                    if review_data.get("snapshot") == qc["snapshot"]:
                        index[slug]["state"] = review_data["verdict"]
                        if qc["errors"] and review_data["verdict"] == "validated":
                            index[slug]["state"] = "rejected"
    write(MEMORY / "index.json", index)
    return index


def snapshot(slug):
    return {"inputs": fingerprint(slug), "processor": digest(__file__),
            "calibration": digest(MEMORY / "calibration.json") if (MEMORY / "calibration.json").exists() else None,
            "qc_processor": digest(Path(__file__).with_name("asset_factory_qc.py")), "files": {
        p.name: digest(p) for p in sorted((asset(slug) / "candidate").glob("*.png"))}}


def qc(slug):
    from asset_factory_qc import check, previews
    directory = asset(slug)
    profile = read(MEMORY / "profile.json")
    results = check(directory / "candidate", ANIMATIONS, profile)
    errors = [f"{name}: {error}" for name, result in results.items() for error in result["errors"]]
    provenance_path = directory / "provenance.json"
    provenance = read(provenance_path) if provenance_path.exists() else {}
    for path in (directory / "candidate").glob("*.png"):
        origin = provenance.get(path.stem)
        if not origin or origin["sha256"] != digest(path) or origin["inputs"] != fingerprint(slug):
            errors.append(f"{path.stem}: provenance absente ou périmée")
    for selected_name, origin in provenance.items():
        response = read(directory / "tours" / origin["tour"] / "response.json")
        boxes = response.get("normalization", {}).get("source_boxes", {})
        factor = response.get("normalization", {}).get("scale", 1)
        for name in ("idle", "walk"):
            if name != selected_name or name not in boxes:
                continue
            row_ids = response.get("normalization", {}).get("source_rows", {}).get(name)
            if row_ids is None:
                errors.append(f"{name}: mesure brute par rangée manquante, retraiter le tour")
                continue
            for row in set(row_ids):
                grounds = [box[3] for box, row_id in zip(boxes[name], row_ids) if row_id == row]
                if (max(grounds) - min(grounds)) * factor > profile["max_ground_drift"]:
                    errors.append(f"{name}: dérive du sol brute masquée par normalisation")
    calibration_file = MEMORY / "calibration.json"
    calibration = read(calibration_file) if calibration_file.exists() else {}
    if calibration.get("verdict") != "validated" or calibration.get("profile_sha256") != digest(MEMORY / "profile.json"):
        errors.append("Seuils provisoires : calibration non validée")
    previews(directory / "candidate", directory / "proofs", ANIMATIONS,
             ROOT / ".cache/asset-factory/baseline" / slug)
    value = {"snapshot": snapshot(slug), "results": results, "errors": sorted(set(errors)),
             "visual_review_required": True}
    write(directory / "qc.json", value)
    status()
    return value


def calibrate():
    """Mesure les témoins ; la revue de ce relevé demeure une étape explicite."""
    from asset_factory_qc import check
    profile = read(MEMORY / "profile.json")
    results = {}
    for slug in PAGES:
        baseline = ROOT / ".cache/asset-factory/baseline" / slug
        results[slug] = check(baseline, ANIMATIONS, profile)
    write(MEMORY / "calibration.json", {"verdict": "review", "profile_sha256": digest(MEMORY / "profile.json"),
                                       "baseline": results,
                                       "rule": "Ne pas augmenter les seuils pour faire accepter les anciens défauts."})


def review(slug, verdict, notes):
    directory = asset(slug)
    current = qc(slug)
    if verdict == "validated" and current["errors"]:
        raise ValueError("QC bloquant : impossible de valider")
    if not notes.strip():
        raise ValueError("Revue visuelle détaillée requise")
    proofs = {p.name: digest(p) for p in (directory / "proofs").glob("*")}
    write(directory / "review.json", {"snapshot": current["snapshot"], "verdict": verdict,
                                      "notes": notes, "proofs": proofs,
                                      "at": datetime.now(timezone.utc).isoformat()})
    status()


def report():
    lines = ["# PoC Fabrique d’assets — état de production", "",
             "Génération dans le chat local. Aucun coût ni usage inventé. Les verdicts indécis bloquent.", "",
             "Voir [le bilan du premier essai](bilan-poc.md). Une réception réussie ne vaut pas validation artistique ni intégration.", ""]
    for slug in PAGES:
        current = qc(slug)
        directory = asset(slug)
        lines += [f"## {slug}", "", f"État : {status()[slug]['state']}", "",
                  f"[Mesures détaillées](assets/Npc/{slug}/qc.json) · [Provenance](assets/Npc/{slug}/provenance.json)", "",
                  f"![Ancien et candidat](assets/Npc/{slug}/proofs/comparison.png)", ""]
        lines += [" · ".join(f"[{p.stem}](assets/Npc/{slug}/proofs/{p.name})" for p in sorted((directory / "proofs").glob("*.gif"))), ""]
        for tour in sorted((directory / "tours").glob("*/response.json")):
            response = read(tour)
            lines.append(f"- {tour.parent.name} : {response['state']} — {response.get('reason', 'réception effectuée, revue distincte')}")
        lines += ["", *[f"- Blocage : {error}" for error in current["errors"]], ""]
        if (directory / "review.json").exists():
            lines += [read(directory / "review.json")["notes"], ""]
    (MEMORY / "rapport-poc.md").write_text("\n".join(lines), encoding="utf-8")


def integrate(slug):
    """Promotion après double verdict, avec sauvegarde et rollback sur exception."""
    branch = subprocess.check_output(["git", "branch", "--show-current"], cwd=ROOT, text=True).strip()
    if not branch or branch in ("main", "master"):
        raise ValueError("Une branche dédiée est obligatoire")
    current = qc(slug)
    approval = read(asset(slug) / "review.json")
    if current["errors"] or approval["verdict"] != "validated" or approval["snapshot"] != snapshot(slug):
        raise ValueError("QC ou revue absents, périmés ou rejetés")
    transaction = ROOT / ".cache/asset-factory/integration" / slug
    if transaction.exists():
        raise ValueError("Transaction précédente à examiner avant reprise")
    transaction.mkdir(parents=True)
    target = NPC / slug
    shutil.copytree(target, transaction / "backup")
    manifest = NPC / "manifest.json"
    shutil.copyfile(manifest, transaction / "manifest-before.json")
    staged = transaction / "staged"
    staged.mkdir()
    for name, (count, width, duration, loop) in ANIMATIONS.items():
        shutil.copyfile(asset(slug) / "candidate" / f"{name}.png", staged / f"{name}.png")
        write(staged / f"{name}.anim.json", {"version": 1, "frameWidth": width, "frameHeight": 64,
              "clips": {name: {"frames": list(range(count)), "frameDuration": duration, "loop": loop}}})
    shutil.copyfile(asset(slug) / "candidate/portrait.png", staged / "portrait.png")
    write(transaction / "journal.json", {"state": "prepared", "snapshot": snapshot(slug)})
    try:
        target.rename(transaction / "previous")
        staged.rename(target)
        data = read(manifest)
        if slug not in data["npcs"]:
            data["npcs"].append(slug)
        write(manifest, data)
        write(transaction / "journal.json", {"state": "installed", "snapshot": snapshot(slug)})
    except OSError:
        if (transaction / "previous").exists():
            if target.exists():
                target.rename(transaction / "failed")
            (transaction / "previous").rename(target)
            shutil.copyfile(transaction / "manifest-before.json", manifest)
        raise
    write(asset(slug) / "integration.json", {"snapshot": snapshot(slug), "state": "installed-awaiting-game-tests"})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("init")
    commands.add_parser("status")
    commands.add_parser("report")
    commands.add_parser("freeze-references")
    commands.add_parser("calibrate")
    for command in ("qc", "integrate"):
        operation = commands.add_parser(command)
        operation.add_argument("slug", choices=PAGES)
    inspection = commands.add_parser("review")
    inspection.add_argument("slug", choices=PAGES)
    inspection.add_argument("verdict", choices=["rejected", "review", "validated"])
    inspection.add_argument("--notes", required=True)
    prepare = commands.add_parser("request")
    prepare.add_argument("slug", choices=PAGES)
    prepare.add_argument("stage", choices=["portrait", "sheet", *ANIMATIONS])
    prepare.add_argument("--correction", default="")
    reception = commands.add_parser("receive")
    reception.add_argument("slug", choices=PAGES)
    reception.add_argument("tour")
    reception.add_argument("image", type=Path)
    rerun = commands.add_parser("reprocess")
    rerun.add_argument("slug", choices=PAGES)
    rerun.add_argument("tour")
    args = parser.parse_args()
    try:
        if args.command == "init":
            init()
        elif args.command == "status":
            print(json.dumps(status(), ensure_ascii=False, indent=2))
        elif args.command == "request":
            request(args.slug, args.stage, args.correction)
        elif args.command == "qc":
            result = qc(args.slug)
            print(json.dumps(result, ensure_ascii=False, indent=2))
            return 1 if result["errors"] else 0
        elif args.command == "report":
            report()
        elif args.command == "freeze-references":
            freeze_references()
        elif args.command == "calibrate":
            calibrate()
        elif args.command == "review":
            review(args.slug, args.verdict, args.notes)
        elif args.command == "integrate":
            integrate(args.slug)
        elif args.command == "reprocess":
            result = reprocess(args.slug, args.tour)
            return 1 if result["state"] == "rejected" else 0
        else:
            result = receive(args.slug, args.tour, args.image)
            return 1 if result["state"] == "rejected" else 0
    except (ValueError, OSError) as error:
        parser.exit(1, f"Erreur : {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
