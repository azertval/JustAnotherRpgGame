"""Clean LOT-105 kit: continuous UVs and union geometry; stage before installation.

No generated artwork is painted here. New material images are mapped onto analytic
surfaces; all junctions share the same world-space UV phase and ground centreline.
"""

import argparse
import hashlib
import json
import shutil
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
import install_hd_asset as hd
import rectify_capital_kit as old

ROOT = Path(__file__).resolve().parent.parent
CAP = ROOT / "Tools/AssetsHD/Regions/central-empire/capital/Common"
EMP = ROOT / "Tools/AssetsHD/Regions/central-empire/Common"
V4 = CAP / "V4"
SRC = V4 / "Sources"
CAL = V4 / "Calibres"
STAGE = V4 / "Scene"
S = 2


def source(name):
    return Image.open(SRC / (name + ".png")).convert("RGBA")


def clean_crop(im):
    a = np.asarray(im).copy()
    a[a[:, :, 3] < 12] = 0
    im = Image.fromarray(a)
    return im.crop(im.getbbox())


def repeat(im, band=6):
    return old.periodic(im, band)


def sample(tex, x, y):
    a = np.asarray(tex)
    h, w = a.shape[:2]
    ix = np.floor(np.mod(x, 1) * w).astype(int) % w
    iy = np.floor(np.mod(y, 1) * h).astype(int) % h
    return a[iy, ix].copy()


def shape(direction, thick, corner=None):
    lo = (1 - thick) / 2
    hi = 1 - lo
    if corner == "inner":
        return [(lo, lo, 1, hi), (lo, hi, hi, 1)]
    if corner == "outer":
        return [(lo, 0, hi, hi), (0, lo, lo, hi)]
    return [(0, lo, 2, hi)] if direction == "u" else [(lo, 0, hi, 2)]


