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
  identifiant GoogleTest, criticité, catégorie, emplacement, objet, étapes, résultat attendu,
  et les exigences ``EX-…`` que le test cite ;
- ``couverture-exigences.md`` : la **matrice de traçabilité** exigence → cas de test, tirée des
  déclarations de ``Documentation/Specification/`` et des citations relevées dans les tests. Une
  exigence en vigueur qu'aucun test ne cite y paraît telle quelle : le cahier dit ce qui est tenu,
  et la matrice dit ce qui ne l'est pas encore.

Une page est écrite **à la main** et jamais engendrée : ``recette-manuelle.md``, les contrôles
qu'aucun test automatisé ne remplace (``HAND_WRITTEN_PAGES``). Le script la laisse en place et la
cite depuis le ``README.md``.

Une page unique de 640 Ko en tableaux de quatre colonnes ne se lisait pas ; un cas par fiche, un
domaine par page, et le site filtre les fiches par texte et par criticité.

Les deux modes échouent si un test du dépôt n'a **pas** de bloc ``\\castest{}``
(``find_undocumented_tests``) : sans ce contrôle, un test jamais documenté n'apparaît d'aucun
côté de la comparaison ``--check``, qui le valide donc en silence.

Usage :
  python scripts/docs/generate_cahier_test.py            # régénère Documentation/CahierTest/
  python scripts/docs/generate_cahier_test.py --check     # vérifie que le dossier est à jour (CI)
"""
import os
import re
import sys

sys.path.insert(0, os.path.join('Planning', 'outils'))

import mini_markdown  # noqa: E402  (chemin ajoute juste au-dessus)

TEST_ROOT = 'Source/Test'
OUTPUT_DIR = 'Documentation/CahierTest'
SPECIFICATION_DIR = 'Documentation/Specification'

# Pages du cahier écrites à la main : ni engendrées, ni supprimées, citées depuis le README.
HAND_WRITTEN_PAGES = {'recette-manuelle.md': 'Recette manuelle'}

# Une exigence, telle que la déclare une spécification (même forme que le moteur du site) et telle
# qu'un test la cite (en clair dans son commentaire ou son corps).
EXIGENCE_RE = re.compile(r'EX-[A-Z]+-\d{3}')
EXIGENCE_DECL_RE = re.compile(r'^\s*[-*]\s+\*\*(EX-[A-Z]+-\d{3})\*\*\s*(?:—|-)?\s*(.*)$')

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
                    'exigences': cited_exigences(content, match.start(), body),
                    **fields,
                })
    return cases


def cited_exigences(content, castest_start, body):
    """Les exigences que cite un cas : dans le commentaire qui porte son bloc (le `@brief` compris)
    et dans son corps. Le commentaire s'étend du dernier `/**` qui précède le bloc au bloc lui-même ;
    ce qui précède ce commentaire appartient au test d'avant."""
    comment_start = content.rfind('/**', 0, castest_start)
    comment = content[comment_start if comment_start != -1 else 0:castest_start]
    return sorted(set(EXIGENCE_RE.findall(comment)) | set(EXIGENCE_RE.findall(body)))


