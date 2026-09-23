#!/usr/bin/env python3
"""LOT-105: project painted surfaces onto the exact 256x159 grid.

Explicitly authorized by the author, 2026-09-23. Sources remain unchanged.
No texture is painted here: measured planes are resampled, cropped and joined.
"""

import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
import install_hd_asset as hd

ROOT = Path(__file__).resolve().parent.parent
CAP = ROOT / "Tools/AssetsHD/Regions/central-empire/capital/Common"
EMP = ROOT / "Tools/AssetsHD/Regions/central-empire/Common"
S = 2
HEIGHT = 220
THICK = 0.125


def homography(source, dest):
    """Coefficients mapping destination pixel centres to source coordinates."""
    rows, rhs = [], []
    for (x, y), (u, v) in zip(dest, source):
        rows += [[x, y, 1, 0, 0, 0, -u * x, -u * y], [0, 0, 0, x, y, 1, -v * x, -v * y]]
        rhs += [u, v]
    return np.linalg.solve(rows, rhs)


def warp(image, source, dest, size):
    coeff = homography(source, dest)
    result = (
        image.convert("RGBa")
        .transform(size, Image.Transform.PERSPECTIVE, tuple(coeff), Image.Resampling.BICUBIC)
        .convert("RGBA")
    )
    mask = Image.new("L", size)
    ImageDraw.Draw(mask).polygon(dest, fill=255)
    a = np.array(result)
    a[:, :, 3] = np.minimum(a[:, :, 3], np.asarray(mask))
    return Image.fromarray(a)


def plane(path, quad, size=(512, 512)):
    w, h = size
    return warp(Image.open(path).convert("RGBA"), quad, [(0, 0), (w, 0), (w, h), (0, h)], size)


def periodic(image, band=12):
    """Match opposite texture edges; blend only the narrow joining strip."""
    a = np.asarray(image.convert("RGBA")).astype(float)
    for axis in (0, 1):
        a = np.swapaxes(a, 0, axis)
        for k in range(band):
            t = (1 - k / band) ** 2
            avg = (a[k] + a[-1 - k]) / 2
            a[k] = (1 - t) * a[k] + t * avg
            a[-1 - k] = (1 - t) * a[-1 - k] + t * avg
        a = np.swapaxes(a, 0, axis)
    a[:, :, 3] = 255
    return Image.fromarray(np.uint8(np.clip(a, 0, 255)))


def project(u, v, z=0):
    return np.array([(u - v) * 128, (u + v) * 79.5 - z])


def painted_plane(canvas, texture, points, origin):
    dest = [tuple((p + origin) * S) for p in points]
    w, h = texture.size
    canvas.alpha_composite(warp(texture, [(0, 0), (w, 0), (w, h), (0, h)], dest, canvas.size))


def box(canvas, u, v, du, dv, height, front_u, front_v, top, origin):
    n, e, s, w = [project(*p) for p in [(u, v), (u + du, v), (u + du, v + dv), (u, v + dv)]]
    up = np.array([0, height])
    painted_plane(canvas, front_v, [w - up, n - up, n, w], origin)
    painted_plane(canvas, front_u, [w - up, s - up, s, w], origin)
    painted_plane(canvas, front_v, [s - up, e - up, e, s], origin)
    cap = top.transpose(Image.Transpose.ROTATE_90) if dv > du else top
    painted_plane(canvas, cap, [n - up, e - up, s - up, w - up], origin)


def wall(u_tex, v_tex, top, direction, corner=None):
    origin = np.array([300, HEIGHT + 20])
    canvas = Image.new("RGBA", (1200, 1050))
    off = (1 - THICK) / 2
    if corner:
        elbow(canvas, u_tex, v_tex, top, HEIGHT, THICK, corner, origin)
    elif direction == "u":
        box(canvas, 0, off, 2, THICK, HEIGHT, u_tex, v_tex.crop((0, 0, 32, 512)), top, origin)
    else:
        box(canvas, off, 0, THICK, 2, HEIGHT, u_tex.crop((0, 0, 32, 512)), v_tex, top, origin)
    return canvas, origin * S


def elbow(canvas, ut, vt, top, height, thickness, kind, origin):
    """Two legs around cell centre; ports at integer neighbouring cell boundaries."""
    off = (1 - thickness) / 2
    end = off + thickness
    if kind == "inner":
        boxes = [(off, off, 1 - off, thickness), (off, end, thickness, 1 - end)]
    else:
        boxes = [(off, 0, thickness, end), (0, off, off, thickness)]
    for u, v, du, dv in boxes:
        # Preserve stone/baluster spacing: crop by physical length, never stretch a full run.
        a = ut.crop((0, 0, max(1, round(du * 256)), ut.height))
        b = vt.crop((0, 0, max(1, round(dv * 256)), vt.height))
        box(canvas, u, v, du, dv, height, a, b, top, origin)


