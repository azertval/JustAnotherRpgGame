#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Rendu Markdown → HTML, réduit à ce que les pages de `Planning/` emploient.

L'environnement des scripts n'embarque aucune bibliothèque Markdown (`pyproject.toml` : zéro
dépendance d'exécution), et le site de planification n'en vaut pas une. Sont rendus : titres,
paragraphes, listes à puces et numérotées (imbriquées par indentation), cases à cocher, tableaux,
blocs de code, citations, filets, et en ligne : gras, italique, code, liens, images. Rien d'autre :
une construction non reconnue sort en paragraphe, lisible, jamais en erreur.
"""
import html
import re

INLINE_CODE_RE = re.compile(r'`([^`]+)`')
IMAGE_RE = re.compile(r'!\[([^\]]*)\]\(([^)\s]+)\)')
LINK_RE = re.compile(r'\[([^\]]+)\]\(([^)\s]+)\)')
BOLD_RE = re.compile(r'\*\*(.+?)\*\*')
ITALIC_RE = re.compile(r'(?<![\*\w])\*(?!\s)(.+?)(?<!\s)\*(?![\*\w])')
LIST_RE = re.compile(r'^(\s*)([-*]|\d+\.)\s+(.*)$')
HEADING_RE = re.compile(r'^(#{1,6})\s+(.*?)\s*(?:\{#([\w-]+)\})?\s*$')
TABLE_RULE_RE = re.compile(r'^\s*\|?\s*:?-{2,}:?\s*(\|\s*:?-{2,}:?\s*)*\|?\s*$')


def slugify(text):
    text = re.sub(r'<[^>]+>', '', text).lower()
    text = re.sub(r'[^\w\s-]', '', text, flags=re.UNICODE)
    return re.sub(r'[\s_]+', '-', text).strip('-')


def render_inline(text, link=None):
    """`link` réécrit la cible d'un lien ou d'une image (les `.md` deviennent des `.html`)."""
    link = link or (lambda target: target)
    stash = []

    def keep(fragment):
        stash.append(fragment)
        return f'\x00{len(stash) - 1}\x00'

    text = INLINE_CODE_RE.sub(lambda m: keep(f'<code>{html.escape(m.group(1))}</code>'), text)
    text = html.escape(text, quote=False)
    text = IMAGE_RE.sub(
        lambda m: keep(f'<img src="{html.escape(link(m.group(2)))}" alt="{m.group(1)}" loading="lazy">'), text)
    text = LINK_RE.sub(lambda m: f'<a href="{html.escape(link(m.group(2)))}">{m.group(1)}</a>', text)
    text = BOLD_RE.sub(r'<strong>\1</strong>', text)
    text = ITALIC_RE.sub(r'<em>\1</em>', text)
    return re.sub(r'\x00(\d+)\x00', lambda m: stash[int(m.group(1))], text)


def split_row(line):
    line = line.strip()
    if line.startswith('|'):
        line = line[1:]
    if line.endswith('|'):
        line = line[:-1]
    return [cell.strip() for cell in re.split(r'(?<!\\)\|', line)]


def render(text, link=None):
    """HTML du document et liste de ses titres `(niveau, ancre, texte)`."""
    lines = text.replace('\r\n', '\n').split('\n')
    out, headings = [], []
    index = 0

    def inline(fragment):
        return render_inline(fragment, link)

    def render_list(start, indent):
        """Rend la liste qui commence à `start` ; renvoie (html, index suivant)."""
        ordered = bool(re.match(r'\d+\.', LIST_RE.match(lines[start]).group(2)))
        tag = 'ol' if ordered else 'ul'
        items, i = [], start
        while i < len(lines):
            match = LIST_RE.match(lines[i])
            if not match or len(match.group(1)) < indent:
                break
            if len(match.group(1)) > indent:
                nested, i = render_list(i, len(match.group(1)))
                items[-1] += nested
                continue
            content = match.group(3)
            i += 1
            # Lignes de continuation : indentées, non vides, et pas une nouvelle puce.
            while i < len(lines) and lines[i].strip() and not LIST_RE.match(lines[i]) \
                    and lines[i].startswith(' '):
                content += ' ' + lines[i].strip()
                i += 1
            box = re.match(r'\[( |x|X)\]\s+(.*)', content)
            if box:
                checked = ' checked' if box.group(1) != ' ' else ''
                content = f'<input type="checkbox" disabled{checked}> ' + inline(box.group(2))
                items.append(f'<li class="task">{content}')
            else:
                items.append('<li>' + inline(content))
        return f'<{tag}>' + ''.join(item + '</li>' for item in items) + f'</{tag}>', i

    while index < len(lines):
        line = lines[index]
        stripped = line.strip()
        if not stripped:
            index += 1
            continue
        if stripped.startswith('```'):
            index += 1
            block = []
            while index < len(lines) and not lines[index].strip().startswith('```'):
                block.append(lines[index])
                index += 1
            index += 1
            out.append('<pre><code>' + html.escape('\n'.join(block)) + '</code></pre>')
            continue
        heading = HEADING_RE.match(line)
        if heading:
            level = len(heading.group(1))
            body = inline(heading.group(2))
            anchor = heading.group(3) or slugify(body)
            headings.append((level, anchor, re.sub(r'<[^>]+>', '', body)))
            out.append(f'<h{level} id="{anchor}">{body}</h{level}>')
            index += 1
            continue
        if re.fullmatch(r'(-{3,}|\*{3,}|_{3,})', stripped):
            out.append('<hr>')
            index += 1
            continue
        if stripped.startswith('>'):
            block = []
            while index < len(lines) and lines[index].strip().startswith('>'):
                block.append(re.sub(r'^\s*>\s?', '', lines[index]))
                index += 1
            inner, _ = render('\n'.join(block), link)
            out.append(f'<blockquote>{inner}</blockquote>')
            continue
        if '|' in line and index + 1 < len(lines) and TABLE_RULE_RE.match(lines[index + 1]):
            header = split_row(line)
            index += 2
            rows = []
            while index < len(lines) and '|' in lines[index] and lines[index].strip():
                rows.append(split_row(lines[index]))
                index += 1
            head = ''.join(f'<th>{inline(cell)}</th>' for cell in header)
            body = ''.join(
                '<tr>' + ''.join(f'<td>{inline(cell)}</td>' for cell in row) + '</tr>' for row in rows)
            out.append(f'<div class="table-wrap"><table><thead><tr>{head}</tr></thead>'
                       f'<tbody>{body}</tbody></table></div>')
            continue
        match = LIST_RE.match(line)
        if match:
            rendered, index = render_list(index, len(match.group(1)))
            out.append(rendered)
            continue
        block = []
        while index < len(lines) and lines[index].strip() and not HEADING_RE.match(lines[index]) \
                and not LIST_RE.match(lines[index]) and not lines[index].strip().startswith(('```', '>')):
            block.append(lines[index].strip())
            index += 1
        out.append('<p>' + inline(' '.join(block)) + '</p>')
    return '\n'.join(out), headings
