#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Prépare les envois au générateur d'une figurine : un dossier par envoi, prêt à copier (LOT-112).

Le générateur reste un outil manuel (consigne 2D HD) : l'auteur colle un texte et joint des images.
Ce script ôte tout ce qui se recopie à la main. Il lit les **commandes** versionnées — la source
unique des textes — et écrit, pour chaque envoi, un dossier `envois/NN-<nom>/` :

- `prompt.txt` : le texte entier à coller, blocs A, B et C assemblés ;
- `1-….png`, `2-….png` : les pièces jointes, dans l'ordre, quand elles existent déjà ;
- `LIRE.txt` : les pièces jointes à ajouter (celles que produit un envoi précédent) et le nom sous
  lequel enregistrer la sortie.

    python scripts/prepare_envois_figure.py essai
    python scripts/prepare_envois_figure.py heros --images 8
    python scripts/prepare_envois_figure.py heros --images 8 --seulement attack-se,death-nw --suffixe reprise

`--seulement` ne prépare que ces bandes (une reprise), et `--suffixe` ajoute un mot au nom de la
sortie (`attack-se-reprise.png`) : une source acceptée n'est jamais écrasée.

Les dossiers `envois/` ne sont pas versionnés (seuls les `.md` et `install.json` le sont sous
`Tools/AssetsHD/`) : ils se régénèrent.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PLANCHE = ROOT / "Tools" / "AssetsHD" / "Arenarea" / "arenarea-planche-reference-v2.png"
ESSAI = ROOT / "Tools" / "AssetsHD" / "Essais" / "lot-112-cadence"
HEROS = ROOT / "Tools" / "AssetsHD" / "Common" / "Characters" / "Heroes" / "brawler"
FACINGS = ("se", "sw", "ne", "nw")
ANIMATIONS = ("idle", "walk", "attack", "hit", "death")
MOTS = {6: "SIX", 8: "EIGHT"}


class CommandeError(Exception):
    """Une commande qui ne se lit pas comme attendu."""


def blocs(page: Path) -> list[str]:
    """Les blocs de code d'une page de commande, dans l'ordre."""
    return re.findall(r"```\n(.*?)```", page.read_text(encoding="utf-8"), flags=re.S)


def table(page: Path, colonnes: int) -> dict[str, list[str]]:
    """Les lignes d'un tableau dont chaque cellule est entre accents graves, par première cellule."""
    lignes = {}
    for ligne in page.read_text(encoding="utf-8").splitlines():
        cellules = re.findall(r"`([^`]*)`", ligne)
        if ligne.startswith("|") and len(cellules) == colonnes:
            lignes[cellules[0]] = cellules[1:]
    return lignes


def prompt_essai() -> str:
    """L'envoi à six images de l'essai : blocs A, B et C déjà assemblés par sa commande."""
    for bloc in blocs(ESSAI / "commande.md"):
        if bloc.startswith("STYLE") and "VARIANTS: SIX" in bloc:
            return bloc
    raise CommandeError(f"{ESSAI / 'commande.md'} : pas d'envoi assemblé à six images")


def source_acceptee(stem: str) -> Path:
    """La source acceptée d'une bande : celle que nomme `selection-acceptee.json` (le choix de
    l'auteur parmi les versions), sinon `<bande>.png`."""
    selection = HEROS / "selection-acceptee.json"
    if selection.is_file():
        nom = json.loads(selection.read_text(encoding="utf-8")).get("sources", {}).get(stem)
        if nom:
            return HEROS / nom
    return HEROS / f"{stem}.png"


def ecrire(dossier: Path, prompt: str, jointes: list[Path], a_joindre: list[str], sortie: str) -> None:
    if dossier.exists():
        shutil.rmtree(dossier)
    dossier.mkdir(parents=True)
    (dossier / "prompt.txt").write_text(prompt.rstrip() + "\n", encoding="utf-8", newline="\n")
    for rang, image in enumerate(jointes, start=1):
        shutil.copyfile(image, dossier / f"{rang}-{image.name}")
    lire = ["Coller prompt.txt dans le générateur."]
    if jointes or a_joindre:
        lire.append("Joindre, dans cet ordre :")
        lire += [f"  {rang}. {image.name}" for rang, image in enumerate(jointes, start=1)]
        lire += [f"  {len(jointes) + rang}. {nom}" for rang, nom in enumerate(a_joindre, start=1)]
    lire.append(f"Enregistrer la sortie sous : {sortie}")
    (dossier / "LIRE.txt").write_text("\n".join(lire) + "\n", encoding="utf-8", newline="\n")


