"""Regression checks for the author's explicitly authorized grid calibration."""

import numpy as np
from PIL import Image
import rectify_capital_kit as R


def test_homography_maps_all_four_measured_corners():
    source = [(8, 15), (98, 27), (88, 109), (3, 84)]
    dest = [(0, 0), (256, 0), (256, 159), (0, 159)]
    c = R.homography(source, dest)
    for (x, y), expected in zip(dest, source):
        divisor = c[6] * x + c[7] * y + 1
        actual = ((c[0] * x + c[1] * y + c[2]) / divisor, (c[3] * x + c[4] * y + c[5]) / divisor)
        assert np.allclose(actual, expected)


def test_periodic_edges_match_without_repainting_the_interior():
    source = np.random.default_rng(8).integers(0, 256, (256, 256, 4), dtype=np.uint8)
    source[:, :, 3] = 255
    output = np.asarray(R.periodic(Image.fromarray(source)))
    assert np.array_equal(output[0], output[-1])
    assert np.array_equal(output[:, 0], output[:, -1])
    assert np.array_equal(output[20:-20, 20:-20], source[20:-20, 20:-20])


def test_floor_is_an_exact_grid_diamond_with_opaque_centre():
    tile = R.floor(Image.new("RGBA", (256, 256), (220, 201, 161, 255)))
    a = np.asarray(tile)[:, :, 3]
    assert tile.size == (512, 318)
    assert a[159, 256] == 255
    assert a[10, 10] == a[10, 500] == a[300, 10] == a[300, 500] == 0
    assert a[5, 256] == a[310, 256] == 255


def test_two_wall_modules_have_no_gap_at_the_shared_joint():
    tex = Image.new("RGBA", (512, 512), (220, 201, 161, 255))
    wall, anchor = R.wall(tex, tex, tex, "u")
    output = Image.new("RGBA", (1900, 1450))
    output.alpha_composite(wall, (0, 0))
    output.alpha_composite(wall, (512, 318))
    xy = anchor + R.project(2, (1 + R.THICK) / 2, R.HEIGHT / 2) * R.S
    x, y = map(round, xy)
    assert np.min(np.asarray(output)[y - 5 : y + 6, x - 5 : x + 6, 3]) == 255


def test_ground_fit_keeps_vertical_lines_vertical(tmp_path):
    a = np.zeros((240, 240, 4), dtype=np.uint8)
    a[20:180, 80:85] = (230, 200, 130, 255)
    path = tmp_path / "post.png"
    Image.fromarray(a).save(path)
    fitted, _ = R.ground_fit(path, [(20, 160), (160, 220), (220, 140)], (0.05, 0.2, 0.95, 0.8))
    rows = np.asarray(fitted)[:, :, 3] > 240
    widths = []
    full_width = max(np.count_nonzero(row) for row in rows)
    for row in rows:
        xx = np.flatnonzero(row)
        if len(xx) == full_width:
            widths.append((xx[0] + xx[-1]) / 2)
    assert len(widths) > 80
    assert max(widths) - min(widths) <= 1


def test_all_corner_ports_join_on_integer_cells():
    tex = Image.new("RGBA", (512, 512), (220, 201, 161, 255))
    for family, height, thick in [("wall", 220, 0.125), ("rail", 95, 0.2), ("hedge", 80, 0.4)]:
        for kind in ["inner", "outer"]:

            def module(direction, corner=None, family=family, height=height, thick=thick):
                if family == "wall":
                    return R.wall(tex, tex, tex, direction, corner)
                return R.low_module(tex, tex, tex, direction, height, thick, corner)

            image, anchor = module("u", kind)
            combined = Image.new("RGBA", (2600, 2400))
            origin = np.array([1300, 1100])

            def place(im, a, u, v, origin=origin, combined=combined):
                pos = origin + R.project(u, v) * R.S - a
                combined.alpha_composite(im, tuple(map(round, pos)))

            place(image, anchor, 0, 0)
            ports = [(1, 0.5), (0.5, 1)] if kind == "inner" else [(0, 0.5), (0.5, 0)]
            for direction, position in zip(
                ["u", "v"], [(1, 0), (0, 1)] if kind == "inner" else [(-2, 0), (0, -2)]
            ):
                im, a = module(direction)
                place(im, a, *position)
            alpha = np.asarray(combined)[:, :, 3]
            for u, v in ports:
                x, y = map(round, origin + R.project(u, v, height / 2) * R.S)
                assert np.min(alpha[y - 2 : y + 3, x - 2 : x + 3]) == 255, (family, kind, u, v)
