# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Contrôle des catalogues de traduction Qt (scripts/checks/check_translations.py)."""
import pytest

from check_translations import problems, tracked_catalogues

HEAD = '<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n'


def catalogue(*messages, language='en_US'):
    body = ''.join(messages)
    lang = ' language="%s"' % language if language else ''
    return '%s<TS version="2.1"%s><context><name>Ecran</name>%s</context></TS>' % (HEAD, lang, body)


def message(source, translation, kind=None, numerus=False):
    attribute = ' type="%s"' % kind if kind else ''
    if numerus:
        forms = ''.join('<numerusform>%s</numerusform>' % f for f in translation)
        return ('<message numerus="yes"><source>%s</source><translation%s>%s</translation>'
                '</message>' % (source, attribute, forms))
    return ('<message><source>%s</source><translation%s>%s</translation></message>'
            % (source, attribute, translation))


def test_catalogue_complet_accepte():
    assert problems(catalogue(message('Nouvelle partie', 'New game'),
                              message('Niveau %1', 'Level %1'))) == []


@pytest.mark.parametrize('xml, attendu', [
    (catalogue(message('Quitter', '', kind='unfinished')), 'inachevée'),
    (catalogue(message('Quitter', 'Quit', kind='unfinished')), 'inachevée'),
    (catalogue(message('Quitter', '   ')), 'inachevée'),
    (catalogue(message('Précédent', 'Previous', kind='vanished')), 'vanished'),
    (catalogue(message('Précédent', 'Previous', kind='obsolete')), 'obsolete'),
    (catalogue(message('%1 sur %2', '%1 of')), 'marqueurs'),
    (catalogue(message('%1 PV', '%L1 HP')), 'marqueurs'),
    (catalogue(message('Or : ', 'Gold:')), 'espaces de bord'),
    (catalogue(message('Or', 'Gold'), language=None), 'language'),
    (catalogue(), 'aucun message'),
    ('<TS><context>', 'mal formé'),
], ids=['vide-unfinished', 'rempli-unfinished', 'blanc', 'vanished', 'obsolete',
        'marqueur-perdu', 'marqueur-localise', 'espace-final-perdu', 'sans-langue', 'vide',
        'xml-casse'])
def test_defaut_refuse(xml, attendu):
    found = problems(xml)
    assert found and any(attendu in f for f in found), found


def test_espace_francais_avant_deux_points_admis():
    assert problems(catalogue(message(' : croix, le curseur', ': d-pad, the cursor'))) == []


def test_formes_plurielles():
    ok = message('%n objet(s)', ['one item', '%n items'], numerus=True)
    assert problems(catalogue(ok)) == []
    vide = message('%n objet(s)', ['one item', ''], numerus=True)
    assert any('inachevée' in f for f in problems(catalogue(vide)))


def test_catalogues_du_depot(root, monkeypatch):
    monkeypatch.chdir(root)
    paths = tracked_catalogues()
    assert paths, 'aucun .ts suivi'
    for path in paths:
        assert problems((root / path).read_text(encoding='utf-8'), path) == []