def collect_exigences(spec_dir):
    """Les exigences déclarées par les spécifications : {id: (page, titre de la page, retirée)}.

    Une exigence retirée reste déclarée (son ancre survit), mais n'attend aucun test ; elle se
    reconnaît à la mention `*(retirée …)*` qui suit son identifiant."""
    exigences = {}
    if not os.path.isdir(spec_dir):
        return exigences
    for filename in sorted(os.listdir(spec_dir)):
        if not filename.endswith('.md') or filename == 'README.md':
            continue
        with open(os.path.join(spec_dir, filename), encoding='utf-8') as handle:
            lines = handle.read().splitlines()
        title = next((line[2:].strip() for line in lines if line.startswith('# ')), filename)
        for line in lines:
            match = EXIGENCE_DECL_RE.match(line)
            if match and match.group(1) not in exigences:
                retiree = match.group(2).lstrip().startswith('*(retir')
                exigences[match.group(1)] = (filename, title, retiree)
    return exigences


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
    lines += [f"*{facts}* — `{case['path']}:{case['line']}`", '']
    if case.get('exigences'):
        lines += ['Exigences : ' + ', '.join(f'`{key}`' for key in case['exigences']), '']
    lines += [case['title'], '']
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

    # Sommaire de la page : un domaine porte jusqu'a deux cents fiches, et sans cette
    # table il fallait derouler la page pour savoir quels fichiers de test il couvre.
    # Les ancres sont celles que le moteur du site calcule pour les titres `##`
    # ci-dessous -- d'ou `mini_markdown.slugify`, plutot qu'une seconde regle a tenir
    # d'accord avec la premiere.
    lines += ['## Ce que cette page couvre', '',
              '| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |',
              '|---|---|---|---|---|---|']
    for filename in sorted(by_file):
        group = by_file[filename]
        per = {c: sum(1 for case in group if criticite_of(case) == c) for c in CRITICITES}
        cells = ' | '.join(str(per[c]) if per[c] else '-' for c in CRITICITES)
        lines.append(f'| [`{filename}`](#{mini_markdown.slugify(filename)}) '
                     f'| {len(group)} | {cells} |')
    lines.append('')

    # Les exigences que la page vérifie, et par quels cas : le chemin inverse de la fiche, pour
    # qui part d'une spécification et cherche ce qui la garde.
    by_exigence = {}
    for case in cases:
        for key in case.get('exigences') or []:
            by_exigence.setdefault(key, []).append(case)
    if by_exigence:
        lines += ['## Exigences vérifiées par cette page', '',
                  'Chaque exigence citée par un cas de cette page, avec les cas qui la citent ; la '
                  '[matrice de traçabilité](couverture-exigences.md) les rassemble toutes.', '',
                  '| Exigence | Cas |', '|---|---|']
        for key in sorted(by_exigence):
            links = ', '.join(case_link(case, '') for case in by_exigence[key])
            lines.append(f'| `{key}` | {links} |')
        lines.append('')

    for filename in sorted(by_file):
        lines += [f'## {filename}', '']
        for case in by_file[filename]:
            lines += render_case(case)
    return '\n'.join(lines).rstrip() + '\n'


def case_id(case):
    return f"{case['suite']}.{case['name']}"


def case_link(case, page):
    """Un lien vers la fiche d'un cas, dans la même page (`page` vide) ou depuis une autre."""
    return f'[`{case_id(case)}`]({page}#{mini_markdown.slugify(case_id(case))})'


def render_coverage(domains, exigences):
    """La matrice de traçabilité : chaque exigence en vigueur, la page qui la déclare, et les cas
    de test qui la citent — ou rien, ce qui se lit aussi."""
    cases_by_exigence = {}
    for slug, (_title, _category, cases) in domains.items():
        for case in cases:
            for key in case.get('exigences') or []:
                cases_by_exigence.setdefault(key, []).append((slug, case))
    active = {key: value for key, value in exigences.items() if not value[2]}
    families = sorted({key.rsplit('-', 1)[0] for key in active})
    covered = {key for key in active if key in cases_by_exigence}
    unknown = sorted(key for key in cases_by_exigence if key not in exigences)

    lines = ['# Couverture des exigences', '',
             f'**{len(covered)} exigences en vigueur sur {len(active)}** sont citées par au moins un cas '
             'de test. Cette matrice est **engendrée** avec le reste du cahier : la colonne de gauche '
             'vient des déclarations des [spécifications](../Specification/README.md), celle de droite '
             'des identifiants `EX-…` que les tests citent dans leur commentaire ou leur corps. Une '
             'exigence sans cas est une exigence que **rien ne garde** : le cahier ne la cache pas, il '
             'la montre. Les exigences retirées ne sont pas comptées — aucun test ne doit les citer.', '',
             '## Par famille', '',
             '| Famille | Déclarée dans | En vigueur | Citées par un test | Sans test |',
             '|---|---|---|---|---|']
    for family in families:
        keys = [key for key in active if key.rsplit('-', 1)[0] == family]
        page_links = ', '.join(dict.fromkeys(
            f'[{active[key][1]}](../Specification/{active[key][0]})' for key in keys))
        with_test = sum(1 for key in keys if key in covered)
        lines.append(f'| `{family}` | {page_links} | {len(keys)} | {with_test} | {len(keys) - with_test} |')
    lines.append(f'| **Total** | | **{len(active)}** | **{len(covered)}** | **{len(active) - len(covered)}** |')
    lines.append('')

    lines += ['## Exigence par exigence', '',
              'Un cas se lit dans la page de son domaine ; le lien y mène. « — » : aucun cas ne cite '
              'l\'exigence.', '']
    for family in families:
        lines += [f'### `{family}`', '', '| Exigence | Spécification | Cas de test |', '|---|---|---|']
        for key in sorted(k for k in active if k.rsplit('-', 1)[0] == family):
            page, title, _retiree = active[key]
            links = ', '.join(case_link(case, f'{slug}.md')
                              for slug, case in cases_by_exigence.get(key, [])) or '—'
            lines.append(f'| `{key}` | [{title}](../Specification/{page}) | {links} |')
        lines.append('')

    if unknown:
        lines += ['## Citées par un test, déclarées nulle part', '',
                  'Un test cite un identifiant qu\'aucune spécification ne déclare : une faute de '
                  'frappe, ou une exigence supprimée sans que le test l\'apprenne.', '']
        lines += [f'- `{key}` : ' + ', '.join(case_link(case, f'{slug}.md')
                                             for slug, case in cases_by_exigence[key])
                  for key in unknown]
        lines.append('')
    return '\n'.join(lines).rstrip() + '\n'