def floor(texture):
    return warp(
        texture,
        [(0, 0), (256, 0), (256, 256), (0, 256)],
        [(256, 0), (512, 159), (256, 318), (0, 159)],
        (512, 318),
    )


def low_module(u_tex, v_tex, top, direction, height, thickness, corner=False):
    origin = np.array([300, height + 30])
    canvas = Image.new("RGBA", (1200, 700))
    # Thin pieces occupy a centred strip within their tactical cell.
    off = (1 - thickness) / 2
    if corner:
        elbow(
            canvas,
            u_tex,
            v_tex,
            top,
            height,
            thickness,
            "inner" if corner is True else corner,
            origin,
        )
    elif direction == "u":
        box(canvas, 0, off, 2, thickness, height, u_tex, v_tex.crop((0, 0, 64, 512)), top, origin)
    else:
        box(canvas, off, 0, thickness, 2, height, u_tex.crop((0, 0, 64, 512)), v_tex, top, origin)
    return canvas, origin * S


def fit_standing(path, height, foot):
    """Place a measured ground contact at the cell centre; don't use foliage width."""
    im = Image.open(path).convert("RGBA")
    bbox = im.getbbox()
    crop = im.crop(bbox)
    scale = height / crop.height
    if scale > 1:
        raise ValueError("Source would be enlarged")
    size = (round(crop.width * scale * S), round(crop.height * scale * S))
    out = crop.resize(size, Image.Resampling.LANCZOS)
    anchor = np.array([(foot[0] - bbox[0]) * scale * S, (foot[1] - bbox[1]) * scale * S - 79.5 * S])
    return out, anchor


def ground_fit(path, points, footprint):
    """Upright affine projection: both ground slopes become +/-159/256.

    Preserve straight lines and vertical posts. Fit the painted base inside its
    tactical footprint instead of forcing a non-square object to fill a square.
    """
    u0, v0, u1, v1 = footprint
    w, s, e = np.asarray(points, dtype=float)
    sx = min((u1 - u0) * 128 / (s[0] - w[0]), (v1 - v0) * 128 / (e[0] - s[0]))
    m1 = (s[1] - w[1]) / (s[0] - w[0])
    m2 = (e[1] - s[1]) / (e[0] - s[0])
    sy = 2 * (159 / 256) * sx / (m1 - m2)
    shear = (159 / 256) * sx - sy * m1
    forward = np.array([[sx, 0], [shear, sy]])
    origin = np.array([180, 300])
    centre = (w + e) / 2
    offset = origin + project((u0 + u1) / 2, (v0 + v1) / 2) - forward @ centre
    inverse = np.linalg.inv(forward * S)
    translation = -inverse @ (offset * S)
    coeff = (
        inverse[0, 0],
        inverse[0, 1],
        translation[0],
        inverse[1, 0],
        inverse[1, 1],
        translation[1],
    )
    im = Image.open(path).convert("RGBa")
    out = im.transform(
        (800, 1200), Image.Transform.AFFINE, coeff, Image.Resampling.BICUBIC
    ).convert("RGBA")
    return out, origin * S


def save_piece(base, name, image, anchor, metadata):
    dest = base / "Calibres" / f"{name}.png"
    dest.parent.mkdir(exist_ok=True)
    image.save(dest)
    metadata[name] = (image, anchor, dest)


