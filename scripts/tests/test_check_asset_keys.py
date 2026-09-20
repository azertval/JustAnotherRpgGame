# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Les figurines de monstres (LOT-93) : ce que `check_asset_keys.figurines` accepte et refuse.

Chaque test monte un dossier `Monsters/` dans un répertoire temporaire : un manifeste, des bandes
PNG aux dimensions voulues, leurs `.anim.json`. Le cas qui justifie le contrôle est le premier :
une créature sans sort ne livre pas de `cast`, et c'est accepté.
"""
import json
import struct
import zlib

import pytest

import check_asset_keys

TEMPLATES = {'medium': {'frame': [48, 64], 'wideFrame': [96, 64], 'deathWide': True},
             'large': {'frame': [96, 96], 'wideFrame': [192, 96], 'deathWide': False}}
SANS_CAST = ['idle', 'walk', 'hit', 'death', 'attack']
# La bête « lion » et la bête « wolf » du catalogue : c'est tout ce que le contrôle lit des clés attendues.
ATTENDUES = [{'cle': 'beast/lion', 'famille': 'beast'}, {'cle': 'beast/wolf', 'famille': 'beast'}]


def png(chemin, largeur, hauteur):
    """Un PNG valide, transparent, aux dimensions données."""
    def bloc(genre, donnees):
        return struct.pack('>I', len(donnees)) + genre + donnees + struct.pack('>I', zlib.crc32(genre + donnees))
    brut = b''.join(b'\x00' + b'\x00' * (4 * largeur) for _ in range(hauteur))
    chemin.write_bytes(b'\x89PNG\r\n\x1a\n' + bloc(b'IHDR', struct.pack('>IIBBBBB', largeur, hauteur, 8, 6, 0, 0, 0))
                       + bloc(b'IDAT', zlib.compress(brut)) + bloc(b'IEND', b''))


def figurine(racine, slug, template, animations, largeurs=None):
    """Le dossier d'une figurine : ses bandes aux dimensions du gabarit, sauf celles que `largeurs` force."""
    dossier = racine / slug
    dossier.mkdir(parents=True)
    gabarit = TEMPLATES[template]
    for animation in animations:
        largeur, hauteur = check_asset_keys.cellule(gabarit, animation)
        images = check_asset_keys.IMAGES_PAR_ANIMATION[animation]
        png(dossier / (animation + '.png'), (largeurs or {}).get(animation, images * largeur), hauteur)
        (dossier / (animation + '.anim.json')).write_text(json.dumps(
            {'version': 1, 'frameWidth': largeur, 'frameHeight': hauteur,
             'clips': {animation: {'frames': list(range(images)), 'frameDuration': 0.1, 'loop': False}}}))
    png(dossier / 'portrait.png', 8, 8)


def manifeste(racine, monstres):
    (racine / 'manifest.json').write_text(json.dumps(
        {'version': 1, 'animations': SANS_CAST + ['cast'], 'templates': TEMPLATES, 'monsters': monstres}))


@pytest.fixture
def monstres(tmp_path, monkeypatch):
    """Un dossier `Monsters/` vide, et la liste d'erreurs du module remise à zéro."""
    monkeypatch.setattr(check_asset_keys, 'erreurs', [])
    return tmp_path / 'Monsters'


def controler(racine):
    resultat = check_asset_keys.figurines(ATTENDUES, racine)
    return resultat, check_asset_keys.erreurs


def test_une_creature_sans_sort_ne_livre_pas_de_cast(monstres):
    figurine(monstres, 'lion', 'large', SANS_CAST)
    figurine(monstres, 'wolf', 'medium', SANS_CAST)
    manifeste(monstres, [{'slug': 'lion', 'template': 'large', 'creature': 'lion', 'animations': SANS_CAST},
                         {'slug': 'wolf', 'template': 'medium', 'creature': 'wolf', 'animations': SANS_CAST}])
    assert controler(monstres) == ((2, 0), [])


def test_un_humanoide_attend_son_bloc(monstres):
    figurine(monstres, 'ironhand-soldier', 'medium', SANS_CAST)
    manifeste(monstres, [{'slug': 'ironhand-soldier', 'template': 'medium', 'creature': None,
                          'awaiting': 'LOT-46', 'animations': SANS_CAST}])
    assert controler(monstres) == ((0, 1), [])


def test_une_creature_qui_lance_des_sorts_livre_son_cast(monstres):
    figurine(monstres, 'wolf', 'medium', SANS_CAST + ['cast'])
    manifeste(monstres, [{'slug': 'wolf', 'template': 'medium', 'creature': 'wolf',
                          'animations': SANS_CAST + ['cast']}])
    assert controler(monstres)[1] == []


@pytest.mark.parametrize('cas, preparer, attendu', [
    ('cast livre sans etre declare',
     lambda r: (figurine(r, 'wolf', 'medium', SANS_CAST + ['cast']),
                manifeste(r, [{'slug': 'wolf', 'template': 'medium', 'creature': 'wolf', 'animations': SANS_CAST}])),
     "cast.png livre, mais 'cast' n'est pas declaree"),
    ('walk manquante',
     lambda r: (figurine(r, 'wolf', 'medium', ['idle', 'hit', 'death', 'attack']),
                manifeste(r, [{'slug': 'wolf', 'template': 'medium', 'creature': 'wolf',
                               'animations': ['idle', 'hit', 'death', 'attack']}])),
     "l'animation 'walk' manque"),
    ('bande a la mauvaise largeur',
     lambda r: (figurine(r, 'lion', 'large', SANS_CAST, largeurs={'death': 6 * 192}),
                manifeste(r, [{'slug': 'lion', 'template': 'large', 'creature': 'lion', 'animations': SANS_CAST}])),
     'death.png fait (1152, 96), attendu (576, 96)'),
    ('creature absente du catalogue',
     lambda r: (figurine(r, 'dragon', 'large', SANS_CAST),
                manifeste(r, [{'slug': 'dragon', 'template': 'large', 'creature': 'dragon', 'animations': SANS_CAST}])),
     "figurine ORPHELINE, la creature 'dragon'"),
    ('ni creature ni lot attendu',
     lambda r: (figurine(r, 'ironhand-soldier', 'medium', SANS_CAST),
                manifeste(r, [{'slug': 'ironhand-soldier', 'template': 'medium', 'creature': None,
                               'animations': SANS_CAST}])),
     'ni creature du catalogue, ni lot attendu'),
    ('dossier hors du manifeste',
     lambda r: (figurine(r, 'wolf', 'medium', SANS_CAST), manifeste(r, [])),
     'Monsters/wolf : dossier de figurine absent du manifeste'),
])
def test_figurine_refusee(monstres, cas, preparer, attendu):
    preparer(monstres)
    _, erreurs = controler(monstres)
    assert any(attendu in e for e in erreurs), '%s : %s' % (cas, erreurs)


def test_pas_de_dossier_pas_de_figurine(monstres):
    assert controler(monstres) == ((0, 0), [])
