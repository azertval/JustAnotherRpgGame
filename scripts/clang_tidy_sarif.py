#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Convertit la sortie texte de clang-tidy en SARIF, pour l'afficher dans la Pull Request.

Le job ``clang-tidy`` de ``ci.yml`` ne bloque que sur ``bugprone-*`` ; les autres familles
(``cppcoreguidelines-*``, ``modernize-*``, ``performance-*``, ``readability-*``) n'étaient visibles
que dans un log de plusieurs milliers de lignes, c'est-à-dire nulle part. En SARIF, chaque
diagnostic devient une annotation sur la ligne de l'onglet *Files changed*, et l'historique se lit
dans *Security > Code scanning* (refonte de la CI, phase 1). Aucun contrôle n'est ajouté : c'est la
même sortie, rendue lisible.

Un convertisseur maison plutôt que ``clang-tidy-sarif`` (sarif-rs) : il n'existe pas de binaire
Windows publié à une version épinglable sans compiler Rust sur le runner, et le format d'entrée
tient en une expression régulière.

Ne sont retenus que les diagnostics sur des fichiers **du dépôt** : un avertissement dans un
en-tête Qt ou dans ``External/`` n'a pas de ligne où s'afficher, et Code scanning le rejetterait.
Les doublons (un en-tête inclus par plusieurs fichiers analysés) sont fusionnés.

Le script s'auto-teste avant de convertir : un journal où aucune ligne ne correspond donnerait un
SARIF vide, donc une PR sans annotation, indiscernable d'une PR propre.

Usage :
  python scripts/clang_tidy_sarif.py clang-tidy.log -o clang-tidy.sarif --summary resume.md
  python scripts/clang_tidy_sarif.py --auto-test
"""
import argparse
import json
import os
import re
import sys
from collections import Counter

# `D:\a\depot\Source\X.cpp:12:5: warning: message [famille-regle,-warnings-as-errors]`
# Le chemin peut commencer par une lettre de lecteur : son `:` ne doit pas couper le motif.
DIAGNOSTIC = re.compile(
    r'^(?P<path>(?:[A-Za-z]:)?[^:\n]+):(?P<line>\d+):(?P<column>\d+): '
    r'(?P<level>warning|error): (?P<message>.*?) \[(?P<checks>[^\]\s]+)\]\s*$')

# Familles dont le nom contient lui-même un tiret.
TWO_WORD_FAMILIES = ('clang-analyzer', 'clang-diagnostic')


def rule_id(checks):
    """``bugprone-x,-warnings-as-errors`` -> ``bugprone-x`` : la première règle nommée."""
    return next(c for c in checks.split(',') if c and not c.startswith('-'))


def family(rule):
    for prefix in TWO_WORD_FAMILIES:
        if rule.startswith(prefix + '-'):
            return prefix
    return rule.split('-', 1)[0]


def help_uri(rule):
    """Page de documentation LLVM de la règle, ou None (diagnostics du compilateur)."""
    fam = family(rule)
    if fam == 'clang-diagnostic':
        return None
    name = rule[len(fam) + 1:]
    return 'https://clang.llvm.org/extra/clang-tidy/checks/%s/%s.html' % (fam, name)


def relative_to(path, root):
    """Chemin POSIX relatif à la racine du dépôt, ou None s'il en sort ou vise External/."""
    path = path.replace('\\', '/')
    root = root.replace('\\', '/').rstrip('/') + '/'
    if not re.match(r'^(?:[A-Za-z]:)?/', path):
        relative = path
    elif path.lower().startswith(root.lower()):
        # Windows ne distingue pas la casse ; le lecteur est parfois écrit `d:` par clang.
        relative = path[len(root):]
    else:
        return None
    relative = os.path.normpath(relative).replace('\\', '/')
    if relative.startswith('../') or relative.startswith('External/'):
        return None
    return relative


def parse(log, root):
    """Renvoie (diagnostics dédoublonnés, nombre de diagnostics hors dépôt)."""
    seen = {}
    outside = 0
    for line in log.splitlines():
        match = DIAGNOSTIC.match(line)
        if match is None:
            continue
        relative = relative_to(match['path'], root)
        if relative is None:
            outside += 1
            continue
        rule = rule_id(match['checks'])
        key = (relative, int(match['line']), int(match['column']), rule, match['message'])
        # Une règle promue en erreur (WarningsAsErrors) l'est dans tous ses rapports.
        if key not in seen or match['level'] == 'error':
            seen[key] = match['level']
    diagnostics = [
        {'path': k[0], 'line': k[1], 'column': k[2], 'rule': k[3], 'message': k[4], 'level': lvl}
        for k, lvl in sorted(seen.items())]
    return diagnostics, outside


def to_sarif(diagnostics, version):
    rules = sorted({d['rule'] for d in diagnostics})
    descriptors = []
    for rule in rules:
        descriptor = {'id': rule, 'shortDescription': {'text': rule}}
        uri = help_uri(rule)
        if uri:
            descriptor['helpUri'] = uri
        descriptors.append(descriptor)
    driver = {'name': 'clang-tidy', 'informationUri': 'https://clang.llvm.org/extra/clang-tidy/',
              'rules': descriptors}
    if version:
        driver['semanticVersion'] = version
    results = [{
        'ruleId': d['rule'],
        'ruleIndex': rules.index(d['rule']),
        'level': d['level'],
        'message': {'text': d['message']},
        'locations': [{'physicalLocation': {
            'artifactLocation': {'uri': d['path'], 'uriBaseId': '%SRCROOT%'},
            'region': {'startLine': d['line'], 'startColumn': d['column']},
        }}],
    } for d in diagnostics]
    return {
        '$schema': 'https://json.schemastore.org/sarif-2.1.0.json',
        'version': '2.1.0',
        'runs': [{'tool': {'driver': driver}, 'results': results}],
    }


