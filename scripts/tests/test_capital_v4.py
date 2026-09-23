"""Check real staged V4 pixels and metadata, including every modular junction."""

import json
import unittest
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
V4 = ROOT / "Tools/AssetsHD/Regions/central-empire/capital/Common/V4"


@unittest.skipUnless(
    (V4 / "build.json").is_file(),
    "sources de production absentes (Tools/AssetsHD n'est pas versionné) : poste de l'auteur seulement",
)
class KitV4Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = json.loads((V4 / "build.json").read_text())

    def test_complete_standard_kit(self):
        self.assertEqual(len(self.data), 38)
        for name, m in self.data.items():
            im = Image.open(V4 / "Scene" / (name + ".png")).copy()
            self.assertEqual(list(im.size), m["entry"]["size"])
            self.assertEqual(im.mode, "RGBA")
            self.assertTrue(all(isinstance(x, int) and x >= 0 for x in m["entry"]["anchor"]))
            if m["entry"]["family"] == "01":
                self.assertEqual(im.size, (256, 159))
                self.assertEqual(m["entry"]["anchor"], [128, 0])

    def test_every_corner_has_two_opaque_connected_ports(self):
        for prefix, height in [
            ("wall-limestone", 218),
            ("balustrade-limestone", 97),
            ("plant-hedge", 82),
        ]:
            for kind in ["inner", "outer"]:
                name = prefix + "-corner-" + kind
                if prefix == "plant-hedge" and kind == "inner":
                    name = "plant-hedge-corner"
                out = Image.new("RGBA", (1400, 1200))
                origin = np.array([700, 550])

                def project(x, y, z=0):
                    return np.array([(x - y) * 128, (x + y) * 79.5 - z])

                def place(n, x, y, origin=origin, out=out):
                    a = np.array(self.data[n]["entry"]["anchor"])
                    pos = origin + project(x, y) - a
                    out.alpha_composite(
                        Image.open(V4 / "Scene" / (n + ".png")).convert("RGBA"),
                        tuple(map(round, pos)),
                    )

                place(name, 0, 0)
                for d, pos in zip(
                    ["u", "v"], [(1, 0), (0, 1)] if kind == "inner" else [(-2, 0), (0, -2)]
                ):
                    place(prefix + "-" + d, *pos)
                alpha = np.asarray(out)[:, :, 3]
                ports = [(1, 0.5), (0.5, 1)] if kind == "inner" else [(0, 0.5), (0.5, 0)]
                for u, v in ports:
                    x, y = map(round, origin + project(u, v, height))
                    self.assertGreaterEqual(int(alpha[y, x]), 220, (name, u, v))

    def test_floor_variants_keep_the_same_joint_geometry(self):
        for stem in ["floor-paving", "floor-flagstone"]:
            images = [
                np.asarray(Image.open(V4 / "Scene" / f"{stem}-0{i}.png")).astype(int)
                for i in range(1, 4)
            ]
            for a in images[1:]:
                self.assertTrue(np.array_equal(a[:, :, 3], images[0][:, :, 3]))
                self.assertLessEqual(
                    int(np.max(abs(a[:, :, :3] - images[0][:, :, :3])[a[:, :, 3] > 128])), 5
                )

    def test_all_calibrated_sources_are_present(self):
        # Final installation is checked separately with install_hd_asset --check.
        for m in self.data.values():
            self.assertTrue(Path(m["spec"]["source"]).is_file())


if __name__ == "__main__":
    unittest.main()
