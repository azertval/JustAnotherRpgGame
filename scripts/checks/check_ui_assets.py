# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Garde-fou : les illustrations livrees et leur manifeste ne doivent pas diverger.

`Source/Elements/Assets/UI/` ne porte qu'une provenance, cataloguee par `illustrations.json` : les
illustrations **produites** par un generateur d'images a partir du cahier des assets
(`Planning/versions/v0.0.0/v0.0.0-fondation/annexes/LOT-87-charte-v2/assets-brief.json`, T2.4), `provenance: "produced"`, qui
portent la cle du cahier (`cahier`), le prompt tel qu'envoye et sa date.

**Une illustration d'interface est produite, jamais extraite** (`EX-IHM-076`, `LOT-94`). Le plan
de la ville, la carte du monde et les planches du corpus sont des oeuvres : le jeu ne les affiche
pas. Toute autre provenance -- `tanares` en tete, celle des deux cartes du monde que le `LOT-94` a
retirees -- est refusee, et non plus seulement declaree.

Ce que ce controle rend impossible, c'est l'ecart SILENCIEUX :

- une image du corpus revenue dans le depot, declaree sous sa provenance (interdite) ;
- une illustration retouchee a la main, ou remplacee par une image venue d'ailleurs (empreinte) ;
- une illustration supprimee, ou ajoutee sans passer par le manifeste (orpheline) ;
- un manifeste redige a la main dont les dimensions ne sont pas celles du fichier ;
- un nom de fichier cite par le QML que le manifeste ne porte pas ;
- une piece du cahier sans image livree ET sans mention explicite « non livree » dans le
  manifeste (section `pending`) -- ce qui la ferait disparaitre sans que rien ne le remarque ;
- depuis le T2.7, une table des pieces livrees (`Source/Ui/Theme/Artwork.qml`, ce que les briques
  de la charte v2 consultent) qui ne suit plus les entrees produites du manifeste, et une brique
  qui nomme une piece absente du cahier.

Ce dernier point sur le code est celui qui casse le plus souvent a l'usage : les trois premiers
protegent une donnee, celui-la protege le LIEN entre la donnee et le code, qui est ce qui lache
quand on renomme un fichier sans y penser.

