#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Engendre le site de documentation depuis `Documentation/`.

Les pages de `Documentation/` sont du Markdown nu, lisible tel quel dans le dépôt : pas une commande
Doxygen, des liens relatifs, des exigences déclarées par une puce (`- **EX-CBT-001** — …`). Ce
script les rend avec le moteur et la charte du site de planification, si bien que le guide, les
spécifications, le cahier de test et la planification se lisent comme un seul site. Doxygen ne
garde que `Source/` : la référence de code, publiée sous `reference/`, annexe du guide.

Pages produites, sous `--out` :

- `index.html` : l'accueil (`Documentation/README.md`) et l'entrée de chaque partie ;
- une page par fichier Markdown d'une partie (même chemin, `.html` ; `README.md` → `index.html`),
  dans l'ordre où le sommaire de la partie les cite ;
- `Specification/exigences.html` : l'index de toutes les exigences, filtrable, avec qui les cite ;
- `search.json` : l'index de la recherche de la barre d'en-tête.

Usage :
  python Documentation/outils/build_docs_site.py --out build/site
"""
import argparse
import html
import json
import posixpath
import re
import shutil
import sys
import xml.etree.ElementTree as ElementTree
from datetime import datetime, timezone
from pathlib import Path

DOCS_ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = DOCS_ROOT.parent
sys.path.insert(0, str(REPO_ROOT / 'Planning' / 'outils'))

import mini_markdown  # noqa: E402
from planning_model import LOT_FILE_RE, PlanningError, load_planning  # noqa: E402

CHARTE = REPO_ROOT / 'Site'
# Les parties du site, dans l'ordre du menu : (dossier, libellé, clé de menu).
SECTIONS = [('Guide', 'Guide'), ('Specification', 'Spécifications'), ('CahierTest', 'Cahier de test')]
SKIPPED_DIRS = {'outils', 'generated', 'SourceBook', '__pycache__', '.pytest_cache'}
IMAGE_SUFFIXES = ('.svg', '.png', '.jpg', '.jpeg', '.webp', '.gif')
REPO_URL = 'https://github.com/azertval/JustAnotherRpgGame/blob/main/'
EXIGENCE_RE = re.compile(r'EX-[A-Z]+-\d{3}')
EXIGENCE_DECL_RE = re.compile(r'^\s*[-*]\s+\*\*(EX-[A-Z]+-\d{3})\*\*\s*(?:—|-)?\s*(.*)$')
MD_LINK_RE = re.compile(r'(?<!!)\[[^\]]+\]\(([^)\s]+)\)')
CRITICITES = ('Bloquant', 'Critique', 'Majeur', 'Mineur')


def esc(value):
    return html.escape(str(value), quote=True)


def is_skipped(relative):
    return bool(SKIPPED_DIRS & set(relative.parts))


def page_of(source_rel):
    """Chemin de la page engendrée pour un fichier Markdown de `Documentation/`."""
    if posixpath.basename(source_rel) == 'README.md':
        return posixpath.join(posixpath.dirname(source_rel), 'index.html')
    return source_rel[:-3] + '.html'


def strip_code(text):
    """Le texte sans ses blocs ni ses spans de code : un lien cité en exemple n'est pas un lien."""
    text = re.sub(r'```.*?```', '', text, flags=re.DOTALL)
    return re.sub(r'`[^`]*`', '', text)


def load_tagfile(path):
    """Les symboles de la référence de code, d'après le fichier d'étiquettes de Doxygen.

    `GENERATE_TAGFILE` écrit, pour chaque classe, espace de noms et membre, la page et l'ancre qui le
    documentent. Un nom de symbole cité en code dans le guide (`core::World`, `hmi::Camera2D::zoom`)
    devient ainsi un lien vers la référence ; sans fichier, il reste du code, et rien ne casse.
    """
    symbols = {}
    try:
        root = ElementTree.parse(path).getroot()
    except (OSError, ElementTree.ParseError):
        return symbols
    for compound in root.iter('compound'):
        if compound.get('kind') not in ('class', 'struct', 'namespace'):
            continue
        name, filename = compound.findtext('name'), compound.findtext('filename')
        if not name or not filename:
            continue
        filename = filename if filename.endswith('.html') else filename + '.html'
        symbols.setdefault(name, filename)
        for member in compound.iter('member'):
            anchor = member.findtext('anchor')
            target = member.findtext('anchorfile') or filename
            if member.findtext('name') and anchor:
                symbols.setdefault(f'{name}::{member.findtext("name")}', f'{target}#{anchor}')
    return symbols


