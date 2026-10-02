# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Le contrôle des assets HD installés (LOT-104) : citations, bornes du standard, poids par zone.

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


def test_une_zone_lourde_passe(assets):
    # Pas de budget de poids par zone (décision de l'auteur, 24 septembre 2026) : une pièce de
    # 50 Mio échoue par sa taille de fichier, jamais une zone par la somme de ses pièces.
    root, _, _ = assets
    report = C.check(root=root, maps_text='')
    assert report.errors == []
    assert not hasattr(C, 'ZONE_BUDGET')
    assert report.weights


def test_une_sous_zone_se_pese_a_part(assets):
    root, _, _ = assets
    donjon = root / 'Regions' / 'r' / 'ville' / 'zone' / 'donjon' / 'Scene'
    donjon.mkdir(parents=True)
    write(donjon / 'manifest.json', {'version': 1, 'tile': [256, 159], 'textures': {}})
    report = C.check(root=root, maps_text='')
    assert report.errors == []
    assert [(p, level) for p, level, _ in report.weights] == [
        ('Regions/r/ville/zone', 'zone'), ('Regions/r/ville/zone/donjon', 'sous-zone')]


# --- Les personnages (LOT-1006) : un modèle, sa fiche, son squelette ; plus aucune bande -----------
FIXTURE = (C.ROOT / 'Source' / 'Test' / 'Fixtures' / 'Characters' / 'Assets' / 'Common' / 'Characters')
SKELETON = {'version': 1, 'silhouette': 'humanoid',
            'bones': [{'name': 'Root', 'parent': ''}, {'name': 'pelvis', 'parent': 'Root'}],
            'clips': [{'name': 'idle', 'duration': 1.0, 'loop': True},
                      {'name': 'attack', 'duration': 0.9, 'loop': False, 'key': 0.4}]}


def heros(root, skeleton=SKELETON, model=None):
    """Un héros en modèle, rangé par classe sous `Common/Characters/Heroes/brawler` : sa fiche, son
    portrait, son jeton, le squelette commun — et son `.glb` si `model` en donne les octets (le kit
    installé ; sans lui, le dépôt nu de la CI)."""
    characters = root / 'Common' / 'Characters'
    folder = characters / 'Heroes' / 'brawler'
    folder.mkdir(parents=True)
    write(characters / 'manifest.json', {
        'version': 1, 'npcs': ['Heroes/brawler'],
        'models': {'Heroes/brawler': {'model': 'Heroes/brawler/brawler.glb', 'skeleton': 'humanoid'}}})
    write(folder / 'character.json', {'version': 1, 'model': 'brawler.glb', 'skeleton': 'humanoid'})
    png(folder / 'portrait.png', 512, 512)
    png(folder / 'token.png', 128, 128)
    (characters / 'Skeletons' / 'humanoid').mkdir(parents=True)
    write(characters / 'Skeletons' / 'humanoid' / 'skeleton.json', skeleton)
    if model is not None:
        (folder / 'brawler.glb').write_bytes(model)
    return folder


def test_un_personnage_en_modele_passe_sans_son_glb(assets):
    """Le dépôt nu : la fiche, le manifeste et le squelette suffisent, le `.glb` est dans le kit."""
    root, _, _ = assets
    heros(root)
    assert errors(root) == []


def test_le_glb_installe_passe_les_controles_de_l_export(assets):
    """Le kit installé : le modèle d'essai du moteur, lié aux 53 os, contre son squelette."""
    root, _, _ = assets
    skeleton = json.loads((FIXTURE / 'Skeletons' / 'humanoid' / 'skeleton.json').read_text(encoding='utf-8'))
    model = (FIXTURE / 'Mannequins' / 'humanoid' / 'humanoid.glb').read_bytes()
    folder = heros(root, skeleton=skeleton, model=model)
    assert errors(root) == []
    # Contre un squelette qui n'est pas le sien, le modèle est refusé, et c'est dit.
    write(folder.parent.parent / 'Skeletons' / 'humanoid' / 'skeleton.json', SKELETON)
    assert any('brawler.glb' in e for e in errors(root))


def test_un_glb_illisible_ou_non_declare_echoue(assets):
    root, _, _ = assets
    folder = heros(root, model=b'pas un glb')
    assert any('brawler.glb' in e and 'illisible' in e for e in errors(root))
    (folder / 'brawler.glb').unlink()
    (folder / 'autre.glb').write_bytes(b'x')
    assert any('autre.glb' in e and 'ne déclare pas' in e for e in errors(root))


@pytest.mark.parametrize('sheet,message', [
    (None, 'sans fiche'),
    ({'version': 1, 'model': 'autre.glb', 'skeleton': 'humanoid'}, "n'est pas déclaré"),
    ({'version': 1, 'model': '../brawler.glb', 'skeleton': 'humanoid'}, 'nomme un .glb du dossier'),
    ({'version': 1, 'model': 'brawler.glb', 'skeleton': 'quadruped'}, 'le manifeste déclare'),
    ({'version': 2, 'model': 'brawler.glb', 'skeleton': 'humanoid'}, 'version'),
])
def test_une_fiche_fautive_echoue(assets, sheet, message):
    root, _, _ = assets
    folder = heros(root)
    if sheet is None:
        (folder / 'character.json').unlink()
    else:
        write(folder / 'character.json', sheet)
    assert any(message in e for e in errors(root)), errors(root)


def test_un_squelette_que_rien_ne_declare_echoue(assets):
    root, _, _ = assets
    folder = heros(root)
    characters = folder.parent.parent
    manifest = json.loads((characters / 'manifest.json').read_text(encoding='utf-8'))
    manifest['models']['Heroes/brawler']['skeleton'] = 'flying'
    write(characters / 'manifest.json', manifest)
    write(folder / 'character.json', {'version': 1, 'model': 'brawler.glb', 'skeleton': 'flying'})
    assert any('squelette `flying` sans' in e for e in errors(root))


