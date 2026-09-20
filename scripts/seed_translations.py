# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Amorce les catalogues Qt (.ts) depuis le catalogue maison (.lang) -- LOT-86.

POURQUOI
--------
Les ecrans du jeu sont passes en QML, ou les textes s'ecrivent en FRANCAIS DANS LE FICHIER
(`qsTr("Nouvelle partie")`) plutot que par une cle. C'est ce qui permet a la conception de juger une
mise en page dans Qt Design Studio : une cle technique ne se lit pas.

Mais les traductions anglaises existaient deja, indexees par cle dans `en.lang`. Les retaper serait
absurde, et les retaper A LA MAIN les ferait diverger du vocabulaire etabli.

Ce script fait le pont : pour chaque source francaise d'un .ts, il cherche la cle dont `fr.lang`
porte exactement ce texte, puis prend la traduction que `en.lang` donne a cette cle.

CE QU'IL NE FAIT PAS
--------------------
Il ne DEVINE rien. Une source sans correspondance exacte reste « unfinished » et il la signale : une
traduction approchee vaudrait moins que pas de traduction, parce qu'on ne saurait plus laquelle
verifier.

C'est un outil de MIGRATION, appele une fois. Ensuite, les traductions se maintiennent dans Qt
Linguist -- un outil de traducteur, pas de developpeur, ce qui est tout l'interet.

Usage :
    python scripts/seed_translations.py Source/Elements/Localization/jadg_en.ts
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LOCALIZATION = ROOT / "Source" / "Elements" / "Localization"

ENTRY = re.compile(r"^\s*([A-Za-z0-9_.]+)\s*=\s*(.*?)\s*$")
UNFINISHED = re.compile(
    r"(<source>)(.*?)(</source>\s*<translation)( type=\"unfinished\")?(></translation>)",
    re.DOTALL,
)


def read_catalog(path: Path) -> dict[str, str]:
    """Lit un `.lang` : `cle = valeur`, `#` en commentaire, premier `=` separateur."""
    catalog: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        match = ENTRY.match(stripped)
        if match:
            catalog[match.group(1)] = match.group(2)
    return catalog


def unescape(text: str) -> str:
    """Le .ts echappe le XML ; les catalogues .lang, non."""
    return (text.replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">")
                .replace("&quot;", '"').replace("&apos;", "'"))


def escape(text: str) -> str:
    return (text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    target = Path(sys.argv[1])
    if not target.is_file():
        print(f"Catalogue introuvable : {target}", file=sys.stderr)
        return 1

    french = read_catalog(LOCALIZATION / "fr.lang")
    english = read_catalog(LOCALIZATION / "en.lang")
    # Index INVERSE du francais : texte -> cle. Un meme texte francais porte par deux cles est
    # signale : il faudrait alors savoir laquelle des deux traductions anglaises s'applique, et
    # deviner serait pire que de le dire.
    by_text: dict[str, list[str]] = {}
    for key, value in french.items():
        by_text.setdefault(value, []).append(key)

    filled, ambiguous, missing = 0, [], []

    def replace(match: re.Match[str]) -> str:
        nonlocal filled
        source = unescape(match.group(2))
        keys = by_text.get(source, [])
        if not keys:
            missing.append(source)
            return match.group(0)
        translations = {english.get(k, "") for k in keys if english.get(k)}
        if len(translations) > 1:
            ambiguous.append(source)
            return match.group(0)
        if not translations:
            missing.append(source)
            return match.group(0)
        filled += 1
        return f"{match.group(1)}{match.group(2)}{match.group(3)}>{escape(next(iter(translations)))}</translation>"

    text = target.read_text(encoding="utf-8")
    target.write_text(UNFINISHED.sub(replace, text), encoding="utf-8")

    print(f"{target.name} : {filled} traduction(s) reprises du catalogue maison.")
    if ambiguous:
        print(f"  {len(ambiguous)} source(s) AMBIGUE(S) -- deux cles, deux traductions :")
        for source in sorted(set(ambiguous)):
            print(f"      « {source} »")
    if missing:
        print(f"  {len(missing)} source(s) sans correspondance, laissees a traduire :")
        for source in sorted(set(missing)):
            print(f"      « {source} »")
    return 0


if __name__ == "__main__":
    sys.exit(main())
