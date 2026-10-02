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


# --- Les personnages (LOT-1006) : un modèle, sa fiche, son squelette ------------------------------

FIXTURE = (M.ROOT / 'Source' / 'Test' / 'Fixtures' / 'Characters' / 'Assets' / 'Common' / 'Characters')


@pytest.fixture
def characters(tmp_path, monkeypatch):
    """Un dossier Characters/ d'avant le lot — un héros en bandes, un mannequin en bandes — et un
    dossier de sources où la chaîne de liaison a déposé un modèle et la description du squelette."""
    assets = tmp_path / 'Assets'
    target = assets / 'Common' / 'Characters'
    for folder, strips in (('Heroes/brawler', ('idle-se', 'walk-se')), ('Placeholders/humanoid', ('idle',))):
        (target / folder).mkdir(parents=True)
        for strip in strips:
            Image.new('RGBA', (192, 256), (9, 9, 9, 255)).save(target / folder / f'{strip}.png')
            (target / folder / f'{strip}.anim.json').write_text('{"version": 1}', encoding='utf-8')
    Image.new('RGBA', (512, 512), (9, 9, 9, 255)).save(target / 'Heroes' / 'brawler' / 'portrait.png')
    Image.new('RGBA', (128, 128), (9, 9, 9, 255)).save(target / 'Heroes' / 'brawler' / 'token.png')
    (target / 'manifest.json').write_text(json.dumps({
        'version': 1, 'tile': list(TILE), 'frame': [192, 256], 'wideFrame': [384, 256], 'ground': 252,
        'animations': ['idle', 'walk'], 'npcs': ['Heroes/brawler', 'Placeholders/humanoid'],
        'sources': {'Heroes/brawler/idle-se.png': {'file': 'a', 'sha256': '0'},
                    'Heroes/brawler/portrait.png': {'file': 'b', 'sha256': '1'},
                    'Placeholders/humanoid/idle.png': {'file': 'c', 'sha256': '2'}}}), encoding='utf-8')
    sources = tmp_path / 'Tools' / 'Assets3D' / 'Lies'
    (sources / 'brawler').mkdir(parents=True)
    (sources / 'brawler' / 'brawler.glb').write_bytes((FIXTURE / 'Mannequins' / 'humanoid' / 'humanoid.glb')
                                                      .read_bytes())
    (sources / 'skeleton.json').write_bytes((FIXTURE / 'Skeletons' / 'humanoid' / 'skeleton.json').read_bytes())
    monkeypatch.setattr(M, 'ASSETS', assets)
    monkeypatch.setattr(M, 'SOURCES', tmp_path / 'Tools' / 'AssetsHD')
    monkeypatch.setattr(M, 'ATELIER_3D', tmp_path / 'Tools' / 'Assets3D')
    return target, sources


def figure_descriptor(folder, figures, target='Common/Characters', **extra):
    path = folder / 'install.json'
    path.write_text(json.dumps({'version': 1, 'target': target, 'figures': figures, **extra}), encoding='utf-8')
    return path


BRAWLER = {'name': 'Heroes/brawler', 'model': 'brawler/brawler.glb', 'skeleton': 'humanoid'}
SKELETONS = [{'name': 'humanoid', 'source': 'skeleton.json'}]


def test_un_modele_s_installe_avec_sa_fiche_et_emporte_les_bandes(characters):
    """Le lot entier, sur un dossier d'avant : le squelette, le modèle et sa fiche s'installent ; les
    bandes du héros et le mannequin en bandes partent ; le manifeste ne décrit plus de bande ; le
    contrôle de la CI passe sur le résultat."""
    target, sources = characters
    descriptor = figure_descriptor(sources, [BRAWLER], skeletons=SKELETONS, remove=['Placeholders/humanoid'])
    assert M.main([str(descriptor), '--check']) == 1, "rien n'est encore installé"
    assert M.main([str(descriptor)]) == 0

    folder = target / 'Heroes' / 'brawler'
    assert sorted(p.name for p in folder.iterdir()) == ['brawler.glb', 'character.json', 'portrait.png',
                                                        'token.png']
    assert (folder / 'brawler.glb').read_bytes() == (sources / 'brawler' / 'brawler.glb').read_bytes()
    assert json.loads((folder / 'character.json').read_text(encoding='utf-8')) == {
        'version': 1, 'model': 'brawler.glb', 'skeleton': 'humanoid'}
    assert not (target / 'Placeholders').exists(), 'le dossier de rangement vide part aussi'
    skeleton = json.loads((target / 'Skeletons' / 'humanoid' / 'skeleton.json').read_text(encoding='utf-8'))
    assert len(skeleton['bones']) == 53

    manifest = json.loads((target / 'manifest.json').read_text(encoding='utf-8'))
    assert manifest['npcs'] == ['Heroes/brawler']
    for key in ('frame', 'wideFrame', 'ground', 'animations', 'portraits'):
        assert key not in manifest
    assert sorted(manifest['sources']) == ['Heroes/brawler/portrait.png'], 'les sources des bandes partent'
    entry = manifest['models']['Heroes/brawler']
    assert entry['model'] == 'Heroes/brawler/brawler.glb' and entry['skeleton'] == 'humanoid'
    assert entry['triangles'] == 4000
    assert entry['source']['file'] == 'Lies/brawler/brawler.glb'
    assert entry['sha256'] == entry['source']['sha256'], 'le modèle est copié octet pour octet'
    assert manifest['skeletons']['humanoid']['file'] == 'Skeletons/humanoid/skeleton.json'

    assert M.main([str(descriptor), '--check']) == 0
    report = check_hd_assets.check(root=target.parent.parent, maps_text='')
    assert report.errors == []

    # Un modèle retouché depuis : l'installé n'est plus à jour.
    (folder / 'brawler.glb').write_bytes(b'autre')
    assert M.main([str(descriptor), '--check']) == 1


