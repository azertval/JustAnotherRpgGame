#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Assemble le site qualité publié sur gh-pages (refonte de la chaîne d'outillage, phase 4).

La chaîne produit déjà ses nombres — couverture, mesures de performance, documentation — mais
chacun vivait dans un artefact de run qu'il fallait télécharger, ou dans une branche à part. Ce
script les réunit sous `qualite/` du site, à côté de la Doxygen qui reste à la racine (le README
pointe vers elle) :

- `qualite/index.html` : la page d'accueil — couverture par domaine du code, dernières mesures de
  performance et leur écart avec la mesure précédente, liens ;
- `qualite/couverture/` : le rapport HTML d'OpenCppCoverage ;
- `qualite/performances/` : la page de courbes de github-action-benchmark (branche `benchmarks`) ;
- `qualite/summary.json` : les mêmes chiffres, lisibles par un script.

Une source absente (couverture non mesurée, aucune mesure nocturne encore poussée) n'empêche pas la
publication : la page le dit à la place du chiffre.

Usage (docs.yml) :
  python scripts/build_quality_site.py --site SITE --coverage coverage.xml \\
      --coverage-html coverage-html --benchmarks dev/bench --commit SHA --run-url URL
"""
import argparse
import html
import json
import re
import shutil
import sys
import xml.etree.ElementTree as ElementTree
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path, PureWindowsPath

# Le rapport liste un chemin absolu du runner (`a\\Depot\\Depot\\Source\\Core\\…`) ; tout ce qui
# précède `Source` est propre à la machine.
SOURCE_RE = re.compile(r'(?:^|[\\/])Source[\\/](.+)$')
BENCHMARK_DATA_RE = re.compile(r'^\s*window\.BENCHMARK_DATA\s*=\s*', re.MULTILINE)


def source_relative(filename):
    match = SOURCE_RE.search(filename)
    return PureWindowsPath(match.group(1)).as_posix() if match else None


def area_of(relative):
    """Domaine d'un fichier : les deux premiers niveaux sous Source/ (`Core/Combat`, `HMI/Runtime`)."""
    parts = relative.split('/')
    return '/'.join(parts[:2]) if len(parts) > 2 else parts[0]


def read_coverage(text):
    """Couverture par domaine, depuis un rapport Cobertura d'OpenCppCoverage.

    Le rapport fusionné porte un paquet par exécutable de test : un même fichier y figure jusqu'à
    trois fois. Une ligne est couverte si l'une des suites l'exécute.
    """
    root = ElementTree.fromstring(text)
    hits = defaultdict(dict)
    for cls in root.iter('class'):
        relative = source_relative(cls.get('filename', ''))
        if relative is None:
            continue
        lines = hits[relative]
        for line in cls.iter('line'):
            number = int(line.get('number'))
            lines[number] = max(lines.get(number, 0), int(line.get('hits', '0')))

    areas = defaultdict(lambda: [0, 0])
    for relative, lines in hits.items():
        area = areas[area_of(relative)]
        area[0] += sum(1 for h in lines.values() if h > 0)
        area[1] += len(lines)
    covered = sum(a[0] for a in areas.values())
    valid = sum(a[1] for a in areas.values())
    # Le chiffre global est celui d'OpenCppCoverage, que le seuil de ci.yml lit aussi : la page ne doit
    # pas afficher un autre nombre que celui qui fait échouer une PR. Sa fusion compte certaines
    # lignes d'en-tête autrement que la déduplication ci-dessus, qui ne sert qu'au détail par domaine.
    if root.get('lines-valid') and root.get('lines-covered'):
        covered, valid = int(root.get('lines-covered')), int(root.get('lines-valid'))
    return {
        'percent': round(100.0 * covered / valid, 2) if valid else None,
        'covered': covered,
        'valid': valid,
        'files': len(hits),
        'areas': [{'name': name, 'covered': c, 'valid': v,
                   'percent': round(100.0 * c / v, 2) if v else None}
                  for name, (c, v) in sorted(areas.items())],
    }


