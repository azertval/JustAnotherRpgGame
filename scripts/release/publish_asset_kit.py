#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Publie un kit d'assets en archive immuable et le verrouille (LOT-108, tâche T1).

Pour chaque chemin de kit donné (relatif à `Source/Elements/Assets/`) :

1. contrôle le kit avec le contrôle de son arbre (`check_hd_assets.py`, `check_map_assets.py` ou
   `check_ui_assets.py`) ;
2. construit l'archive déterministe de ses images (`asset_kits.build_archive`) ;
3. si l'empreinte est celle du verrou, ne fait rien ; sinon prend le numéro suivant — **une retouche
   est un nouveau numéro, jamais une archive remplacée** ;
4. téléverse l'archive sur la release de sa région (`gh release upload`, jamais `--clobber`) ; une
   archive du même nom déjà publiée avec une autre empreinte est un refus ;
5. met à jour `Source/Elements/Assets/kits.lock.json` et le témoin d'installation du poste.

Les sources de production (`Tools/`) ne sont jamais archivées : l'archive part des pièces
installées.

Usage :
    python scripts/release/publish_asset_kit.py Regions/central-empire/capital/arenarea [...]
    python scripts/release/publish_asset_kit.py --dry-run UI     # archive et empreinte, sans publier
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import asset_kits as K  # noqa: E402

CHECKS = {
    "Common": "check_hd_assets.py",
    "Regions": "check_hd_assets.py",
    "Maps": "check_map_assets.py",
    "UI": "check_ui_assets.py",
}


def run(command: list[str], *, capture: bool = False) -> subprocess.CompletedProcess:
    # Les contrôles écrivent en UTF-8 quoi que dise la console du poste (cp1252 sous Windows).
    env = {**os.environ, "PYTHONIOENCODING": "utf-8"}
    return subprocess.run(command, cwd=K.ROOT, check=False, text=True, encoding="utf-8", errors="replace",
                          capture_output=capture, env=env)


def check_tree(path: str) -> None:
    script = K.ROOT / "scripts" / "checks" / CHECKS[path.split("/")[0]]
    result = run([sys.executable, str(script)], capture=True)
    if result.returncode != 0:
        raise K.KitError(f"{path} : {script.name} échoue\n{result.stderr.strip()}")


def release_assets(tag: str) -> list[str] | None:
    """Les archives d'une release, ou None si elle n'existe pas."""
    result = run(["gh", "release", "view", tag, "--repo", K.REPOSITORY, "--json", "assets",
                  "-q", ".assets[].name"], capture=True)
    if result.returncode != 0:
        return None
    return [line for line in result.stdout.splitlines() if line]


def ensure_release(tag: str) -> list[str]:
    names = release_assets(tag)
    if names is not None:
        return names
    notes = ("Archives immuables des kits d'assets (LOT-108). Ne rien remplacer ni supprimer : "
             "`Source/Elements/Assets/kits.lock.json` cite chaque archive par son empreinte ; "
             "`python scripts/fetch_assets.py` les installe.")
    result = run(["gh", "release", "create", tag, "--repo", K.REPOSITORY, "--title", f"Assets — {tag}",
                  "--notes", notes, "--prerelease", "--latest=false", "--target", "main"], capture=True)
    if result.returncode != 0:
        raise K.KitError(f"création de la release {tag} refusée : {result.stderr.strip()}")
    return []


def published_digest(tag: str, asset: str) -> str:
    with tempfile.TemporaryDirectory() as folder:
        result = run(["gh", "release", "download", tag, "--repo", K.REPOSITORY, "--pattern", asset,
                      "--dir", folder], capture=True)
        if result.returncode != 0:
            raise K.KitError(f"lecture de {tag}/{asset} impossible : {result.stderr.strip()}")
        return K.sha256_file(Path(folder) / asset)


def publish(paths: list[str], *, dry_run: bool) -> int:
    locked = K.read_lock()
    by_path = {k.path: k for k in locked}
    all_paths = sorted({*by_path, *paths})
    for path in paths:
        if not (K.ASSETS / path).is_dir():
            raise K.KitError(f"{path} : dossier absent sous {K.ASSETS}")
        tag = K.release_tag(path)
        check_tree(path)
        files = K.kit_files(path, all_paths)
        if not files:
            raise K.KitError(f"{path} : aucune image à publier")
        data = K.build_archive(path, files)
        digest = K.sha256_bytes(data)
        previous = by_path.get(path)
        if previous is not None and previous.sha256 == digest:
            print(f"{previous.id} : à jour ({len(files)} fichiers)")
            continue
        number = previous.number + 1 if previous else 1
        kit = K.Kit(id=f"{path}@{number}", path=path, release=tag, asset=f"{K.slug(path)}-{number}.zip",
                    sha256=digest, bytes=len(data), files=len(files))
        size = f"{len(data) / (1 << 20):.1f} Mio".replace(".", ",")
        if dry_run:
            print(f"{kit.id} : {len(files)} fichiers, {size}, sha256 {digest} (non publié)")
            continue
        names = ensure_release(tag)
        if kit.asset in names:
            if published_digest(tag, kit.asset) != digest:
                raise K.KitError(f"{tag}/{kit.asset} existe avec une autre empreinte : publication refusée "
                                 "(une archive publiée ne se remplace pas)")
        else:
            with tempfile.TemporaryDirectory() as folder:
                archive = Path(folder) / kit.asset
                archive.write_bytes(data)
                result = run(["gh", "release", "upload", tag, str(archive), "--repo", K.REPOSITORY], capture=True)
                if result.returncode != 0:
                    raise K.KitError(f"téléversement de {kit.asset} refusé : {result.stderr.strip()}")
        by_path[path] = kit
        K.write_lock(list(by_path.values()))
        K.write_witness(kit, {name: K.sha256_file(K.ASSETS / path / name) for name in files})
        print(f"{kit.id} : publié sur {tag} ({len(files)} fichiers, {size})")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("paths", nargs="+", help="chemins de kit, relatifs à Source/Elements/Assets/")
    parser.add_argument("--dry-run", action="store_true", help="construire et peser sans publier")
    args = parser.parse_args(argv)
    try:
        return publish([p.strip("/").replace("\\", "/") for p in args.paths], dry_run=args.dry_run)
    except K.KitError as error:
        print(f"publish_asset_kit : {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