def essai() -> list[Path]:
    six = prompt_essai()
    dossiers = []
    for rang, n in enumerate((6, 8), start=1):
        dossier = ESSAI / "envois" / f"{rang:02d}-walk-{n}"
        ecrire(dossier, six.replace("VARIANTS: SIX", f"VARIANTS: {MOTS[n]}"), [PLANCHE], [],
               str(ESSAI / f"walk-{n}.png"))
        dossiers.append(dossier)
    return dossiers


def heros(images: int, seulement: set[str] | None = None, suffixe: str = "") -> list[Path]:
    page = HEROS / "commande.md"
    six = prompt_essai()
    style = six[:six.index("VIEW:")].rstrip()
    vue = six[six.index("VIEW:"):six.index("PIECE:")].rstrip()
    portrait = next((b for b in blocs(page) if b.startswith("FRAMING (portrait)")), None)
    gabarit = next((b for b in blocs(page) if b.startswith("REFERENCE:")), None)
    if portrait is None or gabarit is None:
        raise CommandeError(f"{page} : bloc du portrait ou gabarit des bandes introuvable")
    animations = table(page, 3)
    directions = table(page, 2)
    manquants = [a for a in ANIMATIONS if a not in animations] + [f for f in FACINGS if f not in directions]
    if manquants:
        raise CommandeError(f"{page} : lignes absentes des tableaux : {manquants}")

    envois = HEROS / "envois"
    dossiers = []
    if seulement is None:
        dossiers.append(envois / "01-portrait")
        ecrire(dossiers[0], f"{style}\n\n{portrait}", [PLANCHE], [], str(HEROS / "portrait.png"))
    fin = f"-{suffixe}" if suffixe else ""

    ordre = [(a, "se") for a in ANIMATIONS] + [(a, f) for f in FACINGS[1:] for a in ANIMATIONS]
    for rang, (animation, facing) in enumerate(ordre, start=2):
        if seulement is not None and f"{animation}-{facing}" not in seulement:
            continue
        nom, timing = animations[animation]
        corps = (gabarit.replace("{ANIMATION}", nom).replace("{DIRECTION}", directions[facing][0])
                 .replace("{N}", MOTS[images]).replace("{TIMING}", timing))
        if (animation, facing) == ("idle", "se"):
            corps = corps.replace(corps[:corps.index("PIECE:")],
                                  "REFERENCE: the attached portrait shows THIS character.\n\n")
            a_joindre = [f"portrait.png ({HEROS / 'portrait.png'})"]
        else:
            references = [source_acceptee("idle-se")]
            if facing != "se" and animation != "idle":
                references.append(source_acceptee(f"{animation}-se"))
            a_joindre = [f"{r.name} ({r})" for r in references]
        dossier = envois / f"{rang:02d}-{animation}-{facing}{fin}"
        ecrire(dossier, f"{style}\n\n{vue}\n\n{corps}", [PLANCHE], a_joindre,
               str(HEROS / f"{animation}-{facing}{fin}.png"))
        dossiers.append(dossier)
    return dossiers


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("quoi", choices=("essai", "heros"))
    parser.add_argument("--images", type=int, choices=sorted(MOTS), help="images par animation (héros)")
    parser.add_argument("--seulement", help="bandes à préparer seules, `attack-se,death-nw` (une reprise)")
    parser.add_argument("--suffixe", default="", help="mot ajouté au nom de la sortie (`reprise`)")
    args = parser.parse_args(argv)
    try:
        if args.quoi == "essai":
            dossiers = essai()
        elif args.images is None:
            parser.error("le héros attend --images, le nombre fixé par l'essai")
        else:
            seulement = set(args.seulement.split(",")) if args.seulement else None
            dossiers = heros(args.images, seulement, args.suffixe)
    except CommandeError as error:
        print(f"prepare_envois_figure : {error}", file=sys.stderr)
        return 1
    for dossier in dossiers:
        print(dossier.relative_to(ROOT).as_posix())
    return 0


if __name__ == "__main__":
    sys.exit(main())