def read_benchmarks(text):
    """Dernière série de mesures de github-action-benchmark (`data.js`), avec l'écart à la
    précédente pour chaque mesure qui y figurait."""
    payload = json.loads(BENCHMARK_DATA_RE.sub('', text, count=1).rstrip().rstrip(';'))
    suites = payload.get('entries', {})
    if not suites:
        return None
    name, runs = next(iter(sorted(suites.items())))
    if not runs:
        return None
    latest = runs[-1]
    previous = {b['name']: b['value'] for b in runs[-2]['benches']} if len(runs) > 1 else {}
    benches = []
    for bench in latest['benches']:
        before = previous.get(bench['name'])
        change = round(100.0 * (bench['value'] - before) / before, 1) if before else None
        benches.append({'name': bench['name'], 'value': bench['value'], 'unit': bench['unit'],
                        'change_percent': change})
    commit = latest.get('commit', {})
    return {
        'suite': name,
        'runs': len(runs),
        'date': datetime.fromtimestamp(latest['date'] / 1000, timezone.utc).strftime('%Y-%m-%d %H:%M UTC'),
        'commit': commit.get('id', '')[:9],
        'benches': benches,
    }


STYLE = """
:root { --bg:#f6f4ee; --surface:#fff; --ink:#1d1b16; --muted:#5f5a4e; --line:#ddd6c6;
        --accent:#7a1f12; --good:#2f6b3a; --warn:#9a5b00; --bad:#9a2a1a; --bar:#c8a15a;
        color-scheme: light; }
@media (prefers-color-scheme: dark) {
  :root { --bg:#16140f; --surface:#1f1c16; --ink:#ece6d8; --muted:#aaa28f; --line:#3a352a;
          --accent:#e0a27a; --good:#7fc28a; --warn:#e0b25a; --bad:#f0a58c; --bar:#b08a45;
          color-scheme: dark; }
}
* { box-sizing: border-box; }
body { margin:0; background:var(--bg); color:var(--ink);
       font:16px/1.55 "Segoe UI", system-ui, sans-serif; }
main { max-width: 980px; margin: 0 auto; padding: 32px 16px 64px; }
h1 { font-size: 32px; margin: 0 0 4px; }
h2 { font-size: 22px; margin: 40px 0 12px; padding-top: 16px; border-top: 1px solid var(--line); }
p.meta { color: var(--muted); margin: 0 0 24px; }
a { color: var(--accent); }
nav { display: flex; flex-wrap: wrap; gap: 8px 20px; margin: 16px 0; }
.facts { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 1px;
         background: var(--line); border: 1px solid var(--line); }
.facts div { background: var(--surface); padding: 14px 16px; }
.facts .v { font-size: 28px; font-weight: 700; font-variant-numeric: tabular-nums; }
.facts .k { color: var(--muted); font-size: 14px; }
.wrap { overflow-x: auto; border: 1px solid var(--line); background: var(--surface); }
table { border-collapse: collapse; width: 100%; font-variant-numeric: tabular-nums; }
th, td { padding: 8px 12px; border-bottom: 1px solid var(--line); text-align: left; }
th { font-size: 13px; color: var(--muted); text-transform: uppercase; letter-spacing: .05em; }
td.n { text-align: right; white-space: nowrap; }
.bar { height: 8px; background: var(--line); border-radius: 4px; min-width: 120px; }
.bar span { display: block; height: 100%; border-radius: 4px; background: var(--bar); }
.good { color: var(--good); } .warn { color: var(--warn); } .bad { color: var(--bad); }
.empty { color: var(--muted); font-style: italic; }
"""


def _pct_class(percent):
    if percent is None:
        return ''
    return 'good' if percent >= 85 else ('warn' if percent >= 70 else 'bad')


