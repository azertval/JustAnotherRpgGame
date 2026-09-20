# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Contrôles bloquants : aucun compte complété ni verdict périmé accepté."""
import json

import pytest

import asset_factory as factory

from PIL import Image
import asset_factory_qc as qc  # noqa: E402


def test_missing_hit_is_rejected_without_manufacturing_frame():
    image = Image.new("RGBA", (800, 360))
    for x, y in ((40, 30), (440, 30), (40, 210)):
        image.paste((200, 100, 50, 255), (x, y, x + 40, y + 90))
    bands, metrics = factory.normalize_sheet(image, "hit")
    assert "hit" not in bands
    assert "1 figures" in metrics["errors"]["hit"]


def test_empty_and_opaque_images_are_rejected():
    for color in ((0, 0, 0, 0), (0, 0, 0, 255)):
        with pytest.raises(ValueError):
            factory.normalize_sheet(Image.new("RGBA", (800, 180), color), "hit")
        with pytest.raises(ValueError):
            factory.normalize_portrait(Image.new("RGBA", (128, 128), color))


def test_clipping_is_not_silently_allowed():
    image = Image.new("RGBA", (1000, 300))
    for x, y in ((10, 40), (510, 40), (10, 200), (510, 200)):
        image.paste((200, 100, 50, 255), (x, y, x + 200, y + 20))
    bands, metrics = factory.normalize_sheet(image, "hit")
    assert "hit" not in bands
    assert "rognage" in metrics["errors"]["hit"]


def test_palette_ignores_rgb_of_invisible_pixels():
    image = Image.new("RGBA", (128, 128), (0, 255, 0, 0))
    image.paste((200, 100, 50, 255), (30, 20, 100, 128))
    result = factory.normalize_portrait(image)
    assert result.size == (1024, 1024)
    assert result.getpixel((0, 0)) == (0, 0, 0, 0)
    assert result.getpixel((400, 400)) == (200, 100, 50, 255)


def test_measure_detects_empty_frame_and_semitransparent_pixel(tmp_path):
    image = Image.new("RGBA", (192, 64))
    image.putpixel((10, 20), (200, 100, 50, 120))
    path = tmp_path / "hit.png"
    image.save(path)
    metrics, errors = qc.measure(path, 4, 48)
    assert metrics["semi_transparent_pixels"] == 1
    assert "frame vide" in errors
    assert "alpha non binaire" in errors


def test_repeated_walk_silhouettes_fail(tmp_path):
    image = Image.new("RGBA", (384, 64))
    for index in range(8):
        image.paste((200, 100, 50, 255), (index * 48 + 10, 19, index * 48 + 30, 64))
    image.save(tmp_path / "walk.png")
    profile = {"height": 45, "ground": 63, "colors": 32, "max_height_drift": 4,
               "max_ground_drift": 2, "max_loop_mask_distance": .35,
               "min_walk_distinct_frames": 8}
    result = qc.check(tmp_path, {"walk": factory.ANIMATIONS["walk"]}, profile)
    assert "marche : silhouettes répétées" in result["walk"]["errors"]


def test_fingerprint_changes_when_brief_changes(tmp_path, monkeypatch):
    monkeypatch.setattr(factory, "ROOT", tmp_path)
    monkeypatch.setattr(factory, "MEMORY", tmp_path / "memory")
    directory = factory.asset("anariel")
    directory.mkdir(parents=True)
    for name in ("fiche.json", "prompt_b.txt", "palette.txt"):
        (directory / name).write_text("before", encoding="utf-8")
    factory.write(factory.MEMORY / "profile.json", {})
    before = factory.fingerprint("anariel")
    (directory / "prompt_b.txt").write_text("after", encoding="utf-8")
    assert before != factory.fingerprint("anariel")


def test_atomic_journal_roundtrip(tmp_path):
    path = tmp_path / "state.json"
    factory.write(path, {"state": "requested"})
    factory.write(path, {"state": "rejected"})
    assert json.loads(path.read_text()) == {"state": "rejected"}
    assert not path.with_suffix(".json.tmp").exists()


def test_text_fingerprints_survive_git_line_ending_normalization(tmp_path):
    path = tmp_path / "prompt.txt"
    path.write_bytes(b"A\r\nB\r\n")
    before = factory.digest(path)
    path.write_bytes(b"A\nB\n")
    assert factory.digest(path) == before


