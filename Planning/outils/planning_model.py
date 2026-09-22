#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Modèle de la planification : lit `Planning/` et en tire versions, lots et ordre calculé.

Le dossier `Planning/` est la source ; ce module est la seule chose qui sache le lire. Le lint
(`lint_planning.py`) et le site (`build_planning_site.py`) partent tous deux de `load_planning`,
si bien qu'une fiche que le lint accepte est une fiche que le site sait afficher.

Deux formats, tous deux lus par la bibliothèque standard :

- `versions/versions.toml` : le catalogue des versions, dans l'ordre où elles se livrent ;
- `versions/**/lots/LOT-NNN-*.md` : une fiche par lot, dont l'en-tête TOML est encadré par deux
  lignes `+++` et dont le corps est du Markdown.
"""
import re
import tomllib
from dataclasses import dataclass, field
from pathlib import Path

FRONT_MATTER_RE = re.compile(r'\A\+\+\+\r?\n(.*?)\r?\n\+\+\+\r?\n?(.*)\Z', re.DOTALL)
# Trois générations d'identifiants, toutes gardées : un numéro ne se réattribue ni ne se renomme.
# `LOT-NN` et `LOT-EDITOR-NN` sont ceux de la version 0.0.0 (l'ancienne feuille de route et celle de
# l'éditeur) ; tout lot né depuis porte trois chiffres.
LOT_ID_RE = re.compile(r'^LOT-(?:EDITOR-\d{2}|\d{2,3})$')
LOT_FILE_RE = re.compile(r'^(LOT-(?:EDITOR-\d{2}|\d{2,3}))-[a-z0-9]+(?:-[a-z0-9]+)*\.md$')


def lot_sort_key(lot_id):
    """Ordre des numéros, et non des chaînes : `LOT-50` avant `LOT-100`, l'éditeur à la suite."""
    number = re.search(r'(\d+)$', lot_id)
    return ('EDITOR' in lot_id, int(number.group(1)) if number else 0, lot_id)


FILIERES = {
    'standard': 'Standard et outillage',
    'assets': 'Assets HD',
    'cartes': 'Cartes',
    'pnj': 'PNJ',
    'quete': 'Quêtes et dialogues',
    'moteur': 'Moteur',
    'regles': 'Règles et données',
    'donnees': 'Corpus et extraction',
    'interface': 'Interface',
    'editeur': 'Éditeur de cartes',
    'version': 'Recette et version',
}
STATUTS = ('a-faire', 'en-cours', 'livre', 'abandonne')
NATURES = ('detaillee', 'previsionnelle')
# Poids d'une taille dans l'avancement : un lot XL livré pèse plus qu'un lot S.
TAILLES = {'S': 1, 'M': 2, 'L': 3, 'XL': 5}


class PlanningError(Exception):
    """Fichier de planification illisible : le lint le rapporte, le site s'arrête."""


@dataclass
class Version:
    id: str
    titre: str
    nature: str
    referentiel: str
    resume: str
    dossier: str = ''
    objectifs: list = field(default_factory=list)
    criteres_de_sortie: list = field(default_factory=list)
    sources: list = field(default_factory=list)
    lots: list = field(default_factory=list)

    @property
    def slug(self):
        return 'v' + self.id.replace('.', '-')


@dataclass
class Lot:
    id: str
    titre: str
    version: str
    filiere: str
    statut: str
    taille: str
    path: Path
    resume: str = ''
    prerequis: list = field(default_factory=list)
    livrables: list = field(default_factory=list)
    criteres: list = field(default_factory=list)
    sources: list = field(default_factory=list)
    reprend: list = field(default_factory=list)
    maquettes: list = field(default_factory=list)
    corps: str = ''
    # Calculés par `compute_order`.
    debloque: int = 0
    etat: str = ''
    rang: int = 0

    @property
    def slug(self):
        return self.id.lower()


@dataclass
class Planning:
    root: Path
    versions: list
    lots: dict

    def version(self, version_id):
        return next((v for v in self.versions if v.id == version_id), None)


def split_front_matter(text, path):
    match = FRONT_MATTER_RE.match(text.lstrip('﻿'))
    if not match:
        raise PlanningError(f'{path}: en-tête TOML absent (attendu entre deux lignes `+++`)')
    try:
        return tomllib.loads(match.group(1)), match.group(2)
    except tomllib.TOMLDecodeError as error:
        raise PlanningError(f'{path}: en-tête TOML invalide — {error}') from error


def load_versions(root):
    path = root / 'versions' / 'versions.toml'
    try:
        data = tomllib.loads(path.read_text(encoding='utf-8'))
    except (OSError, tomllib.TOMLDecodeError) as error:
        raise PlanningError(f'{path}: {error}') from error
    versions = []
    for entry in data.get('version', []):
        try:
            versions.append(Version(**entry))
        except TypeError as error:
            raise PlanningError(f'{path}: version {entry.get("id", "?")} — {error}') from error
    return versions