def render(summary):
    esc = html.escape
    coverage = summary.get('coverage')
    benchmarks = summary.get('benchmarks')
    parts = ['<!doctype html><html lang="fr"><head><meta charset="utf-8">',
             '<meta name="viewport" content="width=device-width,initial-scale=1">',
             '<title>Qualité de JustAnotherRpgGame</title><style>%s</style></head><body><main>' % STYLE,
             '<h1>Qualité de JustAnotherRpgGame</h1>',
             '<p class="meta">Publié le %s depuis <code>%s</code>%s.</p>' % (
                 esc(summary['generated']), esc(summary.get('commit') or '?'),
                 (' — <a href="%s">run</a>' % esc(summary['run_url'])) if summary.get('run_url') else ''),
             '<nav><a href="../index.html">Documentation (Doxygen)</a>']
    if summary.get('coverage_html'):
        parts.append('<a href="couverture/index.html">Rapport de couverture détaillé</a>')
    if summary.get('benchmarks_page'):
        parts.append('<a href="performances/index.html">Courbes de performance</a>')
    parts.append('<a href="summary.json">summary.json</a></nav>')

    parts.append('<h2>Couverture de code</h2>')
    if coverage and coverage['percent'] is not None:
        parts.append('<div class="facts"><div><div class="v %s">%.2f %%</div><div class="k">lignes '
                     'couvertes, trois suites fusionnées</div></div><div><div class="v">%d / %d</div>'
                     '<div class="k">lignes</div></div><div><div class="v">%d</div><div class="k">'
                     'fichiers mesurés</div></div></div>' % (
                         _pct_class(coverage['percent']), coverage['percent'], coverage['covered'],
                         coverage['valid'], coverage['files']))
        parts.append('<div class="wrap" style="margin-top:16px"><table><thead><tr><th>Domaine</th>'
                     '<th>Couverture</th><th class="n">%</th><th class="n">Lignes</th></tr></thead><tbody>')
        for area in coverage['areas']:
            percent = area['percent'] or 0.0
            parts.append('<tr><td><code>%s</code></td><td><div class="bar"><span style="width:%.1f%%">'
                         '</span></div></td><td class="n %s">%.1f</td><td class="n">%d / %d</td></tr>' % (
                             esc(area['name']), percent, _pct_class(area['percent']), percent,
                             area['covered'], area['valid']))
        parts.append('</tbody></table></div>')
    else:
        parts.append('<p class="empty">Couverture non mesurée pour cette publication.</p>')

    parts.append('<h2>Performances</h2>')
    if benchmarks:
        parts.append('<p class="meta">Mesure nocturne du %s (commit <code>%s</code>), %d série(s) '
                     'enregistrée(s). Écart par rapport à la mesure précédente ; une hausse est un '
                     'ralentissement.</p>' % (esc(benchmarks['date']), esc(benchmarks['commit']),
                                               benchmarks['runs']))
        parts.append('<div class="wrap"><table><thead><tr><th>Mesure</th><th class="n">Valeur</th>'
                     '<th class="n">Écart</th></tr></thead><tbody>')
        for bench in benchmarks['benches']:
            change = bench['change_percent']
            if change is None:
                cell, css = '—', ''
            else:
                cell = '%+.1f %%' % change
                css = 'bad' if change > 10 else ('good' if change < -10 else '')
            parts.append('<tr><td><code>%s</code></td><td class="n">%s %s</td><td class="n %s">%s</td></tr>'
                         % (esc(bench['name']), '{:,.0f}'.format(bench['value']).replace(',', ' '),
                            esc(bench['unit']), css, cell))
        parts.append('</tbody></table></div>')
    else:
        parts.append('<p class="empty">Aucune mesure nocturne publiée pour l\'instant.</p>')

    parts.append('</main></body></html>\n')
    return ''.join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--site', required=True, type=Path, help='racine du site (gh-pages)')
    parser.add_argument('--coverage', type=Path, help='coverage.xml (Cobertura)')
    parser.add_argument('--coverage-html', type=Path, help='rapport HTML d\'OpenCppCoverage')
    parser.add_argument('--benchmarks', type=Path, help='dossier dev/bench de la branche benchmarks')
    parser.add_argument('--commit', default='')
    parser.add_argument('--run-url', default='')
    arguments = parser.parse_args()

    quality = arguments.site / 'qualite'
    quality.mkdir(parents=True, exist_ok=True)
    summary = {
        'generated': datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M UTC'),
        'commit': arguments.commit[:9],
        'run_url': arguments.run_url,
        'coverage': None,
        'benchmarks': None,
        'coverage_html': False,
        'benchmarks_page': False,
    }

    if arguments.coverage and arguments.coverage.is_file():
        summary['coverage'] = read_coverage(arguments.coverage.read_text(encoding='utf-8'))
    else:
        print('Couverture absente : la page le signale.')
    if arguments.coverage_html and (arguments.coverage_html / 'index.html').is_file():
        shutil.copytree(arguments.coverage_html, quality / 'couverture', dirs_exist_ok=True)
        summary['coverage_html'] = True

    data = arguments.benchmarks / 'data.js' if arguments.benchmarks else None
    if data and data.is_file():
        summary['benchmarks'] = read_benchmarks(data.read_text(encoding='utf-8'))
        shutil.copytree(arguments.benchmarks, quality / 'performances', dirs_exist_ok=True)
        summary['benchmarks_page'] = (quality / 'performances' / 'index.html').is_file()
    else:
        print('Mesures de performance absentes : la page le signale.')

    (quality / 'index.html').write_text(render(summary), encoding='utf-8')
    (quality / 'summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n',
                                          encoding='utf-8')
    coverage = summary['coverage']
    print('Site qualité écrit dans %s (couverture %s, %d mesure(s) de performance).' % (
        quality, '%.2f %%' % coverage['percent'] if coverage else 'absente',
        len(summary['benchmarks']['benches']) if summary['benchmarks'] else 0))
    return 0


if __name__ == '__main__':
    sys.exit(main())
