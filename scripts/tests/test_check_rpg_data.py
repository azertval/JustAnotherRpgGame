# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Le validateur des catalogues RPG, fixture par fixture.

`check_rpg_data.auto_test` rejoue toutes les fixtures d'un bloc ; ici chaque fixture invalide est un
test, et celle qu'aucun contrôle ne refuse plus porte son nom dans le rapport.
"""
import pytest

import check_rpg_data

INVALID = sorted((check_rpg_data.FIXTURES / 'invalide').rglob('*.json'))


@pytest.fixture(scope='module')
def schemas():
    loaded, violations = check_rpg_data.charger_schemas()
    assert violations == []
    return loaded


def test_fixtures_valides_acceptees(schemas):
    violations, _, lus = check_rpg_data.valider_dossier(schemas, check_rpg_data.FIXTURES / 'valide')
    assert violations == []
    assert lus > 0


def test_des_fixtures_invalides_existent():
    assert INVALID, 'aucune fixture invalide : rien ne prouve que le validateur refuse'


@pytest.mark.parametrize('chemin', INVALID,
                         ids=[p.relative_to(check_rpg_data.FIXTURES).as_posix() for p in INVALID])
def test_fixture_invalide_refusee(schemas, chemin):
    violations, _, _ = check_rpg_data.valider_dossier(schemas, chemin.parent)
    assert [v for v in violations if chemin.name in v], '%s acceptée' % chemin.name
