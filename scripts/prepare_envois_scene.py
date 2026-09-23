#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Prépare les envois au générateur des pièces de scène d'une commande : un dossier par envoi (LOT-105).

Le pendant de `prepare_envois_figure.py` pour les pièces de décor. Il lit la **commande** d'une zone
(`Tools/AssetsHD/…/commande.md`, gabarit `Planning/standards/gabarit-commande-zone.md`) et les blocs
A et B de la consigne (`Planning/standards/consigne-2d-hd.md`), et écrit pour chaque pièce un dossier
`envois/NN-<pièce>/` à côté de la commande :

- `prompt.txt` : le texte entier à coller, blocs A, B et C assemblés ;
- `1-….png`, `2-….png` : la planche de référence, puis les références déjà produites ;
- `LIRE.txt` : les références à joindre qui ne sont pas encore produites, et le chemin sous lequel
  enregistrer la sortie.

Une pièce de la commande est un titre `#### <envoi>` suivi d'un bloc de code (le bloc C) et d'une
ligne de champs : `Pièces : …. Emprise : C × R. … Source : `<chemin>`. Référence : `<envoi>`, ….`
Deux lignes du bloc C sont des marqueurs : `FRAMING: <N>` (une planche de N sols) et `REFERENCE`.

    python scripts/prepare_envois_scene.py Tools/AssetsHD/Regions/central-empire/capital/Common/commande.md
    python scripts/prepare_envois_scene.py COMMANDE --seulement wall-limestone-v --suffixe reprise

Les dossiers `envois/` ne sont pas versionnés : ils se régénèrent.
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONSIGNE = ROOT / "Planning" / "standards" / "consigne-2d-hd.md"
PLANCHE = ROOT / "Tools" / "AssetsHD" / "Arenarea" / "arenarea-planche-reference-v2.png"
LOSANGE = (256, 159)
MOTS = {"TWO": 2, "THREE": 3, "FOUR": 4, "FIVE": 5, "SIX": 6}


class CommandeError(Exception):
    """Une commande ou une consigne qui ne se lit pas comme attendu."""


@dataclass
class Envoi:
    nom: str
    bloc: str
    pieces: list[str]
    emprise: tuple[int, int]
    source: Path
    references: list[str] = field(default_factory=list)


def blocs(texte: str) -> list[str]:
    return re.findall(r"```\n(.*?)```", texte, flags=re.S)


def consigne() -> dict[str, str]:
    """Les blocs de la consigne dont le script a besoin, par leur premier mot."""
    trouves = {}
    for bloc in blocs(CONSIGNE.read_text(encoding="utf-8")):
        for cle in ("STYLE", "VIEW:", "FRAMING (floor sheet)", "REFERENCE: the image attached"):
            if bloc.startswith(cle) and cle not in trouves:
                trouves[cle] = bloc.rstrip()
    manquants = {"STYLE", "VIEW:", "FRAMING (floor sheet)", "REFERENCE: the image attached"} - trouves.keys()
    if manquants:
        raise CommandeError(f"{CONSIGNE} : blocs introuvables : {sorted(manquants)}")
    return trouves


def lire_commande(page: Path) -> list[Envoi]:
    texte = page.read_text(encoding="utf-8")
    envois = []
    for section in re.split(r"^#### ", texte, flags=re.M)[1:]:
        nom = section.splitlines()[0].strip()
        code = blocs(section)
        champs = section[section.rindex("```") + 3:] if code else ""
        pieces = re.search(r"Pièces : (.*?)\. Emprise", champs)
        emprise = re.search(r"Emprise : (\d+) × (\d+)", champs)
        source = re.search(r"Source : `([^`]+)`", champs)
        if not code or not pieces or not emprise or not source:
            raise CommandeError(f"{page} : la pièce `{nom}` n'a pas son bloc C, ses pièces, son emprise ou sa source")
        references = re.search(r"Référence : (.*?)\. État", champs)
        envois.append(Envoi(
            nom=nom, bloc=code[0].rstrip(),
            pieces=re.findall(r"`([^`]+)`", pieces.group(1)),
            emprise=(int(emprise.group(1)), int(emprise.group(2))),
            source=(page.parent / source.group(1)).resolve(),
            references=re.findall(r"`([^`]+)`", references.group(1)) if references else []))
    noms = {e.nom for e in envois}
    for envoi in envois:
        inconnues = [r for r in envoi.references if r not in noms]
        if inconnues:
            raise CommandeError(f"{page} : `{envoi.nom}` se réfère à des envois inconnus : {inconnues}")
    return envois


