# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""L'aperçu de marche (LOT-112) : la géométrie de la maquette, et une marche qui avance au pas du jeu."""
import numpy as np
import pytest

import install_hd_asset as install
import preview_figure_walk as P


def test_le_milieu_de_la_place_est_au_milieu_de_l_ecran():
    assert P.screen_of(3.5, 3.5) == pytest.approx((960.0, 540.0))
    # Une case vers le sud-est : un demi-losange à droite et en bas, à 100 px par case.
    x, y = P.screen_of(4.5, 3.5)
    assert (x - 960.0, y - 540.0) == pytest.approx((50.0, 159.0 / 256.0 * 50.0))


def bande(facing, images=8):
    """Une bande installée factice : des cellules pleines de 192 × 256."""
    spec = install.StripSpec(source="walk.png", clip="walk", frames=images, facing=facing, frame_duration=0.1)
    image = np.zeros((256, 192 * images, 4), dtype=np.uint8)
    image[40:252, :, :] = 200
    anim = {"clips": {"walk": {"frameDuration": 0.1}}}
    return install.InstalledStrip(spec=spec, image=image, anim=anim, scale=1.0, cell=(192, 256), frames=[],
                                  split="parts égales")


def test_une_marche_orientee_fait_le_tour_de_la_boucle():
    figure = install.InstalledFigure(name="heros", strips=[bande(f) for f in ("se", "sw", "ne", "nw")])
    frames, notes = P.render(figure, "walk", 1, None, P.WALK_SPEED, 252)
    # Quatre côtés de trois cases, à quatre cases par seconde et trente images par seconde.
    assert len(frames) == 4 * round(3 / P.WALK_SPEED * P.FPS)
    assert [n.split(" ")[0] for n in notes] == ["se", "sw", "nw", "ne"]
    assert frames[0].size == (P.CROP[2] - P.CROP[0], P.CROP[3] - P.CROP[1])


def test_l_essai_de_cadence_ne_marche_que_vers_le_sud_est():
    figure = install.InstalledFigure(name="essai", strips=[bande("se", images=6)])
    frames, notes = P.render(figure, "walk", 2, None, P.WALK_SPEED, 252)
    assert len(frames) == 2 * round(3 / P.WALK_SPEED * P.FPS)
    assert set(n.split(" ")[0] for n in notes) == {"se"}
