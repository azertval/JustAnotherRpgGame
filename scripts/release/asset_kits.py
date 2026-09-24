#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Les kits d'assets hors de l'historique Git (LOT-108, `Planning/standards/assets-hors-git-lot108.md`).

Un **kit** est un dossier d'assets publié d'un seul tenant : un lieu de `Regions/` ou de `Common/`
(sans ses sous-lieux, qui sont d'autres kits), `Maps/` ou `UI/`. Ses **images** partent en archive
immuable sur une release GitHub ; Git ne garde que les manifestes et le verrou
`Source/Elements/Assets/kits.lock.json`, qui donne l'empreinte de chaque archive.

Ce module est la partie commune de `scripts/release/publish_asset_kit.py` (publier) et de
`scripts/fetch_assets.py` (installer) : ce qu'est le fichier d'un kit, l'archive déterministe, le
verrou et les témoins d'installation. Aucune dépendance hors de la bibliothèque standard.
"""

from __future__ import annotations

import hashlib
import io
import json
import os
import zipfile
from dataclasses import asdict, dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "Source" / "Elements" / "Assets"
LOCK = ASSETS / "kits.lock.json"
# Les témoins d'installation, un par kit, hors du suivi (`.gitignore`).
WITNESSES = ASSETS / ".kits"
REPOSITORY = "azertval/JustAnotherRpgGame"
IMAGES = (".png", ".jpg", ".jpeg")
# Les arbres couverts par les kits ; `Fonts/` et `Entities/` restent suivis.
TREES = ("Common", "Regions", "Maps", "UI")
# Date fixe des entrées d'archive : deux publications du même contenu ont la même empreinte.
ZIP_DATE = (1980, 1, 1, 0, 0, 0)
LOCK_VERSION = 1


class KitError(RuntimeError):
    """Un kit, une archive ou le verrou ne tient pas son contrat."""


@dataclass(frozen=True)
class Kit:
    """Une entrée du verrou : un kit publié, à un numéro donné."""

    id: str
    path: str
    release: str
    asset: str
    sha256: str
    bytes: int
    files: int

    @property
    def number(self) -> int:
        return int(self.id.rsplit("@", 1)[1])

    @property
    def slug(self) -> str:
        return slug(self.path)


def slug(path: str) -> str:
    """Le nom de fichier d'un kit : `Regions/central-empire/capital` → `regions-central-empire-capital`."""
    return path.replace("/", "-").lower()


def release_tag(path: str) -> str:
    """La release qui porte un kit : une par région, une pour le commun du monde, les cartes, l'UI.

    L'annexe écrivait `assets/<région>` ; le tiret évite une barre oblique dans l'URL de
    téléchargement.
    """
    parts = path.split("/")
    if parts[0] == "Regions":
        if len(parts) < 2:
            raise KitError(f"{path} : un kit de Regions/ nomme au moins sa région")
        return f"assets-{parts[1]}"
    if parts[0] in TREES:
        return f"assets-{parts[0].lower()}"
    raise KitError(f"{path} : hors des arbres couverts ({', '.join(TREES)})")


def is_image(path: Path) -> bool:
    return path.suffix.lower() in IMAGES


def kit_files(path: str, kits: list[str], assets: Path | None = None) -> list[str]:
    """Les images d'un kit, relatives à son dossier, triées ; les sous-dossiers d'un autre kit exclus."""
    assets = assets or ASSETS
    base = assets / path
    nested = [k for k in kits if k != path and k.startswith(path + "/")]
    files = []
    for file in base.rglob("*"):
        if not file.is_file() or not is_image(file):
            continue
        relative = file.relative_to(assets).as_posix()
        if any(relative.startswith(n + "/") for n in nested):
            continue
        files.append(file.relative_to(base).as_posix())
    return sorted(files)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def build_archive(path: str, files: list[str], assets: Path | None = None) -> bytes:
    """L'archive déterministe d'un kit : entrées triées, date fixe, sans attribut de poste, non compressée.

    Les PNG et JPEG sont déjà compressés : les stocker tels quels garde l'archive déterministe sans
    dépendre de la version de zlib.
    """
    assets = assets or ASSETS
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", compression=zipfile.ZIP_STORED) as archive:
        for name in sorted(files):
            info = zipfile.ZipInfo(name, date_time=ZIP_DATE)
            info.external_attr = 0o100644 << 16
            info.create_system = 3
            archive.writestr(info, (assets / path / name).read_bytes())
    return buffer.getvalue()


def archive_members(data: bytes) -> dict[str, bytes]:
    """Le contenu d'une archive, en refusant tout chemin qui sortirait du dossier du kit."""
    members = {}
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for info in archive.infolist():
            name = info.filename
            if info.is_dir():
                continue
            if name.startswith("/") or ".." in Path(name).parts or ":" in name:
                raise KitError(f"entrée d'archive refusée : {name}")
            members[name] = archive.read(info)
    return members


def read_lock(lock: Path | None = None) -> list[Kit]:
    lock = lock or LOCK
    if not lock.exists():
        return []
    data = json.loads(lock.read_text(encoding="utf-8"))
    if data.get("version") != LOCK_VERSION:
        raise KitError(f"{lock} : version {data.get('version')} inconnue")
    return [Kit(**entry) for entry in data.get("kits", [])]


def write_lock(kits: list[Kit], lock: Path | None = None) -> None:
    lock = lock or LOCK
    ordered = sorted(kits, key=lambda k: k.path)
    payload = {"version": LOCK_VERSION, "kits": [asdict(k) for k in ordered]}
    lock.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")


def download_url(kit: Kit) -> str:
    return f"https://github.com/{REPOSITORY}/releases/download/{kit.release}/{kit.asset}"


def witness_path(kit_path: str, witnesses: Path | None = None) -> Path:
    witnesses = witnesses or WITNESSES
    return witnesses / f"{slug(kit_path)}.json"


def read_witness(kit_path: str, witnesses: Path | None = None) -> dict | None:
    target = witness_path(kit_path, witnesses)
    if not target.exists():
        return None
    try:
        return json.loads(target.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def write_witness(kit: Kit, files: dict[str, str], witnesses: Path | None = None) -> None:
    witnesses = witnesses or WITNESSES
    witnesses.mkdir(parents=True, exist_ok=True)
    payload = {"id": kit.id, "sha256": kit.sha256, "files": files}
    witness_path(kit.path, witnesses).write_text(
        json.dumps(payload, ensure_ascii=False, indent=1, sort_keys=True) + "\n", encoding="utf-8", newline="\n")


def cache_dir() -> Path:
    """Le cache des archives, hors de l'arbre : partagé entre clones et worktrees du poste."""
    override = os.environ.get("JADG_ASSETS_CACHE")
    if override:
        return Path(override)
    base = os.environ.get("LOCALAPPDATA") or os.environ.get("XDG_CACHE_HOME") or str(Path.home() / ".cache")
    return Path(base) / "JadgAssets"