def update_descriptor(base, prepared):
    path = base / "install.json"
    raw = json.loads(path.read_text(encoding="utf-8"))
    known = {p.get("name") for p in raw["pieces"]}
    for name in prepared:
        if name not in known:
            raw["pieces"].append(
                dict(
                    name=name,
                    source="Calibres/" + name + ".png",
                    family="05" if name.startswith("balustrade") else "07",
                    tactical="cover",
                )
            )
    pieces = []
    for old in raw["pieces"]:
        for name in old.get("sheet", [old.get("name")]):
            p = {k: v for k, v in old.items() if k not in ("sheet", "scale", "anchorOffset")}
            p["name"] = name
            if name in prepared:
                im, anchor, dest = prepared[name]
                p["source"] = dest.relative_to(base).as_posix()
                if p["family"] != "01":
                    p["scale"] = 1 / S
            else:
                for k in ("scale", "anchorOffset"):
                    if k in old:
                        p[k] = old[k]
            pieces.append(p)
    raw["pieces"] = pieces
    path.write_text(json.dumps(raw, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    desc = hd.read_descriptor(path)
    for spec, p in zip(desc.pieces, pieces):
        name = p["name"]
        if name not in prepared or spec.is_floor:
            continue
        im, anchor, _ = prepared[name]
        fragment = hd.fragments(np.asarray(im).copy())[0]
        built = hd.install_standing(spec, name, fragment, (256, 159))
        wanted = (anchor - np.array(fragment.box[:2])) / S + hd.MARGE
        # Account for padding introduced by the automatic anchor before correction.
        pad = built.image.shape[0] - round(fragment.image.shape[0] / S) - 2 * hd.MARGE
        wanted[1] += pad
        p["anchorOffset"] = [int(round(a - b)) for a, b in zip(wanted, built.entry["anchor"])]
    path.write_text(json.dumps(raw, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main():
    prepared_cap = {}
    prepared_emp = {}
    uquad = [(144, 110), (1025, 604), (1019, 1227), (139, 680)]
    vquad = [(150, 595), (1181, 153), (1188, 755), (151, 1195)]
    u = plane(CAP / "Facades/wall-limestone-u-v2.png", uquad)
    v = plane(CAP / "Facades/wall-limestone-v-v2.png", vquad)
    wu = plane(CAP / "Facades/wall-limestone-window-u-v2.png", uquad)
    wv = plane(CAP / "Facades/wall-limestone-window-v-v2.png", vquad)
    top = plane(
        CAP / "Facades/wall-limestone-u-v2.png",
        [(241, 60), (1115, 563), (1025, 604), (144, 110)],
        (512, 64),
    )

    # Same plinth, cornice and cut-edge pixels for every facade variant.
    def matching(base, variant):
        a = np.array(variant)
        b = np.array(base)
        a[:35] = b[:35]
        a[-45:] = b[-45:]
        a[:, :25] = b[:, :25]
        a[:, -25:] = b[:, -25:]
        return Image.fromarray(a)

    wu = matching(u, wu)
    wv = matching(v, wv)
    banner_path = EMP / "Bannieres/prop-banner-lion-v2.png"
    banner = Image.open(banner_path).convert("RGBA")
    mask = Image.new("L", banner.size)
    ImageDraw.Draw(mask).polygon(
        [(505, 198), (741, 264), (734, 691), (630, 768), (507, 680)], fill=255
    )
    banner.putalpha(mask)
    cloth = warp(
        banner,
        [(505, 198), (741, 264), (741, 815), (505, 749)],
        [(0, 0), (160, 0), (160, 380), (0, 380)],
        (160, 380),
    )
    bu = u.copy()
    bu.alpha_composite(cloth, (176, 48))
    bv = v.copy()
    bv.alpha_composite(cloth, (176, 48))
    for name, ut, vt, d, corner in [
        ("wall-limestone-u", u, v, "u", None),
        ("wall-limestone-v", u, v, "v", None),
        ("wall-limestone-window-u", wu, v, "u", None),
        ("wall-limestone-window-v", u, wv, "v", None),
        ("wall-limestone-corner-inner", u, v, "u", "inner"),
        ("wall-limestone-corner-outer", u, v, "u", "outer"),
        ("wall-limestone-banner-u", bu, v, "u", None),
        ("wall-limestone-banner-v", u, bv, "v", None),
    ]:
        im, anchor = wall(ut, vt, top, d, corner)
        base = EMP if "banner" in name else CAP
        save_piece(base, name, im, anchor, prepared_emp if base == EMP else prepared_cap)
    for stem in ["floor-paving", "floor-flagstone"]:
        tex = (
            Image.open(CAP / "Sols" / f"{stem}-texture-v3.png")
            .convert("RGBA")
            .resize((256, 256), Image.Resampling.LANCZOS)
        )
        tex = periodic(tex, band=4)
        for i in range(3):
            # Keep all mortar lines stationary. Only mineral tone varies within stones.
            a = np.asarray(tex).astype(float)
            yy, xx = np.mgrid[:256, :256]
            cells = 4 if stem == "floor-paving" else 2
            t = 256 / cells
            envelope = np.sin(np.pi * (xx % t) / t) ** 2 * np.sin(np.pi * (yy % t) / t) ** 2
            noise = np.sin((xx // t) * 2.17 + (yy // t) * 3.71 + i * 2.09) * 2
            a[:, :, :3] += (envelope * noise)[..., None]
            variation = Image.fromarray(np.uint8(np.clip(a, 0, 255)))
            save_piece(CAP, f"{stem}-0{i + 1}", floor(variation), (256, 0), prepared_cap)
        if stem == "floor-paving":
            paving = tex
    # Border strips are taken from the painted flagstone, not procedurally drawn.
    stone = tex.crop((10, 10, 40, 246)).resize((34, 256), Image.Resampling.LANCZOS)
    for suffix, side in [("nw", "left"), ("ne", "top"), ("se", "right"), ("sw", "bottom")]:
        tile = paving.copy()
        if side == "left":
            tile.paste(stone, (0, 0))
        elif side == "right":
            tile.paste(stone, (222, 0))
        elif side == "top":
            tile.paste(stone.rotate(90, expand=True), (0, 0))
        else:
            tile.paste(stone.rotate(90, expand=True), (0, 222))
        save_piece(CAP, "floor-paving-edge-" + suffix, floor(tile), (256, 0), prepared_cap)
    rail_u = plane(
        CAP / "Balustrades/balustrade-limestone-u.png",
        [(238, 268), (941, 671), (941, 1093), (238, 688)],
    )
    rail_v = plane(
        CAP / "Balustrades/balustrade-limestone-v.png",
        [(306, 735), (1026, 305), (1026, 744), (306, 1161)],
    )
    rail_top = plane(
        CAP / "Balustrades/balustrade-limestone-u.png",
        [(310, 211), (996, 602), (941, 633), (238, 232)],
        (512, 64),
    )
    hedge_u = plane(
        CAP / "Vegetal/plant-hedge-u.png", [(91, 331), (1005, 865), (1004, 1118), (89, 591)]
    )
    hedge_v = plane(
        CAP / "Vegetal/plant-hedge-v.png", [(263, 807), (1204, 269), (1204, 595), (260, 1145)]
    )
    hedge_top = plane(
        CAP / "Vegetal/plant-hedge-u.png",
        [(282, 228), (1166, 746), (1005, 855), (97, 331)],
        (512, 128),
    )
    for prefix, ut, vt, tt, height, thick in [
        ("balustrade-limestone", rail_u, rail_v, rail_top, 95, 0.20),
        ("plant-hedge", hedge_u, hedge_v, hedge_top, 80, 0.4),
    ]:
        for direction in ["u", "v"]:
            im, anchor = low_module(ut, vt, tt, direction, height, thick)
            save_piece(CAP, prefix + "-" + direction, im, anchor, prepared_cap)
    for prefix, ut, vt, tt, height, thick in [
        ("balustrade-limestone", rail_u, rail_v, rail_top, 95, 0.20),
        ("plant-hedge", hedge_u, hedge_v, hedge_top, 80, 0.4),
    ]:
        for kind in ["inner", "outer"]:
            name = prefix + "-corner-" + kind
            if prefix == "plant-hedge" and kind == "inner":
                name = "plant-hedge-corner"
            im, anchor = low_module(ut, vt, tt, "u", height, thick, kind)
            save_piece(CAP, name, im, anchor, prepared_cap)
    # Ground contacts measured on the source, rather than silhouette centres.
    for folder, name, height, foot in [
        ("Vegetal", "plant-cypress", 420, (630, 1200)),
        ("Mobilier", "prop-lamppost", 350, (570, 1186)),
        ("Mobilier", "prop-planter", 145, (628, 1108)),
        ("Mobilier", "prop-barrel", 110, (628, 1010)),
        ("Mobilier", "prop-crate", 85, (635, 950)),
        ("Balustrades", "balustrade-limestone-pillar", 122, (626, 1100)),
    ]:
        im, anchor = fit_standing(CAP / folder / (name + ".png"), height, foot)
        save_piece(CAP, name, im, anchor, prepared_cap)
    im, anchor = fit_standing(banner_path, 420, (630, 1146))
    save_piece(EMP, "prop-banner-lion", im, anchor, prepared_emp)
    for base, folder, name, points, footprint in [
        (
            CAP,
            "Mobilier",
            "prop-bench-u",
            [(155, 741), (938, 1133), (1104, 1020)],
            (0.05, 0.325, 0.95, 0.675),
        ),
        (
            CAP,
            "Mobilier",
            "prop-bench-v",
            [(105, 1080), (284, 1183), (1202, 755)],
            (0.325, 0.05, 0.675, 0.95),
        ),
        (
            CAP,
            "Mobilier",
            "prop-stall-empty",
            [(214, 926), (772, 1220), (1093, 1030)],
            (0.05, 0.05, 0.95, 0.95),
        ),
        (CAP, "Vegetal", "plant-flowerbed", [(68, 698), (626, 1015), (1183, 700)], (0, 0, 1, 1)),
        (
            EMP,
            "Bannieres",
            "column-lion-emblem",
            [(405, 1111), (632, 1224), (850, 1107)],
            (0.3, 0.3, 0.7, 0.7),
        ),
    ]:
        source_name = name + "-v2.png" if name == "column-lion-emblem" else name + ".png"
        im, anchor = ground_fit(base / folder / source_name, points, footprint)
        save_piece(base, name, im, anchor, prepared_cap if base == CAP else prepared_emp)
    update_descriptor(CAP, prepared_cap)
    update_descriptor(EMP, prepared_emp)
    for base in (CAP, EMP):
        desc = hd.read_descriptor(base / "install.json")
        hd.write(desc, hd.build(desc))
    print("Projected and installed", len(prepared_cap) + len(prepared_emp), "pieces")


if __name__ == "__main__":
    main()
