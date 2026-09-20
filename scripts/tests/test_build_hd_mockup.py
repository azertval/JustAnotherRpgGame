# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""La maquette du standard 2D HD (LOT-101) : la géométrie et le détourage, sans la planche.

La planche de référence vit dans `Tools/`, qui n'est pas versionné : la CI ne peut pas monter la
maquette. Ce qu'elle peut vérifier, et qui est tout ce qui peut dériver en silence, c'est la
**géométrie** du montage -- la projection, l'étendue que les deux vues cadrent, l'ordre du peintre --
et le **détourage**, éprouvé sur une planche synthétique. Plus un garde-fou : les valeurs du script
sont celles du standard écrit, et pas d'autres.
"""
import re

import pytest

pytest.importorskip('numpy')
pytest.importorskip('PIL.Image')

import numpy as np  # noqa: E402

import build_hd_mockup as M  # noqa: E402

STANDARD = 'Planning/standards/style-2d-hd.md'


def test_les_valeurs_du_montage_sont_celles_du_standard(root):
    """Le montage et le standard ne peuvent pas diverger sans que ce test le dise."""
    texte = (root / STANDARD).read_text(encoding='utf-8')
    assert f'{M.TILE_W} × {M.TILE_H} px' in texte
    assert f'{M.FIGURE_HEIGHT} px' in texte
    assert f'{M.FIGURE_CELL[0]} × {M.FIGURE_CELL[1]}' in texte
    # Le zoom des deux vues : 100 px à 1080p, 200 px à 2160p.
    for spec in M.VIEWS.values():
        assert f'{spec["tile_on_screen"]} px' in texte


def test_la_projection_est_celle_du_moteur():
    assert M.tile_center(0, 0) == (0.0, 0.0)
    # Une colonne va vers la droite et vers le bas, une rangée vers la gauche et vers le bas.
    assert M.tile_center(1, 0) == (M.TILE_W / 2, M.TILE_H / 2)
    assert M.tile_center(0, 1) == (-M.TILE_W / 2, M.TILE_H / 2)
    # Le rapport du losange est celui d'IsoProjection, à un demi-pixel près.
    assert M.TILE_H / M.TILE_W == pytest.approx(0.62, abs=0.005)


def test_les_deux_definitions_cadrent_la_meme_etendue():
    """Un écran plus fin montre le même jeu plus finement, jamais plus de jeu."""
    x0, y0, x1, y1 = M.art_extent()
    assert (x1 - x0) / M.TILE_W == pytest.approx(19.2, abs=0.05)
    assert (y1 - y0) / M.TILE_H == pytest.approx(17.4, abs=0.05)
    # L'étendue est centrée sur le milieu de la place.
    cx, cy = M.tile_center(*M.VIEW_CENTER)
    assert (x0 + x1) / 2 == pytest.approx(cx)
    assert (y0 + y1) / 2 == pytest.approx(cy)


def test_le_sol_remplit_le_cadre():
    """Aucun coin de la vue ne doit rester sur le fond : le champ de dalles déborde l'étendue."""
    extent = M.art_extent()
    cases = M.tiles_in(extent)
    assert len(cases) > 700
    for column in range(M.GRID):
        for row in range(M.GRID):
            assert (column, row) in cases
    x0, y0, x1, y1 = extent
    for corner_x, corner_y in ((x0, y0), (x1, y0), (x0, y1), (x1, y1)):
        proches = [c for c in cases
                   if abs(M.tile_center(*c)[0] - corner_x) < M.TILE_W
                   and abs(M.tile_center(*c)[1] - corner_y) < M.TILE_H]
        assert proches, 'un coin de la vue n\'est couvert par aucune dalle'