CASTEST_HOWTO = "## Ajouter un cas\n\nUn test **sans** bloc `\\castest{}` fait échouer la CI : le cahier est exhaustif par construction,\nsinon il ne vaut rien — un cahier partiel laisse croire que ce qui n'y figure pas n'est pas testé.\nLe bloc se met dans le commentaire du test, juste au-dessus de sa déclaration :\n\n```cpp\n/**\n * @brief Un 20 naturel touche quelle que soit la CA, et double les des de degats.\n * \\castest{<b>Un 20 naturel touche une CA hors d'atteinte, est un critique, et double les des\n * de degats sans doubler le modificateur.</b><br/>\n * \\tcat Unitaire · Combat<br/>\n * \\tcrit Bloquant<br/>\n * \\tetapes 1. L'heroine (+5, 1d8+3) attaque un gobelin a la CA 40.<br/>2. Le d20 est force\n * a 20.<br/>\n * \\tattendu Touche et critique ; deux d8 lances, modificateur 3.\n * }\n */\nTEST(AttackTest, UnVingtNaturelToucheEtDoubleLesDes) {\n```\n\n| Champ | Ce qu'il porte |\n|---|---|\n| `<b>…</b>` | L'**objet** du cas, en une phrase : ce que le test établit, pas ce qu'il fait. |\n| `\\tcat` | La **catégorie**, telle qu'elle paraîtra sur la fiche. |\n| `\\tcrit` | La **criticité**, parmi les quatre du tableau ci-dessus. |\n| `\\tetapes` | Les **étapes**, numérotées, séparées par `<br/>`. |\n| `\\tattendu` | Le **résultat attendu**, en français. |\n\nLe **résultat attendu** d'une fiche n'est toutefois pas recopié de `\\tattendu` quand le test porte\ndes assertions : le générateur lit les assertions GoogleTest du corps de la fonction et les\ntraduit. Une fiche dit donc ce que le test **vérifie réellement**, et non ce que son auteur a écrit\nqu'il vérifiait — les deux divergent au premier remaniement, et c'est toujours le commentaire qui a\ntort. `\\tattendu` ne sert que de repli, pour un cas dont aucune assertion ne se laisse traduire.\n\n## Ce que le cahier ne dit pas\n\nIl recense ce qui est **vérifié automatiquement**, et cela seul. Une règle du jeu qu'aucun test ne\ncouvre n'y laisse aucune trace — l'absence d'une fiche n'est donc pas la preuve qu'un comportement\nest libre, seulement qu'il n'est pas gardé. Ce qui **doit** être vrai se lit dans les\n[spécifications](../Specification/README.md) ; ce cahier dit ce qui est tenu."


