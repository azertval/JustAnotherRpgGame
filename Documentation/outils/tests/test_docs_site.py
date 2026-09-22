# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Tests de l'outillage de `Documentation/` : conventions Markdown, site engendré, lint."""
import json
import sys
from pathlib import Path

import pytest

OUTILS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(OUTILS))

import build_docs_site  # noqa: E402
import lint_docs  # noqa: E402
import mini_markdown  # noqa: E402  (chemin ajouté par build_docs_site)

VERSIONS = '''
[[version]]
id = "0.0.0"
titre = "Fondation"
nature = "detaillee"
referentiel = "0.0.0"
resume = "La fondation."
dossier = "v0.0.0"
'''
FICHE = ('+++\nid = "LOT-19"\ntitre = "Grille"\nversion = "0.0.0"\nfiliere = "regles"\nstatut = "livre"\n'
         'taille = "M"\nresume = "La grille."\nlivrables = ["Un livrable"]\ncriteres = ["Un critère"]\n+++\n\nCorps.\n')
TAGFILE = '''<?xml version="1.0"?>
<tagfile>
  <compound kind="class">
    <name>core::BattleGrid</name>
    <filename>classcore_1_1_battle_grid.html</filename>
    <member kind="function">
      <name>place</name>
      <anchorfile>classcore_1_1_battle_grid.html</anchorfile>
      <anchor>a1b2</anchor>
    </member>
  </compound>
</tagfile>
'''


@pytest.fixture
def depot(tmp_path):
    """Un dépôt miniature : `Documentation/`, `Planning/` avec une fiche, `Source/` avec un test."""
    docs = tmp_path / 'Documentation'
    (docs / 'Guide' / 'captures').mkdir(parents=True)
    (docs / 'Specification').mkdir()
    (docs / 'CahierTest').mkdir()
    (docs / 'README.md').write_text('# Accueil\n\nLe site. Voir le [guide](Guide/README.md).\n', encoding='utf-8')
    (docs / 'reference.md').write_text('# Référence\n', encoding='utf-8')
    (docs / 'Guide' / 'README.md').write_text(
        '# Guide\n\nLe comment.\n\n- [Combat](guide-combat.md)\n- [Boucle](guide-boucle.md)\n', encoding='utf-8')
    (docs / 'Guide' / 'guide-boucle.md').write_text('# Boucle\n\n## Pas fixe {#pas-fixe}\n\nTexte.\n', encoding='utf-8')
    (docs / 'Guide' / 'guide-combat.md').write_text(
        '# Combat\n\nVoir `EX-CBT-001`, `LOT-19`, `core::BattleGrid` et `core::BattleGrid::place()`, '
        'puis [le pas fixe](guide-boucle.md#pas-fixe) et [la fiche](../../Planning/versions/v0.0.0/lots/LOT-19-grille.md).\n\n'
        '![La grille de combat](captures/grille.png)\n\n> **Attention** — Un encadré.\n', encoding='utf-8')
    (docs / 'Guide' / 'captures' / 'grille.png').write_bytes(b'png')
    (docs / 'Specification' / 'README.md').write_text('# Spécifications\n\n- [Combat](combat.md)\n', encoding='utf-8')
    (docs / 'Specification' / 'combat.md').write_text(
        '# Combat\n\n- **EX-CBT-001** — La bascule est explicite.\n- **EX-CBT-002** *(retirée au `LOT-19`)* — Rien.\n',
        encoding='utf-8')
    (docs / 'CahierTest' / 'README.md').write_text('# Cahier\n\n- [Core](core.md)\n', encoding='utf-8')
    (docs / 'CahierTest' / 'core.md').write_text(
        '# Core\n\n## test_a.cpp\n\n### A.Un\n\n*Bloquant · Unitaire* — `a.cpp:1`\n\nObjet.\n\n'
        '### A.Deux\n\n*Mineur · Unitaire* — `a.cpp:9`\n\nObjet.\n', encoding='utf-8')
    lots = tmp_path / 'Planning' / 'versions' / 'v0.0.0' / 'lots'
    lots.mkdir(parents=True)
    (tmp_path / 'Planning' / 'versions' / 'versions.toml').write_text(VERSIONS, encoding='utf-8')
    (lots / 'LOT-19-grille.md').write_text(FICHE, encoding='utf-8')
    tests = tmp_path / 'Source' / 'Test'
    tests.mkdir(parents=True)
    (tests / 'test_a.cpp').write_text('// EX-CBT-001\n', encoding='utf-8')
    (tmp_path / 'reference.tag').write_text(TAGFILE, encoding='utf-8')
    return tmp_path


