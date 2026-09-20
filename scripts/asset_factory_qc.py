# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Mesures et preuves du pilote PNJ. Un verdict visuel ne se déduit pas des mesures."""
from pathlib import Path
import hashlib

from PIL import Image, ImageDraw
import numpy as np


def measure(path, count, width):
    image = Image.open(path)
    errors, metrics = [], {"mode": image.mode, "size": list(image.size)}
    if image.format != "PNG" or image.mode != "RGBA" or image.size != (count * width, 64):
        return metrics, ["format: PNG RGBA et dimensions exactes requis"]
    pixels = np.array(image)
    alpha = pixels[:, :, 3]
    metrics["semi_transparent_pixels"] = int(((alpha > 0) & (alpha < 255)).sum())
    if metrics["semi_transparent_pixels"]:
        errors.append("alpha non binaire")
    metrics["colors"] = len(np.unique(pixels[:, :, :3][alpha > 0], axis=0))
    frames = [pixels[:, i * width:(i + 1) * width] for i in range(count)]
    masks = [frame[:, :, 3] > 0 for frame in frames]
    boxes = [Image.fromarray(frame).getbbox() for frame in frames]
    metrics["boxes"] = boxes
    if any(box is None for box in boxes):
        return metrics, errors + ["frame vide"]
    metrics["heights"] = [box[3] - box[1] for box in boxes]
    metrics["ground"] = [box[3] - 1 for box in boxes]
    metrics["distinct_frames"] = len({hashlib.sha256(frame.tobytes()).hexdigest() for frame in frames})
    metrics["distinct_silhouettes"] = len({mask.tobytes() for mask in masks})
    distances = [float(np.logical_xor(a, b).sum() / max(1, np.logical_or(a, b).sum()))
                 for a, b in zip(masks, masks[1:] + masks[:1])]
    metrics["mask_distances"] = distances
    metrics["loop_distance"] = distances[-1]
    if any(box[0] == 0 or box[2] == width or box[1] == 0 for box in boxes):
        errors.append("dessin au bord utile : rognage possible")
    return metrics, errors


def check(directory, animations, profile):
    results = {}
    for name, (count, width, _, loop) in animations.items():
        path = Path(directory) / f"{name}.png"
        if not path.exists():
            results[name] = {"errors": ["animation absente"], "metrics": {}}
            continue
        metrics, errors = measure(path, count, width)
        if "heights" in metrics:
            if name == "idle" and metrics["heights"][0] != profile["height"]:
                errors.append("pose de garde différente de 45 pixels")
            if name in ("idle", "walk") and max(metrics["heights"]) - min(metrics["heights"]) > profile["max_height_drift"]:
                errors.append("variation de hauteur excessive")
            if any(abs(y - profile["ground"]) > profile["max_ground_drift"] for y in metrics["ground"]):
                errors.append("ligne de sol incorrecte")
            if loop and metrics["loop_distance"] > profile["max_loop_mask_distance"]:
                errors.append("raccord de boucle discontinu")
            if name == "walk" and metrics["distinct_silhouettes"] < profile["min_walk_distinct_frames"]:
                errors.append("marche : silhouettes répétées")
            if metrics["colors"] > profile["colors"]:
                errors.append("palette trop étendue")
        results[name] = {"metrics": metrics, "errors": errors}
    portrait = Path(directory) / "portrait.png"
    portrait_errors = []
    if not portrait.exists():
        portrait_errors.append("portrait absent")
    else:
        image = Image.open(portrait)
        if image.mode != "RGBA" or image.size != (1024, 1024):
            portrait_errors.append("portrait : format incorrect")
        pixels = np.array(image.convert("RGBA"))
        if not (pixels[:, :, 3] == 0).any() or not (pixels[:, :, 3] == 255).any():
            portrait_errors.append("portrait vide ou fond opaque")
        if ((pixels[:, :, 3] > 0) & (pixels[:, :, 3] < 255)).any():
            portrait_errors.append("portrait : alpha non binaire")
        if len(np.unique(pixels[:, :, :3][pixels[:, :, 3] > 0], axis=0)) > profile["colors"]:
            portrait_errors.append("portrait : palette trop étendue")
    results["portrait"] = {"errors": portrait_errors}
    return results


def previews(directory, destination, animations, baseline):
    """Planche ancien/nouveau et GIF : preuves, pas assets destinés au moteur."""
    destination.mkdir(parents=True, exist_ok=True)
    contact = Image.new("RGB", (800, len(animations) * 160), "#d5d5d5")
    draw = ImageDraw.Draw(contact)
    for row, (name, (count, width, duration, loop)) in enumerate(animations.items()):
        (destination / f"{name}.gif").unlink(missing_ok=True)
        draw.text((4, row * 160), f"{name}: ancien puis candidat", fill="black")
        for offset, path in [(20, baseline / f"{name}.png"), (90, directory / f"{name}.png")]:
            if not path.exists():
                continue
            image = Image.open(path).convert("RGBA")
            contact.paste(image, (4, row * 160 + offset), image)
        path = directory / f"{name}.png"
        if not path.exists():
            continue
        strip = Image.open(path).convert("RGBA")
        frames = []
        for i in range(count):
            frame = strip.crop((i * width, 0, (i + 1) * width, 64))
            preview = Image.new("RGBA", frame.size, "#d5d5d5")
            preview.alpha_composite(frame)
            frames.append(preview.convert("RGB").resize((width * 4, 256), Image.Resampling.NEAREST))
        options = {"loop": 0} if loop else {}
        frames[0].save(destination / f"{name}.gif", save_all=True, append_images=frames[1:],
                       duration=round(duration * 1000), disposal=2, **options)
    contact.save(destination / "comparison.png")