def test_l_ordre_du_peintre_suit_le_coin_sud():
    """Une pièce se dessine d'après le coin de son emprise le plus proche de l'œil."""
    assert M.depth(0, 0, (1, 1)) == 0
    assert M.depth(0, 0, (3, 3)) == 4
    # Une pièce plus au sud passe après une pièce plus au nord, quelle que soit son emprise.
    assert M.depth(2, 2, (3, 3)) > M.depth(4, 0, (2, 1))


def test_le_detourage_rend_l_alpha_et_la_couleur_propre():
    """Sur le bord, le pixel est un mélange : on en retire le fond pour retrouver la couleur.

    L'alpha se LIT dans la distance au fond, il n'est donc pas la couverture exacte du pixel ; ce
    qui est garanti, c'est qu'un bord à peine couvert n'est ni opaque ni transparent, et que la
    couleur redressée revient **vers la pièce** au lieu de rester dans le vert sombre de la planche.
    """
    fond = M.SHEET_BG
    piece = np.array([230.0, 220.0, 200.0])
    bord = 0.08 * piece + 0.92 * fond
    image = np.stack([np.tile(fond, (1, 1)), np.tile(piece, (1, 1)), np.tile(bord, (1, 1))])
    front, alpha = M.key_on_dark(image)
    assert alpha[0, 0] == pytest.approx(0.0)
    assert alpha[1, 0] == pytest.approx(1.0)
    assert 0.0 < alpha[2, 0] < 1.0
    assert np.abs(front[2, 0] - piece).sum() < np.abs(bord - piece).sum()


def test_une_dalle_est_un_losange_a_la_taille_du_standard():
    """Le sol se découpe en losange, d'un pixel plus grand : deux dalles jointives laissent une couture."""
    sheet = np.full((900, 400, 3), M.SHEET_BG, dtype=float)
    sheet[520:700, 20:180] = [220.0, 200.0, 170.0]
    dalle = M.cut_floor(sheet, (100.0, 610.0, 68.0, 43.0, 1.0))
    assert dalle.size == (M.TILE_W + 2, M.TILE_H + 2)
    alpha = dalle.split()[3]
    assert alpha.getpixel((dalle.width // 2, dalle.height // 2)) == 255
    assert alpha.getpixel((1, 1)) == 0, 'un coin du losange doit rester transparent'


def test_la_vue_2160_est_deposee_en_fenetre():
    """Un cadre 2160p pèse plus que la limite du dépôt : on en dépose un extrait au pixel près."""
    from PIL import Image

    art = Image.new('RGBA', (M.VIEWS['1080']['size'][0] * 2, M.VIEWS['1080']['size'][1] * 2))
    assert M.view(art, '1080').size == (1920, 1080)
    assert M.view(art, '2160').size == (1920, 1080)
    assert M.VIEWS['2160']['tile_on_screen'] == 2 * M.VIEWS['1080']['tile_on_screen']


def test_les_maquettes_deposees_sont_sous_la_limite_du_depot(root):
    """Cinq mébioctets par fichier (check_binary_files.py) : la maquette ne doit pas les franchir."""
    deposees = sorted(M.MAQUETTES.glob('maquette-2d-hd-*.png'))
    assert len(deposees) == len(M.VIEWS) + 1
    for chemin in deposees:
        assert chemin.stat().st_size < 5 * 1024 * 1024, chemin.name


def test_la_consigne_du_generateur_porte_ses_trois_blocs(root):
    """Le style est écrit : trois blocs, dont deux figés, et la planche pour ancre."""
    texte = (root / 'Planning/standards/consigne-2d-hd.md').read_text(encoding='utf-8')
    for titre in ('## Bloc A', '## Bloc B', '## Bloc C'):
        assert titre in texte
    assert 'arenarea-planche-reference-v2.png' in texte
    # Les trous du bloc B se remplissent depuis la commande : aucun ne doit rester sans emploi.
    trous = set(re.findall(r'\{([A-Z_]+)\}', texte))
    assert trous == {'LOSANGE_L', 'LOSANGE_H', 'EMPRISE_L', 'EMPRISE_H'}
