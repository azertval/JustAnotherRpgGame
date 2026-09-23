#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Contrôle des fichiers JSON du dépôt (refonte de la CI, phase 2).

Près de 600 fichiers JSON — catalogues, niveaux, dialogues, schémas — sont lus par le jeu, et une
bonne partie est écrite à la main. Ce contrôle refuse ce qui est un défaut, avant le commit (hook
pre-commit) et en CI (tout le dépôt) :

- un JSON **invalide** : le jeu ne le chargerait pas, et `check_rpg_data.py` ne couvre que les
  catalogues RPG ;
- une **clé en double** dans un objet : valide pour la norme, mais la seconde valeur écrase la
  première en silence — l'erreur typique d'une entrée copiée puis modifiée ;
- un **BOM** UTF-8 : refusé par `nlohmann::json` et par `json.load` ;
- l'absence de **retour à la ligne final**, que tous les générateurs du dépôt écrivent.

**Pas de mise en forme imposée.** Les générateurs écrivent `json.dumps(indent=2)`, mais les
fichiers écrits à la main — schémas, palettes, cahier des assets — gardent leurs tableaux sur une
ligne, et les éclater triplerait leur longueur sans rien corriger.

Usage :
  python scripts/checks/check_json_files.py FICHIER...   (fichiers passés par pre-commit)
  python scripts/checks/check_json_files.py --all        (tous les .json suivis)
  python scripts/checks/check_json_files.py --auto-test
"""
import argparse
import json
import os
import subprocess
import sys
import tempfile

# Fixtures volontairement invalides : elles éprouvent le refus d'un JSON malformé par le code qui
# les lit. Chacune DOIT rester invalide ; le contrôle échoue si l'une devient lisible, pour que la
# liste ne couvre jamais un fichier qu'elle n'a plus de raison d'excuser.
INTENTIONALLY_INVALID = {
    'Source/Test/Fixtures/Json/tronque.json',
    'Source/Test/Fixtures/Json/virgule-en-trop.json',
    'scripts/fixtures/rpg/invalide/creatures/json-tronque.json',
}


class DuplicateKey(ValueError):
    pass


def _reject_duplicates(pairs):
    seen = set()
    for key, _ in pairs:
        if key in seen:
            raise DuplicateKey('clé « %s » en double dans un même objet' % key)
        seen.add(key)
    return dict(pairs)


def problems(raw):
    """Liste des défauts du contenu @p raw (octets) ; vide s'il est admis."""
    found = []
    if raw.startswith(b'\xef\xbb\xbf'):
        found.append('commence par un BOM UTF-8')
        raw = raw[3:]
    try:
        json.loads(raw.decode('utf-8'), object_pairs_hook=_reject_duplicates)
    except UnicodeDecodeError as error:
        found.append('pas en UTF-8 (%s)' % error)
    except DuplicateKey as error:
        found.append(str(error))
    except json.JSONDecodeError as error:
        found.append('JSON invalide : %s' % error)
    if raw and not raw.endswith(b'\n'):
        found.append('pas de retour à la ligne final')
    return found


def check(paths):
    errors = []
    for path in paths:
        normalized = path.replace('\\', '/')
        if not os.path.isfile(path):
            continue
        with open(path, 'rb') as handle:
            found = problems(handle.read())
        if normalized in INTENTIONALLY_INVALID:
            if not any(p.startswith('JSON invalide') for p in found):
                errors.append('%s : listée comme fixture invalide, mais se lit. La retirer de '
                              'INTENTIONALLY_INVALID.' % normalized)
            continue
        errors.extend('%s : %s.' % (normalized, p) for p in found)
    return errors


def auto_test():
    assert problems(b'{"a": [1, 2], "b": {"c": 1}}\n') == []
    assert problems(b'{"a": 1, "a": 2}\n')[0].startswith('clé')
    assert problems(b'{"a": {"x": 1, "x": 1}}\n')[0].startswith('clé')
    assert problems(b'\xef\xbb\xbf{}\n') == ['commence par un BOM UTF-8']
    assert problems(b'{}') == ['pas de retour à la ligne final']
    assert problems(b'{"a": 1,}\n')[0].startswith('JSON invalide')
    with tempfile.TemporaryDirectory() as root:
        path = os.path.join(root, 'ok.json')
        with open(path, 'wb') as handle:
            handle.write(b'{"a": "\xc3\xa9"}\n')
        assert check([path]) == []
    print('OK : auto-test du contrôle JSON.')


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('files', nargs='*')
    parser.add_argument('--all', action='store_true', help='tous les .json suivis par git')
    parser.add_argument('--auto-test', action='store_true')
    arguments = parser.parse_args()

    if arguments.auto_test:
        auto_test()
        return 0

    if arguments.all:
        os.chdir(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
        listing = subprocess.run(['git', 'ls-files', '-z', '*.json'], capture_output=True,
                                 check=True)
        paths = [p for p in listing.stdout.decode('utf-8').split('\0') if p]
        missing = sorted(p for p in INTENTIONALLY_INVALID if p not in paths)
        if missing:
            print('ERREUR : fixture(s) invalide(s) listée(s) mais absente(s) : %s.'
                  % ', '.join(missing))
            return 1
    else:
        paths = arguments.files

    errors = check(paths)
    for message in errors:
        print('ERREUR : ' + message)
    if errors:
        return 1
    print('OK : %d fichier(s) JSON valides, sans clé en double ni BOM.' % len(paths))
    return 0


if __name__ == '__main__':
    sys.exit(main())
