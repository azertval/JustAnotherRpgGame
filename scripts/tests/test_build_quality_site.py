# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Assemblage du site qualité (scripts/docs/build_quality_site.py)."""
import json
import subprocess
import sys

from build_quality_site import read_benchmarks, read_coverage, render

COVERAGE = """<?xml version="1.0" encoding="utf-8"?>
<coverage line-rate="0.5"><packages>
  <package name="D:\\a\\R\\R\\build\\vs\\bin\\Debug\\UnitTests.exe"><classes>
    <class name="A.cpp" filename="a\\R\\R\\Source\\Core\\Combat\\A.cpp"><lines>
      <line number="1" hits="1"/><line number="2" hits="0"/><line number="3" hits="0"/>
    </lines></class>
    <class name="Main.cpp" filename="a\\R\\R\\Source\\App\\Main.cpp"><lines>
      <line number="1" hits="0"/>
    </lines></class>
    <class name="vector" filename="Program Files\\MSVC\\include\\vector"><lines>
      <line number="1" hits="1"/>
    </lines></class>
  </classes></package>
  <package name="D:\\a\\R\\R\\build\\vs\\bin\\Debug\\IntegrationTests.exe"><classes>
    <class name="A.cpp" filename="a\\R\\R\\Source\\Core\\Combat\\A.cpp"><lines>
      <line number="1" hits="0"/><line number="2" hits="4"/><line number="3" hits="0"/>
    </lines></class>
  </classes></package>
</packages></coverage>
"""

BENCHMARKS = """window.BENCHMARK_DATA = {
  "lastUpdate": 1789500000000,
  "entries": {
    "Benchmark": [
      {"commit": {"id": "aaaaaaaaaaaa"}, "date": 1789400000000,
       "benches": [{"name": "Chemin", "value": 100, "unit": "ns/iter"}]},
      {"commit": {"id": "bbbbbbbbbbbb"}, "date": 1789500000000,
       "benches": [{"name": "Chemin", "value": 125, "unit": "ns/iter"},
                   {"name": "Nouvelle", "value": 10, "unit": "ns/iter"}]}
    ]
  }
}
"""


def test_couverture_fusionne_les_suites_par_ligne():
    coverage = read_coverage(COVERAGE)
    # A.cpp : lignes 1 et 2 couvertes par l'une ou l'autre suite, 3 jamais ; Main.cpp : 0 / 1.
    assert (coverage['covered'], coverage['valid']) == (2, 4)
    assert coverage['percent'] == 50.0
    assert coverage['files'] == 2, 'les en-têtes hors Source/ ne comptent pas'
    assert [(a['name'], a['covered'], a['valid']) for a in coverage['areas']] == [
        ('App', 0, 1), ('Core/Combat', 2, 3)]


def test_mesures_de_performance_et_ecart():
    benchmarks = read_benchmarks(BENCHMARKS)
    assert benchmarks['runs'] == 2
    assert benchmarks['commit'] == 'bbbbbbbbb'
    assert benchmarks['benches'] == [
        {'name': 'Chemin', 'value': 125, 'unit': 'ns/iter', 'change_percent': 25.0},
        {'name': 'Nouvelle', 'value': 10, 'unit': 'ns/iter', 'change_percent': None}]


def test_page_sans_aucune_source():
    page = render({'generated': 'x', 'commit': '', 'run_url': '', 'coverage': None,
                   'benchmarks': None, 'coverage_html': False, 'benchmarks_page': False})
    assert 'Couverture non mesurée' in page
    assert 'Aucune mesure nocturne' in page
    assert 'couverture/index.html' not in page


def test_script_complet(tmp_path, root):
    (tmp_path / 'coverage.xml').write_text(COVERAGE, encoding='utf-8')
    (tmp_path / 'html').mkdir()
    (tmp_path / 'html' / 'index.html').write_text('<html></html>', encoding='utf-8')
    (tmp_path / 'bench').mkdir()
    (tmp_path / 'bench' / 'data.js').write_text(BENCHMARKS, encoding='utf-8')
    (tmp_path / 'bench' / 'index.html').write_text('<html></html>', encoding='utf-8')
    site = tmp_path / 'site'
    subprocess.run([sys.executable, str(root / 'scripts' / 'docs' / 'build_quality_site.py'),
                    '--site', str(site), '--coverage', str(tmp_path / 'coverage.xml'),
                    '--coverage-html', str(tmp_path / 'html'), '--benchmarks', str(tmp_path / 'bench'),
                    '--commit', '0123456789abcdef'], check=True)
    summary = json.loads((site / 'qualite' / 'summary.json').read_text(encoding='utf-8'))
    assert summary['coverage']['percent'] == 50.0
    assert summary['coverage_html'] and summary['benchmarks_page']
    page = (site / 'qualite' / 'index.html').read_text(encoding='utf-8')
    assert 'couverture/index.html' in page and 'performances/index.html' in page
    assert '+25.0 %' in page
    assert (site / 'qualite' / 'couverture' / 'index.html').is_file()
