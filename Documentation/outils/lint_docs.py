#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Lint des pages de `Documentation/` : ce que le site rendrait de travers, dit avant qu'il ne le rende.

Règles :

1. aucune commande Doxygen ne reste dans une page (`@ref`, `@subpage`, `\\anchor`, `\\ref`) : les
   pages sont du Markdown nu, et Doxygen ne les lit plus ;
2. tout lien relatif désigne un fichier qui existe, et, s'il porte une ancre vers une page Markdown,
   une ancre que cette page déclare (titre, ou puce identifiée `- **EX-CBT-001** — …`) ;
3. toute image citée existe ;
4. toute page d'une partie est atteignable depuis son sommaire (`README.md`) : une page que rien
   ne cite n'apparaît dans aucun parcours de lecture ;
5. tout lot cité (`LOT-NN`, `LOT-NNN`, `LOT-EDITOR-NN`) existe : une fiche de `Planning/`, ou, pour
   un lot de l'ancienne feuille de route jamais démarré, une mention dans les archives.

Usage :
  python Documentation/outils/lint_docs.py
"""
import posixpath
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import build_docs_site as site_builder  # noqa: E402
import mini_markdown  # noqa: E402  (chemin ajouté par build_docs_site)

DOXYGEN_RE = re.compile(r'(?<![\w`])(?:[@\\](?:ref|subpage|page|section|copydoc)\b|\\anchor\b)')
LINK_RE = re.compile(r'!?\[[^\]]*\]\(([^)\s]+)\)')
LOT_RE = re.compile(r'\bLOT-(?:EDITOR-\d{2}|\d{2,3})\b')


def anchors_of(path, cache={}):  # noqa: B006 — cache de module voulu
    """Les ancres qu'une page déclare (titres, puces identifiées), mises en cache."""
    if path not in cache:
        text = path.read_text(encoding='utf-8')
        _, headings = mini_markdown.render(text)
        found = {anchor for _, anchor, _ in headings}
        found |= set(re.findall(r'^\s*[-*]\s+\*\*([A-Z]+(?:-[A-Z]+)*-\d+)\*\*', text, flags=re.M))
        cache[path] = found
    return cache[path]


def lint(root):
    """Applique les cinq règles aux pages de `root` ; retourne (erreurs, nombre de pages)."""
    root = Path(root)
    repo = root.parent
    site = site_builder.Site(root, root / 'generated' / 'unused')
    site.load()
    errors = []
    archives = ''
    for archive in (repo / 'Planning' / 'vision' / 'archives').glob('*.md'):
        archives += archive.read_text(encoding='utf-8')
    archived_lots = set(LOT_RE.findall(archives))

    for rel, page in sorted(site.pages.items()):
        source = root / rel
        prose = site_builder.strip_code(page.text)
        for match in DOXYGEN_RE.finditer(prose):
            errors.append(f'{rel}: commande Doxygen « {match.group(0)} » — les pages sont du Markdown nu')
        for target in LINK_RE.findall(prose):
            if '://' in target or target.startswith(('mailto:', '#')):
                if target.startswith('#') and target[1:] not in anchors_of(source):
                    errors.append(f'{rel}: ancre inconnue — {target}')
                continue
            path, _, anchor = target.partition('#')
            resolved = (source.parent / path).resolve()
            if resolved.is_relative_to(root / 'reference'):
                continue  # la référence de code : engendrée par Doxygen, posée là à la publication
            if not resolved.exists():
                errors.append(f'{rel}: lien mort — {target}')
            elif anchor and resolved.suffix == '.md' and anchor not in anchors_of(resolved):
                errors.append(f'{rel}: ancre inconnue — {target}')
        if not rel.startswith('CahierTest/'):
            for lot in sorted(set(LOT_RE.findall(page.text))):
                if lot not in site.lots and lot not in archived_lots:
                    errors.append(f'{rel}: {lot} cité, mais ni fiche dans Planning/ ni mention aux archives')

    for folder, _ in site_builder.SECTIONS:
        if f'{folder}/README.md' not in site.pages:
            continue
        reachable, stack = set(), [f'{folder}/README.md']
        while stack:
            current = stack.pop()
            if current in reachable or current not in site.pages:
                continue
            reachable.add(current)
            for target in site_builder.MD_LINK_RE.findall(site_builder.strip_code(site.pages[current].text)):
                path = target.partition('#')[0]
                if path.endswith('.md') and '://' not in path:
                    stack.append(posixpath.normpath(posixpath.join(posixpath.dirname(current), path)))
        for rel in sorted(site.pages):
            if rel.startswith(folder + '/') and rel not in reachable:
                errors.append(f'{rel}: page orpheline — rien ne la cite depuis {folder}/README.md')
    return errors, len(site.pages)


def main():
    errors, count = lint(site_builder.DOCS_ROOT)
    for error in errors:
        print(f'lint_docs: {error}', file=sys.stderr)
    if errors:
        print(f'lint_docs: {len(errors)} erreur(s)', file=sys.stderr)
        return 1
    print(f'Lint documentation : OK ({count} pages).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
