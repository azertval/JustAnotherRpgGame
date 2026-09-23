#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Extrait du CHANGELOG la section d'une version, pour servir de notes de release.

``release.yml`` publiait jusqu'ici avec ``--generate-notes``, qui produit une liste brute de
messages de commit : lisible par un développeur qui connaît déjà le projet, illisible pour le
non-développeur à qui la release versionnée est justement destinée. Le CHANGELOG contient déjà,
pour chaque jalon, un chapeau rédigé et le détail par lot — autant le servir tel quel plutôt que
d'entretenir deux récits de la même version.

Le script **échoue** si la section n'existe pas : une release publiée avec des notes vides ne se
corrige pas proprement, mieux vaut interrompre le workflow avant la publication.

Usage :
  python scripts/release/extract_release_notes.py v0.0.5            # -> sortie standard
  python scripts/release/extract_release_notes.py 0.0.5 -o notes.md # -> fichier
"""
import argparse
import os
import re
import sys

CHANGELOG_PATH = 'CHANGELOG.md'


def normalize_version(reference):
    """Retire un éventuel préfixe de tag : ``v0.0.5`` -> ``0.0.5``."""
    return reference[1:] if re.fullmatch(r'v\d+\.\d+\.\d+', reference) else reference


def extract(changelog, version):
    """Renvoie le corps de la section ``## [version]``, ou None si elle est absente.

    La section court jusqu'au prochain titre de niveau 2 (version suivante), exclu.
    """
    start = re.compile(r'^## \[' + re.escape(version) + r'\](?: - .*)?$', re.MULTILINE)
    match = start.search(changelog)
    if match is None:
        return None
    rest = changelog[match.end():]
    following = re.search(r'^## ', rest, re.MULTILINE)
    body = rest if following is None else rest[:following.start()]
    return body.strip('\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('version', help='version ou tag (0.0.5 ou v0.0.5)')
    parser.add_argument('-o', '--output', help='fichier de sortie (défaut : sortie standard)')
    arguments = parser.parse_args()

    # Le CHANGELOG est en UTF-8 et cette sortie le recopie tel quel ; sur Windows, la console
    # par défaut est en cp1252 et un simple « → » suffirait à faire échouer l'écriture. Les autres
    # scripts du projet n'ont pas ce besoin : ils n'impriment que des messages ASCII.
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')

    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    os.chdir(root)

    version = normalize_version(arguments.version)
    with open(CHANGELOG_PATH, encoding='utf-8') as handle:
        changelog = handle.read()

    body = extract(changelog, version)
    if not body:
        print('ERREUR : aucune section "## [%s]" dans %s.' % (version, CHANGELOG_PATH),
              file=sys.stderr)
        print('Ajouter la section de la version avant de poser le tag.', file=sys.stderr)
        return 1

    notes = body + '\n\n---\n\nHistorique complet : [CHANGELOG.md](CHANGELOG.md).\n'
    if arguments.output:
        with open(arguments.output, 'w', encoding='utf-8', newline='\n') as handle:
            handle.write(notes)
        print('Notes de release ecrites dans %s (%d caracteres).'
              % (arguments.output, len(notes)))
    else:
        sys.stdout.write(notes)
    return 0


if __name__ == '__main__':
    sys.exit(main())
