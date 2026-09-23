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
  python scripts/docs/build_quality_site.py --site SITE --coverage coverage.xml \\
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


# La charte du site publié, partagée avec la référence de code et le site de planification.
# Cette page n'a plus de feuille à elle : elle emprunte les classes de `theme.css` (`.topbar`,
# `.shell`, `.stats`, `.table-wrap`, `.bar`, `.good`/`.warn`/`.bad`) et recopie les feuilles dans
# `qualite/assets/`, comme le fait la planification dans le sien. `theme.css` importe les deux
# autres : les trois sont nécessaires.
CHARTE = Path(__file__).resolve().parents[2] / 'Site'
CHARTE_FICHIERS = ('theme.css', 'tokens.css', 'topbar.css')


def _pct_class(percent):
    if percent is None:
        return ''
    return 'good' if percent >= 85 else ('warn' if percent >= 70 else 'bad')


def _topbar(summary):
    """La barre commune du site : la marque, les sections de la page, puis les trois parties.

    Le même ordre et les mêmes libellés que sur les deux autres — c'est ce qui les fait lire comme
    un seul site. Une section absente (couverture non mesurée, aucune mesure nocturne) ne laisse
    pas un lien mort : elle ne paraît pas dans la barre, et la page le dit à sa place.
    """
    sections = ['<a class="active" href="index.html">Mesures</a>']
    if summary.get('coverage_html'):
        sections.append('<a href="couverture/index.html">Couverture détaillée</a>')
    if summary.get('benchmarks_page'):
        sections.append('<a href="performances/index.html">Courbes de performance</a>')
    sections.append('<a href="summary.json">summary.json</a>')
    return ('<header class="topbar">'
            '<a class="brand" href="../index.html"><span>Just Another RPG Game</span> Qualité</a>'
            '<nav>%s</nav>'
            '<div class="links"><a href="../index.html">Documentation</a>'
            '<a href="../planning/index.html">Planification</a>'
            '<a class="active" href="index.html">Qualité</a></div>'
            '</header>' % ''.join(sections))


def render(summary):
    esc = html.escape
    coverage = summary.get('coverage')
    benchmarks = summary.get('benchmarks')
    parts = ['<!doctype html><html lang="fr"><head><meta charset="utf-8">',
             '<meta name="viewport" content="width=device-width,initial-scale=1">',
             '<title>Qualité — JustAnotherRpgGame</title>',
             '<link rel="stylesheet" href="assets/theme.css">',
             '</head><body>',
             _topbar(summary),
             '<div class="shell"><main>',
             '<h1>Qualité</h1>',
             '<p class="lede">Ce que la chaîne mesure à chaque intégration sur <code>main</code> : '
             'la couverture des trois suites de tests, et les mesures de performance de la nuit.</p>',
             '<p class="hint">Publié le %s depuis <code>%s</code>%s.</p>' % (
                 esc(summary['generated']), esc(summary.get('commit') or '?'),
                 (' — <a href="%s">run</a>' % esc(summary['run_url'])) if summary.get('run_url') else '')]

    parts.append('<h2>Couverture de code</h2>')
    if coverage and coverage['percent'] is not None:
        parts.append('<div class="stats"><div><b class="%s">%.2f %%</b><span>lignes couvertes, '
                     'trois suites fusionnées</span></div><div><b>%d / %d</b><span>lignes</span>'
                     '</div><div><b>%d</b><span>fichiers mesurés</span></div></div>' % (
                         _pct_class(coverage['percent']), coverage['percent'], coverage['covered'],
                         coverage['valid'], coverage['files']))
        parts.append('<div class="table-wrap"><table><thead><tr><th>Domaine</th>'
                     '<th>Couverture</th><th class="num">%</th><th class="num">Lignes</th></tr>'
                     '</thead><tbody>')
        for area in coverage['areas']:
            percent = area['percent'] or 0.0
            parts.append('<tr><td><code>%s</code></td><td><div class="bar"><span style="width:%.1f%%">'
                         '</span></div></td><td class="num %s">%.1f</td><td class="num">%d / %d</td>'
                         '</tr>' % (esc(area['name']), percent, _pct_class(area['percent']), percent,
                                    area['covered'], area['valid']))
        parts.append('</tbody></table></div>')
    else:
        parts.append('<p class="empty">Couverture non mesurée pour cette publication.</p>')

    parts.append('<h2>Performances</h2>')
    if benchmarks:
        parts.append('<p class="hint">Mesure nocturne du %s (commit <code>%s</code>), %d série(s) '
                     'enregistrée(s). Écart par rapport à la mesure précédente ; une hausse est un '
                     'ralentissement.</p>' % (esc(benchmarks['date']), esc(benchmarks['commit']),
                                               benchmarks['runs']))
        parts.append('<div class="table-wrap"><table><thead><tr><th>Mesure</th>'
                     '<th class="num">Valeur</th><th class="num">Écart</th></tr></thead><tbody>')
        for bench in benchmarks['benches']:
            change = bench['change_percent']
            if change is None:
                cell, css = '—', ''
            else:
                cell = '%+.1f %%' % change
                css = 'bad' if change > 10 else ('good' if change < -10 else '')
            parts.append('<tr><td><code>%s</code></td><td class="num">%s %s</td>'
                         '<td class="num %s">%s</td></tr>'
                         % (esc(bench['name']), '{:,.0f}'.format(bench['value']).replace(',', ' '),
                            esc(bench['unit']), css, cell))
        parts.append('</tbody></table></div>')
    else:
        parts.append('<p class="empty">Aucune mesure nocturne publiée pour l\'instant.</p>')

    parts.append('</main></div>')
    parts.append('<footer>Assemblé par <code>scripts/docs/build_quality_site.py</code> à la publication '
                 'du site. Rien ici ne s\'édite : la source est la chaîne.</footer>')
    parts.append('</body></html>\n')
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
    assets = quality / 'assets'
    assets.mkdir(parents=True, exist_ok=True)
    for name in CHARTE_FICHIERS:
        shutil.copy2(CHARTE / name, assets / name)
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
