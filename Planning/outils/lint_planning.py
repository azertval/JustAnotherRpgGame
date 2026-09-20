#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Verrouille `Planning/` : une planification que rien ne vérifie finit par devenir fausse.

Règles :

 1. `versions.toml` se lit ; chaque version a un identifiant unique, une nature et un référentiel
    connus, et son dossier existe s'il est déclaré.
 2. Une version **détaillée** a au moins un lot, des objectifs et des critères de sortie ; une
    version **prévisionnelle** n'a aucun lot (elle se détaille quand elle approche).
 3. Chaque fiche de lot se lit ; son nom de fichier commence par son identifiant `LOT-NNN`.
 4. Identifiants uniques (assuré au chargement) et bien formés.
 5. Le lot sert une version déclarée, et sa fiche vit dans le dossier de cette version.
 6. Filière, statut et taille appartiennent aux listes connues.
 7. Chaque prérequis est un lot déclaré, et n'est pas le lot lui-même.
 8. Aucun cycle de prérequis.
 9. Un prérequis ne sert jamais une version **ultérieure** à celle du lot qui l'attend.
10. Un lot `livre` ou `en-cours` n'a que des prérequis `livre`.
11. Un lot non abandonné annonce au moins un livrable et un critère d'acceptation.
12. Les maquettes citées par une fiche existent.
13. Les liens relatifs des pages Markdown de `Planning/` pointent sur des fichiers qui existent.

Usage :
  python Planning/outils/lint_planning.py
"""
import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from planning_model import (  # noqa: E402
    FILIERES, LOT_FILE_RE, LOT_ID_RE, NATURES, STATUTS, TAILLES, PlanningError, find_cycle, load_planning,
)

PLANNING_ROOT = Path(__file__).resolve().parents[1]
MD_LINK_RE = re.compile(r'(?<!\\)\]\(([^)\s]+)\)')
CODE_RE = re.compile(r'```.*?```|`[^`\n]*`', re.DOTALL)


def lint(root):
    root = Path(root)
    try:
        planning = load_planning(root)
    except PlanningError as error:
        return [str(error)]
    errors = []
    versions = {v.id: v for v in planning.versions}
    rank = {v.id: index for index, v in enumerate(planning.versions)}

    if len(versions) != len(planning.versions):
        errors.append('versions.toml: identifiant de version en double')
    for version in planning.versions:
        where = f'versions.toml [{version.id}]'
        if version.nature not in NATURES:
            errors.append(f'{where}: nature inconnue « {version.nature} » (attendu : {", ".join(NATURES)})')
        if version.dossier and not (root / 'versions' / version.dossier).is_dir():
            errors.append(f'{where}: dossier absent — versions/{version.dossier}')
        if version.nature == 'detaillee':
            if not version.lots:
                errors.append(f'{where}: version détaillée sans lot')
            if not version.objectifs or not version.criteres_de_sortie:
                errors.append(f'{where}: version détaillée sans objectifs ou sans critères de sortie')
        elif version.lots:
            errors.append(f'{where}: version prévisionnelle qui porte des lots '
                          f'({", ".join(lot.id for lot in version.lots)}) — la passer en `detaillee`')

    for lot in planning.lots.values():
        where = lot.path.relative_to(root).as_posix()
        if not LOT_ID_RE.match(lot.id):
            errors.append(f'{where}: identifiant mal formé « {lot.id} » (attendu : LOT-NNN)')
        match = LOT_FILE_RE.match(lot.path.name)
        if not match or match.group(1) != lot.id:
            errors.append(f'{where}: le nom de fichier doit être `{lot.id}-<objet-en-minuscules>.md`')
        version = versions.get(lot.version)
        if version is None:
            errors.append(f'{where}: version inconnue « {lot.version} »')
        elif version.dossier and (root / 'versions' / version.dossier / 'lots') != lot.path.parent:
            errors.append(f'{where}: la fiche sert la {lot.version} mais ne vit pas dans '
                          f'versions/{version.dossier}/lots/')
        if lot.filiere not in FILIERES:
            errors.append(f'{where}: filière inconnue « {lot.filiere} »')
        if lot.statut not in STATUTS:
            errors.append(f'{where}: statut inconnu « {lot.statut} »')
        if lot.taille not in TAILLES:
            errors.append(f'{where}: taille inconnue « {lot.taille} » (attendu : {", ".join(TAILLES)})')
        for parent_id in lot.prerequis:
            parent = planning.lots.get(parent_id)
            if parent_id == lot.id or parent is None:
                errors.append(f'{where}: prérequis invalide « {parent_id} »')
                continue
            if rank.get(parent.version, -1) > rank.get(lot.version, -1):
                errors.append(f'{where}: attend {parent_id}, qui sert une version ultérieure ({parent.version})')
            if lot.statut in ('livre', 'en-cours') and parent.statut != 'livre':
                errors.append(f'{where}: {lot.statut} alors que {parent_id} n\'est pas livré')
        if lot.statut != 'abandonne' and (not lot.livrables or not lot.criteres):
            errors.append(f'{where}: ni livrable ni critère d\'acceptation — un lot se juge sur pièces')
        for maquette in lot.maquettes:
            if not (lot.path.parent / maquette).resolve().is_file():
                errors.append(f'{where}: maquette absente — {maquette}')

    cycle = find_cycle(planning.lots)
    if cycle:
        errors.append('cycle de prérequis : ' + ' → '.join(cycle))

    for page in sorted(root.rglob('*.md')):
        if {'outils', 'site'} & set(page.relative_to(root).parts):
            continue
        text = CODE_RE.sub('', page.read_text(encoding='utf-8'))
        for target in MD_LINK_RE.findall(text):
            if '://' in target or target.startswith(('#', 'mailto:')):
                continue
            path = target.split('#', 1)[0]
            if path and not (page.parent / path).resolve().exists():
                errors.append(f'{page.relative_to(root).as_posix()}: lien mort — {target}')
    return errors


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--root', default=str(PLANNING_ROOT))
    args = parser.parse_args(argv)
    errors = lint(args.root)
    for error in errors:
        print(f'lint_planning: {error}', file=sys.stderr)
    if errors:
        print(f'lint_planning: {len(errors)} erreur(s)', file=sys.stderr)
        return 1
    print('lint_planning: OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
