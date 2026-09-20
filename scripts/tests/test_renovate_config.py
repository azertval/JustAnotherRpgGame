# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Le motif de renovate.json lit bien chaque FetchContent_Declare du dépôt.

Renovate ne s'exécute pas ici : il tourne chez Mend, et un motif qui ne correspond plus à rien n'y
produit aucune erreur, seulement plus aucune PR. Ce test applique le même motif (syntaxe JavaScript
des groupes nommés convertie pour Python) et exige qu'il retrouve chaque déclaration GitHub.
"""
import json
import re

GROUP_RE = re.compile(r'\(\?<([A-Za-z]+)>')
DECLARE_RE = re.compile(r'FetchContent_Declare\(\s*(\w+)\s+GIT_REPOSITORY\s+(\S+)\s+GIT_TAG\s+(\S+)')


def manager(root):
    config = json.loads((root / 'renovate.json').read_text(encoding='utf-8'))
    managers = [m for m in config['customManagers'] if m['customType'] == 'regex']
    assert len(managers) == 1
    return managers[0]


def test_seul_le_gestionnaire_regex_est_actif(root):
    config = json.loads((root / 'renovate.json').read_text(encoding='utf-8'))
    # Les actions GitHub et uv.lock sont à Dependabot : deux robots, deux PR pour la même montée.
    assert config['enabledManagers'] == ['custom.regex']


def test_chaque_fetchcontent_github_est_suivi(root):
    rule = manager(root)
    pattern = re.compile(GROUP_RE.sub(r'(?P<\1>', rule['matchStrings'][0]))
    files = [p for p in root.rglob('CMakeLists.txt')
             if not {'build', '.claude', '_deps'} & set(p.relative_to(root).parts)]
    file_re = re.compile(rule['managerFilePatterns'][0].strip('/'))

    declared, matched = {}, {}
    for path in files:
        text = path.read_text(encoding='utf-8')
        relative = path.relative_to(root).as_posix()
        for _, repository, tag in DECLARE_RE.findall(text):
            if repository.startswith('https://github.com/'):
                name = repository.removeprefix('https://github.com/').removesuffix('.git')
                declared[name] = tag
        if file_re.search(relative):
            for match in pattern.finditer(text):
                matched[match['depName']] = match['currentValue']

    assert declared, 'aucun FetchContent_Declare lu : le test ne prouverait rien'
    assert matched == declared
