# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Inventaire des ecrans dessines mais pas encore alimentes (LOT-86).

POURQUOI CE RELEVE EXISTE
-------------------------
Sept des neuf ecrans du RPG ont ete DESSINES avant que leurs donnees existent : les lots qui les
produiront ne sont pas ecrits. Leur mise en page, elle, etait decidee, et la jeter en attendant
aurait oblige a la redessiner plus tard -- autrement, sans que personne ne se souvienne de ce qui
avait ete tranche.

Chacun de leurs champs porte donc une CLE D'ATTRIBUTION nommee, qui aboutit a l'ancre
`hmi::PendingData`. Ce script les releve.

Il n'y a AUCUNE liste a tenir a jour : l'inventaire est derive du QML lui-meme. Une liste ecrite a
cote aurait cesse d'etre vraie au premier ecran branche, et personne ne s'en serait apercu -- c'est
exactement le defaut que ce lot corrige ailleurs, ou la palette d'identite etait ecrite deux fois.

CE QU'IL FAUT EN FAIRE
----------------------
Quand un lot fonctionnel livre sa donnee, il remplace `PendingData` par sa vraie vue-modele dans le
fichier de cablage de l'ecran (`Source/App/Game/Qml/Screens/<Ecran>.qml`, module Jadg.App) et
retire `pending: true`. Le FORMULAIRE (`Source/Ui/Screens/<Ecran>Form.ui.qml`) n'a pas a bouger : la mise en page decidee aujourd'hui est
conservee telle quelle. C'est precisement ce que la separation achete.

Usage :
    python scripts/i18n/list_pending_bindings.py            # inventaire lisible
    python scripts/i18n/list_pending_bindings.py --keys     # une cle par ligne (pour un grep)
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
# Les jumeaux de cablage vivent avec l'executable depuis le LOT-87 : Source/Ui ne contient plus que
# des formulaires, qui ne nomment jamais PendingData.
QML = ROOT / "Source" / "App" / "Game" / "Qml"

VALUE_CALL = re.compile(r'PendingData\.value\(\s*"([^"]+)"\s*\)')
IMAGE_CALL = re.compile(r'PendingData\.image\(\s*"([^"]+)"\s*\)')
ROWS_CALL = re.compile(r'PendingData\.rows\(\s*"([^"]+)"\s*,\s*(\d+)\s*\)')
PENDING_FLAG = re.compile(r"^\s*pending\s*:\s*true\b", re.MULTILINE)


def main() -> int:
    only_keys = "--keys" in sys.argv[1:]

    if not QML.is_dir():
        print(f"Repertoire introuvable : {QML}", file=sys.stderr)
        return 1

    screens: list[tuple[str, list[str], bool]] = []
    total = 0
    for path in sorted(QML.rglob("*.qml")):
        text = path.read_text(encoding="utf-8", errors="replace")
        keys = [f"{key}" for key in VALUE_CALL.findall(text)]
        keys += [f"{key} (image)" for key in IMAGE_CALL.findall(text)]
        keys += [f"{key} (liste de {count})" for key, count in ROWS_CALL.findall(text)]
        if not keys:
            continue
        total += len(keys)
        screens.append((path.relative_to(ROOT).as_posix(), sorted(keys),
                        PENDING_FLAG.search(text) is not None))

    if only_keys:
        for _, keys, _ in screens:
            for key in keys:
                print(key.split(" (")[0])
        return 0

    if not screens:
        # Deux lectures possibles, et il faut les distinguer : ou bien tout est branche -- une
        # excellente nouvelle -- ou bien ce script ne lit plus le bon repertoire. Le dire.
        print("Aucune cle en attente sous Source/App/Game/Qml.")
        print("  Soit tous les ecrans sont branches, soit ce releve ne lit plus rien :")
        print(f"  verifier {QML.relative_to(ROOT).as_posix()}.")
        return 0

    print(f"Ecrans dessines, donnees a brancher : {len(screens)} ecran(s), {total} cle(s).\n")
    for name, keys, flagged in screens:
        mark = "" if flagged else "   (sans `pending: true` -- l'ecran ne l'avoue pas a l'utilisateur)"
        print(f"  {name}{mark}")
        for key in keys:
            print(f"      {key}")
        print()

    print("Pour en brancher un : remplacer PendingData par la vue-modele dans le fichier de")
    print("cablage, et retirer `pending: true`. Le formulaire .ui.qml n'a pas a bouger.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