Aucune dependance : l'empreinte est du hashlib, et les dimensions se lisent dans l'en-tete du
fichier. Meme motif que `check_qt_version_pin.py`. La lecture du cahier ne valide pas son schema
(c'est le role de `check_assets_brief.py`, qui a besoin de `jsonschema`) : elle n'en lit que la
forme deja garantie ailleurs.

Usage :
    python scripts/checks/check_ui_assets.py                   # code de sortie non nul si divergence
    python scripts/checks/check_ui_assets.py --write-artwork   # reecrit la table d'Artwork.qml
"""

from __future__ import annotations

import hashlib
import json
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
UI = ROOT / "Source" / "Elements" / "Assets" / "UI"
MANIFEST = UI / "illustrations.json"
CAHIER = ROOT / "Planning" / "versions" / "v0.0.0" / "v0.0.0-fondation" / "annexes" / "LOT-87-charte-v2" / "assets-brief.json"
# Les cartes de l'ecran « Carte » (LOT-94) : peintes par l'auteur, sous Assets/Maps/, avec
# leur propre manifeste, que scripts/checks/check_map_assets.py recoupe.
MAPS_MANIFEST = ROOT / "Source" / "Elements" / "Assets" / "Maps" / "manifest.json"
# La table des pieces livrees que les briques consultent (T2.7), et les briques elles-memes.
ARTWORK = ROOT / "Source" / "Ui" / "Theme" / "Artwork.qml"
ARTWORK_BEGIN = "// --- DEBUT DE LA TABLE ENGENDREE"
ARTWORK_END = "// --- FIN DE LA TABLE ENGENDREE"
CONTROLS = ROOT / "Source" / "Ui" / "Controls"
# Les endroits ou un nom de fichier d'illustration est ecrit. Depuis le LOT-86, ce sont les ecrans
# QML : c'est la conception qui choisit une image, et elle le fait dans Source/Ui.
#
# Le repertoire ENTIER, et non une liste de fichiers : un ecran ajoute qui nommerait une image
# sortirait sinon du recoupement, et l'image se retrouverait declaree mais << nommee par aucun
# code >> -- l'inverse exact de ce que ce controle protege.
NAMING_SOURCES = tuple(sorted((ROOT / "Source" / "Ui").rglob("*.qml")))

PNG_SIGNATURE = bytes([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A])

errors: list[str] = []


def fail(message: str) -> None:
    errors.append(message)


def png_size(data: bytes) -> tuple[int, int]:
    """Largeur et hauteur d'un PNG, lues dans son bloc IHDR : a decalage FIXE, contrairement au
    JPEG. Huit octets de signature, puis un bloc dont les deux premiers entiers sont les
    dimensions."""
    if data[:8] != PNG_SIGNATURE:
        raise ValueError("signature PNG absente")
    width, height = struct.unpack(">II", data[16:24])
    return width, height


def image_size(name: str, data: bytes) -> tuple[int, int]:
    """Dimensions d'une image, selon son extension."""
    return png_size(data) if name.lower().endswith(".png") else jpeg_size(data)


def jpeg_size(data: bytes) -> tuple[int, int]:
    """Largeur et hauteur d'un JPEG, lues dans son premier marqueur de cadre.

    Un JPEG n'a pas d'en-tete a decalage fixe : il faut parcourir ses marqueurs jusqu'au SOFn, qui
    porte les dimensions. Une quinzaine de lignes, et aucune dependance -- la CI n'installe pas
    Pillow, et ce controle doit tourner partout ou tourne un python.
    """
    if data[:2] != b"\xff\xd8":
        raise ValueError("signature JPEG absente")
    offset = 2
    while offset + 4 <= len(data):
        if data[offset] != 0xFF:
            raise ValueError(f"marqueur attendu au decalage {offset}")
        marker = data[offset + 1]
        # SOF0..SOF15, sauf DHT (C4), JPG (C8) et DAC (CC) qui ne sont pas des cadres.
        if 0xC0 <= marker <= 0xCF and marker not in (0xC4, 0xC8, 0xCC):
            height, width = struct.unpack(">HH", data[offset + 5 : offset + 9])
            return width, height
        (length,) = struct.unpack(">H", data[offset + 2 : offset + 4])
        offset += 2 + length
    raise ValueError("aucun marqueur de cadre (SOFn) dans le fichier")


def read_manifest() -> dict:
    if not MANIFEST.is_file():
        print(f"check_ui_assets : {MANIFEST.relative_to(ROOT)} absent.", file=sys.stderr)
        sys.exit(1)
    return json.loads(MANIFEST.read_text(encoding="utf-8"))


def read_maps_manifest() -> list[str]:
    """Les cartes de l'ecran « Carte » (LOT-94), ou rien si le dossier n'existe pas."""
    if not MAPS_MANIFEST.is_file():
        return []
    return [entry["file"] for entry in json.loads(MAPS_MANIFEST.read_text(encoding="utf-8"))["maps"]]


def read_cahier() -> dict | None:
    """Le cahier des assets (T2.4), lu sans validation de schema (role de check_assets_brief.py)."""
    if not CAHIER.is_file():
        return None
    return json.loads(CAHIER.read_text(encoding="utf-8"))


def cahier_keys(cahier: dict) -> dict[str, dict]:
    """Chaque cle qu'engendre le cahier (une piece, ou une par etat/membre) -> la piece qui la porte."""
    keys: dict[str, dict] = {}
    for piece in cahier["pieces"]:
        variants = piece.get("states") or piece.get("members") or []
        if variants:
            for variant in variants:
                keys[f"{piece['key']}/{variant['id']}"] = piece
        else:
            keys[piece["key"]] = piece
    return keys


def check_produced(identifier: str, entry: dict, keys: dict[str, dict]) -> None:
    """Une illustration produite se recoupe avec le cahier, pas avec le code (voir docstring)."""
    key = entry.get("cahier")
    if not key:
        fail(f"`{identifier}` : provenance 'produced' sans cle 'cahier'")
        return
    piece = keys.get(key)
    if piece is None:
        fail(f"`{identifier}` : cle de cahier '{key}' absente de {CAHIER.relative_to(ROOT)}")
        return
    if not entry.get("prompt"):
        fail(f"`{identifier}` : aucun prompt enregistre (celui envoye au generateur)")
    if not entry.get("date"):
        fail(f"`{identifier}` : aucune date de production")
    if piece["display"] == "nine-patch":
        margins = entry.get("margins")
        if not margins:
            fail(f"`{identifier}` : piece 9-patch sans marges enregistrees")
        elif margins != piece["margins"]:
            fail(f"`{identifier}` : marges {margins}, {piece['margins']} annoncees par le cahier")


def check_illustrations(manifest: dict, keys: dict[str, dict]) -> tuple[set[str], set[str]]:
    """Chaque illustration declaree est produite, existe, et est bien celle que le manifeste decrit.

    Renvoie (tous les fichiers declares, les cles du cahier citees par une entree produite).
    """
    declared: dict[str, str] = {}
    cited: set[str] = set()
    for entry in manifest["illustrations"]:
        identifier = entry["id"]
        if identifier in declared:
            fail(f"`{identifier}` declare deux fois dans le manifeste")
        declared[identifier] = entry["file"]

        # La provenance d'abord : une image du corpus est refusee pour ce qu'elle est, avant meme
        # de savoir si son fichier est la (LOT-94).
        provenance = entry.get("provenance")
        if not provenance:
            fail(f"`{identifier}` : aucune provenance declaree")
        elif provenance != "produced":
            fail(
                f"`{identifier}` : provenance '{provenance}' interdite. Une illustration d'interface "
                f"est produite, jamais extraite du corpus (EX-IHM-076, LOT-94) : retirer l'image et "
                f"son entree."
            )
        else:
            check_produced(identifier, entry, keys)
            if entry.get("cahier"):
                cited.add(entry["cahier"])

        path = UI / entry["file"]
        if not path.is_file():
            fail(f"`{identifier}` : {entry['file']} absent de {UI.relative_to(ROOT)}")
            continue
        data = path.read_bytes()

        if hashlib.sha256(data).hexdigest() != entry["sha256"]:
            fail(
                f"`{identifier}` : {entry['file']} n'a plus l'empreinte du manifeste. "
                f"La relivrer :\n    python scripts/assetsGeneration/receive_ui_assets.py"
            )
        if len(data) != entry["bytes"]:
            fail(f"`{identifier}` : {len(data)} octets, {entry['bytes']} annonces")
        try:
            width, height = image_size(entry["file"], data)
        except (ValueError, struct.error) as error:
            fail(f"`{identifier}` : {entry['file']} n'est pas une image lisible ({error})")
            continue
        if [width, height] != entry["size"]:
            fail(f"`{identifier}` : {width}x{height}, {entry['size']} annonces")
    return set(declared.values()), cited


def check_orphan_files(declared_files: set[str]) -> None:
    """Aucun fichier d'image du dossier -- ou d'une de ses familles -- n'echappe au manifeste."""
    for path in sorted(UI.rglob("*")):
        if path.is_dir() or path.suffix.lower() not in (".jpg", ".jpeg", ".png"):
            continue
        relative = path.relative_to(UI).as_posix()
        if relative not in declared_files:
            fail(
                f"{relative} n'est declare par aucune entree du manifeste. Une image deposee a "
                f"la main ne vient d'aucune page ni d'aucun prompt, et rien ne dit d'ou elle sort."
            )


def check_code_keys(declared_files: set[str]) -> None:
    """Tout nom de fichier cite par le QML est declare. L'inverse n'est pas exige : les briques du
    T2.7 ne nomment pas une illustration produite par son fichier mais par sa cle, que
    `check_artwork` recoupe avec le cahier."""
    declared_names = {Path(name).name for name in declared_files}
    used: set[str] = set()
    for source in NAMING_SOURCES:
        if not source.is_file():
            fail(f"{source.relative_to(ROOT)} absent : plus rien ne relie ses images au code")
            continue
        used |= set(
            re.findall(
                # Un chemin facultatif devant le nom : le QML ecrit « ../../Elements/Assets/... ».
                # Seul le NOM DE FICHIER compte au recoupement.
                r'"(?:[A-Za-z0-9_./-]*/)?([A-Za-z0-9_-]+\.(?:jpe?g|png))"',
                source.read_text(encoding="utf-8"))
        )
    # Aucun nom n'est pas une lecture cassee : depuis le LOT-94, plus aucun ecran ne designe une
    # image par son fichier (les briques passent par les cles du cahier, l'arene par son composeur).
    # Les pieces d'une SCENE ne se recoupent pas ici : elles ont leur manifeste, et depuis la
    # table rase du LOT-102 la nouvelle arborescence les porte (controle au LOT-104).
    # De meme les cartes de l'ecran « Carte » : ce que ses formulaires nomment se recoupe avec leur
    # manifeste, par scripts/checks/check_map_assets.py.
    used -= set(read_maps_manifest())
    for name in sorted(used - declared_names):
        fail(f"`{name}` nomme par le code, absent du manifeste")


def check_cahier_coverage(manifest: dict, keys: dict[str, dict], cited: set[str]) -> None:
    """Chaque cle du cahier finit par avoir un fichier (`cited`), ou une mention explicite
    « non livree » dans `pending` -- sans quoi une piece oubliee disparaitrait sans que rien ne
    le remarque."""
    pending = manifest.get("pending", [])
    blanket = False
    specific: set[str] = set()
    for entry in pending:
        if not entry.get("reason"):
            fail("entree 'pending' sans raison")
        if "keys" in entry:
            for key in entry["keys"]:
                if key in specific:
                    fail(f"`{key}` : cle en double dans 'pending'")
                specific.add(key)
        else:
            blanket = True
    for key in sorted((cited | specific) - set(keys)):
        fail(f"`{key}` : citee (produite ou en attente) mais absente du cahier")
    if blanket:
        return
    for key in sorted(set(keys) - cited - specific):
        fail(f"`{key}` : ni produite ni marquee « non livree » dans le manifeste")


def artwork_table(manifest: dict) -> list[str]:
    """Les lignes de la table `delivered` d'Artwork.qml, engendrees depuis les entrees produites
    du manifeste : cle du cahier -> fichier sous Source/Elements/Assets/UI, et marges 9-patch."""
    produced = sorted(
        (entry for entry in manifest["illustrations"] if entry.get("provenance") == "produced"),
        key=lambda entry: entry["cahier"],
    )
    lines = ["    readonly property var delivered: ({"]
    for entry in produced:
        value: dict = {"file": entry["file"]}
        if entry.get("margins"):
            value["margins"] = entry["margins"]
        lines.append(f'        "{entry["cahier"]}": {json.dumps(value, ensure_ascii=False)},')
    lines.append("    })")
    return lines


def splice_artwork(text: str, table: list[str]) -> str | None:
    """Le texte d'Artwork.qml avec la table remplacee entre ses deux marqueurs, ou None s'ils
    manquent."""
    lines = text.split("\n")
    begin = next((i for i, line in enumerate(lines) if ARTWORK_BEGIN in line), None)
    end = next((i for i, line in enumerate(lines) if ARTWORK_END in line), None)
    if begin is None or end is None or end <= begin:
        return None
    return "\n".join(lines[: begin + 1] + table + lines[end:])


def write_artwork(manifest: dict) -> bool:
    """Reecrit la table d'Artwork.qml ; appele par receive_ui_assets.py apres une livraison."""
    text = ARTWORK.read_text(encoding="utf-8")
    spliced = splice_artwork(text, artwork_table(manifest))
    if spliced is None:
        return False
    if spliced != text:
        ARTWORK.write_text(spliced, encoding="utf-8", newline="\n")
    return True


def check_artwork(manifest: dict, cahier: dict | None) -> None:
    """La table d'Artwork.qml suit le manifeste, et chaque piece qu'une brique nomme en toutes
    lettres (`"ui/<famille>/<piece>`) existe dans le cahier. Une table en retard laisserait une
    image livree invisible -- la brique garderait son repli, sans rien signaler ; une cle mal
    ecrite dans une brique ne serait jamais livree, et l'aplat resterait pour toujours."""
    if not ARTWORK.is_file():
        fail(f"{ARTWORK.relative_to(ROOT)} absent : les briques ne savent pas quelles pieces sont livrees")
        return
    text = ARTWORK.read_text(encoding="utf-8")
    spliced = splice_artwork(text, artwork_table(manifest))
    if spliced is None:
        fail(f"{ARTWORK.relative_to(ROOT)} : marqueurs de la table engendree introuvables")
    elif spliced != text:
        fail(
            f"{ARTWORK.relative_to(ROOT)} : la table des pieces livrees ne suit plus le manifeste. "
            f"La reecrire :\n    python scripts/checks/check_ui_assets.py --write-artwork"
        )
    if cahier is None:
        return
    pieces = {piece["key"] for piece in cahier["pieces"]}
    named = 0
    for brick in sorted(CONTROLS.glob("*.ui.qml")):
        body = brick.read_text(encoding="utf-8")
        for key in re.findall(r'"(ui/[a-z-]+/[a-z-]+)', body):
            named += 1
            if key not in pieces:
                fail(f"{brick.relative_to(ROOT)} : nomme la piece '{key}', absente du cahier")
    if named == 0:
        fail(f"aucune brique de {CONTROLS.relative_to(ROOT)} ne nomme une piece du cahier (lecture cassee ?)")


def main() -> None:
    if "--write-artwork" in sys.argv[1:]:
        if not write_artwork(read_manifest()):
            print(f"check_ui_assets : marqueurs introuvables dans {ARTWORK.relative_to(ROOT)}", file=sys.stderr)
            sys.exit(1)
        print(f"check_ui_assets : {ARTWORK.relative_to(ROOT)} reecrit depuis le manifeste.")
        return

    manifest = read_manifest()
    cahier = read_cahier()
    if cahier is None:
        fail(f"{CAHIER.relative_to(ROOT)} absent : impossible de recouper les assets produits")
        keys: dict[str, dict] = {}
    else:
        keys = cahier_keys(cahier)

    declared_files, cited = check_illustrations(manifest, keys)
    check_orphan_files(declared_files)
    check_code_keys(declared_files)
    if cahier is not None:
        check_cahier_coverage(manifest, keys, cited)
    check_artwork(manifest, cahier)

    if errors:
        for message in errors:
            print(f"check_ui_assets : {message}", file=sys.stderr)
        sys.exit(1)
    print(
        f"check_ui_assets : {len(declared_files)} illustration(s) conforme(s) au manifeste "
        f"et nommee(s) par le code."
    )


if __name__ == "__main__":
    main()