def test_un_portrait_d_attente_garde_son_visage_puis_recoit_son_modele(characters):
    """LOT-145 : un héros sans modèle garde son portrait et son jeton — déjà installés, ou donnés —,
    perd ses bandes, et va dans `portraits` ; son modèle installé ensuite le fait passer dans `npcs`."""
    target, sources = characters
    descriptor = figure_descriptor(sources, [{'name': 'Heroes/brawler'}])
    assert M.main([str(descriptor)]) == 0
    folder = target / 'Heroes' / 'brawler'
    assert sorted(p.name for p in folder.iterdir()) == ['portrait.png', 'token.png']
    manifest = json.loads((target / 'manifest.json').read_text(encoding='utf-8'))
    assert manifest['npcs'] == ['Placeholders/humanoid'], "ce que le descripteur ne nomme pas ne bouge pas"
    assert manifest['portraits'] == ['Heroes/brawler']
    assert M.main([str(descriptor), '--check']) == 0

    Image.new('RGBA', (700, 900), (140, 60, 50, 255)).save(sources / 'portrait.png')
    descriptor = figure_descriptor(sources, [{**BRAWLER, 'portrait': 'portrait.png'}], skeletons=SKELETONS)
    assert M.main([str(descriptor)]) == 0
    assert Image.open(folder / 'portrait.png').size == (512, 512)
    assert Image.open(folder / 'token.png').size == (128, 128)
    manifest = json.loads((target / 'manifest.json').read_text(encoding='utf-8'))
    assert 'Heroes/brawler' in manifest['npcs']
    assert 'portraits' not in manifest


def test_la_mesure_n_ecrit_rien(characters, capsys):
    target, sources = characters
    before = sorted(p.as_posix() for p in target.rglob('*'))
    descriptor = figure_descriptor(sources, [BRAWLER], skeletons=SKELETONS)
    assert M.main([str(descriptor), '--measure']) == 0
    assert sorted(p.as_posix() for p in target.rglob('*')) == before
    out = capsys.readouterr().out
    assert '4000 triangles' in out and '53 os' in out


def test_un_modele_non_lie_ou_sans_squelette_est_refuse(characters, capsys):
    target, sources = characters
    # Un `.glb` sans squelette : le maillage reçu de Meshy, avant la liaison.
    static = (M.ROOT / 'Source' / 'Test' / 'Fixtures' / 'Meshes' / 'Assets' / 'Scene' / 'ilot' / 'wall.glb')
    (sources / 'brawler' / 'brawler.glb').write_bytes(static.read_bytes())
    assert M.main([str(figure_descriptor(sources, [BRAWLER], skeletons=SKELETONS))]) == 1
    assert "n'est pas lié" in capsys.readouterr().err
    (sources / 'brawler' / 'brawler.glb').write_bytes(b'pas un glb')
    assert M.main([str(figure_descriptor(sources, [BRAWLER], skeletons=SKELETONS))]) == 1
    assert 'illisible' in capsys.readouterr().err
    (sources / 'brawler' / 'brawler.glb').write_bytes((FIXTURE / 'Mannequins' / 'humanoid' / 'humanoid.glb')
                                                      .read_bytes())
    assert M.main([str(figure_descriptor(sources, [BRAWLER]))]) == 1
    assert 'ni au descripteur' in capsys.readouterr().err
    assert M.main([str(figure_descriptor(sources, [{'name': 'Heroes/mage'}]))]) == 1
    assert 'ni modèle, ni portrait' in capsys.readouterr().err
    assert (target / 'Heroes' / 'brawler' / 'idle-se.png').is_file(), "un refus n'a rien retiré"