@pytest.mark.parametrize('change,message', [
    (lambda s: s['bones'].reverse(), 'pas déclaré avant lui'),
    (lambda s: s['bones'].append({'name': 'Root', 'parent': ''}), 'déclaré deux fois'),
    (lambda s: s['clips'][0].update(duration=0), 'durée positive'),
    (lambda s: s['clips'][1].update(key=1.5), 'sort du clip'),
    (lambda s: s['clips'][0].update(loop='oui'), '`loop` est un booléen'),
    (lambda s: s.update(silhouette='quadruped'), 'le dossier dit'),
    (lambda s: s.update(bones=[]), 'liste non vide'),
])
def test_un_squelette_mal_forme_echoue(assets, change, message):
    root, _, _ = assets
    skeleton = json.loads(json.dumps(SKELETON))
    change(skeleton)
    heros(root, skeleton=skeleton)
    assert any(message in e for e in errors(root)), errors(root)


@pytest.mark.parametrize('name', ['idle-se.png', 'walk.png', 'attack-nw.png'])
def test_une_bande_de_figurine_est_une_erreur(assets, name):
    """LOT-1006 : plus une seule bande sous un dossier `Characters/`, ni son `.anim.json`."""
    root, _, _ = assets
    folder = heros(root)
    png(folder / name, 8 * 192, 256)
    assert any(name in e and 'bande de figurine' in e for e in errors(root))
    (folder / name).unlink()
    write(folder / (name[:-4] + '.anim.json'), {'version': 1})
    assert any('.anim.json' in e and 'bande de figurine' in e for e in errors(root))


def test_un_manifeste_qui_decrit_encore_des_bandes_echoue(assets):
    root, _, _ = assets
    folder = heros(root)
    characters = folder.parent.parent
    manifest = json.loads((characters / 'manifest.json').read_text(encoding='utf-8'))
    write(characters / 'manifest.json', {**manifest, 'animations': ['idle', 'walk'], 'ground': 252})
    found = errors(root)
    assert any('`animations`' in e for e in found) and any('`ground`' in e for e in found)


def test_un_modele_lie_absent_des_pnj_echoue(assets):
    """Un modèle lié à un squelette a sa fiche et son nom dans `npcs` ; un modèle sans squelette (le
    mannequin d'une silhouette qui n'en a pas encore) attend, sans fiche."""
    root, _, _ = assets
    folder = heros(root)
    characters = folder.parent.parent
    manifest = json.loads((characters / 'manifest.json').read_text(encoding='utf-8'))
    manifest['models']['Mannequins/quadruped'] = {'model': 'Mannequins/quadruped/quadruped.glb',
                                                  'silhouette': 'quadruped'}
    write(characters / 'manifest.json', manifest)
    assert errors(root) == []
    manifest['models']['Mannequins/humanoid'] = {'model': 'Mannequins/humanoid/humanoid.glb',
                                                 'skeleton': 'humanoid'}
    write(characters / 'manifest.json', manifest)
    assert any('Mannequins/humanoid' in e and 'absent de `npcs`' in e for e in errors(root))


def test_un_portrait_d_attente_cite_son_visage_et_rien_d_autre(assets):
    """LOT-145 : un personnage qui a son portrait avant son modèle. `portraits` cite le portrait et
    le jeton ; une bande posée là est une erreur ; un portrait manquant, une fiche, ou un nom dans
    les deux listes, échouent. Un héros n'attend plus (LOT-1009) : `Heroes/…` y est refusé."""
    root, _, _ = assets
    folder = heros(root)
    characters = folder.parent.parent
    lion = characters / 'lion'
    lion.mkdir()
    png(lion / 'portrait.png', 512, 512)
    png(lion / 'token.png', 128, 128)
    manifest = json.loads((characters / 'manifest.json').read_text(encoding='utf-8'))
    manifest['portraits'] = ['lion']
    write(characters / 'manifest.json', manifest)
    assert errors(root) == []

    png(lion / 'idle-se.png', 8 * 192, 256)
    assert any('lion/idle-se.png' in e for e in errors(root))
    (lion / 'idle-se.png').unlink()

    (lion / 'portrait.png').unlink()
    assert any('sans `portrait.png`' in e for e in errors(root))
    png(lion / 'portrait.png', 512, 512)

    write(lion / 'character.json', {'version': 1, 'model': 'lion.glb', 'skeleton': 'quadruped'})
    assert any("n'a pas de fiche" in e for e in errors(root))
    (lion / 'character.json').unlink()

    write(characters / 'manifest.json', {**manifest, 'portraits': ['lion', 'Heroes/brawler']})
    assert any('à la fois' in e for e in errors(root))

    mage = characters / 'Heroes' / 'mage'
    mage.mkdir()
    png(mage / 'portrait.png', 512, 512)
    png(mage / 'token.png', 128, 128)
    write(characters / 'manifest.json', {**manifest, 'portraits': ['lion', 'Heroes/mage']})
    assert any('héros sans modèle' in e for e in errors(root))


def test_un_dossier_scene_sans_manifeste_echoue(assets):
    root, _, _ = assets
    (root / 'Regions' / 'r' / 'ville' / 'autre' / 'Scene').mkdir(parents=True)
    assert any('sans manifest.json' in e for e in errors(root))


def test_le_depot_est_conforme():
    """Le dépôt lui-même : les pièces installées par la chaîne, et le poids de chaque zone."""
    report = C.check()
    assert report.errors == []
    assert any(level == 'sous-zone' for _, level, _ in report.weights)
