#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Garde-fou sur les fichiers binaires ajoutés au dépôt (refonte de la CI, phase 2).

Le pack git pèse près de 600 Mio sans Git LFS, dont le quota gratuit ne tiendrait pas à ce volume :
chaque binaire ajouté y reste pour toujours, même supprimé ensuite. Deux règles, vérifiées avant le
commit (hook pre-commit, sur les fichiers indexés) et en CI (sur tout le dépôt) :

1. **Aucun fichier au-delà de MAX_BYTES.** Le plus gros fichier suivi fait 4,2 Mio (une maquette de
   la charte v2) ; au-delà, la place d'un fichier est hors du dépôt, ou il doit être réduit.
2. **Un fichier binaire porte une extension déclarée `binary` dans `.gitattributes`.** C'est la
   liste des familles d'assets admises, écrite une seule fois : un binaire d'une autre famille (un
   PDF du corpus, que `EX-CNT-023` interdit de versionner, un exécutable, une archive) est refusé,
   et l'admettre revient à le déclarer — une décision relue, pas un accident. Un fichier est
   binaire s'il contient un octet nul dans ses premiers 8 Kio, le critère de git lui-même.

Usage :
  python scripts/check_binary_files.py FICHIER...   (fichiers passés par pre-commit)
  python scripts/check_binary_files.py --all        (tous les fichiers suivis)
  python scripts/check_binary_files.py --auto-test
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

MAX_BYTES = 5 * 1024 * 1024
SNIFF_BYTES = 8000
GITATTRIBUTES = '.gitattributes'
BINARY_PATTERN_RE = re.compile(r'^\*(\.[A-Za-z0-9]+)\s+(?:.*\s)?binary(?:\s|$)')


def declared_binary_extensions(gitattributes_text):
    """Extensions (minuscules, point compris) déclarées `binary` par un motif `*.ext`."""
    extensions = set()
    for line in gitattributes_text.splitlines():
        match = BINARY_PATTERN_RE.match(line.strip())
        if match:
            extensions.add(match.group(1).lower())
    return extensions


def is_binary(path):
    with open(path, 'rb') as handle:
        return b'\0' in handle.read(SNIFF_BYTES)


def violations(paths, extensions, max_bytes=MAX_BYTES):
    """Liste des messages d'erreur pour @p paths ; vide si tout est admis."""
    found = []
    for path in paths:
        if not os.path.isfile(path):
            continue  # supprimé dans l'index, ou lien symbolique cassé : rien à peser
        size = os.path.getsize(path)
        if size > max_bytes:
            found.append('%s : %.1f Mio, au-delà du plafond de %.0f Mio.'
                         % (path, size / 1048576, max_bytes / 1048576))
        if is_binary(path) and os.path.splitext(path)[1].lower() not in extensions:
            ext = os.path.splitext(path)[1] or '(sans extension)'
            found.append('%s : fichier binaire d\'extension %s, non déclarée `binary` dans %s.'
                         % (path, ext, GITATTRIBUTES))
    return found


def auto_test():
    """Éprouve les deux règles sur des fichiers écrits en temporaire : un contrôle qui n'a jamais
    refusé quoi que ce soit ne prouve pas qu'il en est capable."""
    extensions = declared_binary_extensions(
        '# commentaire\n*.png binary\n*.PNG  binary\n*.ttf -diff binary\n*.cpp text\n'
        '*.json text\n*.bin -text\n')
    assert extensions == {'.png', '.ttf'}, extensions
    with tempfile.TemporaryDirectory() as root:
        def write(name, data):
            path = os.path.join(root, name)
            with open(path, 'wb') as handle:
                handle.write(data)
            return path
        image = write('image.png', b'\x89PNG\r\n\x1a\n\0\0')
        text = write('notes.md', 'é\n'.encode('utf-8') * 10)
        pdf = write('corpus.pdf', b'%PDF-1.7\n\0\0\0')
        big = write('grand.md', b'a' * 2048)
        assert violations([image, text], extensions, 1024) == []
        assert len(violations([pdf], extensions, 1024)) == 1
        assert len(violations([big], extensions, 1024)) == 1
        assert violations([os.path.join(root, 'absent.png')], extensions, 1024) == []
    print('OK : auto-test du garde-fou binaires.')


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('files', nargs='*')
    parser.add_argument('--all', action='store_true', help='tous les fichiers suivis par git')
    parser.add_argument('--auto-test', action='store_true')
    arguments = parser.parse_args()

    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if arguments.auto_test:
        auto_test()
        return 0

    os.chdir(root)
    with open(GITATTRIBUTES, encoding='utf-8') as handle:
        extensions = declared_binary_extensions(handle.read())
    if not extensions:
        print('ERREUR : aucune extension `binary` lue dans %s ; le motif de lecture est cassé.'
              % GITATTRIBUTES)
        return 1

    if arguments.all:
        listing = subprocess.run(['git', 'ls-files', '-z'], capture_output=True, check=True)
        paths = [p for p in listing.stdout.decode('utf-8').split('\0') if p]
    else:
        paths = arguments.files

    found = violations(paths, extensions)
    for message in found:
        print('ERREUR : ' + message)
    if found:
        print('Réduire le fichier, le laisser hors du dépôt, ou déclarer sa famille dans %s.'
              % GITATTRIBUTES)
        return 1
    print('OK : %d fichier(s) sous %.0f Mio, binaires de familles déclarées (%s).'
          % (len(paths), MAX_BYTES / 1048576, ' '.join(sorted(extensions))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