class Surface:
    def __init__(self):
        self.origin = np.array([340.0, 350.0])
        self.size = (1400, 1400)
        self.rgba = np.zeros((1400, 1400, 4), np.uint8)
        self.depth = np.full((1400, 1400), -1e9)

    def quad(self, points, tex, uv, shade=1, triangle=False):
        p = np.asarray(points, float)
        if not triangle and not np.allclose(p[0] + p[2], p[1] + p[3]):
            for indices in ([0, 1, 2, 2], [0, 2, 3, 3]):
                self.quad(p[indices], tex, np.asarray(uv)[indices], shade, True)
            return
        xy = np.stack([(p[:, 0] - p[:, 1]) * 128, (p[:, 0] + p[:, 1]) * 79.5 - p[:, 2]], axis=1)
        xy = (xy + self.origin) * S
        left, top = np.maximum(np.floor(xy.min(axis=0)).astype(int), 0)
        right, bottom = np.minimum(np.ceil(xy.max(axis=0)).astype(int), self.size)
        if right <= left or bottom <= top:
            return
        yy, xx = np.mgrid[top:bottom, left:right]
        inv = np.linalg.inv(np.column_stack([xy[1] - xy[0], xy[3] - xy[0]]))
        ab = inv @ np.stack([xx.ravel() + 0.5 - xy[0, 0], yy.ravel() + 0.5 - xy[0, 1]])
        a, b = ab.reshape(2, bottom - top, right - left)
        mask = (a >= -1e-8) & (a <= 1 + 1e-8) & (b >= -1e-8) & (b <= 1 + 1e-8)
        if triangle:
            mask &= a + b <= 1 + 1e-8
        world = p[0] + a[..., None] * (p[1] - p[0]) + b[..., None] * (p[3] - p[0])
        depth = world[:, :, 0] + world[:, :, 1] + world[:, :, 2] / 159
        coords = np.asarray(uv)
        t = (
            coords[0]
            + a[..., None] * (coords[1] - coords[0])
            + b[..., None] * (coords[3] - coords[0])
        )
        pixels = sample(tex, t[:, :, 0], t[:, :, 1])
        pixels[:, :, :3] = np.uint8(pixels[:, :, :3].astype(float) * shade)
        mask &= (depth >= self.depth[top:bottom, left:right] - 1e-6) & (pixels[:, :, 3] > 0)
        self.rgba[top:bottom, left:right][mask] = pixels[mask]
        self.depth[top:bottom, left:right][mask] = depth[mask]

    def union(self, rects, z0, z1, face, toptex, period=1, face_height=None, shading=True):
        xs = sorted({r[i] for r in rects for i in (0, 2)})
        ys = sorted({r[i] for r in rects for i in (1, 3)})

        def inside(x, y):
            return any(a <= x <= c and b <= y <= d for a, b, c, d in rects)

        occupied = {
            (i, j)
            for i in range(len(xs) - 1)
            for j in range(len(ys) - 1)
            if inside((xs[i] + xs[i + 1]) / 2, (ys[j] + ys[j + 1]) / 2)
        }
        fh = face_height or z1 - z0
        for i, j in occupied:
            x0, x1 = xs[i : i + 2]
            y0, y1 = ys[j : j + 2]
            self.quad(
                [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)],
                toptex,
                [(x0, y0), (x1, y0), (x1, y1), (x0, y1)],
            )
            if (i, j + 1) not in occupied:
                pts = [(x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0)]
                self.quad(
                    pts,
                    face,
                    [
                        (x0 / period, 0),
                        (x1 / period, 0),
                        (x1 / period, (z1 - z0) / fh),
                        (x0 / period, (z1 - z0) / fh),
                    ],
                )
            if (i + 1, j) not in occupied:
                pts = [(x1, y1, z1), (x1, y0, z1), (x1, y0, z0), (x1, y1, z0)]
                self.quad(
                    pts,
                    face,
                    [
                        (y1 / period, 0),
                        (y0 / period, 0),
                        (y0 / period, (z1 - z0) / fh),
                        (y1 / period, (z1 - z0) / fh),
                    ],
                    0.89 if shading else 1,
                )

    def result(self):
        return Image.fromarray(self.rgba), self.origin * S


def balustrade_volume(renderer, direction, corner, stone):
    """Solid rails and turned balusters, including visible sides in every direction."""
    rects = shape(direction, 0.20, corner)
    renderer.union(rects, 0, 12, stone, stone, 1, 12)
    renderer.union(rects, 88, 100, stone, stone, 1, 12)
    if corner == "inner":
        centres = [(0.5, 0.5), (0.875, 0.5), (0.5, 0.875)]
    elif corner == "outer":
        centres = [(0.5, 0.5), (0.125, 0.5), (0.5, 0.125)]
    else:
        centres = [(t, 0.5) if direction == "u" else (0.5, t) for t in np.arange(0.125, 2, 0.25)]
    # Molded square feet/capitals and a continuous lathed shaft between them.
    profile = [
        (19, 0.052),
        (22, 0.056),
        (25, 0.045),
        (29, 0.047),
        (34, 0.064),
        (41, 0.073),
        (47, 0.068),
        (55, 0.049),
        (65, 0.034),
        (74, 0.035),
        (77, 0.048),
        (81, 0.056),
    ]
    light = np.array([-0.45, -0.65, 0.70])
    light /= np.linalg.norm(light)
    for cx, cy in centres:
        renderer.union(
            [(cx - 0.075, cy - 0.075, cx + 0.075, cy + 0.075)], 12, 19, stone, stone, 1, 7
        )
        renderer.union(
            [(cx - 0.075, cy - 0.075, cx + 0.075, cy + 0.075)], 81, 88, stone, stone, 1, 7
        )
        for (z0, r0), (z1, r1) in zip(profile, profile[1:]):
            for k in range(16):
                t0 = k * np.pi / 8
                t1 = (k + 1) * np.pi / 8
                mid = (t0 + t1) / 2
                normal = np.array([np.cos(mid), np.sin(mid), -(r1 - r0) * 159 / (z1 - z0)])
                normal /= np.linalg.norm(normal)
                shade = float(np.clip(0.84 + 0.23 * np.dot(normal, light), 0.63, 1))
                pts = [
                    (cx + r0 * np.cos(t0), cy + r0 * np.sin(t0), z0),
                    (cx + r0 * np.cos(t1), cy + r0 * np.sin(t1), z0),
                    (cx + r1 * np.cos(t1), cy + r1 * np.sin(t1), z1),
                    (cx + r1 * np.cos(t0), cy + r1 * np.sin(t0), z1),
                ]
                renderer.quad(pts, stone, [(0, 0), (0.3, 0), (0.3, 0.4), (0, 0.4)], shade)


