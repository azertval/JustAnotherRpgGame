#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Vérifie que la version de Qt est identique en CMake et dans les workflows CI.

`Source/CMakeLists.txt` déclare `QT_VERSION_MINIMUM` : la version de référence utilisée comme
version minimale de Qt6 (EX-BUILD-010). `.github/workflows/ci.yml`,
`release.yml` et `nightly.yml` installent Qt à `env.QT_VERSION`. Rien ne reliait ces deux écritures avant ce script
— même défaut que celui corrigé pour le numéro de version par `build_docs.py`.

Usage :
  python scripts/ci/check_qt_version_pin.py
"""
import os
import re
import sys

CMAKELISTS = os.path.join('Source', 'CMakeLists.txt')
WORKFLOWS = [os.path.join('.github', 'workflows', 'ci.yml'),
             os.path.join('.github', 'workflows', 'release.yml'),
             # La nuit (phase 3) analyse et teste le code avec le même Qt ; son job `qt-next` essaie la
             # version suivante, calculée à l'exécution, sans écrire de second `QT_VERSION:`.
             os.path.join('.github', 'workflows', 'nightly.yml'),
             # Le site qualité (phase 4) mesure la couverture de main avec le même Qt.
             os.path.join('.github', 'workflows', 'docs.yml')]

CMAKE_VERSION_RE = re.compile(r'^\s*set\(QT_VERSION_MINIMUM\s+"([^"]+)"\)\s*$', re.MULTILINE)
WORKFLOW_VERSION_RE = re.compile(r'^\s*QT_VERSION:\s*[\'"]([^\'"]+)[\'"]\s*$', re.MULTILINE)


def read_single(path, pattern, label):
    """Extrait l'unique capture de @p pattern dans @p path, ou None en signalant pourquoi."""
    try:
        with open(path, encoding='utf-8') as handle:
            matches = pattern.findall(handle.read())
    except OSError as error:
        print('ERREUR : %s illisible (%s).' % (path, error))
        return None
    if len(matches) != 1:
        print('ERREUR : %s attendu exactement une fois dans %s (trouve %d).'
              % (label, path, len(matches)))
        return None
    return matches[0]


def main():
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    os.chdir(root)

    cmake_version = read_single(CMAKELISTS, CMAKE_VERSION_RE, 'QT_VERSION_MINIMUM')
    if cmake_version is None:
        return 1

    ok = True
    for workflow in WORKFLOWS:
        workflow_version = read_single(workflow, WORKFLOW_VERSION_RE, 'QT_VERSION')
        if workflow_version is None:
            ok = False
            continue
        if workflow_version != cmake_version:
            print('ERREUR : version de Qt incoherente.')
            print('  %s : QT_VERSION_MINIMUM %s' % (CMAKELISTS, cmake_version))
            print('  %s      : QT_VERSION %s' % (workflow, workflow_version))
            ok = False

    if ok:
        print('OK : Qt %s epingle de facon identique dans %s.'
              % (cmake_version, ' et '.join(WORKFLOWS)))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
