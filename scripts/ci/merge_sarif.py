#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Fusionne des fichiers SARIF en un seul, restreint au dépôt, pour Code scanning.

Les analyses statiques de la nuit (refonte de la CI, phase 3) produisent du SARIF natif, mais pas
sous une forme que Code scanning accepte telle quelle :

- **MSVC /analyze** écrit un fichier par unité de compilation, soit quelques centaines de *runs* du
  même outil, que Code scanning refuse dans une même catégorie ;
- un en-tête inclus par cent fichiers y est signalé cent fois ;
- les chemins sont des URI absolues du runner (``file:///D:/a/...``), et une partie vise la STL,
  Qt ou ``_deps/`` : rien où afficher une annotation.

Ce script regroupe tous les *runs* par outil, ne garde que les résultats situés dans le dépôt (hors
``External/`` et dossiers de build), réécrit leurs chemins relativement à la racine, fusionne les
doublons, et peut ajouter un tableau par règle au résumé du job.

Le script s'auto-teste avant de fusionner : une lecture cassée des URI rendrait un SARIF vide,
indiscernable d'un code sans défaut.

Usage :
  python scripts/ci/merge_sarif.py analyze-sarif/ -o msvc-analyze.sarif --title "MSVC /analyze"
  python scripts/ci/merge_sarif.py cppcheck.sarif -o cppcheck-merged.sarif --summary resume.md
  python scripts/ci/merge_sarif.py --auto-test
"""
import argparse
import json
import os
import re
import sys
import tempfile
from collections import Counter
from urllib.parse import unquote, urlparse

# Dossiers du dépôt qui ne sont pas du code du projet.
EXCLUDED_PREFIXES = ('External/', 'build/', 'build-', '_deps/')


def input_files(paths):
    """Les fichiers .sarif désignés : fichiers donnés tels quels, dossiers parcourus."""
    found = []
    for path in paths:
        if os.path.isdir(path):
            for directory, _, names in os.walk(path):
                found.extend(os.path.join(directory, n) for n in names if n.endswith('.sarif'))
        else:
            found.append(path)
    return sorted(found)


def relative_uri(uri, root):
    """URI ou chemin -> chemin POSIX relatif au dépôt, ou None s'il en sort ou vise un exclu."""
    if uri.lower().startswith('file:'):
        path = unquote(urlparse(uri).path)
        # `file:///D:/a/x` donne `/D:/a/x` : le lecteur ne doit pas garder sa barre de tête.
        if re.match(r'^/[A-Za-z]:/', path):
            path = path[1:]
    else:
        path = unquote(uri)
    path = path.replace('\\', '/')
    root = root.replace('\\', '/').rstrip('/') + '/'
    if re.match(r'^(?:[A-Za-z]:)?/', path):
        # Windows ne distingue pas la casse, et les outils n'écrivent pas le lecteur pareil.
        if not path.lower().startswith(root.lower()):
            return None
        path = path[len(root):]
    relative = os.path.normpath(path).replace('\\', '/')
    if relative.startswith('../') or relative == '..':
        return None
    if relative.startswith(EXCLUDED_PREFIXES) or '/_deps/' in relative:
        return None
    return relative


def merge(documents, root):
    """Fusionne des documents SARIF déjà chargés. Renvoie (sarif, résultats par outil, hors dépôt)."""
    tools = {}
    outside = 0
    for document in documents:
        for run in document.get('runs', []):
            driver = run.get('tool', {}).get('driver', {})
            name = driver.get('name', 'inconnu')
            tool = tools.setdefault(name, {'driver': dict(driver, rules=[]), 'rules': {},
                                           'results': [], 'seen': set()})
            for rule in driver.get('rules', []) or []:
                if rule.get('id') and rule['id'] not in tool['rules']:
                    tool['rules'][rule['id']] = rule
            base_ids = run.get('originalUriBaseIds', {})
            for result in run.get('results', []) or []:
                kept = []
                for location in result.get('locations', []) or []:
                    artifact = location.get('physicalLocation', {}).get('artifactLocation', {})
                    uri = artifact.get('uri', '')
                    base = base_ids.get(artifact.get('uriBaseId', ''), {}).get('uri', '')
                    relative = relative_uri(base + uri if base else uri, root)
                    if relative is None:
                        continue
                    location = json.loads(json.dumps(location))
                    location['physicalLocation']['artifactLocation'] = {'uri': relative}
                    kept.append(location)
                if not kept:
                    outside += 1
                    continue
                region = kept[0]['physicalLocation'].get('region', {})
                key = (result.get('ruleId'), kept[0]['physicalLocation']['artifactLocation']['uri'],
                       region.get('startLine'), region.get('startColumn'),
                       result.get('message', {}).get('text'))
                if key in tool['seen']:
                    continue
                tool['seen'].add(key)
                merged = {k: v for k, v in result.items() if k not in ('ruleIndex', 'locations')}
                merged['locations'] = kept
                tool['results'].append(merged)
                rule_id = result.get('ruleId')
                if rule_id and rule_id not in tool['rules']:
                    tool['rules'][rule_id] = {'id': rule_id}

    runs = []
    counts = {}
    for name in sorted(tools):
        tool = tools[name]
        driver = tool['driver']
        driver['rules'] = [tool['rules'][r] for r in sorted(tool['rules'])]
        runs.append({'tool': {'driver': driver}, 'results': tool['results']})
        counts[name] = tool['results']
    sarif = {
        '$schema': 'https://json.schemastore.org/sarif-2.1.0.json',
        'version': '2.1.0',
        'runs': runs,
    }
    return sarif, counts, outside


def summary(title, counts, outside, inputs):
    lines = ['### %s' % title, '']
    total = sum(len(results) for results in counts.values())
    lines.append('%d fichier(s) SARIF lu(s), **%d résultat(s)** dans le dépôt, %d hors dépôt '
                 'écarté(s).' % (inputs, total, outside))
    if total:
        lines += ['', '| Règle | Résultats |', '|---|---|']
        rules = Counter(r.get('ruleId', '?') for results in counts.values() for r in results)
        for rule, count in rules.most_common(30):
            lines.append('| `%s` | %d |' % (rule, count))
        if len(rules) > 30:
            lines.append('| … %d autre(s) règle(s) | |' % (len(rules) - 30))
    return '\n'.join(lines) + '\n'


def auto_test():
    """Renvoie la liste des échecs de l'auto-test ; vide si la fusion est saine."""
    failures = []
    root = 'D:/a/depot/depot'

    def document(uri, rule='C6011', line=12):
        return {'version': '2.1.0', 'runs': [{
            'tool': {'driver': {'name': 'PREfast', 'rules': [{'id': rule}]}},
            'results': [{'ruleId': rule, 'ruleIndex': 0, 'message': {'text': 'déréférence'},
                         'locations': [{'physicalLocation': {
                             'artifactLocation': {'uri': uri},
                             'region': {'startLine': line, 'startColumn': 3}}}]}]}]}

    documents = [
        document('file:///D:/a/depot/depot/Source/Core/A.cpp'),
        # Même défaut vu depuis une autre unité de compilation, lecteur en minuscule.
        document('file:///d:/a/depot/depot/Source/Core/A.cpp'),
        document('file:///D:/a/depot/depot/Source/Core/Mon%20Fichier.h', rule='C26451', line=4),
        document('file:///C:/Program%20Files/MSVC/include/vector'),
        document('file:///D:/a/depot/depot/build/ninja/_deps/json-src/json.hpp'),
        document('D:\\a\\depot\\depot\\External\\x.h'),
    ]
    # Les documents passent par le disque : c'est la lecture réelle des fichiers qui est éprouvée.
    with tempfile.TemporaryDirectory() as directory:
        for index, content in enumerate(documents):
            with open(os.path.join(directory, 'tu%d.sarif' % index), 'w', encoding='utf-8') as out:
                json.dump(content, out)
        loaded = []
        for path in input_files([directory]):
            with open(path, encoding='utf-8-sig') as handle:
                loaded.append(json.load(handle))
    sarif, counts, outside = merge(loaded, root)
    uris = sorted(r['locations'][0]['physicalLocation']['artifactLocation']['uri']
                  for r in counts.get('PREfast', []))
    expected = ['Source/Core/A.cpp', 'Source/Core/Mon Fichier.h']
    if len(loaded) != len(documents):
        failures.append('auto-test : %d fichier(s) lu(s), %d attendus' % (len(loaded), len(documents)))
    if uris != expected:
        failures.append('auto-test : résultats %r, attendus %r' % (uris, expected))
    if outside != 3:
        failures.append('auto-test : %d résultat(s) hors dépôt, 3 attendus' % outside)
    if len(sarif['runs']) != 1:
        failures.append('auto-test : %d run(s), un seul attendu par outil' % len(sarif['runs']))
    elif [r['id'] for r in sarif['runs'][0]['tool']['driver']['rules']] != ['C26451', 'C6011']:
        failures.append('auto-test : règles fusionnées fausses')
    if any('ruleIndex' in r for r in counts.get('PREfast', [])):
        failures.append('auto-test : ruleIndex conservé alors que les règles sont réordonnées')
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', nargs='*', help='fichiers .sarif ou dossiers qui en contiennent')
    parser.add_argument('-o', '--output', help='fichier SARIF fusionné à écrire')
    parser.add_argument('--root', default=os.getcwd(), help='racine du dépôt (défaut : cwd)')
    parser.add_argument('--title', default='Analyse statique', help='titre du résumé')
    parser.add_argument('--summary', help='fichier Markdown où AJOUTER le résumé')
    parser.add_argument('--auto-test', action='store_true', help="n'exécuter que l'auto-test")
    arguments = parser.parse_args()

    failures = auto_test()
    if failures:
        print('\n'.join(failures))
        return 1
    if arguments.auto_test:
        print('merge_sarif : auto-test vert.')
        return 0
    if not arguments.inputs or not arguments.output:
        parser.error('des entrées et --output sont requis hors --auto-test')

    paths = input_files(arguments.inputs)
    documents = []
    for path in paths:
        # MSVC écrit parfois un BOM : utf-8-sig le tolère.
        with open(path, encoding='utf-8-sig') as handle:
            documents.append(json.load(handle))
    sarif, counts, outside = merge(documents, os.path.abspath(arguments.root))
    with open(arguments.output, 'w', encoding='utf-8') as handle:
        json.dump(sarif, handle, ensure_ascii=False, indent=1)

    text = summary(arguments.title, counts, outside, len(paths))
    print(text)
    if arguments.summary:
        with open(arguments.summary, 'a', encoding='utf-8') as handle:
            handle.write(text + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
