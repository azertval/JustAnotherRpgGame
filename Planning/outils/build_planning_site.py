#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Engendre le site de planification depuis `Planning/`.

Le site ne porte aucune information qui ne soit dans le dossier : il le **lit**. Une fiche de lot
modifiée, un statut passé à `livre`, et la page d'accueil, l'avancement de la version, l'ordre
calculé et le graphe des prérequis suivent à la prochaine génération. Publié sous `planning/` du
site gh-pages par `docs.yml`, à côté de la Doxygen (racine) et du site qualité (`qualite/`).

Pages produites :

- `index.html` : la trajectoire des versions, l'avancement, « et maintenant ? » ;
- `versions/<version>.html` : périmètre, critères de sortie, graphe des prérequis, lots, maquettes ;
- `lots/index.html` et `lots/<lot>.html` : tous les lots, filtrables, et la fiche de chacun ;
- une page par fichier Markdown de `Planning/` (même chemin, `.html`) ;
- une page par table TOML de `Planning/referentiels/` (même chemin, `.html`), filtrable.

Usage :
  python Planning/outils/build_planning_site.py --out build/planning-site
"""
import argparse
import html
import json
import posixpath
import shutil
import sys
import tomllib
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import mini_markdown  # noqa: E402
from planning_model import (  # noqa: E402
    FILIERES, LOT_FILE_RE, PlanningError, load_planning, lot_sort_key, progress,
)

PLANNING_ROOT = Path(__file__).resolve().parents[1]
# La charte du site publié (palette, barre d'en-tête, feuilles) : partagée avec la référence de
# code et la page qualité, elle vit à la racine du dépôt et se recopie telle quelle dans `assets/`.
CHARTE = PLANNING_ROOT.parent / 'Site'
# Sections du dossier, dans l'ordre du menu. `versions/` a ses propres pages.
SECTIONS = [('versions', 'Versions'), ('vision', 'Vision'), ('referentiels', 'Référentiels'),
            ('standards', 'Standards')]
SKIPPED_DIRS = {'outils', '__pycache__', '.pytest_cache'}
ETATS = {
    'livre': 'livré', 'en-cours': 'en cours', 'prochain': 'prochain', 'pret': 'prêt',
    'en-attente': 'en attente', 'abandonne': 'abandonné',
}
NATURES = {'detaillee': 'planifiée en détail', 'previsionnelle': 'prévisionnelle'}


def esc(value):
    return html.escape(str(value), quote=True)


class Site:
    def __init__(self, planning, out, docs_url, commit):
        self.planning = planning
        self.out = Path(out)
        self.docs_url = docs_url
        self.commit = commit
        self.pages = []  # (chemin relatif, titre) des pages Markdown et tables, pour les menus

    # -- chemins --------------------------------------------------------------------------------

    def rel(self, from_page, target):
        return posixpath.relpath(target, posixpath.dirname(from_page) or '.')

    def link_rewriter(self, source_rel, page):
        """Réécrit un lien relatif d'un fichier Markdown vers la page engendrée correspondante."""
        source_dir = posixpath.dirname(source_rel)

        def rewrite(target):
            if '://' in target or target.startswith(('#', 'mailto:')):
                return target
            path, _, anchor = target.partition('#')
            resolved = posixpath.normpath(posixpath.join(source_dir, path))
            if resolved.startswith('../Documentation/'):
                # Une page du site de documentation, publié à `docs_url` : même chemin, en `.html`.
                inner = resolved[len('../Documentation/'):]
                if inner.endswith('.md'):
                    inner = inner[:-3] + '.html'
                    if posixpath.basename(inner) == 'README.html':
                        inner = posixpath.join(posixpath.dirname(inner), 'index.html')
                base = self.docs_url if '://' in self.docs_url else self.rel(
                    page, posixpath.normpath(posixpath.join('.', self.docs_url)))
                return posixpath.join(base, inner) + ('#' + anchor if anchor else '')
            name = posixpath.basename(resolved)
            match = LOT_FILE_RE.match(name)
            if match:
                resolved = f'lots/{match.group(1).lower()}.html'
            elif resolved == 'versions/versions.toml':
                resolved = 'index.html'
            elif resolved.endswith('.md'):
                resolved = resolved[:-3] + '.html'
                if posixpath.basename(resolved) == 'README.html':
                    resolved = posixpath.join(posixpath.dirname(resolved), 'index.html')
            elif resolved.endswith('.toml') and resolved.startswith('referentiels/'):
                resolved = resolved[:-5] + '.html'
            result = self.rel(page, resolved)
            return result + ('#' + anchor if anchor else '')

        return rewrite

    # -- gabarit --------------------------------------------------------------------------------

    def layout(self, page, title, body, active='', sidebar=''):
        nav = [('index.html', 'Trajectoire', 'home'), ('lots/index.html', 'Lots', 'lots')]
        nav += [(f'{folder}/index.html', label, folder) for folder, label in SECTIONS
                if (self.planning.root / folder).is_dir()]
        items = ''.join(
            f'<a href="{esc(self.rel(page, target))}"{" class=\"active\"" if key == active else ""}>'
            f'{esc(label)}</a>' for target, label, key in nav)
        # Une adresse relative s'entend depuis la racine du site : elle se recalcule par page.
        docs = self.docs_url if '://' in self.docs_url else posixpath.normpath(
            posixpath.join(self.rel(page, '.'), self.docs_url))
        docs = docs.rstrip('/')
        # Les trois parties du site, dans le même ordre et sous les mêmes libellés partout ; la
        # part courante est marquée. On ne quitte pas le site en suivant ces liens : on y navigue.
        external = (f'<a href="{esc(docs)}/">Documentation</a>'
                    f'<a class="active" href="{esc(self.rel(page, "index.html"))}">Planification</a>'
                    f'<a href="{esc(docs)}/qualite/">Qualité</a>')
        aside = f'<aside class="sidebar">{sidebar}</aside>' if sidebar else ''
        stamp = datetime.now(timezone.utc).strftime('%Y-%m-%d')
        commit = f' · <code>{esc(self.commit[:9])}</code>' if self.commit else ''
        return f'''<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)} — Planification</title>
<link rel="stylesheet" href="{esc(self.rel(page, 'assets/theme.css'))}">
</head>
<body>
<header class="topbar">
  <a class="brand" href="{esc(self.rel(page, 'index.html'))}"><span>Just Another RPG Game</span> Planification</a>
  <nav>{items}</nav>
  <div class="links">{external}</div>
</header>
<div class="shell{' with-sidebar' if sidebar else ''}">
{aside}
<main>
{body}
</main>
</div>
<footer>Engendré depuis <code>Planning/</code> le {stamp}{commit}. Rien ici ne s'édite : la source est le dossier.</footer>
<script src="{esc(self.rel(page, 'assets/site.js'))}"></script>
</body>
</html>
'''

    def write(self, page, content):
        path = self.out / page
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8', newline='\n')

    # -- fragments ------------------------------------------------------------------------------

    def badge(self, etat):
        return f'<span class="badge etat-{esc(etat)}">{esc(ETATS.get(etat, etat))}</span>'

    def bar(self, lots):
        done, total = progress(lots)
        percent = round(100 * done / total) if total else 0
        return (f'<div class="bar" role="img" aria-label="{percent} % livré"><span style="width:{percent}%">'
                f'</span></div>'), percent

    def lot_rows(self, page, lots, with_version=False):
        rows = []
        for lot in lots:
            prereq = ' '.join(
                f'<a class="ref" href="{esc(self.rel(page, f"lots/{p.lower()}.html"))}">{esc(p)}</a>'
                for p in lot.prerequis) or '—'
            version = f'<td>{esc(lot.version)}</td>' if with_version else ''
            rows.append(
                f'<tr data-version="{esc(lot.version)}" data-filiere="{esc(lot.filiere)}" '
                f'data-etat="{esc(lot.etat)}">'
                f'<td class="num">{lot.rang or "—"}</td>'
                f'<td><a class="ref" href="{esc(self.rel(page, f"lots/{lot.slug}.html"))}">{esc(lot.id)}</a></td>'
                f'<td>{esc(lot.titre)}</td>{version}'
                f'<td><span class="filiere f-{esc(lot.filiere)}">{esc(FILIERES.get(lot.filiere, lot.filiere))}</span></td>'
                f'<td class="num">{esc(lot.taille)}</td><td>{prereq}</td>'
                f'<td class="num">{lot.debloque}</td><td>{self.badge(lot.etat)}</td></tr>')
        version_head = '<th>Version</th>' if with_version else ''
        return ('<div class="table-wrap"><table class="lots"><thead><tr><th>#</th><th>Lot</th><th>Objet</th>'
                f'{version_head}<th>Filière</th><th>Taille</th><th>Prérequis</th><th>Débloque</th>'
                f'<th>État</th></tr></thead><tbody>{"".join(rows)}</tbody></table></div>')

    def graph(self, page, lots):
        """Graphe des prérequis d'une version : colonnes par profondeur, liens en courbes."""
        ids = {lot.id for lot in lots}
        depth = {}

        def depth_of(lot, trail=()):
            if lot.id in depth:
                return depth[lot.id]
            inner = [self.planning.lots[p] for p in lot.prerequis if p in ids and p not in trail]
            depth[lot.id] = 1 + max((depth_of(p, trail + (lot.id,)) for p in inner), default=-1)
            return depth[lot.id]

        for lot in lots:
            depth_of(lot)
        columns = {}
        for lot in sorted(lots, key=lambda item: (item.rang or 999, lot_sort_key(item.id))):
            columns.setdefault(depth[lot.id], []).append(lot)
        box_w, box_h, gap_x, gap_y = 190, 46, 70, 16
        position = {}
        for column, members in columns.items():
            for row, lot in enumerate(members):
                position[lot.id] = (20 + column * (box_w + gap_x), 20 + row * (box_h + gap_y))
        width = 40 + len(columns) * (box_w + gap_x) - gap_x
        height = 40 + max(len(m) for m in columns.values()) * (box_h + gap_y) - gap_y
        edges, nodes = [], []
        for lot in lots:
            x2, y2 = position[lot.id]
            for parent in lot.prerequis:
                if parent in position:
                    x1, y1 = position[parent]
                    sx, sy, ex, ey = x1 + box_w, y1 + box_h / 2, x2, y2 + box_h / 2
                    mid = (sx + ex) / 2
                    edges.append(f'<path d="M{sx},{sy} C{mid},{sy} {mid},{ey} {ex},{ey}"/>')
        for lot in lots:
            x, y = position[lot.id]
            title = lot.titre if len(lot.titre) <= 30 else lot.titre[:29] + '…'
            nodes.append(
                f'<a href="{esc(self.rel(page, f"lots/{lot.slug}.html"))}"><g class="node etat-{esc(lot.etat)}">'
                f'<title>{esc(lot.id)} — {esc(lot.titre)}</title>'
                f'<rect x="{x}" y="{y}" width="{box_w}" height="{box_h}" rx="6"/>'
                f'<text x="{x + 10}" y="{y + 18}" class="id">{esc(lot.id)}</text>'
                f'<text x="{x + 10}" y="{y + 35}">{esc(title)}</text></g></a>')
        return (f'<div class="graph"><svg viewBox="0 0 {width} {height}" width="{width}" height="{height}" '
                f'role="img" aria-label="Graphe des prérequis"><g class="edges">{"".join(edges)}</g>'
                f'{"".join(nodes)}</svg></div>')

    # -- pages ----------------------------------------------------------------------------------

    def build_home(self):
        page = 'index.html'
        planning = self.planning
        all_lots = list(planning.lots.values())
        _, percent_total = self.bar(all_lots)
        upcoming = sorted((lot for lot in all_lots if lot.rang), key=lambda lot: lot.rang)[:8]
        current = next((v for v in planning.versions
                        if v.lots and any(lot.statut != 'livre' for lot in v.lots)), None)

        groups = []
        for version in planning.versions:
            if not groups or groups[-1][0] != version.referentiel:
                groups.append((version.referentiel, []))
            groups[-1][1].append(version)
        timeline = []
        for referentiel, versions in groups:
            cards = []
            for version in versions:
                bar, percent = self.bar(version.lots)
                meta = (f'{len(version.lots)} lots · {percent} %' if version.lots
                        else NATURES[version.nature])
                is_current = ' current' if version is current else ''
                cards.append(
                    f'<a class="vcard {esc(version.nature)}{is_current}" '
                    f'href="{esc(self.rel(page, f"versions/{version.slug}.html"))}">'
                    f'<span class="vid">{esc(version.id)}</span><strong>{esc(version.titre)}</strong>'
                    f'<p>{esc(version.resume)}</p>{bar if version.lots else ""}'
                    f'<span class="meta">{esc(meta)}</span></a>')
            timeline.append(f'<section class="milestone"><h3>Référentiel {esc(referentiel)}</h3>'
                            f'<div class="vcards">{"".join(cards)}</div></section>')

        now = ''
        if current:
            bar, percent = self.bar(current.lots)
            now = (f'<section class="panel now"><h2>Et maintenant ?</h2>'
                   f'<p>Version en cours : <a href="{esc(self.rel(page, f"versions/{current.slug}.html"))}">'
                   f'<strong>{esc(current.id)} — {esc(current.titre)}</strong></a> · {percent} % livré</p>{bar}'
                   f'<p class="hint">L\'ordre n\'est pas arbitré, il est <strong>calculé</strong> : parmi les lots '
                   f'dont tous les prérequis sont livrés, celui de la version la plus proche, puis celui qui '
                   f'en débloque le plus.</p>{self.lot_rows(page, upcoming, with_version=True)}</section>')

        delivered = sum(1 for lot in all_lots if lot.statut == 'livre')
        detailed = sum(1 for v in planning.versions if v.nature == 'detaillee')
        stats = (f'<div class="stats"><div><b>{len(planning.versions)}</b><span>versions jusqu\'à la 1.0.0</span></div>'
                 f'<div><b>{detailed}</b><span>planifiées en détail</span></div>'
                 f'<div><b>{len(all_lots)}</b><span>lots déclarés</span></div>'
                 f'<div><b>{delivered}</b><span>livrés ({percent_total} % du poids)</span></div></div>')
        readme = planning.root / 'README.md'
        intro = ''
        if readme.is_file():
            text = readme.read_text(encoding='utf-8')
            first = text.split('\n## ', 1)[0]
            first = '\n'.join(line for line in first.split('\n') if not line.startswith('# '))
            intro, _ = mini_markdown.render(first, self.link_rewriter('README.md', page))
        body = (f'<section class="hero"><h1>La trajectoire jusqu\'au bac à sable de Tanares</h1>'
                f'<div class="lede">{intro}</div>{stats}</section>{now}'
                f'<section><h2>Les versions</h2>{"".join(timeline)}</section>')
        self.write(page, self.layout(page, 'Trajectoire', body, 'home'))

    def build_versions(self):
        for version in self.planning.versions:
            page = f'versions/{version.slug}.html'
            lots = sorted(version.lots, key=lambda lot: (lot.rang == 0, lot.rang, lot_sort_key(lot.id)))
            parts = [f'<p class="crumb"><a href="{esc(self.rel(page, "index.html"))}">Trajectoire</a> › '
                     f'référentiel {esc(version.referentiel)}</p>'
                     f'<h1><span class="vid">{esc(version.id)}</span> {esc(version.titre)}</h1>'
                     f'<p class="lede">{esc(version.resume)}</p>'
                     f'<p><span class="badge nature-{esc(version.nature)}">{esc(NATURES[version.nature])}</span></p>']
            if lots:
                bar, percent = self.bar(lots)
                parts.append(f'<section class="panel"><h2>Avancement — {percent} %</h2>{bar}</section>')
            if version.objectifs:
                parts.append('<section><h2>Ce que la version rend jouable</h2><ul>'
                             + ''.join(f'<li>{mini_markdown.render_inline(o)}</li>' for o in version.objectifs)
                             + '</ul></section>')
            if version.criteres_de_sortie:
                parts.append('<section><h2>Critères de sortie</h2><ul class="checks">'
                             + ''.join(f'<li>{mini_markdown.render_inline(c)}</li>'
                                       for c in version.criteres_de_sortie) + '</ul></section>')
            folder = self.planning.root / 'versions' / version.dossier if version.dossier else None
            if folder and (folder / 'README.md').is_file():
                source_rel = (folder / 'README.md').relative_to(self.planning.root).as_posix()
                text = (folder / 'README.md').read_text(encoding='utf-8')
                text = '\n'.join(line for line in text.split('\n') if not line.startswith('# '))
                rendered, _ = mini_markdown.render(text, self.link_rewriter(source_rel, page))
                parts.append(f'<section class="prose">{rendered}</section>')
            if lots:
                parts.append(f'<section><h2>Graphe des prérequis</h2>{self.graph(page, lots)}</section>')
                parts.append(f'<section><h2>Les lots, dans l\'ordre calculé</h2>{self.lot_rows(page, lots)}</section>')
            if version.sources:
                parts.append('<section><h2>Sources du corpus</h2><ul>'
                             + ''.join(f'<li>{mini_markdown.render_inline(s)}</li>' for s in version.sources)
                             + '</ul></section>')
            sidebar = self.version_sidebar(page, version)
            self.write(page, self.layout(page, f'{version.id} — {version.titre}', ''.join(parts), 'home', sidebar))

    def version_sidebar(self, page, current):
        items = ''.join(
            f'<a href="{esc(self.rel(page, f"versions/{v.slug}.html"))}"'
            f'{" class=\"active\"" if v is current else ""}><span class="vid">{esc(v.id)}</span> {esc(v.titre)}</a>'
            for v in self.planning.versions)
        return f'<h4>Versions</h4>{items}'

    def build_lots(self):
        page = 'lots/index.html'
        lots = sorted(self.planning.lots.values(), key=lambda lot: (lot.rang == 0, lot.rang, lot_sort_key(lot.id)))
        versions = ''.join(f'<option>{esc(v.id)}</option>' for v in self.planning.versions if v.lots)
        filieres = ''.join(f'<option value="{esc(k)}">{esc(v)}</option>' for k, v in FILIERES.items())
        etats = ''.join(f'<option value="{esc(k)}">{esc(v)}</option>' for k, v in ETATS.items())
        filters = (f'<div class="filters" data-filter-table="lots">'
                   f'<input type="search" placeholder="Filtrer…" aria-label="Filtrer les lots">'
                   f'<select data-key="version"><option value="">Toutes les versions</option>{versions}</select>'
                   f'<select data-key="filiere"><option value="">Toutes les filières</option>{filieres}</select>'
                   f'<select data-key="etat"><option value="">Tous les états</option>{etats}</select></div>')
        body = (f'<h1>Tous les lots</h1><p class="lede">{len(lots)} lots déclarés. La colonne # est l\'ordre '
                f'calculé ; « débloque » compte les lots restants qui en dépendent, en cascade.</p>'
                f'{filters}{self.lot_rows(page, lots, with_version=True)}')
        self.write(page, self.layout(page, 'Lots', body, 'lots'))
        for lot in lots:
            self.build_lot(lot)

    def build_lot(self, lot):
        page = f'lots/{lot.slug}.html'
        version = self.planning.version(lot.version)
        source_rel = lot.path.relative_to(self.planning.root).as_posix()
        rewrite = self.link_rewriter(source_rel, page)
        corps, _ = mini_markdown.render(lot.corps, rewrite)

        def refs(ids):
            return ' '.join(f'<a class="ref" href="{esc(self.rel(page, f"lots/{i.lower()}.html"))}">{esc(i)}</a>'
                            for i in ids) or '—'

        dependants = sorted((other.id for other in self.planning.lots.values() if lot.id in other.prerequis),
                            key=lot_sort_key)

        def bullet(title, items, css=''):
            if not items:
                return ''
            return (f'<section><h2>{title}</h2><ul class="{css}">'
                    + ''.join(f'<li>{mini_markdown.render_inline(item, rewrite)}</li>' for item in items)
                    + '</ul></section>')

        maquettes = ''
        if lot.maquettes:
            figures = ''.join(
                f'<figure><a href="{esc(rewrite(m))}"><img src="{esc(rewrite(m))}" alt="" loading="lazy"></a>'
                f'<figcaption>{esc(posixpath.basename(m))}</figcaption></figure>' for m in lot.maquettes)
            maquettes = f'<section><h2>Maquettes</h2><div class="gallery">{figures}</div></section>'
        version_link = (f'<a href="{esc(self.rel(page, f"versions/{version.slug}.html"))}">{esc(version.id)} — '
                        f'{esc(version.titre)}</a>') if version else esc(lot.version)
        body = (f'<p class="crumb"><a href="{esc(self.rel(page, "lots/index.html"))}">Lots</a> › {version_link}</p>'
                f'<h1><span class="vid">{esc(lot.id)}</span> {esc(lot.titre)}</h1>'
                f'<p class="lede">{mini_markdown.render_inline(lot.resume, rewrite)}</p>'
                f'<dl class="facts"><div><dt>État</dt><dd>{self.badge(lot.etat)}</dd></div>'
                f'<div><dt>Filière</dt><dd><span class="filiere f-{esc(lot.filiere)}">'
                f'{esc(FILIERES.get(lot.filiere, lot.filiere))}</span></dd></div>'
                f'<div><dt>Taille</dt><dd>{esc(lot.taille)}</dd></div>'
                f'<div><dt>Ordre calculé</dt><dd>{lot.rang or "—"}</dd></div>'
                f'<div><dt>Prérequis</dt><dd>{refs(lot.prerequis)}</dd></div>'
                f'<div><dt>Débloque</dt><dd>{refs(dependants)}</dd></div>'
                f'<div><dt>Reprend</dt><dd>{esc(", ".join(lot.reprend)) or "—"}</dd></div></dl>'
                f'{bullet("Livrables", lot.livrables)}{bullet("Critères d\'acceptation", lot.criteres, "checks")}'
                f'{maquettes}<section class="prose">{corps}</section>{bullet("Sources du corpus", lot.sources)}'
                f'<p class="source">Source : <code>Planning/{esc(source_rel)}</code></p>')
        self.write(page, self.layout(page, f'{lot.id} — {lot.titre}', body, 'lots'))

    def build_sections(self):
        for folder, label in SECTIONS:
            root = self.planning.root / folder
            if not root.is_dir():
                continue
            entries = []  # (page, titre, profondeur)
            # Les fiches de lots ont leurs propres pages ; seules les tables des référentiels se rendent.
            sources = sorted(p for p in root.rglob('*')
                             if (p.suffix == '.md' or (p.suffix == '.toml' and folder == 'referentiels'))
                             and not (SKIPPED_DIRS | {'lots'}) & set(p.relative_to(self.planning.root).parts))
            sources.sort(key=lambda p: (p.parent.as_posix(), p.name != 'README.md', p.name))
            rendered = []
            for source in sources:
                source_rel = source.relative_to(self.planning.root).as_posix()
                page = source_rel[:-len(source.suffix)] + '.html'
                if source.name == 'README.md':
                    page = posixpath.join(posixpath.dirname(source_rel), 'index.html')
                if source.suffix == '.md':
                    text = source.read_text(encoding='utf-8')
                    content, headings = mini_markdown.render(text, self.link_rewriter(source_rel, page))
                    title = next((t for level, _, t in headings if level == 1), source.stem)
                    content = f'<article class="prose">{content}</article>'
                else:
                    title, content = self.render_table(source)
                depth = len(Path(source_rel).parts) - 2
                entries.append((page, title, depth))
                rendered.append((page, title, content))
            for page, title, content in rendered:
                sidebar = f'<h4>{esc(label)}</h4>' + ''.join(
                    f'<a class="depth-{depth}{" active" if target == page else ""}" '
                    f'href="{esc(self.rel(page, target))}">{esc(text)}</a>' for target, text, depth in entries)
                self.write(page, self.layout(page, title, content, folder, sidebar))

    def render_table(self, source):
        """Table TOML d'un référentiel : `[meta]` (titre, note, colonnes) puis des `[[ligne]]`."""
        data = tomllib.loads(source.read_text(encoding='utf-8'))
        meta = data.get('meta', {})
        columns = meta.get('colonnes') or sorted({key for row in data.get('ligne', []) for key in row})
        labels = meta.get('libelles', {})
        head = ''.join(f'<th>{esc(labels.get(c, c))}</th>' for c in columns)
        rows = ''.join(
            '<tr>' + ''.join(f'<td>{mini_markdown.render_inline(str(row.get(c, "")))}</td>' for c in columns)
            + '</tr>' for row in data.get('ligne', []))
        title = meta.get('titre', source.stem)
        note, _ = mini_markdown.render(meta.get('note', ''))
        content = (f'<h1>{esc(title)}</h1><div class="lede">{note}</div>'
                   f'<div class="filters" data-filter-table="data"><input type="search" placeholder="Filtrer…" '
                   f'aria-label="Filtrer la table"><span class="count">{len(data.get("ligne", []))} lignes</span></div>'
                   f'<div class="table-wrap"><table class="data"><thead><tr>{head}</tr></thead><tbody>{rows}'
                   f'</tbody></table></div><p class="source">Source : <code>Planning/'
                   f'{esc(source.relative_to(self.planning.root).as_posix())}</code></p>')
        return title, content

    def copy_static(self):
        assets = self.out / 'assets'
        assets.mkdir(parents=True, exist_ok=True)
        # `theme.css` importe les deux autres ; `reference.css` et `header.html` ne servent qu'à la
        # référence de code, et n'ont rien à faire ici.
        for name in ('theme.css', 'tokens.css', 'topbar.css', 'site.js'):
            shutil.copy2(CHARTE / name, assets / name)
        # Maquettes et illustrations : tout ce qui n'est ni texte source ni outil suit son chemin.
        for source in self.planning.root.rglob('*'):
            relative = source.relative_to(self.planning.root)
            if not source.is_file() or SKIPPED_DIRS & set(relative.parts):
                continue
            if source.suffix.lower() in ('.svg', '.png', '.jpg', '.jpeg', '.webp', '.html'):
                target = self.out / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)

    def write_summary(self):
        """Les mêmes chiffres, lisibles par un script (patron du site qualité)."""
        summary = {
            'versions': [{
                'id': v.id, 'titre': v.titre, 'nature': v.nature, 'lots': len(v.lots),
                'livres': sum(1 for lot in v.lots if lot.statut == 'livre'),
            } for v in self.planning.versions],
            'lots': [{
                'id': lot.id, 'titre': lot.titre, 'version': lot.version, 'etat': lot.etat,
                'rang': lot.rang, 'debloque': lot.debloque, 'prerequis': lot.prerequis,
            } for lot in sorted(self.planning.lots.values(), key=lambda lot: lot_sort_key(lot.id))],
        }
        self.write('summary.json', json.dumps(summary, ensure_ascii=False, indent=2) + '\n')

    def build(self):
        if self.out.exists():
            shutil.rmtree(self.out)
        self.copy_static()
        self.build_home()
        self.build_versions()
        self.build_lots()
        self.build_sections()
        self.write_summary()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--root', default=str(PLANNING_ROOT), help='dossier Planning/ à lire')
    parser.add_argument('--out', required=True, help='dossier du site engendré (remplacé en entier)')
    parser.add_argument('--docs-url', default='../', help='adresse du site de documentation')
    parser.add_argument('--commit', default='', help='commit publié, rappelé en pied de page')
    args = parser.parse_args(argv)
    try:
        planning = load_planning(args.root)
    except PlanningError as error:
        print(f'build_planning_site: {error}', file=sys.stderr)
        return 1
    Site(planning, args.out, args.docs_url, args.commit).build()
    print(f'Site de planification : {len(planning.versions)} versions, {len(planning.lots)} lots → {args.out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
