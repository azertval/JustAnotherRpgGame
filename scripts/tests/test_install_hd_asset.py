# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""La chaîne de production HD (LOT-104) : détourage, découpe, réduction, ancrage, manifeste.

Les sources vivent dans `Tools/`, qui n'est pas versionné : la CI ne peut pas rejouer l'installation
du Colisée. Ce qu'elle éprouve, sur des sources synthétiques dont on connaît la géométrie, c'est
que la commande retrouve l'échelle et l'ancre qu'on y a mises, et qu'elle refuse ce qu'elle doit
refuser.
"""
import json

import pytest

pytest.importorskip('numpy')
pytest.importorskip('PIL.Image')

import numpy as np  # noqa: E402
from PIL import Image, ImageDraw  # noqa: E402

import check_hd_assets  # noqa: E402
import install_hd_asset as M  # noqa: E402

TILE = (256, 159)
HALF_W, HALF_H = TILE[0] / 2, TILE[1] / 2


def diamond(draw, cx, cy, width, height, fill=(214, 190, 150, 253)):
    draw.polygon([(cx, cy - height / 2), (cx + width / 2, cy), (cx, cy + height / 2), (cx - width / 2, cy)],
                 fill=fill)


def prism(k, columns, rows, height, origin=(700.0, 600.0)):
    """Une pièce debout synthétique : le socle C × R dessiné à l'échelle k de la source, extrudé de
    `height` px. Rend l'image et le sommet nord du socle, en px de source."""
    nx, ny = origin
    col = np.array([HALF_W, HALF_H]) * k
    row = np.array([-HALF_W, HALF_H]) * k
    north = np.array([nx, ny])
    east = north + columns * col
    south = east + rows * row
    west = north + rows * row
    up = np.array([0.0, -height])
    image = Image.new('RGBA', (1400, 1200), (0, 0, 0, 0))
    ImageDraw.Draw(image).polygon([tuple(p) for p in (west, south, east, east + up, north + up, west + up)],
                                  fill=(150, 140, 130, 253))
    return np.asarray(image).copy(), north, west, height


def test_le_voile_tombe_et_l_interieur_monte():
    alpha = np.array([0, 10, 16, 132, 248, 253], np.uint8)
    cleaned = M.clean_alpha(alpha)
    assert list(cleaned[:3]) == [0, 0, 0]
    assert list(cleaned[-2:]) == [255, 255]
    assert 0 < cleaned[3] < 255


