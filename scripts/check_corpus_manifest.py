#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Garde-fou : le manifeste du corpus se charge, et son contrôle d'empreinte fonctionne.

`scripts/sourcebook/corpus.toml` décrit les documents du corpus et leur empreinte (`LOT-30`,
`EX-CNT-020`). Rien, en intégration continue, ne pouvait le vérifier : les PDF ne sont pas sur le
runner et ne le seront pas (`EX-CNT-023`), si bien qu'une faute de frappe dans le manifeste — une
pagination inconnue, un champ manquant, un type mal orthographié — ne se découvrait qu'au prochain
poste qui lançait une extraction, c'est-à-dire potentiellement des semaines plus tard.

Ce contrôle sépare les deux questions :

1. **Le manifeste est-il bien formé ?** Cette question ne demande aucun PDF : elle se pose sur le
   fichier versionné, et se pose donc en CI.
2. **Le document présent est-il celui que le manifeste décrit ?** Celle-là demande le corpus, et
   reste la charge de `python scripts/sourcebook verifier`, sur un poste qui l'a.

L'**auto-test** éprouve d'abord le contrôle sur un corpus fictif, écrit dans un dossier temporaire
— y compris la forme la plus récente et la moins éprouvée, la **collection** de fichiers (`LOT-38`),
dont l'empreinte porte sur la liste de ses membres et non sur un fichier. Sans lui, ce script serait
vert parce qu'il ne vérifie rien, et le resterait s'il était cassé.

Usage :
    python scripts/check_corpus_manifest.py     # code de sortie non nul si problème
"""
from __future__ import annotations

import hashlib
import sys
import tempfile
from pathlib import Path

RACINE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(RACINE / 'scripts'))

from sourcebook.corpus import Corpus, CorpusError  # noqa: E402  (après l'ajout au sys.path)

MANIFESTE_FICTIF = """
version = 1

[documents.planche]
fichier = "planche.jpg"
type = "image"
sha256 = "%(planche)s"
pagination = "aucune"
decalage = 0
provenance = "tanares"
langue = "en"
resume = "une planche seule"

[documents.jetons]
fichier = "jetons"
type = "collection"
motif = "*.png"
nombre = 2
sha256 = "%(jetons)s"
pagination = "aucune"
decalage = 0
provenance = "tanares"
langue = "en"
resume = "un dossier de jetons"
"""


def _empreinte(chemin: Path) -> str:
    digest = hashlib.sha256()
    digest.update(chemin.read_bytes())
    return digest.hexdigest()


def _empreinte_collection(dossier: Path, motif: str) -> str:
    digest = hashlib.sha256()
    for fichier in sorted(dossier.glob(motif)):
        digest.update(('%s:%s' % (fichier.name, _empreinte(fichier)) + chr(10)).encode('utf-8'))
    return digest.hexdigest()


def auto_test() -> list[str]:
    """Éprouve le chargement et la vérification sur un corpus fictif."""
    echecs: list[str] = []
    with tempfile.TemporaryDirectory() as brut:
        racine = Path(brut)
        (racine / 'planche.jpg').write_bytes(b'une planche')
        dossier = racine / 'jetons'
        dossier.mkdir()
        (dossier / 'a.png').write_bytes(b'jeton a')
        (dossier / 'b.png').write_bytes(b'jeton b')

        manifeste = racine / 'corpus.toml'
        manifeste.write_text(
            MANIFESTE_FICTIF % {
                'planche': _empreinte(racine / 'planche.jpg'),
                'jetons': _empreinte_collection(dossier, '*.png'),
            },
            encoding='utf-8')

        corpus = Corpus.charger(racine_corpus=racine, manifeste=manifeste)
        for cle in ('planche', 'jetons'):
            try:
                corpus[cle].verifier()
            except CorpusError as erreur:
                echecs.append('auto-test « %s conforme » : refusé — %s' % (cle, erreur))

        # Un jeton retouché doit être vu : c'est toute la raison d'une empreinte de collection.
        (dossier / 'b.png').write_bytes(b'jeton b retouche')
        try:
            corpus['jetons'].verifier()
        except CorpusError:
            pass
        else:
            echecs.append('auto-test : un jeton retouché passe le contrôle d\'empreinte')

        # Un jeton retiré doit l'être aussi, et par le COMPTE avant l'empreinte : le message doit
        # dire ce qui manque, pas seulement que l'empreinte diffère.
        (dossier / 'b.png').unlink()
        try:
            corpus['jetons'].verifier()
        except CorpusError as erreur:
            if 'déclarés dans le manifeste' not in str(erreur):
                echecs.append('auto-test : un jeton retiré est signalé sans dire qu\'il manque')
        else:
            echecs.append('auto-test : un jeton retiré passe le contrôle')

        # Un type inconnu doit être refusé au CHARGEMENT, pas à l'usage.
        manifeste.write_text(
            manifeste.read_text(encoding='utf-8').replace('type = "image"', 'type = "planche"'),
            encoding='utf-8')
        try:
            Corpus.charger(racine_corpus=racine, manifeste=manifeste)
        except CorpusError:
            pass
        else:
            echecs.append('auto-test : un type inconnu est accepté au chargement')
    return echecs


def main() -> int:
    echecs = auto_test()
    if echecs:
        print('check_corpus_manifest : AUTO-TEST EN ÉCHEC — le contrôle lui-même est cassé.\n')
        for echec in echecs:
            print('  - ' + echec)
        return 1

    try:
        corpus = Corpus.charger()
    except CorpusError as erreur:
        print('check_corpus_manifest : %s' % erreur, file=sys.stderr)
        return 1

    collections = sum(1 for document in corpus if document.collection)
    print('check_corpus_manifest : OK (%d document(s) déclaré(s), dont %d collection(s) ; '
          'auto-test vert).' % (len(corpus), collections))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
