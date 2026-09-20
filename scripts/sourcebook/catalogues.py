#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Relire les catalogues **déjà livrés**, pour que le lot suivant s'y raccroche.

Un lot de contenu ne part jamais d'une page blanche : le `LOT-33` avait besoin des compétences et
des langues du `LOT-43`, le `LOT-36` a besoin des mêmes langues. Chacun doit retrouver, depuis le
mot **français du livre**, l'identifiant anglais que le catalogue porte — et c'est là que ça se
joue, parce que le mot du livre n'est presque jamais le nom du catalogue.

Trois écarts, tous rencontrés :

- le nom du catalogue porte des **variantes** héritées du lexique — `skills/deception.json`
  s'appelle « Tromperie / Supercherie », et le livre n'en emploie qu'une ;
- le **lexique** connaît des graphies que le catalogue ne porte pas — il traduit *Sylvan* par
  « sylvestre », que les blocs de créature emploient ;
- le **livre** en emploie d'autres encore, tranchées au `LOT-43` dans ``options.ALIAS_DU_LIVRE`` :
  la langue des elfes s'y nomme « elfe », quand la table des *Basic Rules* p. 38 et tous les blocs
  écrivent « elfique ».

Les trois se cumulent, et une seule fonction les cumule. C'est la raison d'être de ce module :
deux modules d'extraction qui referaient ce rapprochement chacun de leur côté n'en couvriraient
pas les mêmes cas, et la divergence ne se verrait que par une chouette géante muette en elfique.
"""
from __future__ import annotations

import json
import unicodedata
import re

from .glossaire import normaliser, normaliser_cle
from .options import ALIAS_DU_LIVRE


def identifiant(nom: str) -> str:
    """Un nom en identifiant kebab-case ASCII : « Giant Eagle » → ``giant-eagle``."""
    sans_accent = unicodedata.normalize('NFD', normaliser(nom))
    sans_accent = ''.join(c for c in sans_accent if unicodedata.category(c) != 'Mn')
    return re.sub(r'-+', '-', re.sub(r'[^a-z0-9]+', '-', sans_accent.lower())).strip('-')


def catalogue_francais(racine, dossier: str) -> dict:
    """Un catalogue livré, indexé par **toutes** ses graphies françaises et son identifiant."""
    index: dict = {}
    for chemin in sorted((racine / dossier).glob('*.json')):
        donnee = json.loads(chemin.read_text(encoding='utf-8'))
        for variante in donnee['name'].split('/'):
            index.setdefault(normaliser_cle(variante).strip(), donnee['id'])
        index.setdefault(normaliser_cle(donnee['id']), donnee['id'])
    return index


def index_avec_lexique(racine, dossier: str, lexique: list) -> dict:
    """Le catalogue d'un dossier, augmenté des graphies du lexique et des alias du livre.

    Le nom du catalogue seul ne suffit pas — voir l'en-tête du module. Les trois sources sont
    empilées dans cet ordre : le catalogue d'abord (il fait foi), puis le lexique, puis les alias
    du livre. ``setdefault`` garantit qu'une source plus tardive ne remplace jamais une plus
    ancienne.
    """
    index = catalogue_francais(racine, dossier)
    par_anglais = {normaliser_cle(identifiant(i)): i for i in set(index.values())}
    for entree in lexique:
        cible = par_anglais.get(normaliser_cle(identifiant(entree.anglais)))
        if cible is None:
            continue
        for variante in entree.francais.split('/'):
            index.setdefault(normaliser_cle(variante).strip(), cible)
    for forme, anglais in ALIAS_DU_LIVRE.items():
        cible = par_anglais.get(normaliser_cle(identifiant(anglais)))
        if cible is not None:
            index.setdefault(normaliser_cle(forme), cible)
    return index
