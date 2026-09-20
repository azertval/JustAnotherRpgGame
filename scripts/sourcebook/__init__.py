# -*- coding: utf-8 -*-
"""Chaîne d'extraction du corpus source de JustAnotherRpgGame (LOT-30).

``Documentation/SourceBook/`` rassemble huit PDF — environ 1 200 pages, 280 Mo — dont sortiront
les créatures, l'équipement, les sorts, les espèces, les classes et les dix régions du jeu. Ce
paquet est l'outil qui les en tire, et le manifeste qui dit à quoi il a affaire.

- :mod:`corpus` — le manifeste : empreintes, pagination, provenance.
- :mod:`extraction` — texte, tableaux par coordonnée, images par rendu clippé, cache.
- :mod:`glossaire` — la première sortie de la chaîne : le lexique bilingue.

Ligne de commande : ``python scripts/sourcebook --aide`` ou ``python -m sourcebook`` depuis
``scripts/``.
"""

from .corpus import Corpus, CorpusError, Document
from .extraction import ExtractionError, Extracteur, Region

__all__ = [
    'Corpus',
    'CorpusError',
    'Document',
    'Extracteur',
    'ExtractionError',
    'Region',
]
