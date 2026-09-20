# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Tests de l'outillage de `Planning/` : modèle, ordre calculé, lint, Markdown et site."""
import json
import sys
from pathlib import Path

import pytest

OUTILS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(OUTILS))

import build_planning_site  # noqa: E402
import lint_planning  # noqa: E402
import mini_markdown  # noqa: E402
from planning_model import load_planning, progress  # noqa: E402

VERSIONS = '''
[[version]]
id = "0.0.1"
titre = "Démo"
nature = "detaillee"
referentiel = "0.1.0"
resume = "La démo."
dossier = "v0.1.0/v0.0.1"
objectifs = ["Jouer la quête"]
criteres_de_sortie = ["La quête se termine"]

[[version]]
id = "0.0.2"
titre = "Combat"
nature = "detaillee"
referentiel = "0.1.0"
resume = "Le combat."
dossier = "v0.1.0/v0.0.2"
objectifs = ["Se battre à quatre"]
criteres_de_sortie = ["Un combat de groupe se joue"]

[[version]]
id = "0.2.0"
titre = "Compagnie"
nature = "previsionnelle"
referentiel = "0.2.0"
resume = "La compagnie."
'''


def lot(lot_id, version, prerequis=(), statut='a-faire', taille='M', extra=''):
    parents = ', '.join(f'"{p}"' for p in prerequis)
    return (f'+++\nid = "{lot_id}"\ntitre = "Lot {lot_id}"\nversion = "{version}"\nfiliere = "moteur"\n'
            f'statut = "{statut}"\ntaille = "{taille}"\nresume = "Résumé."\nprerequis = [{parents}]\n'
            f'livrables = ["Un livrable"]\ncriteres = ["Un critère"]\n{extra}+++\n\n## Pourquoi\n\nParce que.\n')


class Dossier:
    """Un dossier `Planning/` de test : un chemin, et de quoi y poser une fiche de lot."""

    def __init__(self, root):
        self.root = root

    def __fspath__(self):
        return str(self.root)

    def __truediv__(self, other):
        return self.root / other

    def add(self, lot_id, version, **kwargs):
        folder = self.root / 'versions' / 'v0.1.0' / f'v{version}' / 'lots'
        (folder / f'{lot_id}-objet.md').write_text(lot(lot_id, version, **kwargs), encoding='utf-8')


@pytest.fixture
def planning_root(tmp_path):
    root = tmp_path / 'Planning'
    (root / 'versions' / 'v0.1.0' / 'v0.0.1' / 'lots').mkdir(parents=True)
    (root / 'versions' / 'v0.1.0' / 'v0.0.2' / 'lots').mkdir(parents=True)
    (root / 'site').mkdir()
    (root / 'site' / 'style.css').write_text('body{}', encoding='utf-8')
    (root / 'versions' / 'versions.toml').write_text(VERSIONS, encoding='utf-8')
    (root / 'README.md').write_text('# Planification\n\nIntroduction.\n\n## Suite\n', encoding='utf-8')

    return Dossier(root)


def test_ordre_calcule_version_puis_deblocage(planning_root):
    planning_root.add('LOT-101', '0.0.1', statut='livre')
    planning_root.add('LOT-102', '0.0.1', prerequis=['LOT-101'])
    planning_root.add('LOT-103', '0.0.1', prerequis=['LOT-101'])
    planning_root.add('LOT-104', '0.0.1', prerequis=['LOT-103'])
    planning_root.add('LOT-120', '0.0.2')
    lots = load_planning(planning_root).lots
    # LOT-103 débloque LOT-104 : il passe devant LOT-102 ; la 0.0.2 attend, bien que prête.
    assert [lots[i].rang for i in ('LOT-103', 'LOT-102', 'LOT-104', 'LOT-120')] == [1, 2, 3, 4]
    assert lots['LOT-103'].etat == 'prochain'
    assert lots['LOT-102'].etat == 'pret'
    assert lots['LOT-104'].etat == 'en-attente'
    assert lots['LOT-101'].etat == 'livre'
    assert lots['LOT-101'].debloque == 3


def test_avancement_pondere_par_la_taille(planning_root):
    planning_root.add('LOT-101', '0.0.1', statut='livre', taille='L')
    planning_root.add('LOT-102', '0.0.1', taille='S')
    planning = load_planning(planning_root)
    assert progress(planning.version('0.0.1').lots) == (3, 4)


def test_lint_accepte_un_dossier_sain(planning_root):
    planning_root.add('LOT-101', '0.0.1')
    planning_root.add('LOT-120', '0.0.2', prerequis=['LOT-101'])
    assert lint_planning.lint(planning_root) == []


