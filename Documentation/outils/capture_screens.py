#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Refait les captures d'écran du guide depuis un build du jeu et de l'éditeur.

Les captures du guide ne se font pas à la main : le jeu sait ouvrir un écran nommé et s'y
photographier (`--screen=<Nom> --screenshot=<fichier>`), l'éditeur aussi (`--screenshot=`), et
`LevelEditor --render` rend une carte sans fenêtre. Ce script les enchaîne, puis réduit chaque image
(JPEG, 1280 px de large) : une capture PNG pèse 2 Mo, vingt captures n'ont rien à faire à ce poids
dans le dépôt.

Demande Pillow (présent dans l'environnement `uv` des scripts) et un build : `scripts/build.ps1`.

Usage :
  python Documentation/outils/capture_screens.py --bin build/ninja/bin
"""
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image

DOCS_ROOT = Path(__file__).resolve().parents[1]
OUTPUT = DOCS_ROOT / 'Guide' / 'captures'
# Les écrans du jeu, par le nom que connaît `ScreenStack.qml`.
GAME_SCREENS = ['MainMenu', 'Options', 'Credits', 'Pause', 'GameView', 'CharacterSheet', 'Skills',
                'Inventory', 'Journal', 'WorldMap', 'Dialogue', 'Merchant', 'Company', 'CombatHud',
                'Arena', 'AssetGallery']


def shrink(source, target, width=1280, quality=82):
    with Image.open(source) as image:
        image = image.convert('RGB')
        if image.width > width:
            image = image.resize((width, round(image.height * width / image.width)), Image.LANCZOS)
        image.save(target, 'JPEG', quality=quality, optimize=True)


def capture(command, target, cwd):
    with tempfile.TemporaryDirectory() as folder:
        raw = Path(folder) / 'capture.png'
        result = subprocess.run(command + [f'--screenshot={raw}'], cwd=cwd, timeout=120,
                                capture_output=True, check=False)
        if result.returncode != 0 or not raw.is_file():
            print(f'  échec ({result.returncode}) : {" ".join(command)}', file=sys.stderr)
            return False
        shrink(raw, target)
        print(f'  {target.relative_to(DOCS_ROOT)}')
        return True


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--bin', required=True, help='dossier des exécutables (build/ninja/bin)')
    parser.add_argument('--only', nargs='*', help='ne refaire que ces écrans')
    parser.add_argument('--editor-args', nargs='*', default=[], help="arguments passés à l'éditeur")
    args = parser.parse_args(argv)
    binaries = Path(args.bin).resolve()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    failures = 0
    game = binaries / 'JustAnotherRpgGame.exe'
    for screen in GAME_SCREENS:
        if args.only and screen not in args.only:
            continue
        target = OUTPUT / f'jeu-{screen.lower()}.jpg'
        failures += not capture([str(game), f'--screen={screen}', '--window-size=1280x720'], target, binaries)
    if not args.only or 'Editor' in args.only:
        editor = binaries / 'LevelEditor.exe'
        failures += not capture([str(editor)] + args.editor_args, OUTPUT / 'editeur-fenetre.jpg', binaries)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