def test_une_planche_se_decoupe_dans_l_ordre_de_lecture_sans_ses_points_perdus():
    image = Image.new('RGBA', (1300, 800), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    diamond(draw, 950, 200, 600, 340, fill=(10, 200, 10, 253))   # en haut à droite
    diamond(draw, 320, 210, 600, 340, fill=(200, 10, 10, 253))   # en haut à gauche
    diamond(draw, 320, 600, 600, 340, fill=(10, 10, 200, 253))   # en bas
    draw.rectangle((1200, 700, 1203, 703), fill=(255, 255, 255, 255))  # un point perdu
    draw.rectangle((0, 0, 1299, 799), outline=(0, 0, 0, 8))             # le voile du générateur
    found = M.fragments(np.asarray(image).copy())
    assert len(found) == 3
    assert [tuple(f.image[f.image.shape[0] // 2, f.image.shape[1] // 2, :3]) for f in found] == [
        (200, 10, 10), (10, 200, 10), (10, 10, 200)]
    assert found[0].image[..., 3].max() == 255


def test_une_dalle_devient_exactement_le_losange_du_lieu():
    image = Image.new('RGBA', (700, 400), (0, 0, 0, 0))
    diamond(ImageDraw.Draw(image), 350, 200, 592, 336)
    fragment = M.fragments(np.asarray(image).copy())[0]
    spec = M.PieceSpec(source='s.png', names=['floor-sand-01'], family='01')
    installed = M.install_floor(spec, 'floor-sand-01', fragment, TILE)
    assert installed.image.shape == (159, 256, 4)
    assert installed.entry['size'] == [256, 159]
    assert installed.entry['anchor'] == [128, 0]
    assert installed.entry['class'] == 'floor'
    # Le bord garde un alpha continu : ni tout opaque, ni binaire.
    alpha = installed.image[..., 3]
    assert ((alpha > 0) & (alpha < 255)).any()


def test_une_dalle_trop_petite_est_refusee():
    image = Image.new('RGBA', (300, 200), (0, 0, 0, 0))
    diamond(ImageDraw.Draw(image), 150, 100, 200, 124)
    fragment = M.fragments(np.asarray(image).copy())[0]
    spec = M.PieceSpec(source='s.png', names=['floor-sand-01'], family='01')
    with pytest.raises(M.DescriptorError, match='jamais agrandi'):
        M.install_floor(spec, 'floor-sand-01', fragment, TILE)


@pytest.mark.parametrize('columns, rows', [(3, 1), (1, 3), (2, 2), (1, 1)])
def test_une_piece_debout_retrouve_son_echelle_et_son_ancre(columns, rows):
    k = 1.6
    rgba, north, west, height = prism(k, columns, rows, 500)
    fragment = M.fragments(rgba)[0]
    spec = M.PieceSpec(source='s.png', names=['p'], family='02', footprint=(columns, rows))
    installed = M.install_standing(spec, 'p', fragment, TILE)
    assert installed.scale == pytest.approx(1 / k, rel=0.02)
    # Le sommet nord, dans l'image installée : décalé de la boîte de l'art, réduit, plus la marge.
    x0, y0 = fragment.box[:2]
    expected = ((north[0] - x0) / k + M.MARGE, (north[1] - y0) / k + M.MARGE)
    assert installed.entry['anchor'][0] == pytest.approx(expected[0], abs=2)
    assert installed.entry['anchor'][1] == pytest.approx(expected[1], abs=2)
    assert installed.entry['class'] == ('tall' if (columns, rows) == (1, 1) else 'wide')


def test_une_corniche_qui_deborde_ne_trompe_pas_la_mesure():
    """L'angle rentrant du Colisée : sa corniche déborde plus à droite que son socle."""
    k = 1.6
    rgba, north, _, _ = prism(k, 2, 2, 500)
    image = Image.fromarray(rgba, 'RGBA')
    # Une corniche en haut, qui dépasse de 40 px à gauche et à droite.
    box = np.nonzero(rgba[..., 3])
    top, left, right = box[0].min(), box[1].min(), box[1].max()
    ImageDraw.Draw(image).rectangle((left - 40, top, right + 40, top + 30), fill=(150, 140, 130, 253))
    fragment = M.fragments(np.asarray(image).copy())[0]
    spec = M.PieceSpec(source='s.png', names=['p'], family='02', footprint=(2, 2))
    installed = M.install_standing(spec, 'p', fragment, TILE)
    assert installed.scale == pytest.approx(1 / k, rel=0.02)


def test_un_mur_sans_epaisseur_se_colle_au_bord_nord_si_on_le_demande():
    k = 1.6
    rgba, north, _, _ = prism(k, 3, 0.05, 500)
    fragment = M.fragments(rgba)[0]
    centred = M.install_standing(M.PieceSpec(source='s.png', names=['p'], family='02', footprint=(3, 1)),
                                 'p', fragment, TILE)
    flush = M.install_standing(M.PieceSpec(source='s.png', names=['p'], family='02', footprint=(3, 1),
                                           align='north'), 'p', fragment, TILE)
    x0, y0 = fragment.box[:2]
    # Collé au nord : l'ancre est le sommet nord du mur lui-même.
    assert flush.entry['anchor'][0] == pytest.approx((north[0] - x0) / k + M.MARGE, abs=2)
    assert flush.entry['anchor'][1] == pytest.approx((north[1] - y0) / k + M.MARGE, abs=2)
    # Centré : le mur est au milieu de sa rangée, l'emprise commence une demi-case plus au nord-est.
    assert centred.entry['anchor'][0] - flush.entry['anchor'][0] == pytest.approx(0.475 * HALF_W, abs=2)
    assert flush.entry['anchor'][1] - centred.entry['anchor'][1] == pytest.approx(0.475 * HALF_H, abs=2)


def test_une_source_qu_il_faudrait_agrandir_est_refusee():
    rgba, _, _, _ = prism(0.5, 2, 1, 200)
    fragment = M.fragments(rgba)[0]
    spec = M.PieceSpec(source='s.png', names=['p'], family='02', footprint=(2, 1))
    with pytest.raises(M.DescriptorError, match='agrandir'):
        M.install_standing(spec, 'p', fragment, TILE)


def write_descriptor(folder, pieces, target='Regions/r/ville/zone/Scene'):
    path = folder / 'install.json'
    path.write_text(json.dumps({'version': 1, 'target': target, 'pieces': pieces}), encoding='utf-8')
    return path


@pytest.mark.parametrize('piece, message', [
    ({'source': 'a.png', 'name': 'x', 'family': '11'}, 'famille'),
    ({'source': 'a.png', 'name': 'Mur', 'family': '02'}, 'nom'),
    ({'source': 'a.png', 'name': 'x', 'family': '02', 'foorprint': [1, 1]}, 'inconnu'),
    ({'source': 'a.png', 'name': 'x', 'family': '01', 'footprint': [2, 1]}, 'un sol'),
    ({'source': 'a.png', 'name': 'x', 'family': '02', 'scale': 1.5}, 'agrandi'),
    ({'source': 'a.png', 'name': 'x', 'family': '02', 'tactical': 'mou'}, 'tactique'),
])
def test_un_descripteur_fautif_est_refuse_et_nomme(tmp_path, piece, message):
    with pytest.raises(M.DescriptorError, match=message):
        M.read_descriptor(write_descriptor(tmp_path, [piece]))


@pytest.fixture
def zone(tmp_path, monkeypatch):
    """Un dépôt d'assets et un dossier de sources temporaires, avec une zone vide."""
    assets = tmp_path / 'Assets'
    scene = assets / 'Regions' / 'r' / 'ville' / 'zone' / 'Scene'
    scene.mkdir(parents=True)
    (assets / 'Regions' / 'r' / 'region.json').write_text(json.dumps({'tile': list(TILE)}), encoding='utf-8')
    (scene / 'manifest.json').write_text(json.dumps({
        'version': 1, 'disposition': 'zone', 'comment': 'Vide : la zone (LOT-1).',
        'tile': list(TILE), 'textures': {}}), encoding='utf-8')
    sources = tmp_path / 'Tools' / 'AssetsHD' / 'Zone'
    sources.mkdir(parents=True)
    monkeypatch.setattr(M, 'ASSETS', assets)
    monkeypatch.setattr(M, 'SOURCES', tmp_path / 'Tools' / 'AssetsHD')
    return assets, scene, sources


def test_de_la_source_au_manifeste_puis_au_controle_de_la_ci(zone):
    assets, scene, sources = zone
    sheet = Image.new('RGBA', (1300, 420), (0, 0, 0, 0))
    draw = ImageDraw.Draw(sheet)
    diamond(draw, 320, 210, 600, 340)
    diamond(draw, 960, 210, 600, 340)
    sheet.save(sources / 'sols.png')
    Image.fromarray(prism(1.6, 3, 1, 500)[0], 'RGBA').save(sources / 'mur.png')
    descriptor = write_descriptor(sources, [
        {'source': 'sols.png', 'family': '01', 'tactical': 'open', 'sheet': ['floor-sand-01', 'floor-sand-02']},
        {'source': 'mur.png', 'name': 'wall-arcade-u', 'family': '02', 'footprint': [3, 1]},
    ])

    assert M.main([str(descriptor)]) == 0
    manifest = json.loads((scene / 'manifest.json').read_text(encoding='utf-8'))
    assert 'comment' not in manifest
    assert set(manifest['textures']) == {'scene/zone/floor-sand-01', 'scene/zone/floor-sand-02',
                                         'scene/zone/wall-arcade-u'}
    assert manifest['textures']['scene/zone/floor-sand-01']['tactical'] == 'open'
    assert manifest['textures']['scene/zone/wall-arcade-u']['source']['file'] == 'Zone/mur.png'
    assert (scene / 'wall-arcade-u.png').is_file()

    # Ce que la commande écrit, le contrôle de la CI l'accepte.
    report = check_hd_assets.check(root=assets, maps_text='')
    assert report.errors == []
    assert [w[0] for w in report.weights] == ['Regions/r/ville/zone']

    # À jour, puis plus à jour dès qu'une image est retouchée à la main.
    assert M.main([str(descriptor), '--check']) == 0
    Image.new('RGBA', (4, 4)).save(scene / 'floor-sand-02.png')
    assert M.main([str(descriptor), '--check']) == 1


def test_une_planche_dont_le_compte_ne_tombe_pas_juste_est_refusee(zone):
    _, _, sources = zone
    sheet = Image.new('RGBA', (700, 420), (0, 0, 0, 0))
    diamond(ImageDraw.Draw(sheet), 320, 210, 600, 340)
    sheet.save(sources / 'sols.png')
    descriptor = M.read_descriptor(write_descriptor(sources, [
        {'source': 'sols.png', 'family': '01', 'sheet': ['floor-sand-01', 'floor-sand-02']}]))
    with pytest.raises(M.DescriptorError, match='1 morceau'):
        M.build(descriptor)


# --- Les figurines (LOT-112) ---------------------------------------------------------------------

PITCH = 300          # le pas des images dans la bande du générateur, en px de source
GROUND_SRC = 450     # la ligne de sol de la bande du générateur
BODY_H = 340         # hauteur d'une figurine debout dans la source : échelle 170 / 340 = 0,5


def figure_strip(count, lift=None, shift=None, pitch=PITCH, width=90, spear=0):
    """Une bande synthétique : `count` figurines (corps en gélule, tête, deux pieds) à pas constant.

    `lift` {rang: px} lève une image (le haut d'une foulée), `shift` {rang: px} la décale dans sa
    case (une fente) ; `spear` ajoute une lance horizontale de cette longueur de chaque côté.
    """
    lift, shift = lift or {}, shift or {}
    image = Image.new('RGBA', (pitch * count + 40, 520), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    for i in range(count):
        cx = 20 + pitch * i + pitch / 2 + shift.get(i, 0)
        bottom = GROUND_SRC - lift.get(i, 0)
        top = bottom - BODY_H
        colour = (60 + 30 * i, 120, 200 - 20 * i, 253)
        draw.rounded_rectangle((cx - width / 2, top + 60, cx + width / 2, bottom - 30), radius=30, fill=colour)
        draw.ellipse((cx - 35, top, cx + 35, top + 70), fill=colour)
        draw.ellipse((cx - width / 2, bottom - 40, cx - 5, bottom), fill=colour)   # pied gauche
        draw.ellipse((cx + 5, bottom - 40, cx + width / 2, bottom), fill=colour)   # pied droit
        if spear:
            draw.rectangle((cx - spear, top + 150, cx + spear, top + 160), fill=colour)
    return image


@pytest.fixture
def characters(tmp_path, monkeypatch):
    """Un dossier Characters/ vide, comme la table rase les a laissés, et son dossier de sources."""
    assets = tmp_path / 'Assets'
    target = assets / 'Common' / 'Characters'
    target.mkdir(parents=True)
    (target / 'manifest.json').write_text(json.dumps({
        'version': 1, 'tile': list(TILE), 'comment': 'Vide : héros (LOT-102).',
        'frame': [192, 256], 'wideFrame': [384, 256],
        'animations': ['idle', 'walk', 'attack', 'hit', 'death', 'cast'], 'npcs': []}), encoding='utf-8')
    sources = tmp_path / 'Tools' / 'AssetsHD' / 'Heros'
    sources.mkdir(parents=True)
    monkeypatch.setattr(M, 'ASSETS', assets)
    monkeypatch.setattr(M, 'SOURCES', tmp_path / 'Tools' / 'AssetsHD')
    return target, sources


def figure_descriptor(folder, figures, target='Common/Characters'):
    path = folder / 'install.json'
    path.write_text(json.dumps({'version': 1, 'target': target, 'figures': figures}), encoding='utf-8')
    return path


def lowest_row(cell):
    return int(np.nonzero((cell[..., 3] >= M.OPAQUE).any(axis=1))[0].max())


def box_centre(cell):
    xs = np.nonzero((cell[..., 3] >= M.OPAQUE).any(axis=0))[0]
    return (xs.min() + xs.max() + 1) / 2


def test_une_bande_devient_des_cellules_posees_sur_le_sol(characters):
    target, sources = characters
    figure_strip(6, lift={2: 20}, shift={4: 40}).save(sources / 'walk-se.png')
    Image.new('RGBA', (700, 900), (140, 60, 50, 255)).save(sources / 'portrait.png')
    descriptor = figure_descriptor(sources, [{
        'name': 'Heroes/brawler', 'portrait': 'portrait.png',
        'strips': [{'source': 'walk-se.png', 'clip': 'walk', 'facing': 'se', 'frames': 6,
                    'frameDuration': 0.12}]}])

    assert M.main([str(descriptor)]) == 0
    folder = target / 'Heroes' / 'brawler'
    strip = np.asarray(Image.open(folder / 'walk-se.png'))
    assert strip.shape == (256, 6 * 192, 4)
    cells = [strip[:, i * 192:(i + 1) * 192] for i in range(6)]

    # L'image de repos : pied sur le sol, 170 px de haut, au milieu de sa cellule.
    assert lowest_row(cells[0]) == 252
    rows = np.nonzero((cells[0][..., 3] >= M.OPAQUE).any(axis=1))[0]
    assert abs((rows.max() + 1 - rows.min()) - 170) <= 2
    assert abs(box_centre(cells[0]) - 96) <= 1
    # Le mouvement du générateur est gardé, à l'échelle : levée de 20 px, fente de 40 px.
    assert abs(lowest_row(cells[2]) - (252 - 10)) <= 1
    assert abs(box_centre(cells[4]) - (96 + 20)) <= 1
    for i in (1, 3, 5):
        assert lowest_row(cells[i]) == 252
        assert abs(box_centre(cells[i]) - 96) <= 1
    # La marge de chaque cellule reste transparente.
    for cell in cells:
        assert cell[:, :M.MARGE_CELLULE, 3].max() == 0
        assert cell[:, -M.MARGE_CELLULE:, 3].max() == 0
    # Un alpha continu : le bord garde sa pente.
    assert len(np.unique(strip[..., 3])) > 10

    anim = json.loads((folder / 'walk-se.anim.json').read_text(encoding='utf-8'))
    assert anim == {'version': 1, 'frameWidth': 192, 'frameHeight': 256,
                    'clips': {'walk': {'frames': [0, 1, 2, 3, 4, 5], 'frameDuration': 0.12, 'loop': True}}}

    assert Image.open(folder / 'portrait.png').size == (512, 512)
    token = np.asarray(Image.open(folder / 'token.png'))
    assert token.shape == (128, 128, 4)
    assert token[0, 0, 3] == token[0, -1, 3] == token[-1, 0, 3] == token[-1, -1, 3] == 0
    assert token[64, 64, 3] == 255

    manifest = json.loads((target / 'manifest.json').read_text(encoding='utf-8'))
    assert manifest['npcs'] == ['Heroes/brawler']
    assert 'comment' not in manifest
    assert manifest['frame'] == [192, 256]
    assert set(manifest['sources']) == {'Heroes/brawler/walk-se.png', 'Heroes/brawler/portrait.png',
                                        'Heroes/brawler/token.png'}
    assert manifest['sources']['Heroes/brawler/walk-se.png']['file'] == 'Heros/walk-se.png'

    # À jour, puis plus à jour dès qu'une bande est retouchée à la main.
    assert M.main([str(descriptor), '--check']) == 0
    Image.new('RGBA', (4, 4)).save(folder / 'walk-se.png')
    assert M.main([str(descriptor), '--check']) == 1


def test_des_images_qui_se_touchent_se_partagent_en_parts_egales(characters):
    _, sources = characters
    figure_strip(4, pitch=100, width=110).save(sources / 'idle.png')
    descriptor = M.read_descriptor(figure_descriptor(sources, [{
        'name': 'guard', 'strips': [{'source': 'idle.png', 'clip': 'idle', 'frames': 4}]}]))
    (figure,) = M.build_figures(descriptor)
    (strip,) = figure.strips
    assert strip.split == 'parts égales'
    assert strip.image.shape == (256, 4 * 192, 4)
    assert all(frame.bottom == 252 for frame in strip.frames)


def test_une_attaque_prend_la_cellule_large(characters):
    _, sources = characters
    figure_strip(3, spear=150).save(sources / 'attack.png')
    descriptor = M.read_descriptor(figure_descriptor(sources, [{
        'name': 'guard', 'strips': [{'source': 'attack.png', 'clip': 'attack', 'facing': 'ne', 'frames': 3,
                                     'wide': True, 'loop': False}]}]))
    (strip,) = M.build_figures(descriptor)[0].strips
    assert strip.image.shape == (256, 3 * 384, 4)
    assert strip.anim['frameWidth'] == 384
    assert strip.anim['clips']['attack']['loop'] is False


def test_une_figurine_trop_large_pour_sa_cellule_est_refusee(characters):
    _, sources = characters
    figure_strip(3, spear=200, pitch=500).save(sources / 'walk.png')
    descriptor = M.read_descriptor(figure_descriptor(sources, [{
        'name': 'guard', 'strips': [{'source': 'walk.png', 'clip': 'walk', 'frames': 3}]}]))
    with pytest.raises(M.DescriptorError, match='ne tient pas'):
        M.build_figures(descriptor)


def test_une_bande_qu_il_faudrait_agrandir_est_refusee(characters):
    _, sources = characters
    figure_strip(2).resize((250, 200)).save(sources / 'walk.png')
    descriptor = M.read_descriptor(figure_descriptor(sources, [{
        'name': 'guard', 'strips': [{'source': 'walk.png', 'clip': 'walk', 'frames': 2}]}]))
    with pytest.raises(M.DescriptorError, match='agrandir'):
        M.build_figures(descriptor)


def test_la_mesure_n_ecrit_rien_et_donne_les_appuis(characters, capsys):
    target, sources = characters
    figure_strip(2).save(sources / 'walk.png')
    descriptor = figure_descriptor(sources, [{
        'name': 'guard', 'strips': [{'source': 'walk.png', 'clip': 'walk', 'frames': 2}]}])
    assert M.main([str(descriptor), '--measure']) == 0
    assert 'appuis' in capsys.readouterr().out
    assert not (target / 'guard').exists()


@pytest.mark.parametrize('figure, message', [
    ({'name': 'Guard', 'strips': [{'source': 'a.png', 'clip': 'walk', 'frames': 6}]}, 'nom'),
    ({'name': 'guard', 'strips': []}, 'strips'),
    ({'name': 'guard', 'strips': [{'source': 'a.png', 'clip': 'walk', 'frames': 0}]}, 'frames'),
    ({'name': 'guard', 'strips': [{'source': 'a.png', 'clip': 'walk', 'frames': 6, 'facing': 's'}]}, 'facing'),
    ({'name': 'guard', 'strips': [{'source': 'a.png', 'clip': 'walk', 'frames': 6, 'standingFrame': 6}]},
     'standingFrame'),
    ({'name': 'guard', 'strips': [{'source': 'a.png', 'clip': 'walk', 'frames': 6},
                                  {'source': 'b.png', 'clip': 'walk', 'frames': 8}]}, 'deux fois'),
    ({'name': 'guard', 'strips': [{'source': 'a.png', 'clip': 'walk', 'frames': 6, 'fps': 8}]}, 'inconnu'),
    ({'name': 'guard', 'token': 't.png', 'strips': [{'source': 'a.png', 'clip': 'walk', 'frames': 6}]},
     'sans portrait'),
])
def test_un_descripteur_de_figurine_fautif_est_refuse_et_nomme(tmp_path, figure, message):
    with pytest.raises(M.DescriptorError, match=message):
        M.read_descriptor(figure_descriptor(tmp_path, [figure]))


def test_des_pieces_dans_un_dossier_characters_sont_refusees(tmp_path):
    path = tmp_path / 'install.json'
    path.write_text(json.dumps({'version': 1, 'target': 'Common/Characters', 'pieces': []}), encoding='utf-8')
    with pytest.raises(M.DescriptorError, match='figures'):
        M.read_descriptor(path)
