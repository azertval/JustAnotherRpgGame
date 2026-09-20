# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Le schéma publié du format de carte v4 (`LOT-EDITOR-12`, règle 1 de la feuille de route de
l'éditeur) : il accepte les cartes livrées et les fixtures v4, et refuse ce que la v4 a retiré.

Le chargeur C++ reste la référence ; le schéma est ce que lisent les outils Python et l'éditeur de
texte. S'ils divergent, c'est ici que ça se voit.
"""
import json

import jsonschema
import pytest

SCHEMA = 'Documentation/Editeur/level.schema.json'


@pytest.fixture
def valider(root):
    schema = json.loads((root / SCHEMA).read_text(encoding='utf-8'))
    jsonschema.Draft202012Validator.check_schema(schema)
    validateur = jsonschema.Draft202012Validator(schema)
    return lambda carte: sorted(e.message for e in validateur.iter_errors(carte))


def cartes_v4(root):
    # Les annexes de l'editeur (`*.editor.json`, LOT-EDITOR-04) vivent a cote des cartes sans en etre.
    livrees = sorted(p for p in (root / 'Source/Elements/Levels').rglob('*.json')
                     if not p.name.endswith('.editor.json'))
    fixtures = [root / 'Source/Test/Fixtures/Levels' / nom
                for nom in ('format-v4.json', 'variante/base.json', 'variante/quartier/nuit.json')]
    return livrees + fixtures


def test_les_cartes_v4_suivent_le_schema(root, valider):
    cartes = cartes_v4(root)
    assert len(cartes) >= 6
    for chemin in cartes:
        erreurs = valider(json.loads(chemin.read_text(encoding='utf-8')))
        assert not erreurs, f'{chemin.relative_to(root)} : {erreurs[:3]}'


def test_une_carte_v3_ne_suit_pas_le_schema(root, valider):
    v3 = json.loads((root / 'Source/Test/Fixtures/Levels/format-v3.json').read_text(encoding='utf-8'))
    assert valider(v3), 'une v3 (texture racine, entités sans id) ne doit pas passer pour une v4'


def test_une_texture_racine_est_refusee(valider):
    carte = {'version': 4, 'name': 'x', 'width': 1, 'height': 1,
             'tiles': [{'x': 0, 'y': 0, 'type': 'entry', 'texture': 'wall-left'}]}
    assert valider(carte)


def test_une_variante_ne_porte_pas_de_cases(valider):
    variante = {'version': 4, 'name': 'x', 'base': 'coliseum', 'tiles': []}
    assert valider(variante)
