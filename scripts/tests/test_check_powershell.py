# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Lint PowerShell (scripts/check_powershell.py) : ce qui se vérifie sans PowerShell.

L'analyse elle-même est rejouée par le job lint-exigences, qui installe le module.
"""
from check_powershell import annotation, expected_version

FINDING = {'File': 'scripts/build.ps1', 'Line': 12, 'Column': 5, 'Rule': 'PSUseCompatibleSyntax',
           'Severity': 'Warning', 'Message': "L'opérateur '&&' n'existe pas en 5.1\n100 %"}


def test_version_lue_dans_le_bloc_env_de_ci(root):
    version = expected_version((root / '.github' / 'workflows' / 'ci.yml').read_text(encoding='utf-8'))
    assert version and version.count('.') == 2


def test_version_absente():
    assert expected_version('env:\n  AUTRE_VERSION: 1.0.0\n') is None


def test_annotation_hors_ci(monkeypatch):
    monkeypatch.delenv('GITHUB_ACTIONS', raising=False)
    assert annotation(FINDING) == ("scripts/build.ps1:12:5: warning [PSUseCompatibleSyntax] "
                                   "L'opérateur '&&' n'existe pas en 5.1\n100 %")


def test_annotation_en_ci_echappe_le_message(monkeypatch):
    monkeypatch.setenv('GITHUB_ACTIONS', 'true')
    command = annotation(FINDING).splitlines()[-1]
    assert command == ("::warning file=scripts/build.ps1,line=12,col=5,title=PSUseCompatibleSyntax::"
                       "L'opérateur '&&' n'existe pas en 5.1%0A100 %25")
