#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Écrit le résumé d'un job de build dans GITHUB_STEP_SUMMARY.

Tests, couverture, taille des exécutables : la CI produisait déjà ces nombres, mais il fallait
ouvrir un log de 20 000 lignes ou télécharger un artefact pour les lire. Le résumé les affiche en
tête de la page du run (refonte de la CI, phase 1). Rien n'est vérifié ici : le script ne fait
jamais échouer un job, il ne fait que rendre visible ce que les étapes précédentes ont mesuré —
les seuils restent là où ils sont écrits, dans ``ci.yml``.

Chaque source est facultative : un résumé partiel vaut mieux qu'aucun quand le build a échoué
avant les tests. Une source absente est dite absente, pas passée sous silence.

Usage :
  python scripts/ci/ci_summary.py --title "Debug (vs)" --junit test-results/junit-debug.xml \
      --coverage coverage.xml --threshold 85 --file build/vs/bin/Debug/JustAnotherRpgGame.exe
"""
import argparse
import os
import sys
import xml.etree.ElementTree as ElementTree

SLOWEST_SHOWN = 5


def junit_section(path):
    """Compte les tests d'un rapport JUnit de CTest (``ctest --output-junit``)."""
    if not os.path.isfile(path):
        return ['**Tests** : aucun rapport (`%s` absent — le build a-t-il échoué ?).' % path]
    root = ElementTree.parse(path).getroot()
    cases = root.iter('testcase')
    total = failed = skipped = 0
    timings = []
    failures = []
    for case in cases:
        total += 1
        name = case.get('name', '?')
        if case.find('failure') is not None or case.get('status') == 'fail':
            failed += 1
            failures.append(name)
        elif case.find('skipped') is not None or case.get('status') in ('disabled', 'notrun'):
            skipped += 1
        timings.append((float(case.get('time') or 0), name))
    passed = total - failed - skipped
    lines = ['| Tests | Réussis | Échoués | Ignorés | Durée |', '|---:|---:|---:|---:|---:|',
             '| %d | %d | %d | %d | %.0f s |'
             % (total, passed, failed, skipped, sum(t for t, _ in timings))]
    if failures:
        lines += ['', '**Échecs** :'] + ['- `%s`' % name for name in failures[:20]]
        if len(failures) > 20:
            lines.append('- … et %d autres' % (len(failures) - 20))
    slowest = sorted(timings, reverse=True)[:SLOWEST_SHOWN]
    if slowest:
        lines += ['', '<details><summary>Tests les plus lents</summary>', '']
        lines += ['- %.2f s — `%s`' % (seconds, name) for seconds, name in slowest]
        lines += ['', '</details>']
    return lines


def coverage_section(path, threshold):
    if not os.path.isfile(path):
        return ['**Couverture** : aucun rapport (`%s` absent).' % path]
    root = ElementTree.parse(path).getroot()
    percent = float(root.get('line-rate', 0)) * 100
    covered, valid = root.get('lines-covered'), root.get('lines-valid')
    detail = ' (%s / %s lignes)' % (covered, valid) if covered and valid else ''
    verdict = ''
    if threshold is not None:
        verdict = ' — seuil %g %% %s' % (threshold, 'tenu' if percent >= threshold else '**franchi**')
    return ['**Couverture** : %.2f %%%s%s.' % (percent, detail, verdict)]


def file_section(paths):
    lines = []
    for path in paths:
        if os.path.isfile(path):
            lines.append('- `%s` : %.1f Mio' % (path, os.path.getsize(path) / (1024 * 1024)))
        else:
            lines.append('- `%s` : **absent**' % path)
    return ['**Exécutables** :'] + lines if lines else []


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--title', required=True)
    parser.add_argument('--junit', help='rapport JUnit de CTest')
    parser.add_argument('--coverage', help='rapport Cobertura (OpenCppCoverage)')
    parser.add_argument('--threshold', type=float, help='seuil de couverture, pour mémoire')
    parser.add_argument('--file', action='append', default=[], help='fichier dont donner la taille')
    arguments = parser.parse_args()

    sections = [['### ' + arguments.title]]
    if arguments.junit:
        sections.append(junit_section(arguments.junit))
    if arguments.coverage:
        sections.append(coverage_section(arguments.coverage, arguments.threshold))
    if arguments.file:
        sections.append(file_section(arguments.file))
    text = '\n\n'.join('\n'.join(section) for section in sections) + '\n'

    target = os.environ.get('GITHUB_STEP_SUMMARY')
    if target:
        with open(target, 'a', encoding='utf-8') as stream:
            stream.write(text)
    else:
        if hasattr(sys.stdout, 'reconfigure'):
            sys.stdout.reconfigure(encoding='utf-8')
        sys.stdout.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main())
