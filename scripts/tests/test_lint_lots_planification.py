# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Le lint de la feuille de route figée face aux lots de `Planning/` (LOT-101).

Sa règle 12 -- « tout `LOT-NN` cité par une spécification désigne un lot existant » -- ne lisait que
`roadmap.md`. Depuis le `LOT-100`, les lots à venir se numérotent à partir de 100 et vivent dans
`Planning/` : la règle devait apprendre à les y chercher, sans cesser de refuser un numéro qui
n'existe nulle part.
"""
import lint_lots


def test_les_fiches_de_la_planification_sont_reconnues():
    lots = lint_lots.lots_de_la_planification()
    assert 'LOT-100' in lots
    assert 'LOT-101' in lots
    # Aucun lot de l'ancienne page ne s'y glisse : la planification commence à 100.
    assert all(int(lot.removeprefix('LOT-')) >= 100 for lot in lots)


def test_un_numero_inexistant_reste_refuse():
    assert 'LOT-999' not in lint_lots.lots_de_la_planification()
