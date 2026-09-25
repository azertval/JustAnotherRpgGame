#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Vérifie qu'une Pull Request ajoute au moins une ligne à la section ``## [Non publié]``.

``extract_release_notes.py`` échoue quand la section d'une version manque — mais seulement au tag,
des semaines après les PR qui auraient dû la remplir. Ce contrôle déplace l'erreur au moment où
elle se corrige en une ligne : la PR elle-même (refonte de la CI, phase 1).

Toucher ``CHANGELOG.md`` ne suffit pas : une PR qui corrige une coquille dans la section d'une
version publiée n'a rien consigné de ce qu'elle apporte. Il faut une ligne non vide **ajoutée**
entre ``## [Non publié]`` et le titre de version suivant.

Une PR sans effet notable (CI interne, coquille, montée d'action par Dependabot) porte le label
``no-changelog`` ; le workflow ``changelog.yml`` l'exempte avant d'appeler ce script.

Le script s'auto-teste avant de se prononcer : un analyseur de diff cassé qui ne voit aucune ligne
refuserait toutes les PR, et un qui les voit toutes n'en refuserait aucune.

Usage :
  python scripts/ci/check_changelog.py <sha de base> <sha de tête>
  python scripts/ci/check_changelog.py --auto-test
"""
import argparse
import re
import subprocess
import sys

CHANGELOG_PATH = 'CHANGELOG.md'
UNRELEASED = re.compile(r'^## \[Non publié\]\s*$')
HUNK = re.compile(r'^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@')


def added_lines(diff):
    """Numéros (dans la version de tête) des lignes non vides ajoutées par un diff ``-U0``."""
    numbers = []
    current = None
    for line in diff.splitlines():
        hunk = HUNK.match(line)
        if hunk:
            current = int(hunk.group(1))
            continue
        if current is None or line.startswith(('+++', '---')):
            continue
        if line.startswith('+'):
            if line[1:].strip():
                numbers.append(current)
            current += 1
        elif line.startswith(' '):
            current += 1
    return numbers


def unreleased_range(changelog):
    """(première, dernière) ligne du corps de la section, numérotées à partir de 1 ; ou None."""
    lines = changelog.splitlines()
    start = next((i for i, line in enumerate(lines) if UNRELEASED.match(line)), None)
    if start is None:
        return None
    end = next((i for i in range(start + 1, len(lines)) if lines[i].startswith('## ')), len(lines))
    return start + 2, end


def verdict(diff, changelog):
    """Message d'erreur, ou None si la PR consigne son apport."""
    section = unreleased_range(changelog)
    if section is None:
        return 'CHANGELOG.md n\'a plus de section « ## [Non publié] ».'
    first, last = section
    if not any(first <= n <= last for n in added_lines(diff)):
        return ('Aucune ligne ajoutée à la section « ## [Non publié] » de CHANGELOG.md. '
                'Consigner ce que la PR apporte, ou poser le label no-changelog si elle '
                'n\'apporte rien de notable.')
    return None


def auto_test():
    """Éprouve l'analyseur de diff et le verdict sur des cas connus ; retourne les échecs."""
    changelog = '\n'.join([
        '# Changelog', '', '## [Non publié]', '', '- Nouveau.', '', '## [0.0.4] - 2026-09-01',
        '', '- Ancien.'])
    inside = '--- a/CHANGELOG.md\n+++ b/CHANGELOG.md\n@@ -4,0 +5,1 @@\n+- Nouveau.\n'
    outside = '--- a/CHANGELOG.md\n+++ b/CHANGELOG.md\n@@ -8,1 +9,1 @@\n-- Ancin.\n+- Ancien.\n'
    blank = '@@ -3,0 +4,1 @@\n+\n'
    failures = []
    if added_lines('@@ -1,2 +1,3 @@\n a\n+b\n c\n+d\n') != [2, 4]:
        failures.append('auto-test : numérotation des lignes ajoutées fausse')
    if verdict(inside, changelog) is not None:
        failures.append('auto-test : une ligne ajoutée dans la section est refusée')
    if verdict(outside, changelog) is None:
        failures.append('auto-test : une retouche d\'une version publiée est acceptée')
    if verdict(blank, changelog) is None:
        failures.append('auto-test : une ligne vide ajoutée est acceptée')
    if verdict(inside, changelog.replace('Non publié', 'Unreleased')) is None:
        failures.append('auto-test : un CHANGELOG sans section « Non publié » est accepté')
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('base', nargs='?')
    parser.add_argument('head', nargs='?')
    parser.add_argument('--auto-test', action='store_true')
    arguments = parser.parse_args()

    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(encoding='utf-8')

    failures = auto_test()
    if failures:
        print('\n'.join(failures), file=sys.stderr)
        return 2
    if arguments.auto_test:
        print('check_changelog : auto-test vert.')
        return 0
    if not arguments.base or not arguments.head:
        parser.error('les SHA de base et de tête sont requis hors --auto-test')

    def git(*args):
        return subprocess.run(['git', *args], check=True, capture_output=True,
                              encoding='utf-8').stdout

    diff = git('diff', '-U0', '%s...%s' % (arguments.base, arguments.head), '--', CHANGELOG_PATH)
    changelog = git('show', '%s:%s' % (arguments.head, CHANGELOG_PATH))
    message = verdict(diff, changelog)
    if message:
        # Annotation GitHub : le message s'affiche en tête du run et sur le fichier dans la PR.
        print('::error file=%s::%s' % (CHANGELOG_PATH, message))
        return 1
    print('check_changelog : la PR consigne son apport dans « ## [Non publié] ».')
    return 0


if __name__ == '__main__':
    sys.exit(main())
