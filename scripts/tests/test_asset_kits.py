# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Les kits d'assets hors de Git (LOT-108) : archive déterministe, verrou, installation.

Éprouvé sur une arborescence temporaire, sans réseau : le téléchargement est remplacé par le cache
du poste, déjà rempli, ou par une doublure qui compte ses appels.
"""
import io
import os
import time
import zipfile

import pytest

import asset_kits as K
import fetch_assets as F
import publish_asset_kit as P


@pytest.fixture
def tree(tmp_path, monkeypatch):
    assets = tmp_path / 'Assets'
    monkeypatch.setattr(K, 'ASSETS', assets)
    monkeypatch.setattr(K, 'LOCK', assets / 'kits.lock.json')
    monkeypatch.setattr(K, 'WITNESSES', assets / '.kits')
    monkeypatch.setenv('JADG_ASSETS_CACHE', str(tmp_path / 'cache'))
    zone = assets / 'Regions' / 'r' / 'ville' / 'zone'
    for name, data in {'Scene/floors/a.png': b'A', 'Scene/b.png': b'B', 'Scene/manifest.json': b'{}',
                       'donjon/Scene/c.png': b'C'}.items():
        (zone / name).parent.mkdir(parents=True, exist_ok=True)
        (zone / name).write_bytes(data)
    return assets


def lock_kit(path, assets, number=1):
    """Verrouille un kit tel qu'il est sur le disque, et met son archive dans le cache du poste."""
    paths = [k.path for k in K.read_lock()] + [path]
    files = K.kit_files(path, paths)
    data = K.build_archive(path, files)
    kit = K.Kit(id=f'{path}@{number}', path=path, release=K.release_tag(path),
                asset=f'{K.slug(path)}-{number}.zip', sha256=K.sha256_bytes(data), bytes=len(data), files=len(files))
    cache = K.cache_dir()
    cache.mkdir(parents=True, exist_ok=True)
    (cache / f'{kit.sha256}.zip').write_bytes(data)
    K.write_lock([k for k in K.read_lock() if k.path != path] + [kit])
    return kit


ZONE = 'Regions/r/ville/zone'


def test_un_kit_exclut_les_sous_kits_et_les_manifestes(tree):
    assert K.kit_files(ZONE, [ZONE]) == ['Scene/b.png', 'Scene/floors/a.png', 'donjon/Scene/c.png']
    assert K.kit_files(ZONE, [ZONE, ZONE + '/donjon']) == ['Scene/b.png', 'Scene/floors/a.png']


def test_l_archive_est_deterministe(tree):
    files = K.kit_files(ZONE, [ZONE])
    first = K.build_archive(ZONE, files)
    later = time.time() + 3600
    for name in files:
        os.utime(tree / ZONE / name, (later, later))
    assert K.build_archive(ZONE, files) == first
    (tree / ZONE / 'Scene/b.png').write_bytes(b'B2')
    assert K.build_archive(ZONE, files) != first


def test_une_archive_qui_sort_du_kit_est_refusee():
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, 'w') as archive:
        archive.writestr('../evasion.png', b'x')
    with pytest.raises(K.KitError):
        K.archive_members(buffer.getvalue())


def test_une_release_par_region():
    assert K.release_tag('Regions/central-empire/capital/arenarea') == 'assets-central-empire'
    assert K.release_tag('UI') == 'assets-ui'
    assert K.release_tag('Maps') == 'assets-maps'
    with pytest.raises(K.KitError):
        K.release_tag('Fonts')


def test_un_kit_en_place_ne_se_telecharge_pas(tree, monkeypatch):
    lock_kit(ZONE, tree)
    monkeypatch.setattr(F, 'download', lambda *a: pytest.fail('téléchargement inutile'))
    assert F.main([]) == 0
    assert F.main(['--check']) == 0
    assert K.read_witness(ZONE)['sha256'] == K.read_lock()[0].sha256


def test_un_clone_neuf_retrouve_l_arbre_a_l_identique(tree):
    kit = lock_kit(ZONE, tree)
    for name in K.kit_files(ZONE, [ZONE]):
        (tree / ZONE / name).unlink()
    assert F.main(['--check']) == 1
    assert F.main([]) == 0
    assert (tree / ZONE / 'Scene/floors/a.png').read_bytes() == b'A'
    assert F.main(['--check']) == 0
    assert K.read_witness(ZONE)['id'] == kit.id


def test_une_image_modifiee_a_la_main_arrete_l_installation(tree, capsys):
    lock_kit(ZONE, tree)
    (tree / ZONE / 'Scene/b.png').write_bytes(b'retouche locale')
    assert F.main([]) == 1
    assert 'Scene/b.png' in capsys.readouterr().err
    assert (tree / ZONE / 'Scene/b.png').read_bytes() == b'retouche locale'
    assert F.main(['--force']) == 0
    assert (tree / ZONE / 'Scene/b.png').read_bytes() == b'B'


def test_une_image_ajoutee_a_la_main_arrete_l_installation(tree):
    lock_kit(ZONE, tree)
    F.main([])
    (tree / ZONE / 'Scene/nouvelle.png').write_bytes(b'N')
    assert F.main([]) == 1
    assert F.main(['--force']) == 0
    assert not (tree / ZONE / 'Scene/nouvelle.png').exists()


def test_une_nouvelle_version_remplace_l_ancienne_intacte(tree):
    lock_kit(ZONE, tree)
    F.main([])
    (tree / ZONE / 'Scene/b.png').write_bytes(b'B-v2')
    (tree / ZONE / 'Scene/floors/a.png').unlink()
    lock_kit(ZONE, tree, number=2)
    # Le poste revient à la version 1 : la 2 doit s'installer par-dessus sans conflit.
    (tree / ZONE / 'Scene/b.png').write_bytes(b'B')
    (tree / ZONE / 'Scene/floors/a.png').write_bytes(b'A')
    assert F.main([]) == 0
    assert (tree / ZONE / 'Scene/b.png').read_bytes() == b'B-v2'
    assert not (tree / ZONE / 'Scene/floors/a.png').exists()


def test_publier_a_blanc_ne_touche_pas_au_verrou(tree, monkeypatch, capsys):
    monkeypatch.setattr(P, 'check_tree', lambda path: None)
    assert P.main(['--dry-run', ZONE]) == 0
    assert 'non publié' in capsys.readouterr().out
    assert K.read_lock() == []


def test_republier_un_kit_inchange_ne_fait_rien(tree, monkeypatch, capsys):
    monkeypatch.setattr(P, 'check_tree', lambda path: None)
    monkeypatch.setattr(P, 'ensure_release', lambda tag: pytest.fail('aucune publication attendue'))
    lock_kit(ZONE, tree)
    assert P.main([ZONE]) == 0
    assert 'à jour' in capsys.readouterr().out


def test_une_archive_publiee_ne_se_remplace_pas(tree, monkeypatch):
    monkeypatch.setattr(P, 'check_tree', lambda path: None)
    monkeypatch.setattr(P, 'ensure_release', lambda tag: [f'{K.slug(ZONE)}-1.zip'])
    monkeypatch.setattr(P, 'published_digest', lambda tag, asset: '0' * 64)
    assert P.main([ZONE]) == 1
    assert K.read_lock() == []