class Page:
    def __init__(self, source_rel, text):
        self.source_rel = source_rel
        self.page = page_of(source_rel)
        self.text = text
        self.section = source_rel.split('/')[0] if '/' in source_rel else ''
        self.depth = max(0, source_rel.count('/') - 1) + (0 if source_rel.endswith('README.md') else 1)
        if source_rel.count('/') == 1 and source_rel.endswith('README.md'):
            self.depth = 0
        self.title = ''
        self.headings = []
        self.html = ''


class Site:
    def __init__(self, root, out, commit='', planning_url='planning/', reference_url='reference/',
                 source_root=None, tagfile=None):
        self.root = Path(root)
        self.out = Path(out)
        self.commit = commit
        self.planning_url = planning_url
        self.reference_url = reference_url
        self.source_root = Path(source_root) if source_root else self.root.parent / 'Source'
        self.pages = {}       # source_rel -> Page
        self.exigences = {}   # id -> dict(page, source, texte, retiree)
        self.lots = {}        # id -> slug, d'après `Planning/`
        self.symbols = load_tagfile(tagfile) if tagfile else {}  # `core::World` -> page de la référence
        try:
            planning = load_planning(self.root.parent / 'Planning')
            self.lots = {lot.id: lot.slug for lot in planning.lots.values()}
        except (PlanningError, OSError):
            pass

    # -- lecture ----------------------------------------------------------------------------------

    def load(self):
        for source in sorted(self.root.rglob('*.md')):
            relative = source.relative_to(self.root)
            if is_skipped(relative):
                continue
            rel = relative.as_posix()
            # À la racine, seul l'accueil est une page : `reference.md` est la page d'accueil de Doxygen.
            if rel != 'README.md' and rel.split('/')[0] not in {folder for folder, _ in SECTIONS}:
                continue
            self.pages[rel] = Page(rel, source.read_text(encoding='utf-8'))
        for page in self.pages.values():
            in_code = False
            for line in page.text.split('\n'):
                if line.strip().startswith('```'):
                    in_code = not in_code
                match = None if in_code else EXIGENCE_DECL_RE.match(line)
                if match and match.group(1) not in self.exigences:
                    self.exigences[match.group(1)] = {
                        'page': page.page, 'source': page.source_rel, 'texte': match.group(2),
                        'retiree': 'retirée' in match.group(2)[:40].lower() or 'retirees' in page.source_rel,
                    }

    def ordered(self, folder):
        """Les pages d'une partie dans l'ordre où son sommaire (et ses sous-sommaires) les cite."""
        order, seen = [], set()

        def visit(rel):
            if rel in seen or rel not in self.pages:
                return
            seen.add(rel)
            order.append(rel)
            for target in MD_LINK_RE.findall(strip_code(self.pages[rel].text)):
                path = target.partition('#')[0]
                if path.endswith('.md') and '://' not in path:
                    resolved = posixpath.normpath(posixpath.join(posixpath.dirname(rel), path))
                    if resolved.startswith(folder + '/'):
                        visit(resolved)

        visit(f'{folder}/README.md')
        for rel in sorted(self.pages):
            if rel.startswith(folder + '/'):
                visit(rel)
        return [self.pages[rel] for rel in order]

    # -- liens ------------------------------------------------------------------------------------

    def rel(self, from_page, target):
        return posixpath.relpath(target, posixpath.dirname(from_page) or '.')

    def link_rewriter(self, source_rel, page):
        source_dir = posixpath.dirname(source_rel)

        def rewrite(target):
            if '://' in target or target.startswith(('#', 'mailto:')):
                return target
            path, _, anchor = target.partition('#')
            suffix = '#' + anchor if anchor else ''
            resolved = posixpath.normpath(posixpath.join('Documentation', source_dir, path))
            if resolved.startswith('Documentation/'):
                inner = resolved[len('Documentation/'):]
                if inner.endswith('.md'):
                    inner = page_of(inner)
                return self.rel(page, inner) + suffix
            if resolved.startswith('Planning/'):
                inner = resolved[len('Planning/'):]
                name = posixpath.basename(inner)
                match = LOT_FILE_RE.match(name)
                if match:
                    inner = f'lots/{match.group(1).lower()}.html'
                elif inner.endswith('.md'):
                    inner = page_of(inner)
                elif inner == 'versions/versions.toml':
                    inner = 'index.html'
                return self.rel(page, posixpath.join(self.planning_url, inner)) + suffix
            # Tout le reste du dépôt (code, scripts, données) se lit sur la forge.
            return REPO_URL + resolved + suffix

        return rewrite

    def autolink(self, content, page):
        """Un identifiant d'exigence ou de lot cité en code devient un lien vers sa déclaration."""
        def replace(match):
            inner = match.group(1)
            if inner in self.exigences:
                entry = self.exigences[inner]
                return (f'<a class="ref" href="{esc(self.rel(page, entry["page"]))}#{inner}">'
                        f'<code>{inner}</code></a>')
            if inner in self.lots:
                target = posixpath.join(self.planning_url, f'lots/{self.lots[inner]}.html')
                return f'<a class="ref" href="{esc(self.rel(page, target))}"><code>{inner}</code></a>'
            symbol = html.unescape(inner).removesuffix('()')
            if symbol in self.symbols:
                target = posixpath.join(self.reference_url, self.symbols[symbol])
                return f'<a class="ref" href="{esc(self.rel(page, target))}"><code>{inner}</code></a>'
            return match.group(0)

        pieces = re.split(r'(<a\b.*?</a>|<pre>.*?</pre>)', content, flags=re.DOTALL)
        for index in range(0, len(pieces), 2):
            pieces[index] = re.sub(r'<code>((?:EX|LOT)-[A-Z0-9-]+|(?:core|hmi)::[\w:]+(?:\(\))?)</code>',
                                   replace, pieces[index])
        return ''.join(pieces)

    # -- gabarit ----------------------------------------------------------------------------------

    def layout(self, page, title, body, active='', sidebar=''):
        nav = [('index.html', 'Accueil', 'home')]
        nav += [(f'{folder}/index.html', label, folder) for folder, label in SECTIONS
                if f'{folder}/README.md' in self.pages]
        items = ''.join(
            f'<a href="{esc(self.rel(page, target))}"{" class=\"active\"" if key == active else ""}>'
            f'{esc(label)}</a>' for target, label, key in nav)
        external = (f'<a class="active" href="{esc(self.rel(page, "index.html"))}">Documentation</a>'
                    f'<a href="{esc(self.rel(page, self.planning_url))}/">Planification</a>'
                    f'<a href="{esc(self.rel(page, "qualite"))}/">Qualité</a>')
        aside = f'<aside class="sidebar">{sidebar}</aside>' if sidebar else ''
        stamp = datetime.now(timezone.utc).strftime('%Y-%m-%d')
        commit = f' · <code>{esc(self.commit[:9])}</code>' if self.commit else ''
        root = self.rel(page, '.')
        return f'''<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)} — Documentation</title>
<link rel="stylesheet" href="{esc(self.rel(page, 'assets/theme.css'))}">
</head>
<body data-root="{esc(root)}">
<header class="topbar">
  <a class="brand" href="{esc(self.rel(page, 'index.html'))}"><span>Just Another RPG Game</span> Documentation</a>
  <nav>{items}</nav>
  <form class="search" role="search"><input type="search" placeholder="Chercher…" aria-label="Chercher dans la documentation"><div class="results" hidden></div></form>
  <div class="links">{external}</div>
</header>
<div class="shell{' with-sidebar' if sidebar else ''}">
{aside}
<main>
{body}
</main>
</div>
<footer>Engendré depuis <code>Documentation/</code> le {stamp}{commit}. Rien ici ne s'édite : la source est le dossier.</footer>
<script src="{esc(self.rel(page, 'assets/site.js'))}"></script>
<script src="{esc(self.rel(page, 'assets/docs.js'))}"></script>
</body>
</html>
'''

    def write(self, page, content):
        path = self.out / page
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8', newline='\n')

    # -- pages ------------------------------------------------------------------------------------

    def render_page(self, page):
        content, headings = mini_markdown.render(page.text, self.link_rewriter(page.source_rel, page.page))
        page.headings = headings
        page.title = next((text for level, _, text in headings if level == 1),
                          Path(page.source_rel).stem)
        page.html = self.autolink(content, page.page)

    def sidebar(self, label, pages, current):
        items = [f'<h4>{esc(label)}</h4>']
        for entry in pages:
            active = ' active' if entry is current else ''
            items.append(f'<a class="depth-{min(entry.depth, 2)}{active}" '
                         f'href="{esc(self.rel(current.page, entry.page))}">{esc(entry.title)}</a>')
            if entry is current:
                items += [f'<a class="toc depth-{min(entry.depth, 2) + 1}" href="#{esc(anchor)}">{esc(text)}</a>'
                          for level, anchor, text in entry.headings if level == 2]
        return ''.join(items)

    def build_section(self, folder, label):
        pages = self.ordered(folder)
        for index, page in enumerate(pages):
            body = page.html
            if folder == 'CahierTest' and page.depth > 0:
                body = self.case_cards(body)
            pager = []
            if index > 0:
                previous = pages[index - 1]
                pager.append(f'<a class="prev" href="{esc(self.rel(page.page, previous.page))}">'
                             f'<span>Précédent</span>{esc(previous.title)}</a>')
            if index + 1 < len(pages):
                following = pages[index + 1]
                pager.append(f'<a class="next" href="{esc(self.rel(page.page, following.page))}">'
                             f'<span>Suivant</span>{esc(following.title)}</a>')
            source = (f'<p class="source">Source : <code>Documentation/{esc(page.source_rel)}</code></p>')
            body = (f'<article class="prose">{body}</article>'
                    f'<nav class="pager">{"".join(pager)}</nav>{source}')
            self.write(page.page, self.layout(page.page, page.title, body, folder,
                                              self.sidebar(label, pages, page)))

    def case_cards(self, content):
        """Cahier de test : chaque cas (un `h3` et ce qui le suit) devient une carte filtrable."""
        pieces = re.split(r'(?=<h[23] )', content)
        out, cases = [], 0
        for piece in pieces:
            if piece.startswith('<h3 '):
                found = re.search(r'<em>([^<]+)</em>', piece)
                crit = next((c for c in CRITICITES if found and c in found.group(1)), '')
                out.append(f'<section class="case" data-crit="{esc(crit)}">{piece}</section>')
                cases += 1
            else:
                out.append(piece)
        if not cases:
            return content
        options = ''.join(f'<option>{c}</option>' for c in CRITICITES)
        filters = (f'<div class="filters" data-filter-cases><input type="search" placeholder="Filtrer les cas…" '
                   f'aria-label="Filtrer les cas de test"><select><option value="">Toutes les criticités</option>'
                   f'{options}</select><span class="count">{cases} cas</span></div>')
        head, _, rest = ''.join(out).partition('</h1>')
        return head + '</h1>' + filters + rest

    def build_home(self):
        page = self.pages.get('README.md')
        if not page:
            return
        cards = []
        for folder, label in SECTIONS:
            index = self.pages.get(f'{folder}/README.md')
            if not index:
                continue
            first = next((block for block in re.split(r'\n\s*\n', index.text)
                          if block.strip() and not block.lstrip().startswith(('#', '>', '-', '|', '!'))), '')
            count = sum(1 for rel in self.pages if rel.startswith(folder + '/'))
            # La carte est elle-même un lien : un lien dans le résumé s'y imbriquerait, ce que le
            # navigateur refuse en refermant la carte au premier `<a>`. Le résumé garde ses mots.
            summary = re.sub(r'<a\b[^>]*>(.*?)</a>', r'\1',
                             mini_markdown.render_inline(' '.join(first.split())), flags=re.DOTALL)
            cards.append(f'<a class="vcard" href="{esc(self.rel(page.page, index.page))}">'
                         f'<strong>{esc(label)}</strong><p>{summary}</p>'
                         f'<span class="meta">{count} pages</span></a>')
        cards.append(f'<a class="vcard" href="{esc(self.planning_url)}"><strong>Planification</strong>'
                     f'<p>Les versions, les lots livrés et à venir, leur ordre calculé et leurs maquettes.</p>'
                     f'<span class="meta">{len(self.lots)} lots</span></a>')
        cards.append('<a class="vcard" href="qualite/"><strong>Qualité</strong><p>Couverture de code par domaine '
                     'et mesures de performance de la nuit.</p><span class="meta">mise à jour à chaque publication'
                     '</span></a>')
        cards.append(f'<a class="vcard" href="{esc(self.reference_url)}"><strong>Référence du code</strong>'
                     f'<p>Classes, espaces de noms et fichiers de <code>Source/</code>, engendrés par Doxygen : '
                     f'l\'annexe du guide.</p><span class="meta">Doxygen</span></a>')
        stats = (f'<div class="stats"><div><b>{len(self.pages)}</b><span>pages</span></div>'
                 f'<div><b>{sum(1 for e in self.exigences.values() if not e["retiree"])}</b>'
                 f'<span>exigences en vigueur</span></div>'
                 f'<div><b>{len(self.lots)}</b><span>lots planifiés</span></div></div>')
        head, _, rest = page.html.partition('</h1>')
        body = (f'<section class="hero">{head}</h1>{stats}</section><div class="vcards">{"".join(cards)}</div>'
                f'<article class="prose">{rest}</article>')
        self.write('index.html', self.layout('index.html', page.title, body, 'home'))

    def citations(self):
        """Pour chaque exigence, les fichiers de code et de test qui la citent."""
        cited = {key: {'code': set(), 'tests': set()} for key in self.exigences}
        if not self.source_root.is_dir():
            return cited
        for source in self.source_root.rglob('*'):
            if source.suffix not in ('.h', '.hpp', '.cpp', '.qml', '.py') or not source.is_file():
                continue
            try:
                text = source.read_text(encoding='utf-8', errors='ignore')
            except OSError:
                continue
            kind = 'tests' if 'Test' in source.relative_to(self.source_root).parts else 'code'
            for key in set(EXIGENCE_RE.findall(text)):
                if key in cited:
                    cited[key][kind].add(source.relative_to(self.source_root.parent).as_posix())
        return cited

    def build_exigences(self):
        if 'Specification/README.md' not in self.pages:
            return
        page = 'Specification/exigences.html'
        cited = self.citations()
        rows = []
        for key in sorted(self.exigences):
            entry = self.exigences[key]
            family = key.rsplit('-', 1)[0]
            state = 'retiree' if entry['retiree'] else ('couverte' if cited[key]['tests'] else
                                                        ('citee' if cited[key]['code'] else 'nue'))
            label = {'retiree': 'retirée', 'couverte': 'testée', 'citee': 'citée par le code',
                     'nue': 'sans citation'}[state]
            texte = re.sub(r'\s+', ' ', entry['texte'])
            texte = texte if len(texte) <= 150 else texte[:149] + '…'
            document = self.pages[entry['source']].title
            rows.append(
                f'<tr data-famille="{esc(family)}" data-etat="{state}">'
                f'<td><a class="ref" href="{esc(self.rel(page, entry["page"]))}#{key}">{key}</a></td>'
                f'<td>{mini_markdown.render_inline(texte)}</td><td>{esc(document)}</td>'
                f'<td class="num">{len(cited[key]["code"])}</td><td class="num">{len(cited[key]["tests"])}</td>'
                f'<td><span class="badge trace-{state}">{label}</span></td></tr>')
        families = sorted({key.rsplit('-', 1)[0] for key in self.exigences})
        family_options = ''.join(f'<option>{esc(f)}</option>' for f in families)
        body = (f'<p class="crumb"><a href="index.html">Spécifications</a></p><h1>Index des exigences</h1>'
                f'<p class="lede">{len(self.exigences)} exigences déclarées dans les spécifications. Les colonnes '
                f'« code » et « tests » comptent les fichiers de <code>Source/</code> qui citent l\'identifiant : '
                f'c\'est la traçabilité, relevée à chaque génération.</p>'
                f'<div class="filters" data-filter-table="data"><input type="search" placeholder="Filtrer…" '
                f'aria-label="Filtrer les exigences"><select data-key="famille"><option value="">Toutes les familles'
                f'</option>{family_options}</select><select data-key="etat"><option value="">Tous les états</option>'
                f'<option value="couverte">testée</option><option value="citee">citée par le code</option>'
                f'<option value="nue">sans citation</option><option value="retiree">retirée</option></select>'
                f'<span class="count">{len(rows)} lignes</span></div>'
                f'<div class="table-wrap"><table class="data"><thead><tr><th>Exigence</th><th>Énoncé</th>'
                f'<th>Document</th><th>Code</th><th>Tests</th><th>État</th></tr></thead>'
                f'<tbody>{"".join(rows)}</tbody></table></div>')
        pages = self.ordered('Specification')
        sidebar = ('<h4>Spécifications</h4>' + ''.join(
            f'<a class="depth-{min(entry.depth, 2)}" href="{esc(self.rel(page, entry.page))}">{esc(entry.title)}</a>'
            for entry in pages) + '<a class="depth-1 active" href="exigences.html">Index des exigences</a>')
        self.write(page, self.layout(page, 'Index des exigences', body, 'Specification', sidebar))

    def write_search(self):
        entries = []
        for page in self.pages.values():
            text = re.sub(r'<[^>]+>', ' ', page.html)
            entries.append({
                'u': page.page, 't': page.title,
                'h': [text_ for level, _, text_ in page.headings if level in (2, 3)],
                'x': html.unescape(re.sub(r'\s+', ' ', text))[:6000],
            })
        self.write('search.json', json.dumps(entries, ensure_ascii=False, separators=(',', ':')))

    def copy_static(self):
        assets = self.out / 'assets'
        assets.mkdir(parents=True, exist_ok=True)
        for name in ('theme.css', 'tokens.css', 'topbar.css', 'site.js', 'docs.js'):
            shutil.copy2(CHARTE / name, assets / name)
        for source in self.root.rglob('*'):
            relative = source.relative_to(self.root)
            if source.is_file() and not is_skipped(relative) and source.suffix.lower() in IMAGE_SUFFIXES:
                target = self.out / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)

    def build(self):
        if self.out.exists():
            shutil.rmtree(self.out)
        self.load()
        self.copy_static()
        for page in self.pages.values():
            self.render_page(page)
        self.build_home()
        for folder, label in SECTIONS:
            self.build_section(folder, label)
        self.build_exigences()
        self.write_search()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--root', default=str(DOCS_ROOT), help='dossier Documentation/ à lire')
    parser.add_argument('--out', required=True, help='dossier du site engendré (remplacé en entier)')
    parser.add_argument('--commit', default='', help='commit publié, rappelé en pied de page')
    parser.add_argument('--tagfile', default='', help="fichier d'étiquettes de Doxygen : relie les symboles "
                                                      'cités en code à la référence')
    args = parser.parse_args(argv)
    site = Site(args.root, args.out, args.commit, tagfile=args.tagfile or None)
    site.build()
    print(f'Site de documentation : {len(site.pages)} pages, {len(site.exigences)} exigences, '
          f'{len(site.symbols)} symboles relies, dans {args.out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
