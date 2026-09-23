"""La toiture de la Capitale (LOT-129) : les pièces qu'elle nomme, et la géométrie d'une pièce."""

import sys
from pathlib import Path

import numpy as np
import pytest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import build_capital_roofs as roofs  # noqa: E402
import install_hd_asset as hd  # noqa: E402


def test_les_marges_invisibles_ne_reduisent_pas_la_corniche(tmp_path, monkeypatch):
    monkeypatch.setattr(roofs, "SOURCES", tmp_path)
    image = Image.new("RGBA", (64, 64))
    image.paste((230, 220, 200, 255), (0, 20, 64, 40))
    image.putpixel((3, 0), (255, 0, 0, 1))
    image.putpixel((3, 19), (230, 220, 200, 90))
    image.save(tmp_path / "gable-cornice.png")
    band = roofs.load("gable-cornice")
    assert band.size == (64, 24)
    assert band.getpixel((3, 1))[3] == 90


def test_les_pieces_couvrent_chaque_profondeur_dans_les_deux_sens():
    names = [piece[0] for piece in roofs.pieces()]
    assert len(names) == len(set(names))
    # Par sens : une rangée par case de profondeur, de 2 à 5, et quatre positions le long du faîtage.
    assert len(names) == 2 * sum(roofs.DEPTHS) * 4
    assert "roof-u-d2-r1-gable" in names
    assert "roof-v-d5-c4-single" in names


@pytest.mark.skipif(not roofs.WALL_SURFACE.is_file(),
                    reason="sources de production absentes (Tools/AssetsHD n'est pas versionné)")
@pytest.mark.parametrize("name", ["roof-u-d3-r2-gable", "roof-v-d2-c0-start", "roof-u-d4-r1"])
def test_une_piece_est_d_un_seul_tenant_et_ancree_a_sa_case(name):
    mats = roofs.materials(use_provisional=True)
    (piece,) = [p for p in roofs.pieces() if p[0] == name]
    image, anchor = roofs.render_piece(mats, *piece[1:])
    assert len(hd.fragments(np.asarray(image).copy())) == 1
    # L'ancre est le sommet nord de la case, à l'étage : dans le canevas, pas sur un bord.
    assert 0 < anchor[0] < image.width and 0 < anchor[1] < image.height


def test_une_matiere_provisoire_ne_s_installe_jamais():
    with pytest.raises(SystemExit):
        roofs.main(["--provisional", "--install"])