@pytest.mark.parametrize('prepare, attendu', [
    (lambda r: (r.add('LOT-101', '0.0.1', prerequis=['LOT-120']), r.add('LOT-120', '0.0.2')),
     'version ultérieure'),
    (lambda r: (r.add('LOT-101', '0.0.1', prerequis=['LOT-102']), r.add('LOT-102', '0.0.1', prerequis=['LOT-101']),
                r.add('LOT-120', '0.0.2')), 'cycle de prérequis'),
    (lambda r: (r.add('LOT-101', '0.0.1'), r.add('LOT-102', '0.0.1', prerequis=['LOT-101'], statut='livre'),
                r.add('LOT-120', '0.0.2')), "n'est pas livré"),
    (lambda r: (r.add('LOT-101', '0.0.1', prerequis=['LOT-999']), r.add('LOT-120', '0.0.2')),
     'prérequis invalide'),
    (lambda r: r.add('LOT-101', '0.0.1'), 'version détaillée sans lot'),
    (lambda r: (r.add('LOT-101', '0.0.1', extra='maquettes = ["../absente.svg"]\n'), r.add('LOT-120', '0.0.2')),
     'maquette absente'),
])
def test_lint_refuse(planning_root, prepare, attendu):
    prepare(planning_root)
    assert any(attendu in error for error in lint_planning.lint(planning_root))


def test_lint_refuse_un_lien_mort(planning_root):
    planning_root.add('LOT-101', '0.0.1')
    planning_root.add('LOT-120', '0.0.2')
    (planning_root / 'README.md').write_text('[ici](vision/absente.md) et `[pas](un-lien.md)`', encoding='utf-8')
    assert lint_planning.lint(planning_root) == ['README.md: lien mort — vision/absente.md']


def test_markdown_rend_les_constructions_employees():
    rendu, titres = mini_markdown.render(
        '# Titre {#ancre}\n\nUn **gras**, un *italique*, du `code <b>` et un [lien](a.md).\n\n'
        '- un\n  - imbriqué\n- [x] fait\n\n| A | B |\n|---|---|\n| 1 | `x|y` |\n\n```\n<brut>\n```\n',
        link=lambda cible: cible.replace('.md', '.html'))
    assert titres == [(1, 'ancre', 'Titre')]
    assert '<h1 id="ancre">Titre</h1>' in rendu
    assert '<strong>gras</strong>' in rendu and '<em>italique</em>' in rendu
    assert '<code>code &lt;b&gt;</code>' in rendu
    assert '<a href="a.html">lien</a>' in rendu
    assert '<ul><li>un<ul><li>imbriqué</li></ul></li><li class="task">' in rendu
    assert '<th>A</th>' in rendu and '<td>1</td>' in rendu
    assert '&lt;brut&gt;' in rendu


def test_site_complet(planning_root, tmp_path):
    planning_root.add('LOT-101', '0.0.1', statut='livre')
    planning_root.add('LOT-120', '0.0.2', prerequis=['LOT-101'])
    (planning_root / 'referentiels').mkdir()
    (planning_root / 'referentiels' / 'pnj.toml').write_text(
        '[meta]\ntitre = "PNJ"\ncolonnes = ["nom"]\n[[ligne]]\nnom = "Un garde"\n', encoding='utf-8')
    (planning_root / 'referentiels' / 'README.md').write_text(
        '# Référentiels\n\n[Les PNJ](pnj.toml) et [le lot](../versions/v0.1.0/v0.0.1/lots/LOT-101-objet.md).\n',
        encoding='utf-8')
    out = tmp_path / 'site'
    assert build_planning_site.main(['--root', str(planning_root.root), '--out', str(out)]) == 0
    accueil = (out / 'index.html').read_text(encoding='utf-8')
    assert 'LOT-120' in accueil and 'versions/v0-0-2.html' in accueil
    assert (out / 'lots' / 'lot-101.html').is_file()
    assert 'Un garde' in (out / 'referentiels' / 'pnj.html').read_text(encoding='utf-8')
    index = (out / 'referentiels' / 'index.html').read_text(encoding='utf-8')
    assert 'href="pnj.html"' in index and 'href="../lots/lot-101.html"' in index
    assert 'href="../../qualite/"' in (out / 'lots' / 'index.html').read_text(encoding='utf-8')
    summary = json.loads((out / 'summary.json').read_text(encoding='utf-8'))
    assert summary['versions'][0] == {'id': '0.0.1', 'titre': 'Démo', 'nature': 'detaillee', 'lots': 1, 'livres': 1}
