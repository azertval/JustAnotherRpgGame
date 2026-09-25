# -*- coding: utf-8 -*-
"""Chaîne d'extraction du corpus source de JustAnotherRpgGame (LOT-30).

``Documentation/SourceBook/`` rassemble les dix-sept documents du manifeste ``corpus.toml`` —
quatorze PDF (environ 1 600 pages, 280 Mo), deux cartes et un dossier de 172 jetons — dont sortent
les créatures, l'équipement, les espèces, les classes et les dix régions du jeu. Ce paquet est
l'outil qui les en tire, et le manifeste qui dit à quoi il a affaire.

- :mod:`corpus` — le manifeste : empreintes, pagination, provenance.
- :mod:`extraction` — texte, tableaux par coordonnée, images par rendu clippé, cache.
- :mod:`mise_en_page` — la mise en page à deux colonnes des documents aidedd.
- :mod:`catalogues` — relecture des catalogues déjà livrés, où le lot suivant se raccroche.
- :mod:`glossaire` — la première sortie de la chaîne : le lexique bilingue.
- :mod:`options`, :mod:`bestiaire`, :mod:`personnage`, :mod:`equipement`, :mod:`atlas` — les
  catalogues produits : options de personnage, bêtes, espèces et classes, équipement, régions.

Ligne de commande : ``python scripts/sourcebook -h`` ou ``python -m sourcebook -h`` depuis
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