@pytest.mark.parametrize('figure,message', [
    ({'name': 'Garde'}, 'nom'),
    ({'name': 'guard', 'strips': []}, 'plus en bandes'),
    ({'name': 'guard', 'model': 'a.glb'}, 'vont ensemble'),
    ({'name': 'guard', 'skeleton': 'humanoid'}, 'vont ensemble'),
    ({'name': 'guard', 'model': 'a.png', 'skeleton': 'humanoid'}, '.glb'),
    ({'name': 'guard', 'model': 'a.glb', 'skeleton': 'Humanoid'}, 'squelette'),
    ({'name': 'guard', 'fps': 8}, 'inconnu'),
    ({'name': 'guard', 'token': 't.png'}, 'sans portrait'),
])
def test_un_descripteur_de_personnage_fautif_est_refuse_et_nomme(tmp_path, figure, message):
    with pytest.raises(M.DescriptorError, match=message):
        M.read_descriptor(figure_descriptor(tmp_path, [figure]))


@pytest.mark.parametrize('extra,message', [
    ({'skeletons': [{'name': 'humanoid'}]}, 'attendus'),
    ({'skeletons': [{'name': 'Humanoid', 'source': 's.json'}]}, 'nom'),
    ({'remove': ['../dehors']}, 'remove'),
    ({}, 'aucun personnage'),
])
def test_un_descripteur_de_squelette_ou_de_retrait_fautif_est_refuse(tmp_path, extra, message):
    with pytest.raises(M.DescriptorError, match=message):
        M.read_descriptor(figure_descriptor(tmp_path, [], **extra))


def test_des_pieces_dans_un_dossier_characters_sont_refusees(tmp_path):
    path = tmp_path / 'install.json'
    path.write_text(json.dumps({'version': 1, 'target': 'Common/Characters', 'pieces': []}), encoding='utf-8')
    with pytest.raises(M.DescriptorError, match='figures'):
        M.read_descriptor(path)


def test_un_essai_se_mesure_mais_ne_s_installe_pas(characters, capsys):
    target, sources = characters
    descriptor = figure_descriptor(sources, [BRAWLER], skeletons=SKELETONS, previewOnly=True)
    assert M.main([str(descriptor), '--measure']) == 0
    assert M.main([str(descriptor)]) == 1
    assert 'ne s\'installe pas' in capsys.readouterr().err
    assert not (target / 'Heroes' / 'brawler' / 'brawler.glb').exists()


def test_un_kit_se_range_en_sous_dossiers_et_le_controle_le_suit(zone):
    """LOT-129 : une règle `folders` range chaque pièce d'après son nom ; réinstallée ailleurs, une
    pièce ne laisse pas son ancien fichier ; la CI accepte l'arborescence."""
    assets, scene, sources = zone
    Image.fromarray(prism(1.6, 1, 1, 300)[0], 'RGBA').save(sources / 'toit.png')
    Image.fromarray(prism(1.6, 1, 1, 200)[0], 'RGBA').save(sources / 'tonneau.png')
    pieces = [{'source': 'toit.png', 'name': 'roof-l-d3-ne-c0r0', 'family': '09'},
              {'source': 'tonneau.png', 'name': 'prop-barrel', 'family': '08'}]
    assert M.main([str(write_descriptor(sources, pieces))]) == 0
    assert (scene / 'roof-l-d3-ne-c0r0.png').is_file()

    path = sources / 'install.json'
    data = json.loads(path.read_text(encoding='utf-8'))
    data['folders'] = [{'match': r'^roof-l-d(\d)', 'folder': r'roofs/l/d\1'}]
    path.write_text(json.dumps(data), encoding='utf-8')
    assert M.main([str(path)]) == 0

    manifest = json.loads((scene / 'manifest.json').read_text(encoding='utf-8'))
    assert manifest['textures']['scene/zone/roof-l-d3-ne-c0r0']['file'] == 'roofs/l/d3/roof-l-d3-ne-c0r0.png'
    assert manifest['textures']['scene/zone/prop-barrel']['file'] == 'prop-barrel.png'
    assert (scene / 'roofs' / 'l' / 'd3' / 'roof-l-d3-ne-c0r0.png').is_file()
    assert not (scene / 'roof-l-d3-ne-c0r0.png').exists()
    report = check_hd_assets.check(root=assets, maps_text='')
    assert report.errors == []


@pytest.mark.parametrize('folder', ['../dehors', '/racine', ''])
def test_un_dossier_hors_de_la_cible_est_refuse(tmp_path, folder):
    path = write_descriptor(tmp_path, [])
    data = json.loads(path.read_text(encoding='utf-8'))
    data['folders'] = [{'match': '^roof-', 'folder': folder}]
    path.write_text(json.dumps(data), encoding='utf-8')
    with pytest.raises(M.DescriptorError, match='dossier'):
        M.read_descriptor(path)
