# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Les auto-tests écrits dans les scripts, rejoués comme tests nommés.

Chaque script de contrôle s'éprouve lui-même avant de se prononcer (un contrôle vert par vacuité ne
prouve rien, la panne du LOT-78). Ces épreuves restent dans les scripts, qui les jouent à chaque
exécution ; pytest les rend visibles et comptées dans le rapport de tests de la PR, et remplace les
étapes `--auto-test` que le job lint-exigences alignait une par une.

Deux conventions coexistent : `auto_test()` renvoie la liste des échecs, ou lève AssertionError.
"""
import importlib

import pytest

# Auto-tests qui renvoient la liste de leurs échecs.
RETURNING = ['check_glossary', 'check_corpus_manifest', 'check_changelog', 'merge_sarif',
             'clang_tidy_sarif']
# Auto-tests qui lèvent AssertionError.
ASSERTING = ['check_binary_files', 'check_json_files', 'check_commit_message']


@pytest.mark.parametrize('module_name', RETURNING)
def test_auto_test_renvoie_aucun_echec(module_name):
    failures = importlib.import_module(module_name).auto_test()
    assert failures in (None, []), failures


@pytest.mark.parametrize('module_name', ASSERTING)
def test_auto_test_sans_assertion_levee(module_name):
    importlib.import_module(module_name).auto_test()


def test_lecture_de_ci_yml_par_check_py(root):
    import check
    check.auto_test(str(root))