def assembler(envoi: Envoi, textes: dict[str, str]) -> str:
    vue = (textes["VIEW:"].replace("{LOSANGE_L}", str(LOSANGE[0])).replace("{LOSANGE_H}", str(LOSANGE[1]))
           .replace("{EMPRISE_L}", str(envoi.emprise[0])).replace("{EMPRISE_H}", str(envoi.emprise[1])))
    lignes = envoi.bloc.splitlines()
    corps = []
    for ligne in lignes:
        planche = re.fullmatch(r"FRAMING: (\w+)", ligne)
        if planche:
            n = planche.group(1)
            if MOTS.get(n) != len(envoi.pieces):
                raise CommandeError(f"`{envoi.nom}` : planche de {n} sols pour {len(envoi.pieces)} pièces")
            # Le cadrage de la planche remplace celui d'une pièce seule, et la sortie devient celle
            # d'une bande (consigne, « le cas d'une planche de sols »).
            vue = (vue[:vue.index("FRAMING:")] + textes["FRAMING (floor sheet)"].replace("{N}", n)
                   + "\n\nOUTPUT: landscape canvas, as wide as available, PNG with alpha.")
        elif ligne == "REFERENCE":
            if not envoi.references:
                raise CommandeError(f"`{envoi.nom}` : marqueur REFERENCE sans ligne « Référence »")
            corps.append(textes["REFERENCE: the image attached"])
            corps.append("")
        else:
            corps.append(ligne)
    if envoi.references and "REFERENCE" not in lignes:
        raise CommandeError(f"`{envoi.nom}` : une référence est jointe mais le bloc C ne le dit pas")
    return f"{textes['STYLE']}\n\n{vue.rstrip()}\n\n" + "\n".join(corps).rstrip() + "\n"


def ecrire(dossier: Path, prompt: str, jointes: list[Path], a_joindre: list[Path], sortie: Path) -> None:
    if dossier.exists():
        shutil.rmtree(dossier)
    dossier.mkdir(parents=True)
    (dossier / "prompt.txt").write_text(prompt, encoding="utf-8", newline="\n")
    for rang, image in enumerate(jointes, start=1):
        shutil.copyfile(image, dossier / f"{rang}-{image.name}")
    lire = ["Coller prompt.txt dans le générateur.", "Joindre, dans cet ordre :"]
    lire += [f"  {rang}. {image.name}" for rang, image in enumerate(jointes, start=1)]
    lire += [f"  {len(jointes) + rang}. {image.name} ({image}) — à produire d'abord"
             for rang, image in enumerate(a_joindre, start=1)]
    lire.append(f"Enregistrer la sortie sous : {sortie}")
    (dossier / "LIRE.txt").write_text("\n".join(lire) + "\n", encoding="utf-8", newline="\n")


def preparer(page: Path, seulement: set[str] | None, suffixe: str) -> list[Path]:
    textes = consigne()
    envois = lire_commande(page)
    par_nom = {e.nom: e for e in envois}
    if seulement:
        inconnus = seulement - par_nom.keys()
        if inconnus:
            raise CommandeError(f"{page} : envois inconnus : {sorted(inconnus)}")
    fin = f"-{suffixe}" if suffixe else ""
    dossiers = []
    for rang, envoi in enumerate(envois, start=1):
        if seulement and envoi.nom not in seulement:
            continue
        references = [par_nom[r].source for r in envoi.references]
        jointes = [PLANCHE] + [r for r in references if r.is_file()]
        a_joindre = [r for r in references if not r.is_file()]
        sortie = envoi.source.with_name(f"{envoi.source.stem}{fin}{envoi.source.suffix}")
        dossier = page.parent / "envois" / f"{rang:02d}-{envoi.nom}{fin}"
        ecrire(dossier, assembler(envoi, textes), jointes, a_joindre, sortie)
        dossiers.append(dossier)
    return dossiers


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("commande", type=Path, help="la commande d'une zone (commande.md)")
    parser.add_argument("--seulement", help="envois à préparer seuls, `wall-limestone-v,prop-crate` (une reprise)")
    parser.add_argument("--suffixe", default="", help="mot ajouté au nom de la sortie (`reprise`)")
    args = parser.parse_args(argv)
    try:
        seulement = set(args.seulement.split(",")) if args.seulement else None
        dossiers = preparer(args.commande.resolve(), seulement, args.suffixe)
    except CommandeError as error:
        print(f"prepare_envois_scene : {error}", file=sys.stderr)
        return 1
    for dossier in dossiers:
        print(dossier.relative_to(ROOT).as_posix())
    return 0


if __name__ == "__main__":
    sys.exit(main())