def load_lot(path):
    meta, corps = split_front_matter(path.read_text(encoding='utf-8'), path)
    try:
        return Lot(path=path, corps=corps, **meta)
    except TypeError as error:
        raise PlanningError(f'{path}: {error}') from error


def load_planning(root):
    root = Path(root)
    versions = load_versions(root)
    lots = {}
    for path in sorted((root / 'versions').rglob('lots/LOT-*.md')):
        lot = load_lot(path)
        if lot.id in lots:
            raise PlanningError(f'{path}: {lot.id} déjà déclaré par {lots[lot.id].path}')
        lots[lot.id] = lot
    by_id = {v.id: v for v in versions}
    for lot in lots.values():
        if lot.version in by_id:
            by_id[lot.version].lots.append(lot)
    planning = Planning(root=root, versions=versions, lots=lots)
    compute_order(planning)
    return planning


def descendants(lots):
    """Pour chaque lot, l'ensemble des lots qui en dépendent, directement ou en cascade."""
    children = {lot_id: set() for lot_id in lots}
    for lot in lots.values():
        for parent in lot.prerequis:
            if parent in children:
                children[parent].add(lot.id)
    closure = {}

    def visit(lot_id, trail):
        if lot_id in closure:
            return closure[lot_id]
        if lot_id in trail:  # cycle : le lint le signale, ici on ne boucle pas
            return set()
        result = set()
        for child in children[lot_id]:
            result.add(child)
            result |= visit(child, trail | {lot_id})
        closure[lot_id] = result
        return result

    for lot_id in lots:
        visit(lot_id, frozenset())
    return closure


def find_cycle(lots):
    """Un cycle de prérequis sous forme de liste d'identifiants, ou None."""
    state = {}

    def visit(lot_id, trail):
        state[lot_id] = 1
        for parent in lots[lot_id].prerequis:
            if parent not in lots:
                continue
            if state.get(parent) == 1:
                return trail[trail.index(parent):] + [parent] if parent in trail else [parent]
            if parent not in state:
                found = visit(parent, trail + [parent])
                if found:
                    return found
        state[lot_id] = 2
        return None

    for lot_id in lots:
        if lot_id not in state:
            found = visit(lot_id, [lot_id])
            if found:
                return found
    return None


def compute_order(planning):
    """L'ordre n'est pas arbitré, il se calcule.

    À chaque pas, parmi les lots dont tous les prérequis sont faits : celui de la version la plus
    proche ; à version égale, celui qui débloque le plus de lots ; à égalité, le plus petit numéro.
    C'est la règle de l'ancienne feuille de route, reprise telle quelle.
    """
    lots = planning.lots
    closure = descendants(lots)
    version_rank = {v.id: index for index, v in enumerate(planning.versions)}
    for lot in lots.values():
        lot.debloque = sum(1 for d in closure[lot.id] if lots[d].statut not in ('livre', 'abandonne'))

    done = {i for i, lot in lots.items() if lot.statut in ('livre', 'abandonne')}
    remaining = {i for i in lots if i not in done}
    for lot_id in remaining:
        lot = lots[lot_id]
        ready = all(p in done or p not in lots for p in lot.prerequis)
        lot.etat = 'en-cours' if lot.statut == 'en-cours' else ('pret' if ready else 'en-attente')
    for lot_id in done:
        lots[lot_id].etat = lots[lot_id].statut

    rank = 0
    simulated = set(done)
    while remaining:
        ready = [i for i in remaining if all(p in simulated or p not in lots for p in lots[i].prerequis)]
        if not ready:  # cycle : les lots restants gardent le rang 0
            break
        ready.sort(key=lambda i: (version_rank.get(lots[i].version, 999), -lots[i].debloque, lot_sort_key(i)))
        chosen = ready[0]
        rank += 1
        lots[chosen].rang = rank
        simulated.add(chosen)
        remaining.discard(chosen)

    pending = [lot for lot in lots.values() if lot.rang and lot.statut != 'en-cours']
    if pending:
        first = min(pending, key=lambda lot: lot.rang)
        if first.etat == 'pret':
            first.etat = 'prochain'


def progress(lots):
    """(poids livré, poids total) d'une liste de lots ; les lots abandonnés ne comptent pas."""
    counted = [lot for lot in lots if lot.statut != 'abandonne']
    total = sum(TAILLES.get(lot.taille, 1) for lot in counted)
    done = sum(TAILLES.get(lot.taille, 1) for lot in counted if lot.statut == 'livre')
    return done, total
