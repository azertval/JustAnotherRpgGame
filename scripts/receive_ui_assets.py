#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Réception des images produites pour la charte v2 (LOT-87, T2.6).

Le cahier des assets (`assets-brief.json`, T2.4) dit à un générateur d'images ce qu'il doit
produire ; ce script réceptionne ce qui revient. Un dossier de PNG, chacun nommé par la clé du
cahier qu'il livre -- `ui/<famille>/<pièce>`, ou `ui/<famille>/<pièce>/<variante>` pour un état ou
un membre -- avec chaque « / » remplacé par « __ » (`ui__frame__dark-panel.png`,
`ui__button__apply__hover.png`) :

- une clé absente du cahier, ou déjà réceptionnée, est refusée ;
- les dimensions et la présence d'un canal alpha sont vérifiées contre ce que la pièce annonce ;
- ce qui passe est installé sous `Source/Elements/Assets/UI/<famille>/...` (la clé, préfixe `ui/`
  ôté, en donne le chemin -- exactement ce que `installRoot` du cahier décrit), et une entrée
  `provenance: "produced"` est ajoutée à `illustrations.json` (T2.5) : le prompt assemblé tel qu'il
  serait envoyé (`check_assets_brief.assembler`), la date du jour, les marges 9-patch de la pièce ;
- la table des pièces livrées (`Source/Ui/Theme/Artwork.qml`) est réécrite : c'est ce qui fait
  poser l'image par les briques de la charte v2 (T2.7), à la place de leur aplat de repli.

Ce que ce script NE vérifie PAS : l'absence de lettres incrustées, la fidélité de la matière au
prompt, un filigrane qui déborderait de ses marges. Ce sont des jugements sur une image, pas des
mesures qu'un script peut faire sans dépendance de vision -- une relecture humaine reste due avant
de lancer ce script, comme `--annotate` de `check_assets_brief.py` l'est avant d'écrire une zone.

Usage :
    python scripts/receive_ui_assets.py <dossier>              # installe ce qui passe le controle
    python scripts/receive_ui_assets.py <dossier> --dry-run    # rapporte sans rien ecrire
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_assets_brief as brief  # reutilise lire_jetons(), variantes(), assembler()
import check_ui_assets  # reutilise write_artwork() : la table des pieces livrees (T2.7)

ROOT = Path(__file__).resolve().parent.parent
UI = ROOT / "Source" / "Elements" / "Assets" / "UI"
MANIFEST = UI / "illustrations.json"
CAHIER = ROOT / "Documentation" / "Lot" / "LOT-87-charte-v2" / "assets-brief.json"

PNG_SIGNATURE = bytes([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A])


def png_size(data: bytes) -> tuple[int, int]:
    if data[:8] != PNG_SIGNATURE:
        raise ValueError("signature PNG absente")
    width, height = struct.unpack(">II", data[16:24])
    return width, height


def has_alpha(data: bytes) -> bool:
    """Type de couleur PNG (octet 25 de l'en-tete IHDR) : 4 (gris + alpha) ou 6 (vrai + alpha)."""
    return data[25] in (4, 6)


def key_from_filename(name: str) -> str:
    return name.rsplit(".", 1)[0].replace("__", "/")


def install_path(key: str) -> str:
    assert key.startswith("ui/"), key
    return key[len("ui/"):] + ".png"


def index_cahier(cahier: dict) -> dict[str, tuple[dict, dict | None]]:
    """Chaque cle (piece, ou piece/variante) -> (la piece, sa variante si elle en a une)."""
    index: dict[str, tuple[dict, dict | None]] = {}
    for piece in cahier["pieces"]:
        variants = brief.variantes(piece)
        if variants:
            for variant in variants:
                index[f"{piece['key']}/{variant['id']}"] = (piece, variant)
        else:
            index[piece["key"]] = (piece, None)
    return index


def receive(folder: Path, dry_run: bool) -> int:
    if not folder.is_dir():
        print(f"receive_ui_assets : {folder} n'est pas un dossier.", file=sys.stderr)
        return 1

    cahier = json.loads(CAHIER.read_text(encoding="utf-8"))
    tokens = brief.lire_jetons(cahier)
    index = index_cahier(cahier)
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    already = {
        entry["cahier"]
        for entry in manifest["illustrations"]
        if entry.get("provenance") == "produced"
    }

    accepted: list[str] = []
    refused: list[tuple[str, str]] = []

    for source in sorted(folder.glob("*.png")):
        key = key_from_filename(source.name)
        entry = index.get(key)
        if entry is None:
            refused.append((source.name, f"cle '{key}' inconnue du cahier"))
            continue
        if key in already:
            refused.append((source.name, f"'{key}' deja receptionnee"))
            continue
        piece, variant = entry
        data = source.read_bytes()
        try:
            width, height = png_size(data)
        except (ValueError, struct.error) as error:
            refused.append((source.name, f"PNG illisible ({error})"))
            continue
        if [width, height] != piece["size"]:
            refused.append((source.name, f"{width}x{height} livres, {piece['size']} attendus"))
            continue
        alpha = has_alpha(data)
        if piece["transparent"] != alpha:
            expected = "un canal alpha" if piece["transparent"] else "aucun canal alpha"
            refused.append((source.name, f"{expected} attendu, l'inverse livre"))
            continue

        relative = install_path(key)
        if dry_run:
            accepted.append(key)
            continue

        destination = UI / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)

        new_entry: dict = {
            "id": key.replace("/", "-"),
            "file": relative,
            "provenance": "produced",
            "cahier": key,
            "prompt": brief.assembler(cahier, tokens, piece, variant),
            "date": datetime.date.today().isoformat(),
            "size": [width, height],
            "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        }
        if piece["display"] == "nine-patch":
            new_entry["margins"] = piece["margins"]
        manifest["illustrations"].append(new_entry)
        already.add(key)
        accepted.append(key)

    if not dry_run and accepted:
        MANIFEST.write_text(
            json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
        )
        # Les briques ne posent une image que si Artwork.qml la dit livree : sans cette ligne, la
        # piece serait installee et declaree, et les ecrans garderaient leur aplat de repli.
        if not check_ui_assets.write_artwork(manifest):
            print("receive_ui_assets : table d'Artwork.qml introuvable, non mise a jour.", file=sys.stderr)
            return 1

    for name, reason in refused:
        print(f"receive_ui_assets : REFUSE {name} -- {reason}", file=sys.stderr)
    verb = "a installer" if dry_run else "installee(s)"
    print(f"receive_ui_assets : {len(accepted)} piece(s) {verb}, {len(refused)} refusee(s).")
    return 1 if refused else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("dossier", type=Path, help="dossier des PNG livres par le generateur")
    parser.add_argument(
        "--dry-run", action="store_true", help="verifie sans installer ni ecrire le manifeste"
    )
    arguments = parser.parse_args()
    return receive(arguments.dossier, arguments.dry_run)


if __name__ == "__main__":
    sys.exit(main())
