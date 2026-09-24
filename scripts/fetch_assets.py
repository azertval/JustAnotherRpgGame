#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Installe les kits d'assets que le verrou attend (LOT-108, tâche T2).

Les images de `Source/Elements/Assets/{Common,Regions,Maps,UI}` ne sont pas suivies par Git : elles
viennent d'archives immuables publiées sur les releases du dépôt, dont
`Source/Elements/Assets/kits.lock.json` donne l'empreinte. Pour chaque kit du verrou :

- **déjà en place** : l'archive reconstruite depuis le disque a l'empreinte du verrou (l'archive
  est déterministe) — rien n'est téléchargé ni réécrit ;
- sinon l'archive est prise dans le cache du poste (`%LOCALAPPDATA%\\JadgAssets`, ou
  `JADG_ASSETS_CACHE`), téléchargée au besoin, vérifiée, puis extraite dans le dossier du kit ;
- **le travail local n'est jamais écrasé** : une image modifiée ou ajoutée à la main dans un kit
  verrouillé arrête l'installation, sauf `--force`. Une image d'une version précédente du kit,
  intacte (le témoin d'installation la connaît), est remplacée ou retirée sans question.

Chaque kit installé laisse un témoin dans `Source/Elements/Assets/.kits/` (non suivi) : la
configuration CMake le vérifie et échoue avec cette commande à lancer.

Usage :
    python scripts/fetch_assets.py            # installe ce qui manque
    python scripts/fetch_assets.py --check    # vérifie sans rien télécharger ; code non nul si écart
    python scripts/fetch_assets.py --force    # remplace aussi les images modifiées à la main
"""

from __future__ import annotations

import argparse
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "release"))
import asset_kits as K  # noqa: E402

ATTEMPTS = 3


def download(kit: K.Kit, cache: Path) -> bytes:
    """L'archive d'un kit, depuis le cache ou la release ; son empreinte est toujours vérifiée."""
    cached = cache / f"{kit.sha256}.zip"
    if cached.exists():
        data = cached.read_bytes()
        if K.sha256_bytes(data) == kit.sha256:
            return data
        cached.unlink()
    url = K.download_url(kit)
    last_error: Exception | None = None
    for attempt in range(ATTEMPTS):
        try:
            with urllib.request.urlopen(url, timeout=120) as response:  # noqa: S310 (URL du dépôt)
                data = response.read()
            break
        except (urllib.error.URLError, TimeoutError) as error:
            last_error = error
            time.sleep(2 * (attempt + 1))
    else:
        raise K.KitError(f"{kit.id} : téléchargement impossible ({url}) : {last_error}")
    digest = K.sha256_bytes(data)
    if digest != kit.sha256:
        raise K.KitError(f"{kit.id} : l'archive téléchargée a l'empreinte {digest}, le verrou attend {kit.sha256}")
    cache.mkdir(parents=True, exist_ok=True)
    temporary = cached.with_suffix(".part")
    temporary.write_bytes(data)
    temporary.replace(cached)
    return data


def in_place(kit: K.Kit, paths: list[str]) -> tuple[bool, list[str]]:
    files = K.kit_files(kit.path, paths)
    if not files:
        return False, files
    return K.sha256_bytes(K.build_archive(kit.path, files)) == kit.sha256, files


def install(kit: K.Kit, paths: list[str], cache: Path, *, force: bool) -> str:
    ok, on_disk = in_place(kit, paths)
    if ok:
        if (K.read_witness(kit.path) or {}).get("sha256") != kit.sha256:
            K.write_witness(kit, {n: K.sha256_file(K.ASSETS / kit.path / n) for n in on_disk})
        return "en place"
    members = K.archive_members(download(kit, cache))
    expected = {name: K.sha256_bytes(data) for name, data in members.items()}
    known = (K.read_witness(kit.path) or {}).get("files", {})
    base = K.ASSETS / kit.path
    conflicts, stale = [], []
    for name in on_disk:
        current = K.sha256_file(base / name)
        if expected.get(name) == current:
            continue
        if known.get(name) == current:
            if name not in expected:
                stale.append(name)
            continue
        conflicts.append(name if name in expected else f"{name} (absente du kit)")
    if conflicts and not force:
        listed = "\n  ".join(conflicts[:20]) + ("\n  …" if len(conflicts) > 20 else "")
        raise K.KitError(f"{kit.id} : {len(conflicts)} image(s) modifiée(s) ou ajoutée(s) à la main, "
                         f"installation arrêtée (--force pour les remplacer) :\n  {listed}")
    if force:
        stale += [c.removesuffix(" (absente du kit)") for c in conflicts if c.endswith("(absente du kit)")]
    for name, data in members.items():
        target = base / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() or K.sha256_file(target) != expected[name]:
            target.write_bytes(data)
    for name in stale:
        (base / name).unlink(missing_ok=True)
    K.write_witness(kit, expected)
    return f"installé ({len(members)} fichiers)"


def check(kits: list[K.Kit], paths: list[str]) -> int:
    errors = 0
    for kit in kits:
        ok, files = in_place(kit, paths)
        if ok:
            print(f"{kit.id} : conforme ({len(files)} fichiers)")
        else:
            errors += 1
            print(f"{kit.id} : absent ou différent du verrou", file=sys.stderr)
    if errors:
        print(f"{errors} kit(s) à installer : python scripts/fetch_assets.py", file=sys.stderr)
    return 1 if errors else 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="vérifier sans télécharger")
    parser.add_argument("--force", action="store_true", help="remplacer les images modifiées à la main")
    args = parser.parse_args(argv)
    try:
        kits = K.read_lock()
        paths = [k.path for k in kits]
        if args.check:
            return check(kits, paths)
        cache = K.cache_dir()
        for kit in kits:
            print(f"{kit.id} : {install(kit, paths, cache, force=args.force)}")
    except K.KitError as error:
        print(f"fetch_assets : {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
