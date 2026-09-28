# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Le garde-fou doit refuser le patinage et ne jamais accepter une preuve périmée."""
import json
from pathlib import Path
import tempfile
import unittest

from PIL import Image, ImageDraw

import check_figure_walk as Q


class WalkQualityTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.manifest = self.root / "manifest.json"
        self.manifest.write_text(json.dumps({"tile": [256, 159], "ground": 252,
                                             "frame": [192, 256]}))
        sheet = Image.new("RGBA", (1536, 256))
        for i in range(8):
            ImageDraw.Draw(sheet).rectangle((i*192+16, 50, i*192+175, 252),
                                            fill=(100+i*10, 80, 50, 255))
        for facing in Q.FACINGS:
            sheet.save(self.root / f"walk-{facing}.png")
            (self.root / f"walk-{facing}.anim.json").write_text(json.dumps({
                "frameWidth": 192, "frameHeight": 256,
                "clips": {"walk": {"frames": list(range(8)), "loop": True,
                                    "frameDuration": 0.0625}}}))

    def entry(self, facing):
        stem = f"walk-{facing}"
        paths = [self.root / f"{stem}.png", self.root / f"{stem}.anim.json", self.manifest]
        sx, sy = Q.FACINGS[facing]
        # Séquences synthétiques parfaites, deux transitions de chaque côté.
        points = [[100-sx*16*i, 200-sy*9.9375*i] for i in range(4)]
        return {"fingerprint": Q.fingerprint(paths), "reviewer": "test fixture",
                "evidence": "synthetic test, not an artistic validation",
                "contacts": {"left": points+[None]*4, "right": [None]*4+points},
                "support": {"left": ["stance"]*4+["swing"]*4,
                            "right": ["swing"]*4+["stance"]*4},
                "visual": dict.fromkeys(Q.VISUAL_CHECKS, True)}

    def test_world_contact_compensates_all_four_directions(self):
        for facing, (sx, sy) in Q.FACINGS.items():
            self.assertAlmostEqual(Q.contact_slip([90, 220], [90-sx*16, 220-sy*9.9375],
                                                  facing, 0.0625, [256, 159]), 0)
            self.assertGreater(Q.contact_slip([90, 220], [90, 220], facing,
                                              0.0625, [256, 159]), 18)

    def test_good_metadata_does_not_replace_review(self):
        r = Q.inspect_strip(self.root, self.manifest, "se", {})
        self.assertEqual(r["status"], "pending")

    def test_complete_evidence_can_pass(self):
        review = {f"walk-{f}": self.entry(f) for f in Q.FACINGS}
        for f in Q.FACINGS:
            r = Q.inspect_strip(self.root, self.manifest, f, review)
            self.assertEqual(r["status"], "accepted", r)

    def test_stationary_feet_are_rejected(self):
        entry = self.entry("se")
        entry["contacts"]["left"] = [[100, 200]]*3+[None]*5
        r = Q.inspect_strip(self.root, self.manifest, "se", {"walk-se": entry})
        self.assertEqual(r["status"], "rejected")
        self.assertTrue(any("Glissement" in s for s in r["errors"]))

    def test_contacts_across_loop_are_measured(self):
        frames = [Image.new("RGBA", (192, 256), (100, 100, 100, 255))]*8
        contacts = {"left": [[100, 200]]+[None]*6+[[100, 200]], "right": [None]*8}
        errors, pending, metrics = Q.check_contacts(contacts, "se", 0.0625, [256, 159], frames)
        self.assertTrue(errors)
        self.assertTrue(pending)
        self.assertEqual((metrics[0]["from"], metrics[0]["to"]), (7, 0))

    def test_changed_asset_invalidates_review(self):
        entry = self.entry("se")
        path = self.root / "walk-se.png"
        with Image.open(path) as im:
            edited = im.copy()
        edited.putpixel((20, 60), (120, 120, 120, 255))
        edited.save(path)
        r = Q.inspect_strip(self.root, self.manifest, "se", {"walk-se": entry})
        self.assertEqual(r["status"], "pending")
        self.assertTrue(any("périmée" in s for s in r["pending"]))

    def test_duplicate_frames_and_bad_duration_are_rejected(self):
        path = self.root / "walk-se.png"
        Image.new("RGBA", (1536, 256)).save(path)
        r = Q.inspect_strip(self.root, self.manifest, "se", {})
        self.assertEqual(r["status"], "rejected")
        self.assertTrue(any("identiques" in s for s in r["errors"]))
        path = self.root / "walk-sw.anim.json"
        value = Q.read_json(path)
        value["clips"]["walk"]["frameDuration"] = 0.1
        path.write_text(json.dumps(value))
        r = Q.inspect_strip(self.root, self.manifest, "sw", {})
        self.assertTrue(any("Cadence" in s for s in r["errors"]))

    def test_missing_direction_and_malformed_json_fail_closed(self):
        (self.root / "walk-ne.png").unlink()
        (self.root / "walk-sw.anim.json").write_text("[]")
        out = self.root / "report"
        self.assertEqual(Q.run(self.root, self.manifest, out), 1)
        report = Q.read_json(out / "report.json")
        self.assertEqual(len(report["strips"]), 4)
        self.assertEqual(report["status"], "rejected")
        self.assertTrue((out / "index.html").exists())

    def test_pending_exit_code_and_review_not_overwritten(self):
        out = self.root / "report"
        self.assertEqual(Q.run(self.root, self.manifest, out), 2)
        review = self.root / "review.json"
        original = json.dumps({f"walk-{f}": self.entry(f) for f in Q.FACINGS})
        review.write_text(original)
        self.assertEqual(Q.run(self.root, self.manifest, out, review), 0)
        self.assertEqual(review.read_text(), original)

    def test_invalid_contact_and_explicit_visual_rejection_block_acceptance(self):
        entry = self.entry("se")
        entry["contacts"]["left"][0] = [float("nan"), 200]
        entry["visual"]["facing"] = False
        r = Q.inspect_strip(self.root, self.manifest, "se", {"walk-se": entry})
        self.assertEqual(r["status"], "rejected")
        self.assertTrue(any("coordonnées invalides" in e for e in r["errors"]))
        self.assertTrue(any("facing" in e for e in r["errors"]))

    def test_output_collision_cannot_overwrite_review(self):
        out = self.root / "report"
        out.mkdir()
        path = out / "review-template.json"
        path.write_text("{}")
        with self.assertRaises(ValueError):
            Q.run(self.root, self.manifest, out, path)
        self.assertEqual(path.read_text(), "{}")

    def test_synchronized_feet_are_rejected(self):
        entry = self.entry("se")
        entry["support"]["right"] = entry["support"]["left"]
        entry["contacts"]["right"] = entry["contacts"]["left"]
        errors, _ = Q.check_support(entry["support"], entry["contacts"])
        self.assertTrue(any("non alternés" in e for e in errors))

    def test_permanent_support_is_rejected(self):
        errors, _ = Q.check_support({"left": ["stance"]*8, "right": ["stance"]*8}, {})
        self.assertTrue(errors)

    def test_unknown_support_cannot_pass(self):
        errors, pending = Q.check_support({"left": [None]*8, "right": [None]*8}, {})
        self.assertFalse(errors)
        self.assertTrue(pending)

    def test_alternation_across_loop_is_valid(self):
        entry = self.entry("se")
        for field in ("support", "contacts"):
            for foot in ("left", "right"):
                values = entry[field][foot]
                entry[field][foot] = values[2:]+values[:2]
        self.assertEqual(Q.check_support(entry["support"], entry["contacts"]), ([], []))


if __name__ == "__main__":
    unittest.main()
