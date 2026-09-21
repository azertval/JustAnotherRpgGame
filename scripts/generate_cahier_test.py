#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Génère le cahier de test (`Documentation/CahierTest/`) depuis les blocs ``\\castest{...}`` du code.

Les blocs ``\\castest{...}`` restent la source de vérité unique (un seul endroit à maintenir, au
plus près du test qu'ils décrivent) ; ce script ne fait que les collecter et les réorganiser en
pages de Markdown nu, que le site de documentation rend (`Documentation/outils/build_docs_site.py`) :

- ``README.md`` : le mode d'emploi du cahier et sa synthèse — cas par domaine et par criticité ;
- une page par domaine de ``Source/Test/`` (``core-combat.md``, ``hmi-graphics.md``,
  ``integration.md``…), où chaque fichier de test est une section et chaque cas une fiche :
  identifiant GoogleTest, criticité, catégorie, emplacement, objet, étapes, résultat attendu.

Une page unique de 640 Ko en tableaux de quatre colonnes ne se lisait pas ; un cas par fiche, un
domaine par page, et le site filtre les fiches par texte et par criticité.

Les deux modes échouent si un test du dépôt n'a **pas** de bloc ``\\castest{}``
(``find_undocumented_tests``) : sans ce contrôle, un test jamais documenté n'apparaît d'aucun
côté de la comparaison ``--check``, qui le valide donc en silence.

Usage :
  python scripts/generate_cahier_test.py            # régénère Documentation/CahierTest/
  python scripts/generate_cahier_test.py --check     # vérifie que le dossier est à jour (CI)
"""
import os
import re
import sys

TEST_ROOT = 'Source/Test'
OUTPUT_DIR = 'Documentation/CahierTest'

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
    return text


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


CATEGORY_TITLES = {
    'Unit': 'Tests unitaires',
    'Integration': "Tests d'intégration",
    'Systeme': 'Tests système',
}
CRITICITES = ('Bloquant', 'Critique', 'Majeur', 'Mineur')


def domain_of(case):
    """(fichier de page, titre, catégorie) du domaine d'un cas, d'après son dossier de test."""
    relative_dir = os.path.relpath(os.path.dirname(case['path']), TEST_ROOT)
    parts = [part for part in relative_dir.replace('\\', '/').split('/') if part not in ('', '.')]
    category = parts[0] if parts else 'Unit'
    if category != 'Unit':
        return category.lower(), CATEGORY_TITLES.get(category, category), category
    inner = parts[1:3] or ['racine']
    return '-'.join(part.lower() for part in inner), ' · '.join(inner), category


def criticite_of(case):
    return next((c for c in CRITICITES if c.lower() in case.get('crit', '').lower()), 'Majeur')


def render_case(case):
    lines = [f"### {case['suite']}.{case['name']}", '']
    category = case.get('cat', '').strip()
    facts = ' · '.join(part for part in (criticite_of(case), category) if part)
    lines += [f"*{facts}* — `{case['path']}:{case['line']}`", '', case['title'], '']
    etapes = [re.sub(r'^\d+\.\s*', '', step.strip()) for step in case.get('etapes', '').split('<br/>')]
    etapes = [step for step in etapes if step]
    if etapes:
        lines += ['**Étapes**', ''] + [f'{number}. {step}' for number, step in enumerate(etapes, 1)] + ['']
    # Résultat attendu : les assertions GoogleTest réellement vérifiées par le test (extraites du
    # corps de la fonction), traduites en français — pas une phrase qui reformulerait l'objet.
    assertions = [clean_fragment(translate_assertion(a)) for a in case.get('assertions') or []]
    if not assertions and case.get('attendu'):
        assertions = [case['attendu']]
    lines += ['**Résultat attendu**', '']
    lines += [f'- {assertion}' for assertion in assertions] or ['- *(aucune assertion trouvée)*']
    lines.append('')
    return lines


def render_domain(title, category, cases):
    counts = {c: sum(1 for case in cases if criticite_of(case) == c) for c in CRITICITES}
    summary = ', '.join(f'{count} {name.lower()}{"s" if count > 1 else ""}'
                        for name, count in counts.items() if count)
    lines = [f'# {title}', '',
             f'{CATEGORY_TITLES.get(category, category)} — **{len(cases)} cas** ({summary}). '
             f'[Retour à la synthèse](README.md).', '']
    by_file = {}
    for case in cases:
        by_file.setdefault(os.path.basename(case['path']), []).append(case)
    for filename in sorted(by_file):
        lines += [f'## {filename}', '']
        for case in by_file[filename]:
            lines += render_case(case)
    return '\n'.join(lines).rstrip() + '\n'


def render_readme(domains):
    total = sum(len(cases) for _, _, cases in domains.values())
    lines = [
        '# Cahier de test', '',
        f'**{total} cas de test**, un par test automatisé du dépôt. Le cahier est **engendré** depuis les '
        'blocs `\\castest{…}` écrits au-dessus de chaque test par `scripts/generate_cahier_test.py` : il ne '
        's\'édite pas — on corrige le commentaire du test, puis on relance le script. La CI refuse un '
        'cahier périmé, et refuse un test sans bloc.', '',
        '## Lire une fiche', '',
        'Chaque cas porte l\'**identifiant GoogleTest** (`Suite.Nom`, retrouvable tel quel dans le code et '
        'dans le rapport `ctest`), sa **criticité**, sa **catégorie**, son **emplacement** '
        '(`fichier:ligne`), son objet en une phrase, ses **étapes**, et le **résultat attendu** — les '
        'assertions réellement vérifiées par le test, traduites en français.', '',
        '| Criticité | Ce qu\'un échec signifie |', '|---|---|',
        '| **Bloquant** | Le jeu ne démarre pas, corrompt une donnée ou fausse une règle : rien ne se livre. |',
        '| **Critique** | Une fonction centrale rend un résultat faux ; la version ne sort pas en l\'état. |',
        '| **Majeur** | Un comportement attendu manque ou dévie, avec contournement possible. |',
        '| **Mineur** | Un confort, un message, une valeur par défaut. |', '',
        '## Synthèse par domaine', '',
        '| Domaine | Type | Cas | ' + ' | '.join(CRITICITES) + ' |',
        '|---|---|---|' + '---|' * len(CRITICITES),
    ]
    totals = {c: 0 for c in CRITICITES}
    for slug in sorted(domains, key=lambda key: (list(CATEGORY_TITLES).index(domains[key][1])
                                                 if domains[key][1] in CATEGORY_TITLES else 9, key)):
        title, category, cases = domains[slug]
        counts = [sum(1 for case in cases if criticite_of(case) == c) for c in CRITICITES]
        for name, count in zip(CRITICITES, counts):
            totals[name] += count
        lines.append(f'| [{title}]({slug}.md) | {CATEGORY_TITLES.get(category, category)} | {len(cases)} | '
                     + ' | '.join(str(count or '—') for count in counts) + ' |')
    lines.append(f'| **Total** | | **{total}** | ' + ' | '.join(f'**{totals[c]}**' for c in CRITICITES) + ' |')
    lines += ['', '## Lancer les tests', '',
              '```', 'powershell -File scripts/build.ps1      # compile (préréglage ninja)',
              'ctest --preset ninja                     # exécute tous les cas',
              'ctest --preset ninja -R AttackTest       # une suite', '```', '',
              'Un cas qui échoue se retrouve ici par son identifiant (la recherche du site le trouve), '
              'et dans le code par l\'emplacement que donne sa fiche.', '']
    return '\n'.join(lines)


def render_all(cases):
    """Le contenu de chaque fichier du cahier : {nom de fichier: texte}."""
    domains = {}
    for case in cases:
        slug, title, category = domain_of(case)
        domains.setdefault(slug, (title, category, []))[2].append(case)
    files = {'README.md': render_readme(domains)}
    for slug, (title, category, items) in domains.items():
        files[f'{slug}.md'] = render_domain(title, category, items)
    return files


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

    files = render_all(cases)
    existing = set()
    if os.path.isdir(OUTPUT_DIR):
        existing = {name for name in os.listdir(OUTPUT_DIR) if name.endswith('.md')}

    if '--check' in sys.argv:
        stale = sorted(existing - set(files))
        for name, rendered in files.items():
            path = os.path.join(OUTPUT_DIR, name)
            current = None
            if os.path.isfile(path):
                with open(path, encoding='utf-8') as handle:
                    current = handle.read()
            if current != rendered:
                stale.append(name)
        if stale:
            print(f'{OUTPUT_DIR} n\'est pas a jour ({", ".join(sorted(set(stale)))}) : relancer '
                  '"python scripts/generate_cahier_test.py".', file=sys.stderr)
            return 1
        print(f'Cahier de test a jour ({len(cases)} cas de test, {len(files)} pages).')
        return 0

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    for name in existing - set(files):
        os.remove(os.path.join(OUTPUT_DIR, name))
    for name, rendered in files.items():
        with open(os.path.join(OUTPUT_DIR, name), 'w', encoding='utf-8', newline='\n') as handle:
            handle.write(rendered)
    print(f'{OUTPUT_DIR} regenere ({len(cases)} cas de test, {len(files)} pages).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
