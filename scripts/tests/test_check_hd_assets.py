# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Le contrôle des assets HD installés (LOT-104) : citations, bornes du standard, budget par zone.

Éprouvé sur des arborescences écrites en temporaire : un contrôle vert sur le seul dépôt, où la
plupart des dossiers sont encore vides, ne prouverait rien (la panne du LOT-78).
"""
import json
import struct
import zlib

import pytest

import check_hd_assets as C


def png(path, width, height, colour=6):
    """Un PNG minimal valide, sans Pillow : signature, IHDR, un IDAT vide, IEND."""
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    path.write_bytes(C.PNG_SIGNATURE + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, colour, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(b'')) + chunk(b'IEND', b''))


@pytest.fixture
def assets(tmp_path):
    root = tmp_path / 'Assets'
    region = root / 'Regions' / 'r'
    region.mkdir(parents=True)
    (region / 'region.json').write_text(json.dumps({'tile': [256, 159]}), encoding='utf-8')
    scene = region / 'ville' / 'zone' / 'Scene'
    scene.mkdir(parents=True)
    png(scene / 'floor-sand-01.png', 256, 159)
    png(scene / 'wall-arcade-u.png', 426, 539)
    manifest = {'version': 1, 'tile': [256, 159], 'textures': {
        'scene/zone/floor-sand-01': {'file': 'floor-sand-01.png', 'class': 'floor', 'footprint': [1, 1],
                                     'size': [256, 159], 'anchor': [128, 0]},
        'scene/zone/wall-arcade-u': {'file': 'wall-arcade-u.png', 'class': 'wide', 'footprint': [3, 1],
                                     'size': [426, 539], 'anchor': [6, 295]},
    }}
    write(scene / 'manifest.json', manifest)
    return root, scene, manifest


def write(path, value):
    path.write_text(json.dumps(value), encoding='utf-8')


def errors(root):
    return C.check(root=root, maps_text='').errors


def test_une_zone_conforme_passe_et_se_pese(assets):
    root, _, _ = assets
    report = C.check(root=root, maps_text='')
    assert report.errors == []
    assert report.weights == [('Regions/r/ville/zone', 'zone', report.weights[0][2])]
    assert report.weights[0][2] > 0
    assert '`Regions/r/ville/zone`' in C.summary(report)


def test_une_image_que_le_manifeste_ne_cite_pas_echoue(assets):
    root, scene, _ = assets
    png(scene / 'orpheline.png', 256, 159)
    assert any('orpheline.png' in e and 'EX-CNT-042' in e for e in errors(root))


def test_un_fichier_cite_absent_echoue(assets):
    root, scene, _ = assets
    (scene / 'wall-arcade-u.png').unlink()
    assert any('absent' in e for e in errors(root))


@pytest.mark.parametrize('change, message', [
    (lambda m: m['textures']['scene/zone/wall-arcade-u'].update(size=[400, 539]), 'image 426 × 539'),
    (lambda m: m['textures']['scene/zone/wall-arcade-u'].update(anchor=[500, 10]), 'ancre'),
    (lambda m: m['textures']['scene/zone/wall-arcade-u'].update(footprint=[0, 1]), 'emprise'),
    (lambda m: m['textures']['scene/zone/wall-arcade-u'].update({'class': 'floor'}), 'losange du lieu'),
    (lambda m: m.update(tile=[68, 42]), 'la région déclare'),
])
def test_une_piece_hors_des_bornes_du_standard_echoue(assets, change, message):
    root, scene, manifest = assets
    change(manifest)
    write(scene / 'manifest.json', manifest)
    assert any(message in e for e in errors(root)), errors(root)


def test_un_png_sans_alpha_echoue(assets):
    root, scene, _ = assets
    png(scene / 'floor-sand-01.png', 256, 159, colour=2)
    assert any('RGBA 8 bits' in e for e in errors(root))


def test_une_zone_au_dela_du_budget_echoue(assets, monkeypatch):
    root, _, _ = assets
    monkeypatch.setattr(C, 'ZONE_BUDGET', 10)
    assert any('budget' in e for e in errors(root))


def test_une_sous_zone_se_pese_a_part(assets):
    root, _, _ = assets
    donjon = root / 'Regions' / 'r' / 'ville' / 'zone' / 'donjon' / 'Scene'
    donjon.mkdir(parents=True)
    write(donjon / 'manifest.json', {'version': 1, 'tile': [256, 159], 'textures': {}})
    report = C.check(root=root, maps_text='')
    assert report.errors == []
    assert [(p, level) for p, level, _ in report.weights] == [
        ('Regions/r/ville/zone', 'zone'), ('Regions/r/ville/zone/donjon', 'sous-zone')]


def test_les_images_d_un_pnj_sont_citees_par_le_manifeste_des_figurines(assets):
    root, _, _ = assets
    characters = root / 'Regions' / 'r' / 'ville' / 'zone' / 'Characters'
    (characters / 'mere').mkdir(parents=True)
    write(characters / 'manifest.json', {'version': 1, 'animations': ['idle'], 'npcs': ['mere']})
    png(characters / 'mere' / 'portrait.png', 512, 512)
    png(characters / 'mere' / 'idle.png', 1200, 272)
    write(characters / 'mere' / 'idle.anim.json', {'version': 1})
    assert errors(root) == []
    png(characters / 'mere' / 'danse.png', 192, 256)
    assert any('danse.png' in e for e in errors(root))


def heros(root, facings=C.FACINGS, clips=('idle', 'walk', 'attack')):
    """Un héros rangé par classe, sous `Common/Characters/Heroes/brawler`, orienté dans @p facings."""
    characters = root / 'Common' / 'Characters'
    folder = characters / 'Heroes' / 'brawler'
    folder.mkdir(parents=True)
    write(characters / 'manifest.json', {'version': 1, 'animations': ['idle', 'walk', 'attack', 'cast'],
                                         'npcs': ['Heroes/brawler']})
    png(folder / 'portrait.png', 512, 512)
    png(folder / 'token.png', 128, 128)
    for clip in clips:
        for facing in facings:
            png(folder / f'{clip}-{facing}.png', 8 * 192, 256)
            write(folder / f'{clip}-{facing}.anim.json', {'version': 1})
    return folder


def test_un_heros_oriente_range_par_classe_passe(assets):
    root, _, _ = assets
    heros(root)
    assert errors(root) == [], "quatre orientations, pas de sort : un Brawler est complet"


def test_une_animation_orientee_a_moitie_echoue(assets):
    root, _, _ = assets
    folder = heros(root)
    (folder / 'attack-nw.png').unlink()
    assert any('`attack` orientée à moitié' in e and 'attack-nw.png' in e for e in errors(root))


def test_une_bande_sans_sa_description_echoue(assets):
    root, _, _ = assets
    folder = heros(root)
    (folder / 'walk-sw.anim.json').unlink()
    assert any('walk-sw.anim.json' in e for e in errors(root))


def test_une_figurine_sans_marche_echoue(assets):
    root, _, _ = assets
    heros(root, clips=('idle',))
    assert any('pas de bande `walk`' in e for e in errors(root))


def test_un_dossier_scene_sans_manifeste_echoue(assets):
    root, _, _ = assets
    (root / 'Regions' / 'r' / 'ville' / 'autre' / 'Scene').mkdir(parents=True)
    assert any('sans manifest.json' in e for e in errors(root))


def test_le_depot_est_conforme():
    """Le dépôt lui-même : les pièces installées par la chaîne, et le poids de chaque zone."""
    report = C.check()
    assert report.errors == []
    assert any(level == 'sous-zone' for _, level, _ in report.weights)