@pytest.mark.parametrize("failed_step", ["original", "staged"])
def test_failed_install_preserves_original_and_manifest(tmp_path, monkeypatch, failed_step):
    from pathlib import Path
    monkeypatch.setattr(factory, "ROOT", tmp_path)
    monkeypatch.setattr(factory, "MEMORY", tmp_path / "memory")
    monkeypatch.setattr(factory, "NPC", tmp_path / "Npc")
    monkeypatch.setattr(factory.subprocess, "check_output", lambda *a, **k: "lot/test")
    monkeypatch.setattr(factory, "qc", lambda slug: {"errors": []})
    monkeypatch.setattr(factory, "snapshot", lambda slug: {"test": 1})
    original = factory.NPC / "anariel"
    original.mkdir(parents=True)
    (original / "sentinel.txt").write_text("original")
    manifest = {"npcs": ["anariel"], "replaces": {"legacy": "anariel"}}
    factory.write(factory.NPC / "manifest.json", manifest)
    candidate = factory.asset("anariel") / "candidate"
    candidate.mkdir(parents=True)
    for name in ["portrait", *factory.ANIMATIONS]:
        Image.new("RGBA", (1, 1)).save(candidate / f"{name}.png")
    factory.write(factory.asset("anariel") / "review.json",
                  {"verdict": "validated", "snapshot": {"test": 1}})
    real_rename = Path.rename

    def fail_rename(path, target):
        if (failed_step == "original" and path == original) or (failed_step == "staged" and path.name == "staged"):
            raise OSError("simulated interrupted installation")
        return real_rename(path, target)

    monkeypatch.setattr(Path, "rename", fail_rename)
    with pytest.raises(OSError, match="simulated"):
        factory.integrate("anariel")
    assert (original / "sentinel.txt").read_text() == "original"
    assert factory.read(factory.NPC / "manifest.json") == manifest


def test_unsafe_tour_identifier_is_rejected():
    with pytest.raises(ValueError, match="Identifiant"):
        factory.receive("anariel", "../../elsewhere", "unused.png")
    with pytest.raises(ValueError, match="Identifiant"):
        factory.reprocess("anariel", "../../elsewhere")


def test_cli_qc_failure_returns_unsuccessful_exit(monkeypatch):
    monkeypatch.setattr("sys.argv", ["asset_factory.py", "qc", "anariel"])
    monkeypatch.setattr(factory, "qc", lambda slug: {"errors": ["missing animation"]})
    assert factory.main() == 1


def test_missing_candidate_removes_stale_gif(tmp_path):
    candidate, proofs, baseline = (tmp_path / name for name in ("candidate", "proofs", "baseline"))
    proofs.mkdir()
    (proofs / "walk.gif").write_bytes(b"old proof")
    qc.previews(candidate, proofs, {"walk": factory.ANIMATIONS["walk"]}, baseline)
    assert not (proofs / "walk.gif").exists()


def test_corrupt_response_invalidates_old_candidate_and_is_journaled(tmp_path, monkeypatch):
    monkeypatch.setattr(factory, "ROOT", tmp_path)
    monkeypatch.setattr(factory, "MEMORY", tmp_path / "memory")
    monkeypatch.setattr(factory, "fingerprint", lambda slug: {})
    monkeypatch.setattr(factory, "status", lambda: {})
    directory = factory.asset("anariel")
    candidate = directory / "candidate"
    candidate.mkdir(parents=True)
    (candidate / "hit.png").write_bytes(b"old candidate")
    factory.write(directory / "provenance.json", {"hit": {"tour": "hit-000"}})
    factory.write(directory / "review.json", {"verdict": "validated"})
    tour = directory / "tours/hit-001"
    tour.mkdir(parents=True)
    (tour / "prompt.txt").write_text("prompt")
    factory.write(tour / "request.json", {"stage": "hit", "inputs": {}, "references": [],
                                         "prompt_sha256": factory.digest(tour / "prompt.txt")})
    source = tmp_path / "corrupt.png"
    source.write_bytes(b"not an image")
    factory.receive("anariel", "hit-001", source)
    assert factory.read(tour / "response.json")["state"] == "rejected"
    assert not (candidate / "hit.png").exists()
    assert not (directory / "review.json").exists()
    assert "hit" not in factory.read(directory / "provenance.json")