def ground(tex):
    yy, xx = np.mgrid[:318, :512]
    u = (yy + 0.5) / 318 + (xx + 0.5 - 256) / 512
    v = (yy + 0.5) / 318 - (xx + 0.5 - 256) / 512
    a = sample(tex, u, v)
    a[(u < 0) | (u > 1) | (v < 0) | (v > 1)] = 0
    return Image.fromarray(a), np.array([256, 0])


def hedge_volume(renderer, direction, corner, hedge, stone):
    renderer.union(shape(direction, 0.42, corner), 0, 14, stone, stone, 1, 14)
    rects = shape(direction, 0.40, corner)
    renderer.union(rects, 14, 68, hedge, hedge, 1, 72)
    if corner == "inner":
        segments = [((0.5, 0.5), (1, 0.5)), ((0.5, 0.5), (0.5, 1))]
    elif corner == "outer":
        segments = [((0, 0.5), (0.5, 0.5)), ((0.5, 0), (0.5, 0.5))]
    else:
        segments = [((0, 0.5), (2, 0.5))] if direction == "u" else [((0.5, 0), (0.5, 2))]

    def height(x, y):
        pt = np.array([x, y])
        dist = 10.0
        for start, end in segments:
            a = np.array(start)
            b = np.array(end)
            t = np.clip(np.dot(pt - a, b - a) / np.dot(b - a, b - a), 0, 1)
            dist = min(dist, float(np.linalg.norm(pt - a - t * (b - a))))
        return 68 + 18 * np.sqrt(max(0, 1 - (dist / 0.20) ** 2))

    # Rounded crown follows the union of the two axes, without a seam at corners.
    step = 0.025
    for x in np.arange(min(r[0] for r in rects), max(r[2] for r in rects) - 0.001, step):
        for y in np.arange(min(r[1] for r in rects), max(r[3] for r in rects) - 0.001, step):
            if not any(a <= x + step / 2 <= c and b <= y + step / 2 <= d for a, b, c, d in rects):
                continue
            uv = [(x, y), (x + step, y), (x + step, y + step), (x, y + step)]
            pts = [(u, v, height(u, v)) for u, v in uv]
            dzx = (pts[1][2] + pts[2][2] - pts[0][2] - pts[3][2]) / (2 * step * 159)
            dzy = (pts[2][2] + pts[3][2] - pts[0][2] - pts[1][2]) / (2 * step * 159)
            normal = np.array([-dzx, -dzy, 1])
            normal /= np.linalg.norm(normal)
            shade = float(np.clip(0.80 + 0.22 * np.dot(normal, [-0.45, -0.65, 0.70]), 0.60, 1))
            renderer.quad(pts, hedge, uv, shade)
            # Close exposed crown ends; shared module ends use the same section.
            for k in range(4):
                j = (k + 1) % 4
                a = np.array(uv[k])
                b = np.array(uv[j])
                mid = (a + b) / 2
                out = mid + (mid - np.array([x + step / 2, y + step / 2])) * 0.05
                if any(lo <= out[0] <= hi and low <= out[1] <= high for lo, low, hi, high in rects):
                    continue
                if max(pts[k][2], pts[j][2]) <= 68.001:
                    continue
                if min(pts[k][2], pts[j][2]) <= 68.001:
                    # A triangle avoids the zero-height end of the rounded section.
                    highidx = k if pts[k][2] > pts[j][2] else j
                    lowidx = j if highidx == k else k
                    p = [pts[highidx], pts[lowidx], (*uv[highidx], 68)]
                    renderer.quad(
                        [p[0], p[1], p[2], p[2]],
                        hedge,
                        [(0, 0), (0.2, 0), (0, 0.2), (0, 0.2)],
                        0.80,
                        True,
                    )
                else:
                    renderer.quad(
                        [pts[k], pts[j], (*uv[j], 68), (*uv[k], 68)],
                        hedge,
                        [(0, 0), (0.2, 0), (0.2, 0.2), (0, 0.2)],
                        0.80,
                    )