def build(depot, **options):
    site = build_docs_site.Site(depot / 'Documentation', depot / 'site', **options)
    site.build()
    return site


def test_les_conventions_de_page_se_rendent():
    rendered, _ = mini_markdown.render(
        '- **EX-CBT-001** — Texte.\n\n![Légende](a.png)\n\n> **Note** — Bon à savoir.\n\n```cpp\nint a;\n```\n')
    assert '<li id="EX-CBT-001" class="identified">' in rendered
    assert '<figure>' in rendered and '<figcaption>Légende</figcaption>' in rendered
    assert '<blockquote class="callout note">' in rendered
    assert '<code class="language-cpp">' in rendered
    assert mini_markdown.render_inline('<kbd>F5</kbd> et <script>') == '<kbd>F5</kbd> et &lt;script&gt;'


def test_le_site_suit_l_ordre_du_sommaire_et_relie_ce_qui_est_cite(depot):
    site = build(depot, tagfile=depot / 'reference.tag')
    assert [page.source_rel for page in site.ordered('Guide')] == [
        'Guide/README.md', 'Guide/guide-combat.md', 'Guide/guide-boucle.md']
    assert 'reference.md' not in site.pages  # la page d'accueil de Doxygen n'est pas une page du site
    combat = (depot / 'site' / 'Guide' / 'guide-combat.html').read_text(encoding='utf-8')
    assert 'href="../Specification/combat.html#EX-CBT-001"' in combat
    assert 'href="../planning/lots/lot-19.html"' in combat
    assert 'href="../reference/classcore_1_1_battle_grid.html"' in combat
    assert 'href="../reference/classcore_1_1_battle_grid.html#a1b2"' in combat
    assert 'href="guide-boucle.html#pas-fixe"' in combat
    assert (depot / 'site' / 'Guide' / 'captures' / 'grille.png').is_file()
    assert (depot / 'site' / 'assets' / 'docs.js').is_file()
    search = json.loads((depot / 'site' / 'search.json').read_text(encoding='utf-8'))
    assert {entry['u'] for entry in search} >= {'index.html', 'Guide/guide-combat.html'}


def test_l_index_des_exigences_dit_qui_les_cite(depot):
    site = build(depot)
    assert set(site.exigences) == {'EX-CBT-001', 'EX-CBT-002'}
    index = (depot / 'site' / 'Specification' / 'exigences.html').read_text(encoding='utf-8')
    assert 'data-etat="couverte"' in index  # EX-CBT-001 : citée par un test
    assert 'data-etat="retiree"' in index   # EX-CBT-002


def test_le_cahier_de_test_devient_des_cartes_filtrables(depot):
    build(depot)
    page = (depot / 'site' / 'CahierTest' / 'core.html').read_text(encoding='utf-8')
    assert page.count('<section class="case"') == 2
    assert 'data-crit="Bloquant"' in page and 'data-crit="Mineur"' in page
    assert 'data-filter-cases' in page


def test_le_lint_accepte_un_dossier_sain(depot):
    errors, count = lint_docs.lint(depot / 'Documentation')
    assert errors == []
    assert count == 8


@pytest.mark.parametrize('ajout, attendu', [
    ('Voir @ref guide-boucle.\n', 'commande Doxygen'),
    ('Voir [rien](absent.md).\n', 'lien mort'),
    ('Voir [ancre](guide-boucle.md#absente).\n', 'ancre inconnue'),
    ('![Image](captures/absente.png)\n', 'lien mort'),
    ('Le `LOT-999` fera cela.\n', 'LOT-999'),
])
def test_le_lint_refuse(depot, ajout, attendu):
    page = depot / 'Documentation' / 'Guide' / 'guide-boucle.md'
    page.write_text(page.read_text(encoding='utf-8') + '\n' + ajout, encoding='utf-8')
    errors, _ = lint_docs.lint(depot / 'Documentation')
    assert any(attendu in error for error in errors), errors


def test_le_lint_refuse_une_page_orpheline(depot):
    (depot / 'Documentation' / 'Guide' / 'guide-perdu.md').write_text('# Perdu\n', encoding='utf-8')
    errors, _ = lint_docs.lint(depot / 'Documentation')
    assert any('page orpheline' in error and 'guide-perdu.md' in error for error in errors)
