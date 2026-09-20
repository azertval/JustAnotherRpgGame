# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Réglages communs des tests des scripts (refonte de la chaîne d'outillage, phase 4).

Le dossier `scripts/` est sur le chemin d'import (`pythonpath` de pyproject.toml) : un test importe
un script comme un module. Aucun script n'agit à l'import, tout passe par `main()`.
"""
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture
def root():
    """Racine du dépôt."""
    return ROOT
