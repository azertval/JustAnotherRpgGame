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
    # Plus le carré d'angle des toits en L : quatre angles, depth² cases chacun.
    assert len(names) == 2 * sum(roofs.DEPTHS) * 4 + 9 * sum(d * d for d in roofs.DEPTHS)
    assert "roof-u-d2-r1-gable" in names
    assert "roof-v-d5-c4-single" in names
    assert "roof-l-d3-ne-c2r0" in names
    assert "roof-t-d5-w-c4r4" in names
    assert "roof-x-d2-all-c0r0" in names


@pytest.mark.skipif(not roofs.WALL_SURFACE.is_file(),
                    reason="sources de production absentes (Tools/AssetsHD n'est pas versionné)")
@pytest.mark.parametrize("name", ["roof-u-d3-r2-gable", "roof-v-d2-c0-start", "roof-u-d4-r1",
                                  "roof-l-d4-ne-c0r1", "roof-l-d3-sw-c2r2",
                                  "roof-t-d2-n-c0r0", "roof-t-d3-e-c2r1",
                                  "roof-t-d4-s-c1r2", "roof-t-d5-w-c2r2", "roof-x-d3-all-c1r1"])
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


@pytest.mark.parametrize("kind,direction", [("t", d) for d in "nesw"] + [("x", "all")])
def test_junctions_have_unique_cells_and_continuous_arms(kind, direction):
    import validate_capital_roofs as validation
    for depth in roofs.DEPTHS:
        placed = validation.junction(8, 8, depth, kind, direction, wing=4)
        positions = {(x, y) for x, y, _ in placed}
        assert len(positions) == len(placed) == depth**2 + len(roofs.junction_sides(kind, direction))*4*depth
        catalog = {p[0] for p in roofs.pieces()}
        assert all(name in catalog for _, _, name in placed)
        # Toutes les cases sont reliées ; aucun trou ni recouvrement au raccord des ailes.
        seen, todo = set(), [(8, 8)]
        while todo:
            x, y = todo.pop()
            if (x, y) in seen or (x, y) not in positions:
                continue
            seen.add((x, y))
            todo.extend([(x-1,y),(x+1,y),(x,y-1),(x,y+1)])
        assert seen == positions


def test_local_view_keeps_the_entire_visible_piece():
    mats = roofs.provisional()
    depth, column, row = 5, 0, 0
    clip = (-roofs.OVERLAP, -roofs.OVERLAP, 1+roofs.OVERLAP, 1+roofs.OVERLAP)
    full = roofs.Surface(clip, clip_after_visibility=True)
    roofs.junction_roof(full, mats, depth, "t", "n", (column, row))
    image, _ = full.result()
    box = image.getbbox()
    left, top = int((760-250)*roofs.S), int((420-300)*roofs.S)
    assert box[0] >= left and box[1] >= top
    assert box[2] <= left + 500*roofs.S and box[3] <= top + 600*roofs.S
    local, _ = roofs.render_junction_piece(mats, depth, "t", "n", column, row)
    assert np.array_equal(np.asarray(image.crop((left,top,left+1000,top+1200)))[:,:,3], np.asarray(local)[:,:,3])


def test_internal_roof_faces_cannot_reappear_below_a_gable():
    surface = roofs.Surface((-10,-10,10,10), local_view=True)
    surface.roof_union = [("u", 3, -3, 6)]
    tex = Image.new("RGBA", (4,4), (180,50,40,255))
    face = [(.5,.5,0),(1.5,.5,0),(1.5,1.5,0),(.5,1.5,0)]
    uv = [(0,0),(1,0),(1,1),(0,1)]
    surface.quad(face, tex, uv, roof_top=True)
    assert surface.result()[0].getbbox() is None
    surface.quad(face, tex, uv)
    assert surface.result()[0].getbbox() is not None
