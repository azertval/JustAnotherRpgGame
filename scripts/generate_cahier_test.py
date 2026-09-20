#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Génère Documentation/CahierTest.md à partir des blocs ``\\castest{...}`` du code de test.

Le Cahier de test agrégeait auparavant tous les cas de test sur une seule page plate (mécanisme
Doxygen ``\\xrefitem``, sans aucune section) — illisible dès que le nombre de tests dépasse
quelques dizaines (321 aujourd'hui). Ce script reconstruit une page structurée, avec des titres
Markdown reflétant l'arborescence réelle de ``Source/Test/`` (Unitaire/Intégration/Système, puis
module), que Doxygen transforme en arbre de navigation repliable (comme toute autre page du site).

Les blocs ``\\castest{...}`` restent la source de vérité unique (un seul endroit à maintenir, au
plus près du test qu'ils décrivent) ; ce script ne fait que les collecter et les réorganiser.

Les deux modes échouent si un test du dépôt n'a **pas** de bloc ``\\castest{}``
(``find_undocumented_tests``) : sans ce contrôle, un test jamais documenté n'apparaît d'aucun
côté de la comparaison ``--check``, qui le valide donc en silence.

Usage :
  python scripts/generate_cahier_test.py            # régénère Documentation/CahierTest.md
  python scripts/generate_cahier_test.py --check     # vérifie que le fichier est à jour (CI)
"""
import os
import re
import sys

TEST_ROOT = 'Source/Test'
OUTPUT_PATH = 'Documentation/CahierTest.md'

# Déclaration d'un test GoogleTest, sous ses trois formes. `TEST_F`/`TEST_P` prennent en premier
# argument la *fixture*, dont GoogleTest tire justement le nom de la suite : le premier groupe
# reste donc la suite dans les trois cas. Ne reconnaître que `TEST(` — comme à l'origine — faisait
# disparaître du cahier, sans le moindre message, les cas de test attachés à une fixture (tout
# `test_image_encode.cpp` par exemple), alors que leur bloc `\castest{}` était bien écrit.
TEST_DECLARATION = r'TEST(?:_F|_P)?\(\s*(?P<suite>[^,]+),\s*(?P<name>[^)]+)\)'

# Un bloc castest s'étend de `\castest{` jusqu'à la fermeture `}` puis `*/`, immédiatement suivi
# de la déclaration du test : les deux sont capturés ensemble pour associer chaque cas de test à
# son emplacement dans le code.
CASTEST_RE = re.compile(
    r'\\castest\{(?P<content>.*?)\}\s*\n\s*\*/\s*\n' + TEST_DECLARATION,
    re.DOTALL)

# Même déclaration, en début de ligne, pour recenser **tous** les tests du dépôt et repérer ceux
# qui n'ont pas de bloc `\castest{}` (cf. find_undocumented_tests).
TEST_DECLARATION_RE = re.compile(r'^' + TEST_DECLARATION, re.MULTILINE)

FIELD_RE = re.compile(r'\\t(cat|crit|etapes|attendu)\s+')
TAG_RE = re.compile(r'</?b>')
BR_RE = re.compile(r'<br\s*/?>')

# Assertions GoogleTest : c'est l'état réellement vérifié par le test, plus fiable et plus
# concis qu'une phrase de description (qui ne fait souvent que reformuler le brief).
ASSERTION_RE = re.compile(r'\b(?:EXPECT|ASSERT)_[A-Z_]+\s*\(')


def skip_literal_or_comment(text, index):
    """Si `text[index:]` commence par une chaîne/caractère/commentaire C++, renvoie l'index juste
    après cette construction (pour ne pas compter les accolades/parenthèses qu'elle contient) ;
    sinon `None`."""
    if text.startswith('//', index):
        end = text.find('\n', index)
        return len(text) if end == -1 else end + 1
    if text.startswith('/*', index):
        end = text.find('*/', index + 2)
        return len(text) if end == -1 else end + 2
    if text[index] in ('"', "'"):
        quote = text[index]
        cursor = index + 1
        while cursor < len(text) and text[cursor] != quote:
            cursor += 2 if text[cursor] == '\\' else 1
        return cursor + 1
    return None


def find_matching(text, open_index, open_char, close_char):
    """Renvoie l'index de `close_char` refermant `open_char` situé en `open_index` (profondeur),
    en ignorant ce qui apparaît dans une chaîne/un commentaire. -1 si jamais refermé."""
    depth = 0
    cursor = open_index
    while cursor < len(text):
        skip_to = skip_literal_or_comment(text, cursor)
        if skip_to is not None:
            cursor = skip_to
            continue
        if text[cursor] == open_char:
            depth += 1
        elif text[cursor] == close_char:
            depth -= 1
            if depth == 0:
                return cursor
        cursor += 1
    return -1


def extract_assertions(body_text):
    """Liste les appels `EXPECT_*`/`ASSERT_*` (texte brut, aplati) trouvés dans le corps d'un test."""
    assertions = []
    for match in ASSERTION_RE.finditer(body_text):
        open_paren = match.end() - 1
        close_paren = find_matching(body_text, open_paren, '(', ')')
        if close_paren == -1:
            continue
        call_text = body_text[match.start():close_paren + 1]
        assertions.append(re.sub(r'\s+', ' ', call_text).strip())
    return assertions


def split_top_level_args(text):
    """Découpe les arguments d'un appel de fonction par les virgules de premier niveau
    (celles qui ne sont pas à l'intérieur d'une parenthèse/accolade/crochet imbriqué)."""
    parts = []
    depth = 0
    current = []
    index = 0
    while index < len(text):
        skip_to = skip_literal_or_comment(text, index)
        if skip_to is not None:
            current.append(text[index:skip_to])
            index = skip_to
            continue
        char = text[index]
        if char in '([{':
            depth += 1
            current.append(char)
        elif char in ')]}':
            depth -= 1
            current.append(char)
        elif char == ',' and depth == 0:
            parts.append(''.join(current).strip())
            current = []
        else:
            current.append(char)
        index += 1
    if current:
        parts.append(''.join(current).strip())
    return parts


ASSERTION_CALL_RE = re.compile(r'^(?:EXPECT|ASSERT)_([A-Z_]+)\((.*)\)$')


def code(fragment):
    return f'`{fragment}`' if fragment else '`?`'


# Traduction en français de l'intention de chaque macro GoogleTest, phrasée comme le serait un
# commentaire écrit à la main au-dessus de l'assertion (« Vérifie que... »), pas une transcription
# mécanique de l'opérateur de comparaison.
ASSERTION_TEMPLATES = {
    'EQ': lambda a: f'Vérifie que {code(a[0])} vaut {code(a[1])}.',
    'NE': lambda a: f'Vérifie que {code(a[0])} diffère de {code(a[1])}.',
    'TRUE': lambda a: f'Vérifie que {code(a[0])} est vrai.',
    'FALSE': lambda a: f'Vérifie que {code(a[0])} est faux.',
    'GT': lambda a: f'Vérifie que {code(a[0])} est strictement supérieur à {code(a[1])}.',
    'LT': lambda a: f'Vérifie que {code(a[0])} est strictement inférieur à {code(a[1])}.',
    'GE': lambda a: f'Vérifie que {code(a[0])} est supérieur ou égal à {code(a[1])}.',
    'LE': lambda a: f'Vérifie que {code(a[0])} est inférieur ou égal à {code(a[1])}.',
    'NEAR': lambda a: f'Vérifie que {code(a[0])} vaut {code(a[1])}, à '
                      f'{code(a[2]) if len(a) > 2 else "?"} près.',
    'FLOAT_EQ': lambda a: f'Vérifie que {code(a[0])} vaut {code(a[1])} (comparaison flottante).',
    'DOUBLE_EQ': lambda a: f'Vérifie que {code(a[0])} vaut {code(a[1])} (comparaison flottante).',
    'THROW': lambda a: f'Vérifie que l\'opération lève bien une exception '
                       f'{code(a[1]) if len(a) > 1 else "?"}.',
}


def translate_assertion(call_text):
    """Traduit un appel `EXPECT_*`/`ASSERT_*` en une phrase française décrivant l'état vérifié.

    Repli sur le code brut (entre guillemets) si la macro n'est pas reconnue ou si l'analyse des
    arguments échoue — mieux vaut du code affiché que rien, mais cela ne devrait pas arriver pour
    les macros de comparaison/booléennes courantes déjà couvertes ci-dessus.
    """
    match = ASSERTION_CALL_RE.match(call_text)
    if not match:
        return code(call_text)
    kind, args_text = match.groups()
    template = ASSERTION_TEMPLATES.get(kind)
    if template is None:
        return code(call_text)
    try:
        return template(split_top_level_args(args_text))
    except IndexError:
        return code(call_text)


def extract_test_body(content, search_from):
    """Renvoie le corps `{ ... }` du test dont la déclaration se termine juste avant `search_from`."""
    body_start = content.find('{', search_from)
    if body_start == -1:
        return ''
    body_end = find_matching(content, body_start, '{', '}')
    if body_end == -1:
        return ''
    return content[body_start:body_end]


def unwrap_comment_lines(text):
    """Recolle les lignes d'un commentaire Doxygen (` * suite...`) en un texte continu."""
    return re.sub(r'\n\s*\*\s?', ' ', text)


def clean_fragment(text, keep_breaks=False):
    text = TAG_RE.sub('', text)
    text = BR_RE.sub('<br/>' if keep_breaks else ' ', text)
    text = re.sub(r'[ \t]+', ' ', text).strip()
    text = re.sub(r'(<br/>\s*)+$', '', text)  # pas de saut de ligne final superflu
    # Un `\` littéral (ex. littéral C++ `L'\xE9'`) serait lu par Doxygen comme le début d'une
    # commande (`\xE9` -> "commande inconnue", erreur bloquante avec WARN_AS_ERROR) : échapper
    # AVANT le pipe, pour ne pas ré-échapper le `\` que l'échappement du pipe introduirait.
    text = text.replace('\\', '\\\\')
    # Un `|` littéral casserait la cellule du tableau Markdown.
    return text.replace('|', '\\|')


def parse_castest_content(raw_content):
    """Découpe un bloc castest nettoyé en (titre, categorie, criticite, etapes, attendu)."""
    flat = unwrap_comment_lines(raw_content)
    pieces = FIELD_RE.split(flat)
    # pieces alterne : [avant_premier_marqueur, marqueur1, texte1, marqueur2, texte2, ...]
    fields = {'title': clean_fragment(pieces[0])}
    for index in range(1, len(pieces), 2):
        keep_breaks = pieces[index] == 'etapes'
        fields[pieces[index]] = clean_fragment(pieces[index + 1], keep_breaks=keep_breaks)
    return fields


def collect_cases(root):
    """Retourne la liste des cas de test, chacun un dict (chemin, ligne, suite, nom, champs...)."""
    cases = []
    for dirpath, _dirnames, filenames in sorted(os.walk(root)):
        for filename in sorted(filenames):
            if not filename.startswith('test_') or not filename.endswith('.cpp'):
                continue
            path = os.path.join(dirpath, filename)
            with open(path, encoding='utf-8') as handle:
                content = handle.read()
            for match in CASTEST_RE.finditer(content):
                line = content.count('\n', 0, match.start()) + 1
                fields = parse_castest_content(match.group('content'))
                body = extract_test_body(content, match.end())
                assertions = extract_assertions(body)
                cases.append({
                    'path': path.replace('\\', '/'),
                    'line': line,
                    'suite': match.group('suite').strip(),
                    'name': match.group('name').strip(),
                    'assertions': assertions,
                    **fields,
                })
    return cases


def find_undocumented_tests(root):
    """Liste les tests (chemin, ligne, suite, nom) dépourvus de bloc ``\\castest{}``.

    Le mode ``--check`` compare le fichier généré au résultat du script : il détecte une
    régénération oubliée, mais **pas** un test jamais documenté — un test sans bloc n'apparaît
    simplement dans aucun des deux côtés de la comparaison. Le cahier a ainsi pu perdre 15 % des
    tests sans qu'aucun garde-fou ne bronche. Ce contrôle ferme le trou : le cahier décrit soit
    tous les tests, soit rien.
    """
    undocumented = []
    for dirpath, _dirnames, filenames in sorted(os.walk(root)):
        for filename in sorted(filenames):
            if not filename.startswith('test_') or not filename.endswith('.cpp'):
                continue
            path = os.path.join(dirpath, filename)
            with open(path, encoding='utf-8') as handle:
                content = handle.read()
            # Les blocs castest sont repérés par la position de la déclaration qu'ils précèdent :
            # un test documenté est un test dont la déclaration termine un bloc.
            documented = {match.start('suite') for match in CASTEST_RE.finditer(content)}
            for match in TEST_DECLARATION_RE.finditer(content):
                if match.start('suite') in documented:
                    continue
                undocumented.append({
                    'path': path.replace('\\', '/'),
                    'line': content.count('\n', 0, match.start()) + 1,
                    'suite': match.group('suite').strip(),
                    'name': match.group('name').strip(),
                })
    return undocumented


def group_key(relative_dir):
    """Éclate un chemin de dossier relatif à TEST_ROOT en (categorie, sous-dossiers...)."""
    parts = [part for part in relative_dir.replace('\\', '/').split('/') if part not in ('', '.')]
    return tuple(parts)


CATEGORY_TITLES = {
    'Unit': 'Tests unitaires',
    'Integration': "Tests d'intégration",
    'Systeme': 'Tests système',
}


TABLE_HEADER = ('| Titre (criticité) | Brief | Étapes | Résultat attendu |\n'
                '|---|---|---|---|')


def render_row(case):
    # Colonne 1 : l'identifiant GoogleTest (retrouvable tel quel dans le code / le rapport ctest)
    # et la criticité ; la référence fichier:ligne en petit, pour remonter au test en un clic d'œil.
    title_cell = (f"**{case['suite']}.{case['name']}** ({case.get('crit', '?')})"
                  f"<br/><sub>`{case['path']}:{case['line']}`</sub>")
    brief = case['title']
    etapes = case.get('etapes', '')
    # État attendu : les assertions GoogleTest réellement vérifiées par le test (extraites du
    # corps de la fonction), traduites en français — pas une phrase qui ne ferait souvent que
    # reformuler le brief, pas du code brut non plus.
    assertions = case.get('assertions') or []
    if assertions:
        attendu = '<br/>'.join(clean_fragment(translate_assertion(a)) for a in assertions)
    else:
        attendu = case.get('attendu') or '*(aucune assertion trouvée)*'
    return f"| {title_cell} | {brief} | {etapes} | {attendu} |"


def render_table(cases):
    return '\n'.join([TABLE_HEADER] + [render_row(case) for case in cases])


def render_markdown(cases):
    by_category = {}
    for case in cases:
        relative_dir = os.path.relpath(os.path.dirname(case['path']), TEST_ROOT)
        key = group_key(relative_dir)
        by_category.setdefault(key[0], {}).setdefault(key[1:], []).append(case)

    total = len(cases)
    lines = [
        '# Cahier de test {#cahiertest}',
        '',
        f'**{total} cas de test**, générés depuis les blocs `\\castest{{...}}` du code par '
        '`scripts/generate_cahier_test.py` (ne pas éditer directement — modifier le commentaire '
        'du test concerné puis relancer le script). Organisés ici selon l\'arborescence de '
        '`Source/Test/` pour rester lisibles page par page.',
        '',
    ]

    for category in ('Unit', 'Integration', 'Systeme'):
        if category not in by_category:
            continue
        subgroups = by_category[category]
        category_total = sum(len(items) for items in subgroups.values())
        lines.append(f'## {CATEGORY_TITLES[category]} ({category_total})')
        lines.append('')

        if category == 'Unit':
            # Deux niveaux : namespace racine (Core/HMI) puis module (Ecs, Editor, ...).
            top_levels = {}
            for subpath, items in subgroups.items():
                top = subpath[0] if subpath else '(racine)'
                top_levels.setdefault(top, {}).setdefault(subpath[1:], []).extend(items)
            for top in sorted(top_levels):
                lines.append(f'### {top}')
                lines.append('')
                modules = top_levels[top]
                for module_path in sorted(modules, key=lambda p: p or ('',)):
                    module_name = module_path[0] if module_path else None
                    items = modules[module_path]
                    if module_name:
                        lines.append(f'#### {module_name} ({len(items)})')
                        lines.append('')
                    by_file = {}
                    for case in items:
                        by_file.setdefault(os.path.basename(case['path']), []).append(case)
                    for filename in sorted(by_file):
                        lines.append(f'**`{filename}`**')
                        lines.append('')
                        lines.append(render_table(by_file[filename]))
                        lines.append('')
        else:
            # Integration/Systeme : pas de sous-dossiers, un groupe par fichier de test.
            by_file = {}
            for items in subgroups.values():
                for case in items:
                    by_file.setdefault(os.path.basename(case['path']), []).append(case)
            for filename in sorted(by_file):
                items = by_file[filename]
                subtitle = items[0].get('cat', filename).split('·')[-1].strip()
                lines.append(f'### {subtitle} — `{filename}` ({len(items)})')
                lines.append('')
                lines.append(render_table(items))
                lines.append('')

    return '\n'.join(lines).rstrip() + '\n'


def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(repo_root)

    cases = collect_cases(TEST_ROOT)
    if not cases:
        print('Aucun bloc \\castest trouve : verifier le chemin TEST_ROOT.', file=sys.stderr)
        return 1

    undocumented = find_undocumented_tests(TEST_ROOT)
    if undocumented:
        print(f'{len(undocumented)} test(s) sans bloc \\castest : ils seraient absents du cahier.',
              file=sys.stderr)
        for test in undocumented:
            print(f"  - {test['suite']}.{test['name']} ({test['path']}:{test['line']})",
                  file=sys.stderr)
        print('Documenter chaque test au-dessus de sa declaration, puis relancer.',
              file=sys.stderr)
        return 1

    rendered = render_markdown(cases)

    if '--check' in sys.argv:
        with open(OUTPUT_PATH, encoding='utf-8') as handle:
            current = handle.read()
        if current != rendered:
            print(f'{OUTPUT_PATH} n\'est pas a jour : relancer '
                  '"python scripts/generate_cahier_test.py".', file=sys.stderr)
            return 1
        print(f'Cahier de test a jour ({len(cases)} cas de test).')
        return 0

    with open(OUTPUT_PATH, 'w', encoding='utf-8', newline='\n') as handle:
        handle.write(rendered)
    print(f'{OUTPUT_PATH} regenere ({len(cases)} cas de test).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
