#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Vérifie le sujet d'un message de commit (hook `commit-msg`, refonte de la CI, phase 2).

CONTRIBUTING.md définit un format de message que rien ne vérifiait. Deux formes sont admises, parce
que l'historique en porte deux et que les deux disent quelque chose d'utile :

- **Conventional Commits** : `<type>(<portée facultative>): <description>`, avec les types de
  CONTRIBUTING (`feat`, `fix`, `docs`, `refactor`, `test`, `build`, `ci`, `chore`) ;
- **commit de lot** : `LOT-NN — <description>` (tiret cadratin), la forme des branches `lot/…`, qui
  rattache le commit à la feuille de route.

Un sujet sans l'une ni l'autre (« Suppression des tests obsolètes », « wip ») est refusé. Les
messages que git ou GitHub écrivent eux-mêmes sont admis : fusion, `Revert "…"`, `fixup!`,
`squash!`, `amend!`.

Usage :
  python scripts/ci/check_commit_message.py CHEMIN_DU_MESSAGE   (passé par pre-commit)
  python scripts/ci/check_commit_message.py --auto-test
"""
import re
import sys

TYPES = ('feat', 'fix', 'docs', 'refactor', 'test', 'build', 'ci', 'chore')
CONVENTIONAL_RE = re.compile(r'^(?:%s)(?:\([a-z0-9][a-z0-9._/-]*\))?!?: \S' % '|'.join(TYPES))
LOT_RE = re.compile(r'^LOT-\d+ — \S')
GENERATED_RE = re.compile(r'^(?:Merge |Revert "|fixup! |squash! |amend! )')


def subject(message):
    """Première ligne non vide hors commentaires git (`#`)."""
    for line in message.splitlines():
        if line.startswith('#'):
            continue
        if line.strip():
            return line.rstrip()
    return ''


def verdict(message):
    """None si le message est admis, sinon la raison du refus."""
    first = subject(message)
    if not first:
        return 'message vide'
    if CONVENTIONAL_RE.match(first) or LOT_RE.match(first) or GENERATED_RE.match(first):
        return None
    return 'sujet « %s » ni au format Conventional Commits ni au format de lot' % first


def auto_test():
    admitted = [
        'feat(core): ajouter la détection de collision AABB',
        'fix: corriger le ratio',
        'ci!: changer le déclencheur',
        'build(deps): monter GoogleTest',
        'LOT-86 — Phase 7 : suppression du chemin QML déclaratif',
        "Merge branch 'main' into ci/phase-1",
        'Revert "feat: x"',
        'fixup! ci: refonte phase 1',
        '# Please enter the commit message\n\nci: refonte phase 2\n\nCorps.',
    ]
    refused = [
        '',
        '# seulement un commentaire',
        'Suppression des tests obsolètes',
        'feature: pas un type du tableau',
        'feat:sans espace',
        'Feat: majuscule',
        'LOT-86 - tiret simple',
        'LOT-86 : deux-points',
    ]
    for message in admitted:
        assert verdict(message) is None, message
    for message in refused:
        assert verdict(message) is not None, message
    print('OK : auto-test du contrôle des messages de commit.')


def main():
    if sys.argv[1:] == ['--auto-test']:
        auto_test()
        return 0
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    with open(sys.argv[1], encoding='utf-8', errors='replace') as handle:
        reason = verdict(handle.read())
    if reason is None:
        return 0
    print('ERREUR : %s.' % reason)
    print('Formes admises (CONTRIBUTING.md) :')
    print('  <type>(<portée>): <description>   types : %s' % ', '.join(TYPES))
    print('  LOT-NN — <description>')
    return 1


if __name__ == '__main__':
    sys.exit(main())