def summary(diagnostics, outside, analysed):
    """Résumé Markdown pour GITHUB_STEP_SUMMARY."""
    lines = ['### clang-tidy', '']
    if analysed is not None:
        lines.append('%d fichier(s) analysé(s).' % analysed)
    if not diagnostics:
        lines.append('Aucun diagnostic sur les fichiers du dépôt.')
    else:
        errors = Counter(family(d['rule']) for d in diagnostics if d['level'] == 'error')
        warnings = Counter(family(d['rule']) for d in diagnostics if d['level'] == 'warning')
        lines += ['', '| Famille | Bloquants | Non bloquants |', '|---|---:|---:|']
        for fam in sorted(set(errors) | set(warnings)):
            lines.append('| `%s` | %d | %d |' % (fam, errors[fam], warnings[fam]))
        lines.append('| **Total** | **%d** | **%d** |'
                     % (sum(errors.values()), sum(warnings.values())))
    if outside:
        lines += ['', '%d diagnostic(s) hors dépôt (Qt, External/) ignoré(s).' % outside]
    return '\n'.join(lines) + '\n'


def auto_test():
    """Renvoie la liste des échecs de l'auto-test ; vide si le convertisseur est sain."""
    root = 'D:\\a\\JADG\\JADG'
    log = '\n'.join([
        r'D:\a\JADG\JADG\Source\Core\Grid.cpp:12:5: warning: use auto [modernize-use-auto]',
        r'd:\a\JADG\JADG\Source\Core\Grid.h:3:1: error: moved [bugprone-use-after-move,-warnings-as-errors]',
        r'D:\a\JADG\JADG\Source\Core\Grid.h:3:1: error: moved [bugprone-use-after-move,-warnings-as-errors]',
        r'D:\a\JADG\JADG\Source\Core\Grid.cpp:14:2: note: declared here',
        r'C:\Qt\6.11.2\include\QtCore\qglobal.h:9:1: warning: macro [cppcoreguidelines-macro-usage]',
        r'D:\a\JADG\JADG\External\json.hpp:1:1: warning: x [readability-magic-numbers]',
        'Source/Core/Path.cpp:7:9: warning: deref [clang-analyzer-core.NullDereference]',
        '42 warnings generated.',
    ])
    diagnostics, outside = parse(log, root)
    failures = []
    expected = [
        ('Source/Core/Grid.cpp', 12, 'modernize-use-auto', 'warning'),
        ('Source/Core/Grid.h', 3, 'bugprone-use-after-move', 'error'),
        ('Source/Core/Path.cpp', 7, 'clang-analyzer-core.NullDereference', 'warning'),
    ]
    got = [(d['path'], d['line'], d['rule'], d['level']) for d in diagnostics]
    if sorted(got) != sorted(expected):
        failures.append('auto-test : diagnostics lus %r, attendus %r' % (got, expected))
    if outside != 2:
        failures.append('auto-test : %d diagnostic(s) hors dépôt, 2 attendus' % outside)
    uri = help_uri('clang-analyzer-core.NullDereference')
    if not uri or not uri.endswith('/clang-analyzer/core.NullDereference.html'):
        failures.append('auto-test : lien de documentation faux (%s)' % uri)
    sarif = to_sarif(diagnostics, None)
    location = sarif['runs'][0]['results'][0]['locations'][0]['physicalLocation']
    if '\\' in location['artifactLocation']['uri']:
        failures.append('auto-test : chemin SARIF non POSIX')
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('log', nargs='?', help='sortie de clang-tidy')
    parser.add_argument('-o', '--output', help='fichier SARIF à écrire')
    parser.add_argument('--root', default=os.getcwd(), help='racine du dépôt (défaut : cwd)')
    parser.add_argument('--summary', help='fichier Markdown où AJOUTER le résumé')
    parser.add_argument('--analysed', type=int, help='nombre de fichiers analysés, pour le résumé')
    parser.add_argument('--clang-tidy-version', help='version de LLVM, reportée dans le SARIF')
    parser.add_argument('--auto-test', action='store_true', help="n'exécuter que l'auto-test")
    arguments = parser.parse_args()

    # Messages accentués : la console Windows par défaut (cp1252) les rendrait illisibles.
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(encoding='utf-8')

    failures = auto_test()
    if failures:
        print('\n'.join(failures), file=sys.stderr)
        return 2
    if arguments.auto_test:
        print('clang_tidy_sarif : auto-test vert.')
        return 0
    if not arguments.log or not arguments.output:
        parser.error('le journal et --output sont requis hors --auto-test')

    with open(arguments.log, encoding='utf-8', errors='replace') as stream:
        diagnostics, outside = parse(stream.read(), arguments.root)
    with open(arguments.output, 'w', encoding='utf-8') as stream:
        json.dump(to_sarif(diagnostics, arguments.clang_tidy_version), stream, indent=1)
    if arguments.summary:
        with open(arguments.summary, 'a', encoding='utf-8') as stream:
            stream.write(summary(diagnostics, outside, arguments.analysed))
    print('clang_tidy_sarif : %d diagnostic(s) écrit(s) dans %s, %d hors dépôt ignoré(s).'
          % (len(diagnostics), arguments.output, outside))
    return 0


if __name__ == '__main__':
    sys.exit(main())