def stand(name, height, foot=None):
    im = clean_crop(source(name))
    scale = height / im.height
    assert scale <= 1
    out = (
        im.convert("RGBa")
        .resize(
            (round(im.width * scale * S), round(im.height * scale * S)), Image.Resampling.LANCZOS
        )
        .convert("RGBA")
    )
    # Base centre lies above the south tip by half the projected base depth.
    ground_depth = {
        "prop-crate": 0.38,
        "prop-barrel": 0.26,
        "prop-planter": 0.28,
        "balustrade-limestone-pillar": 0.22,
        "column-lion-emblem": 0.42,
        "prop-banner-lion": 0.23,
        "prop-lamppost": 0.20,
    }.get(name, 0.1)
    alpha = np.asarray(out)[:, :, 3]
    ys, xs = np.nonzero(alpha[int(out.height * 0.98) :] > 128)
    px = float(np.mean(xs)) if len(xs) else out.width / 2
    py = out.height - ground_depth * 79.5 * S
    return out, np.array([px, py - 79.5 * S])


def stage_piece(name, im, anchor, entry, metadata):
    CAL.mkdir(parents=True, exist_ok=True)
    STAGE.mkdir(parents=True, exist_ok=True)
    path = CAL / (name + ".png")
    im.save(path)
    # Same installer as production; preserve the explicit geometric anchor after its crop.
    spec = hd.PieceSpec(source=path, names=[name], family=entry["family"])
    # Construct from its parser to retain all standard defaults.
    specdata = dict(
        source=str(path),
        name=name,
        family=entry["family"],
        footprint=entry["footprint"],
        tactical=entry.get("tactical", "solid"),
        scale=0.5,
    )
    if entry["family"] == "01":
        frag = hd.fragments(np.asarray(im).copy())[0]
        built = hd.install_floor(spec, name, frag, (256, 159))
    else:
        from dataclasses import replace

        spec = replace(spec, footprint=tuple(entry["footprint"]), scale=0.5)
        frag = hd.fragments(np.asarray(im).copy())[0]
        built = hd.install_standing(spec, name, frag, (256, 159))
        wanted = (anchor - np.array(frag.box[:2])) / S + hd.MARGE
        pad = built.image.shape[0] - round(frag.image.shape[0] / S) - 2 * hd.MARGE
        wanted[1] += pad
        specdata["anchorOffset"] = [
            int(round(a - b)) for a, b in zip(wanted, built.entry["anchor"])
        ]
        spec = replace(spec, anchor_offset=tuple(specdata["anchorOffset"]))
        built = hd.install_standing(spec, name, frag, (256, 159))
    Image.fromarray(built.image).save(STAGE / (name + ".png"))
    result = dict(entry, **built.entry)
    result["source"] = dict(
        file=path.relative_to(hd.SOURCES).as_posix(),
        sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
    )
    metadata[name] = dict(entry=result, spec=specdata)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--install", action="store_true")
    args = parser.parse_args()
    old_entries = {}
    for base in ["capital/Common", "Common"]:
        d = ROOT / "Source/Elements/Assets/Regions/central-empire" / base / "Scene"
        for e in json.loads((d / "manifest.json").read_text())["textures"].values():
            old_entries[Path(e["file"]).stem] = e
    wall = repeat(source("wall-surface").resize((512, 512), Image.Resampling.LANCZOS), 4)
    stone = (
        source("wall-surface").crop((40, 25, 270, 125)).resize((256, 128), Image.Resampling.LANCZOS)
    )
    stone = repeat(stone, 8)
    rail = clean_crop(source("rail-face"))
    rail = rail.crop((0, 0, rail.width // 2, rail.height)).resize(
        (512, 256), Image.Resampling.LANCZOS
    )
    # Only horizontal edge matching; the openings keep their generated transparency.
    a = np.asarray(rail).copy()
    a[:, :2] = a[:, -2:]
    rail = Image.fromarray(a)
    hedge = repeat(
        source("hedge-surface")
        .crop((0, 0, 1000, 640))
        .resize((512, 256), Image.Resampling.LANCZOS),
        12,
    )
    window = clean_crop(source("window-face"))
    metadata = {}
    for name, e in old_entries.items():
        if name.startswith("floor-"):
            stem = "paving" if "paving" in name else "flagstone"
            tex = repeat(source(stem + "-surface").resize((256, 256), Image.Resampling.LANCZOS), 3)
            if "-edge-" in name:
                # Decorative border uses the same smooth stone palette and continuous joints.
                strip = stone.resize((20, 256), Image.Resampling.LANCZOS)
                suffix = name[-2:]
                if suffix == "nw":
                    tex.paste(strip, (0, 0))
                elif suffix == "se":
                    tex.paste(strip, (236, 0))
                elif suffix == "ne":
                    tex.paste(strip.transpose(Image.Transpose.ROTATE_90), (0, 0))
                else:
                    tex.paste(strip.transpose(Image.Transpose.ROTATE_90), (0, 236))
            else:
                idx = int(name[-1]) - 1
                a = np.array(tex).astype(float)
                yy, xx = np.mgrid[:256, :256]
                cells = 4 if stem == "paving" else 2
                t = 256 / cells
                envelope = np.sin(np.pi * (xx % t) / t) ** 2 * np.sin(np.pi * (yy % t) / t) ** 2
                a[:, :, :3] += (
                    envelope[..., None]
                    * np.sin((xx // t) * 2.17 + (yy // t) * 3.71 + idx * 2.09)[..., None]
                    * 1.8
                )
                tex = Image.fromarray(np.uint8(np.clip(a, 0, 255)))
            im, anchor = ground(tex)
        elif name.startswith(
            ("wall-", "plant-hedge", "balustrade-limestone-")
        ) and not name.endswith("pillar"):
            direction = "v" if name.endswith("-v") else "u"
            corner = ("outer" if name.endswith("outer") else "inner") if "corner" in name else None
            renderer = Surface()
            if name.startswith("wall-"):
                renderer.union(shape(direction, 0.18, corner), 16, 212, wall, stone, 1, 196)
                renderer.union(shape(direction, 0.24, corner), 0, 16, stone, stone, 1, 16)
                renderer.union(shape(direction, 0.24, corner), 212, 224, stone, stone, 1, 12)
                if "window" in name:
                    # A single alpha overlay projected on the existing continuous wall plane.
                    if direction == "u":
                        pts = [
                            (0.65, 0.592, 188),
                            (1.35, 0.592, 188),
                            (1.35, 0.592, 48),
                            (0.65, 0.592, 48),
                        ]
                    else:
                        pts = [
                            (0.592, 1.35, 188),
                            (0.592, 0.65, 188),
                            (0.592, 0.65, 48),
                            (0.592, 1.35, 48),
                        ]
                    renderer.quad(pts, window, [(0, 0), (0.999, 0), (0.999, 0.999), (0, 0.999)])
                if "banner" in name:
                    cloth = source("banner-face")
                    mask = Image.new("L", cloth.size)
                    ImageDraw.Draw(mask).polygon(
                        [(269, 55), (755, 55), (755, 1120), (512, 1405), (269, 1120)], fill=255
                    )
                    cloth.putalpha(mask)
                    cloth = clean_crop(cloth)
                    if direction == "u":
                        pts = [
                            (0.7, 0.595, 204),
                            (1.3, 0.595, 204),
                            (1.3, 0.595, 35),
                            (0.7, 0.595, 35),
                        ]
                    else:
                        pts = [
                            (0.595, 1.3, 204),
                            (0.595, 0.7, 204),
                            (0.595, 0.7, 35),
                            (0.595, 1.3, 35),
                        ]
                    renderer.quad(pts, cloth, [(0, 0), (0.999, 0), (0.999, 0.999), (0, 0.999)])
            elif name.startswith("plant-hedge"):
                hedge_volume(renderer, direction, corner, hedge, stone)
            else:
                balustrade_volume(renderer, direction, corner, stone)
            im, anchor = renderer.result()
        else:
            heights = {
                "plant-cypress": 400,
                "prop-lamppost": 350,
                "prop-planter": 145,
                "prop-barrel": 90,
                "prop-crate": 85,
                "balustrade-limestone-pillar": 120,
                "prop-banner-lion": 390,
                "column-lion-emblem": 340,
                "prop-bench-u": 145,
                "prop-bench-v": 145,
                "prop-stall-empty": 265,
                "plant-flowerbed": 105,
            }
            bases = {
                "prop-bench-u": (
                    [(104, 718), (884, 1218), (1150, 1080)],
                    (0.05, 0.325, 0.95, 0.675),
                ),
                "prop-bench-v": (
                    [(143, 1050), (401, 1178), (1188, 830)],
                    (0.325, 0.05, 0.675, 0.95),
                ),
                "prop-stall-empty": (
                    [(190, 1034), (501, 1210), (1085, 997)],
                    (0.08, 0.08, 0.92, 0.92),
                ),
                "plant-flowerbed": (
                    [(134, 808), (628, 1089), (1116, 811)],
                    (0.04, 0.04, 0.96, 0.96),
                ),
            }
            if name in bases:
                points, footprint = bases[name]
                im, anchor = old.ground_fit(SRC / (name + ".png"), points, footprint)
            else:
                im, anchor = stand(name, heights[name])
        stage_piece(name, im, anchor, e, metadata)
    (V4 / "build.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    manifest = dict(
        version=1,
        disposition="lot105-v4",
        tile=[256, 159],
        textures={"scene/lot105-v4/" + n: m["entry"] for n, m in metadata.items()},
    )
    (STAGE / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    if args.install:
        for base in [CAP, EMP]:
            desc = base / "install.json"
            raw = json.loads(desc.read_text())
            target = hd.ASSETS / raw["target"]
            archive = base / "Versions/before-v4"
            archive.mkdir(parents=True, exist_ok=True)
            if not (archive / "install.json").exists():
                shutil.copy2(desc, archive / "install.json")
                shutil.copytree(target, archive / "Scene", dirs_exist_ok=True)
            for spec in raw["pieces"]:
                data = metadata[spec["name"]]["spec"]
                data = (
                    dict(data, source=Path(data["source"]).relative_to(base).as_posix())
                    if Path(data["source"]).is_relative_to(base)
                    else dict(
                        data,
                        source=__import__("os")
                        .path.relpath(data["source"], base)
                        .replace("\\", "/"),
                    )
                )
                spec.clear()
                spec.update(data)
            desc.write_text(json.dumps(raw, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            d = hd.read_descriptor(desc)
            hd.write(d, hd.build(d))
    print("V4 staged:", len(metadata), "pieces", STAGE)


if __name__ == "__main__":
    main()