def render_readme(domains):
    total = sum(len(cases) for _, _, cases in domains.values())
    lines = [
        '# Cahier de test', '',
        f'**{total} cas de test**, un par test automatisé du dépôt. Le cahier est **engendré** depuis les '
        'blocs `\\castest{…}` écrits au-dessus de chaque test par `scripts/docs/generate_cahier_test.py` : il ne '
        's\'édite pas — on corrige le commentaire du test, puis on relance le script. La CI refuse un '
        'cahier périmé, et refuse un test sans bloc. Seule la [recette manuelle](recette-manuelle.md) '
        's\'écrit à la main.', '',
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
    lines += ['', '## Trois étages de vérification', '',
              'Le dépôt vérifie à trois hauteurs, et le cahier range chaque cas à la sienne : la '
              'colonne *Type* de la synthèse vient du dossier du test (`Source/Test/Unit`, '
              '`Integration`, `Systeme`).', '',
              '| Étage | Ce qu\'il prouve | Ce qu\'il ne prouve pas | Où |', '|---|---|---|---|',
              '| **Unitaire** | Une fonction ou une classe tient son contrat, seule, sans fenêtre ni GPU '
              '(`EX-NFR-010`) : un jet, une grille, un chargeur, une vue-modèle. | Que les pièces '
              's\'assemblent. | `Source/Test/Unit/<Core, HMI, Editor>/<domaine>/` |',
              '| **Intégration** | Plusieurs modules jouent ensemble sur des données réelles du dépôt : '
              'une carte livrée se charge, se compose et se parcourt. | Que l\'exécutable démarre. | '
              '`Source/Test/Integration/` |',
              '| **Système** | Un parcours **de bout en bout**, tel qu\'un utilisateur le ferait — '
              'l\'auteur dessine une carte dans l\'éditeur, puis le jeu la joue —, sans fenêtre. | '
              'Le ressenti : fluidité, lisibilité, plaisir. | `Source/Test/Systeme/` ; l\'archive '
              'publiée a son test de fumée (`scripts/release/smoke_test_release.ps1`) et la '
              '[recette manuelle](recette-manuelle.md) le reste |', '',
              'Les écrans Qt Quick ont en plus leurs tests de référence (`Source/Test/Qml/`, images '
              'comparées pixel à pixel) et les scripts Python les leurs (`scripts/tests/`, pytest) ; '
              'ni les uns ni les autres ne sont des cas GoogleTest, et ils sont décrits dans '
              '[Build, tests et intégration continue](../Guide/guide-outils.md#les-suites-de-tests).',
              '', '## Ce que chaque exigence a pour garde', '',
              'La [matrice de traçabilité](couverture-exigences.md) donne, pour chaque exigence en '
              'vigueur des spécifications, les cas de test qui la citent — et laisse visibles celles '
              'qu\'aucun test ne cite. Chaque fiche porte de même ses exigences, et chaque page de '
              'domaine récapitule celles qu\'elle vérifie.', '',
              '## Ce que les tests ne remplacent pas', '',
              'La [recette manuelle](recette-manuelle.md) est la seule page du cahier écrite à la main : '
              'les contrôles qu\'un humain fait avant de dire « livré » — fluidité, lisibilité, son, '
              'manette — avec, pour chacun, ce qu\'on regarde et ce qui doit se voir.', '',
              '## Lancer les tests', '',
              '```', 'powershell -File scripts/build.ps1      # compile (préréglage ninja)',
              'ctest --preset ninja                     # exécute tous les cas',
              'ctest --preset ninja -R AttackTest       # une suite', '```', '',
              'Un cas qui échoue se retrouve ici par son identifiant (la recherche du site le trouve), '
              'et dans le code par l\'emplacement que donne sa fiche.', '']
    lines += ['', CASTEST_HOWTO, '']
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
    files['couverture-exigences.md'] = render_coverage(domains, collect_exigences(SPECIFICATION_DIR))
    return files


def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
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
        existing = {name for name in os.listdir(OUTPUT_DIR)
                    if name.endswith('.md') and name not in HAND_WRITTEN_PAGES}
    missing = [name for name in HAND_WRITTEN_PAGES
               if not os.path.isfile(os.path.join(OUTPUT_DIR, name))]
    if missing:
        print(f'Page(s) manuscrite(s) absente(s) de {OUTPUT_DIR} : {", ".join(missing)} — le README '
              'les cite, le lint du site les exigerait.', file=sys.stderr)
        return 1

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
                  '"python scripts/docs/generate_cahier_test.py".', file=sys.stderr)
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
