#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Génère la documentation Doxygen, depuis n'importe quel répertoire courant.

Doxygen résout les chemins de son fichier de configuration (``INPUT``,
``OUTPUT_DIRECTORY``) relativement au **répertoire courant**, pas à
l'emplacement du ``Doxyfile``. Or ``Documentation/Doxyfile`` utilise des chemins
relatifs (``index.md``, ``Guide``, ``../Source``, ``generated``) : lancer
``doxygen Documentation/Doxyfile`` depuis la racine du dépôt échoue avec une
série de ``source '...' is not a readable file or directory``, alors que le même
fichier fonctionne parfaitement depuis ``Documentation/``.

Le piège est d'autant plus facile que les trois autres scripts de vérification
(``lint_exigences.py``, ``generate_cahier_test.py``, ``check_demo_sequence.py``)
se lancent, eux, **depuis la racine**. Ce script rétablit la symétrie : il se
place dans ``Documentation/`` avant d'appeler Doxygen, quel que soit le
répertoire d'où on l'invoque.

Le ``Doxyfile`` est configuré en ``QUIET = YES`` et
``WARN_AS_ERROR = FAIL_ON_WARNINGS`` : une génération réussie n'affiche donc
rien et renvoie 0, un avertissement fait échouer la commande — même garde-fou
qu'en intégration continue.

``PROJECT_NUMBER`` n'est plus écrit dans le ``Doxyfile`` versionné : ce script lit la ``VERSION``
du ``project()`` racine — la source unique de vérité du numéro de version, dont
``core::Engine::version()`` est également dérivé — et l'injecte en la passant à Doxygen sur
l'entrée standard (le ``Doxyfile`` suivi d'une ligne ``PROJECT_NUMBER = ...`` qui le surcharge ;
Doxygen retient la dernière occurrence d'une clé). Avant ce mécanisme, le ``Doxyfile`` portait sa
propre copie du numéro, et les deux ont divergé pendant quatre jalons (Doxyfile bumpé à chaque
release, CMake resté à sa valeur d'amorçage) sans qu'aucune génération ne le signale.

Usage :
  python scripts/build_docs.py
"""
import os
import re
import subprocess
import sys

DOC_DIRECTORY = 'Documentation'
DOXYFILE = 'Doxyfile'
ROOT_CMAKELISTS = 'CMakeLists.txt'

PROJECT_VERSION_RE = re.compile(r'^\s*VERSION\s+(\S+)\s*$', re.MULTILINE)


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


def build(root):
    """Lance Doxygen depuis @p root/Documentation (PROJECT_NUMBER injecte) et retourne son code."""
    documentation = os.path.join(root, DOC_DIRECTORY)
    doxyfile_path = os.path.join(documentation, DOXYFILE)
    if not os.path.isfile(doxyfile_path):
        print('ERREUR : %s introuvable.' % doxyfile_path)
        return 1

    version = read_single(os.path.join(root, ROOT_CMAKELISTS), PROJECT_VERSION_RE, 'VERSION')
    if version is None:
        return 1

    with open(doxyfile_path, encoding='utf-8') as handle:
        config = handle.read()
    config += '\nPROJECT_NUMBER = %s\n' % version

    try:
        completed = subprocess.run(['doxygen', '-'], cwd=documentation,
                                   input=config, text=True)
    except FileNotFoundError:
        print("ERREUR : doxygen introuvable dans le PATH.")
        print('Installez Doxygen (https://www.doxygen.nl) ou ajoutez-le au PATH.')
        return 1

    if completed.returncode != 0:
        print('Documentation : ECHEC (avertissements traites comme des erreurs, '
              'cf. WARN_AS_ERROR).')
        return completed.returncode

    print('Documentation : OK (%s, generee dans %s).'
          % (version, os.path.join(DOC_DIRECTORY, 'generated', 'html')))
    return 0


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return build(root)


if __name__ == '__main__':
    sys.exit(main())
