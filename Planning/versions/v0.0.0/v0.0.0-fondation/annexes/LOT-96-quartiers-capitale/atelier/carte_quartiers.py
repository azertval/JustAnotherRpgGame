#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Trace les cartes des quartiers de la Capitale (`LOT-96`).

> **Retiré le 19 septembre 2026 (`LOT-EDITOR-06`).** L'éditeur fait foi (décision D4 de la
> feuille de route de l'éditeur) : `capital/martpart.json` et `capital/arenarea.json` se modifient
> dans `LevelEditor`, à la souris ou par `LevelEditor --apply`, et `LevelEditor --check` les garde
> en CI. Le script reste ici comme trace de la façon dont les cartes ont été posées. Sa dernière
> génération a été commitée telle quelle, puis retouchée dans l'éditeur ; il n'écrit donc plus
> jamais dans `Source/Elements` : il trace dans le dossier que nomme `--sortie`, et son `--check`
> est parti.

Même méthode que le Colisée du `LOT-09` : la carte se **pose par script**, puis se retouche dans
l'éditeur (`LOT-11`). Le fichier produit est un fichier de niveau ordinaire, sans marque d'origine.

## Ce que le script relève sur le plan, et ce qu'il décide

Le plan de la ville est celui que l'auteur a peint (`city-central-empire-the-capital-city.jpg`,
`LOT-94`) ; les douze quartiers y sont placés en fractions (`world-maps.json`). Le script en tire
une seule chose : **la direction** de chaque quartier voisin, vue du quartier qu'on trace. Une
porte vers un quartier se pose sur le bord de la carte que coupe cette direction ; c'est ce qui
fait qu'on sort de Martpart vers le nord-ouest pour aller à Arenarea, comme sur le plan.

Il décide le reste, qui n'est écrit nulle part : les rues, la place, les îlots de maisons, les
ruelles, les étals. Un quartier du livre n'a pas de plan rue par rue.

## Les trois grilles, comme au Colisée

- la grille **racine** est la collision : `wall` où l'on ne passe pas ; une case franchissable qui
  porte une pièce reçoit `dirt` pour que son assignation de texture soit émise ;
- la couche **sol** porte la fente de matière de chaque case (`dirt` la rue, `solid` la place,
  `bridge` la ruelle, `stairs` le pas de porte), que `Assets/Scene/<lieu>/appearance.json` traduit ;
- la couche **décor** porte `wall` sur chaque case de relief, dont l'assignation nomme la pièce.

Depuis le `LOT-EDITOR-12`, ce tracé v3 passe par `LevelEditor --migrate` avant d'être écrit : les
cartes commitées sont en v4. Il faut donc avoir construit l'éditeur.

Usage :

    py -3.13 Planning/versions/v0.0.0/v0.0.0-fondation/annexes/LOT-96-quartiers-capitale/atelier/carte_quartiers.py --sortie DOSSIER

Les tracés s'écrivent dans `DOSSIER/<quartier>.json` ; le dossier des cartes du jeu est refusé.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import math
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

RACINE = Path(__file__).resolve().parents[4]
NIVEAUX = RACINE / "Source" / "Elements" / "Levels" / "capital"

# -- Format v4 (LOT-EDITOR-12) -------------------------------------------------------------------
#
# Ce script trace une carte v3 ; le dépôt ne garde que des v4. La conversion n'est pas refaite ici :
# c'est l'éditeur qui la porte (`LevelEditor --migrate`), une seule fois pour toutes les cartes. Le
# script en dépend donc : il faut avoir construit l'éditeur (`scripts/build.ps1`). Il a vécu
# jusqu'au LOT-EDITOR-06, qui a fait de l'éditeur la source des cartes faites à la main.
EDITEUR = RACINE / "build" / "ninja" / "bin" / "LevelEditor.exe"


def en_v4(texte_v3: str) -> str:
    """La carte v3 que trace ce script, migrée en v4 canonique par l'éditeur."""
    if not EDITEUR.is_file():
        raise SystemExit(f"{EDITEUR} est absent : construire l'éditeur d'abord (scripts/build.ps1).")
    with tempfile.TemporaryDirectory() as dossier:
        entree = Path(dossier) / "carte.json"
        sortie = Path(dossier) / "carte-v4.json"
        entree.write_text(texte_v3, encoding="utf-8", newline="\n")
        subprocess.run([str(EDITEUR), "--data", str(RACINE / "Source" / "Elements"), "--migrate",
                        str(entree), "--output", str(sortie)], check=True, capture_output=True)
        return sortie.read_text(encoding="utf-8")

PLAN = RACINE / "Source" / "Elements" / "Maps" / "world-maps.json"
VILLE_JOUABLE = RACINE / "Source" / "Elements" / "World" / "cities" / "capital.json"

# La sentinelle d'une porte gardée (phase 4) : son dialogue, et sa figurine — celle du soldat
# Ironhand de l'atelier des monstres (LOT-93), qui l'a posée dans les cartes sans que ce script la
# suive ; alignée au LOT-EDITOR-12.
SENTINELLE_DIALOGUE = "sentinelle-ironhand"
SENTINELLE_FIGURINE = "Monsters/ironhand-soldier"

# La console Windows est en cp1252 : les messages portent des accents.
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

VILLE = "central-empire-the-capital-city"
# Le plan fait 1920 x 1080 : les fractions se ramènent en pixels pour que les directions ne
# soient pas écrasées par le format.
PLAN_LARGEUR, PLAN_HAUTEUR = 1920, 1080

# Écart minimal entre deux portes d'un même bord, en cases : trois de rue et deux de maison.
ECART_PORTES = 6
# Profondeur de la rue d'une porte avant qu'elle ne tourne vers le cœur du quartier.
PROFONDEUR_PORTE = 4


def rect(x1: int, y1: int, x2: int, y2: int):
    """Les cases d'un rectangle, bornes incluses."""
    for y in range(y1, y2 + 1):
        for x in range(x1, x2 + 1):
            yield x, y


def variante(x: int, y: int, n: int) -> int:
    """Un choix stable par case : la même case donne toujours la même pièce."""
    return (x * 7 + y * 13 + (x * y) % 5) % n


@dataclass
class Porte:
    """Une porte du quartier, sur un bord de la carte."""

    vers: str  # identifiant du quartier voisin (fiche d'atlas)
    nom: str  # nom court, qui nomme aussi le point d'arrivée
    # La carte du voisin, s'il en a une : la porte est alors un portail, qui arrive au point
    # nommé d'après ce quartier-ci. Sans carte, la porte est gardée (phase 4).
    carte: str | None = None
    # Vrai si la porte est gardée : une sentinelle Ironhand se tient sur la case du bord.
    gardee: bool = False
    x: int = 0
    y: int = 0
    bord: str = ""  # "N", "S", "O", "E"

    def interieur(self, pas: int) -> tuple[int, int]:
        """La case à @p pas cases de la porte, vers l'intérieur de la carte."""
        dx, dy = {"N": (0, 1), "S": (0, -1), "O": (1, 0), "E": (-1, 0)}[self.bord]
        return self.x + dx * pas, self.y + dy * pas


@dataclass
class Quartier:
    ident: str  # identifiant de carte : capital/<ident>
    fiche: str  # identifiant de la fiche d'atlas
    nom: str
    lieu: str  # le dossier de planches : Assets/Scene/<lieu>/
    largeur: int
    hauteur: int
    place: tuple[int, int, int, int]  # la place, cœur du quartier
    batiments: list[tuple[int, int, int, int]]  # grands bâtiments fermés (arènes, temples)
    portes: list[Porte]
    depart: Porte | None = None  # la porte de la ville où « Nouvelle partie » pose le héros
    # Rues secondaires, en L, d'un point à un autre : ce que les portes ne desservent pas.
    rues: list[tuple[tuple[int, int], tuple[int, int]]] = field(default_factory=list)
    # Vrai si la place est un marché : des rangées d'étals. Un parvis n'en a pas.
    etals: bool = True
    # Les îlots que le plan de la ville montre (entités `cityBlock`) : un nom, qui est aussi la clé
    # de son libellé (`city_block.<nom>`), et un rectangle de cases, bornes incluses.
    ilots: list[tuple[str, tuple[int, int, int, int]]] = field(default_factory=list)
    sol: dict = field(default_factory=dict)
    relief: dict = field(default_factory=dict)
    obstacles: set = field(default_factory=set)


# --------------------------------------------------------------------------------------------------
# Le plan : directions et bords
# --------------------------------------------------------------------------------------------------


def points_du_plan() -> dict[str, tuple[float, float]]:
    plan = json.loads(PLAN.read_text(encoding="utf-8"))
    places = plan["cities"][VILLE]["places"]
    return {cle[len(VILLE) + 1:]: (x * PLAN_LARGEUR, y * PLAN_HAUTEUR) for cle, (x, y) in places.items()}


def poser_portes(q: Quartier, points: dict[str, tuple[float, float]]) -> None:
    """Pose chaque porte sur le bord que coupe la direction du quartier voisin, vue du plan."""
    ox, oy = points[q.ident]
    cx, cy = (q.largeur - 1) / 2, (q.hauteur - 1) / 2
    for porte in q.portes:
        vx, vy = points[porte.vers[len(VILLE) + 1:]]
        dx, dy = vx - ox, vy - oy
        # Le rayon depuis le centre de la carte ; le premier bord qu'il touche porte la porte.
        tx = (cx / abs(dx)) if dx else math.inf
        ty = (cy / abs(dy)) if dy else math.inf
        if tx <= ty:
            porte.bord = "E" if dx > 0 else "O"
            porte.x = q.largeur - 1 if dx > 0 else 0
            porte.y = round(cy + dy * tx)
        else:
            porte.bord = "S" if dy > 0 else "N"
            porte.y = q.hauteur - 1 if dy > 0 else 0
            porte.x = round(cx + dx * ty)
    # Deux portes d'un même bord s'écartent, dans l'ordre où le plan les met : on ne passe pas
    # deux rues dans la même.
    for bord in "NSOE":
        sur_le_bord = [p for p in q.portes + ([q.depart] if q.depart else []) if p.bord == bord]
        horizontal = bord in "NS"
        sur_le_bord.sort(key=lambda p: p.x if horizontal else p.y)
        limite = (q.largeur if horizontal else q.hauteur) - 3
        for i, porte in enumerate(sur_le_bord):
            position = porte.x if horizontal else porte.y
            position = max(position, 3 if i == 0 else sur_le_bord[i - 1].x + ECART_PORTES
                           if horizontal else sur_le_bord[i - 1].y + ECART_PORTES)
            position = min(position, limite - ECART_PORTES * (len(sur_le_bord) - 1 - i))
            if horizontal:
                porte.x = position
            else:
                porte.y = position


# --------------------------------------------------------------------------------------------------
# Le tracé
# --------------------------------------------------------------------------------------------------


def peindre(q: Quartier, cases, matiere: str) -> None:
    for x, y in cases:
        if 0 <= x < q.largeur and 0 <= y < q.hauteur:
            q.sol[(x, y)] = matiere


def rue(q: Quartier, a: tuple[int, int], b: tuple[int, int]) -> None:
    """Une rue de trois cases de large, en L : d'abord le long de x, puis le long de y."""
    (x1, y1), (x2, y2) = a, b
    peindre(q, rect(min(x1, x2) - 1, y1 - 1, max(x1, x2) + 1, y1 + 1), "dirt")
    peindre(q, rect(x2 - 1, min(y1, y2) - 1, x2 + 1, max(y1, y2) + 1), "dirt")


def rue_depuis_porte(q: Quartier, porte: Porte, anneau: tuple[int, int, int, int]) -> None:
    """La rue d'une porte : droit vers l'intérieur, puis vers l'anneau qui ceint la place."""
    coude = porte.interieur(PROFONDEUR_PORTE)
    x1, y1, x2, y2 = anneau
    # Le point de l'anneau le plus proche du coude.
    cible = (min(max(coude[0], x1 + 1), x2 - 1), min(max(coude[1], y1 + 1), y2 - 1))
    if porte.bord in "NS":
        rue(q, (porte.x, porte.y), (porte.x, coude[1]))
        # Le long de la ligne du coude jusqu'à la colonne de l'anneau, puis le long de la colonne.
        rue(q, (porte.x, coude[1]), (cible[0], cible[1]))
    else:
        rue(q, (porte.x, porte.y), (coude[0], porte.y))
        rue(q, (coude[0], porte.y), (cible[0], cible[1]))


def tracer_voirie(q: Quartier) -> tuple[int, int, int, int]:
    px1, py1, px2, py2 = q.place
    anneau = (px1 - 3, py1 - 3, px2 + 3, py2 + 3)
    ax1, ay1, ax2, ay2 = anneau
    # L'anneau : trois cases de rue autour de la place.
    for x, y in rect(*anneau):
        if not (px1 <= x <= px2 and py1 <= y <= py2):
            q.sol[(x, y)] = "dirt"
    peindre(q, rect(*q.place), "solid")
    for porte in q.portes + ([q.depart] if q.depart else []):
        rue_depuis_porte(q, porte, (ax1 + 1, ay1 + 1, ax2 - 1, ay2 - 1))
    for a, b in q.rues:
        rue(q, a, b)
    return anneau


def percer_ruelles(q: Quartier) -> None:
    """Les ruelles : une grille de passages d'une case à travers les îlots de maisons.

    Elles ne traversent ni les grands bâtiments ni le bord de la carte ; un tronçon qui ne
    rejoint aucune rue est rebouché ensuite (`reboucher`), plutôt que de laisser une cour murée.
    """
    # Une ruelle court entre deux croisements ; un tronçon sur trois manque, ce qui fait des
    # impasses et des îlots de tailles inégales plutôt qu'un damier.
    for x, y in rect(3, 3, q.largeur - 4, q.hauteur - 4):
        if (x, y) in q.sol:
            continue
        if any(bx1 - 1 <= x <= bx2 + 1 and by1 - 1 <= y <= by2 + 1
               for bx1, by1, bx2, by2 in q.batiments):
            continue
        verticale = x % 6 == 3 and variante(x // 6, y // 5, 3) != 0
        horizontale = y % 5 == 2 and variante(x // 6 + 1, y // 5 + 2, 3) != 0
        if verticale or horizontale:
            q.sol[(x, y)] = "bridge"


def reboucher(q: Quartier, depart: tuple[int, int]) -> None:
    """Ce qu'on n'atteint pas depuis la porte redevient maison : aucune cour inatteignable."""
    vus = {depart}
    pile = [depart]
    while pile:
        x, y = pile.pop()
        for voisin in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if voisin in q.sol and voisin not in vus:
                vus.add(voisin)
                pile.append(voisin)
    for case in list(q.sol):
        if case not in vus:
            del q.sol[case]


def habiller(q: Quartier) -> None:
    """Les pièces : façades des maisons, grands bâtiments, étals de la place, lanternes."""

    def franchissable(x: int, y: int) -> bool:
        return (x, y) in q.sol and (x, y) not in q.obstacles

    # La place : des rangées d'étals, avec des allées entre elles ; une bannière au milieu.
    px1, py1, px2, py2 = q.place
    for y in range(py1 + 2, py2 - 1, 4) if q.etals else ():
        for x in range(px1 + 2, px2 - 1, 3):
            piece = ("prop-1", "prop-2", "prop-4", "prop-1")[variante(x, y, 4)]
            q.relief[(x, y)] = piece
            q.obstacles.add((x, y))
    centre = ((px1 + px2) // 2, (py1 + py2) // 2)
    q.relief[centre] = "prop-3"
    q.obstacles.add(centre)
    for coin in ((px1, py1), (px2, py1), (px1, py2), (px2, py2)):
        q.relief[coin] = "light"
        q.obstacles.add(coin)

    # Les grands bâtiments : un bloc de murs, une arche sur la face qui regarde la place.
    for bx1, by1, bx2, by2 in q.batiments:
        for x, y in rect(bx1, by1, bx2, by2):
            q.sol.pop((x, y), None)

    cours: list[tuple[int, int]] = []
    for y in range(q.hauteur):
        for x in range(q.largeur):
            if (x, y) in q.sol:
                continue
            nord, sud = franchissable(x, y - 1), franchissable(x, y + 1)
            ouest, est = franchissable(x - 1, y), franchissable(x + 1, y)
            if (nord or sud) and (ouest or est):
                q.relief[(x, y)] = "wall-corner"
            elif nord or sud:
                q.relief[(x, y)] = ("window-right", "wall-right", "door-right", "window-right",
                                    "wall-right")[variante(x, y, 5)]
            elif ouest or est:
                q.relief[(x, y)] = ("window-left", "wall-left", "door-left", "window-left",
                                    "wall-left")[variante(x, y, 5)]
            else:
                # Le cœur d'un îlot : ni mur (des murs pleins y dessinaient un treillis de
                # clôtures) ni pavé (voir plus bas).
                cours.append((x, y))

    # Les cours ne reçoivent rien : ni sol ni pièce. La planche n'a pas de toit, et un pavé de
    # place les faisait lire comme des places ouvertes derrière des murets.
    del cours

    # Les grands bâtiments : leurs faces portent des arches et des bannières, pas des fenêtres.
    for bx1, by1, bx2, by2 in q.batiments:
        for x, y in rect(bx1, by1, bx2, by2):
            if (x, y) in q.relief and q.relief[(x, y)].startswith(("window-", "door-")):
                q.relief[(x, y)] = "prop-3" if (x + y) % 4 == 0 else q.relief[(x, y)].replace(
                    "window", "wall").replace("door", "wall")

    # Un pas de porte devant chaque porte de maison.
    for (x, y), piece in list(q.relief.items()):
        if piece.startswith("door-"):
            for voisin in ((x, y + 1), (x, y - 1), (x + 1, y), (x - 1, y)):
                if q.sol.get(voisin) == "dirt" and voisin not in q.obstacles:
                    q.sol[voisin] = "stairs"
                    break

    # Lanternes le long des rues, du côté des maisons : une toutes les sept cases.
    for (x, y), matiere in sorted(q.sol.items()):
        if matiere != "dirt" or (x, y) in q.obstacles or (x * 3 + y) % 7:
            continue
        # Jamais au bord : c'est là que se tiennent les portes, et leurs sentinelles.
        if x in (0, q.largeur - 1) or y in (0, q.hauteur - 1):
            continue
        if sum(1 for v in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)) if v in q.sol) == 3:
            q.relief[(x, y)] = "light"
            q.obstacles.add((x, y))


def tracer(q: Quartier, points: dict[str, tuple[float, float]]) -> dict:
    poser_portes(q, points)
    tracer_voirie(q)
    percer_ruelles(q)
    depart = q.depart or q.portes[0]
    reboucher(q, (depart.x, depart.y))
    habiller(q)

    entree = depart.interieur(1)
    entites: list[dict] = []
    for porte in q.portes:
        if porte.gardee:
            # La sentinelle se tient au bout de la rue, sur la case du bord : on lui parle, on ne
            # passe pas. Pas de point d'arrivée : on n'arrive de nulle part par une porte fermée.
            entites.append({"type": "npc", "x": porte.x, "y": porte.y,
                            "dialogue": SENTINELLE_DIALOGUE, "figure": SENTINELLE_FIGURINE,
                            "guards": porte.vers})
            continue
        entites.append({"type": "spawnPoint", "x": porte.interieur(2)[0], "y": porte.interieur(2)[1],
                        "name": porte.nom})
        if porte.carte:
            # Le portail est la case du bord ; on arrive chez le voisin au point qui porte le nom
            # de ce quartier-ci, deux pas à l'intérieur : jamais sur son portail de retour.
            entites.append({"type": "portal", "x": porte.x, "y": porte.y, "targetMap": porte.carte,
                            "arrival": q.ident})
    if q.depart:
        entites.append({"type": "spawnPoint", "x": entree[0], "y": entree[1], "name": q.depart.nom})
    for nom, (x1, y1, x2, y2) in q.ilots:
        entites.append({"type": "cityBlock", "x": x1, "y": y1, "name": nom,
                        "width": x2 - x1 + 1, "height": y2 - y1 + 1})

    racine: list[dict] = []
    for y in range(q.hauteur):
        for x in range(q.largeur):
            case = (x, y)
            piece = q.relief.get(case)
            if case in q.sol and case not in q.obstacles:
                if case == entree:
                    type_ = "entry"
                elif piece is not None:
                    type_ = "dirt"
                else:
                    continue
            else:
                # Toute case où l'on ne marche pas est un mur de la grille racine, cœurs d'îlots
                # compris : une case absente y vaudrait « vide », c'est-à-dire franchissable.
                type_ = "wall"
            tuile = {"x": x, "y": y, "type": type_}
            if piece is not None:
                tuile["texture"] = piece
            racine.append(tuile)

    return {
        "version": 3,
        "name": q.nom,
        "width": q.largeur,
        "height": q.hauteur,
        "tiles": racine,
        "layers": [
            {
                "name": "sol",
                "kind": "ground",
                "scene": q.lieu,
                "note": "La matière de chaque case ; la table du lieu (Assets/Scene/%s/appearance.json) la traduit en pièce de la planche du LOT-92." % q.lieu,
                "tiles": [{"x": x, "y": y, "type": m}
                          for (x, y), m in sorted(q.sol.items(), key=lambda i: (i[0][1], i[0][0]))],
            },
            {
                "name": "relief",
                "kind": "decor",
                "note": "Une case de relief par pièce dressée ; l'assignation de texture de la grille racine nomme la pièce exacte.",
                "tiles": [{"x": x, "y": y, "type": "wall"}
                          for (x, y) in sorted(q.relief, key=lambda c: (c[1], c[0]))],
            },
        ],
        "entities": entites,
    }


# --------------------------------------------------------------------------------------------------
# Les quartiers
# --------------------------------------------------------------------------------------------------


def fiche(quartier: str) -> str:
    return "%s-%s" % (VILLE, quartier)


def quartiers() -> list[Quartier]:
    return [
        Quartier(
            ident="martpart",
            fiche=fiche("martpart"),
            nom="Martpart",
            lieu="martpart",
            largeur=48,
            hauteur=40,
            # La place du marché, au cœur du quartier ; l'arène Illu Die à l'est, fermée : on
            # n'y entre qu'au LOT-27.
            place=(16, 15, 27, 24),
            batiments=[(33, 11, 42, 21)],
            portes=[
                Porte(vers=fiche("arenarea"), nom="arenarea", carte="capital/arenarea"),
            ],
            depart=Porte(vers="", nom="porte-est", bord="E", x=47, y=30),
            # La rue des Lanternes : elle fait le tour de l'arène par le nord et rejoint la rue
            # de la porte de l'Est.
            rues=[((29, 8), (45, 8)), ((45, 8), (45, 29))],
            ilots=[
                ("place-du-marche", (12, 11, 31, 28)),
                ("avenue-d-arenarea", (0, 0, 17, 11)),
                ("arene-illu-die", (32, 5, 47, 24)),
                ("porte-de-l-est", (26, 25, 47, 33)),
                ("ruelles-du-sud", (0, 28, 25, 39)),
            ],
        ),
        Quartier(
            ident="arenarea",
            fiche=fiche("arenarea"),
            nom="Arenarea",
            # Faute de planche propre, Arenarea emprunte celle de Martpart : même ville, mêmes
            # pavés, mêmes maisons (journal du lot, phase 2).
            lieu="martpart",
            largeur=48,
            hauteur=40,
            # Le parvis, au pied du Colisée.
            place=(18, 17, 29, 24),
            # Les deux arènes donnent directement sur la rue qui ceint le parvis.
            batiments=[
                # Le Colisée, au nord, comme sur le plan ; on y entre au LOT-27.
                (13, 2, 28, 13),
                # L'Arène du Destin, à l'est du parvis (LOT-27).
                (33, 18, 42, 27),
            ],
            etals=False,
            ilots=[
                ("parvis", (14, 13, 33, 28)),
                ("colisee", (10, 0, 35, 12)),
                ("arene-du-destin", (33, 14, 47, 28)),
                ("porte-de-martpart", (29, 29, 47, 39)),
                ("portes-du-sud", (0, 27, 28, 39)),
            ],
            portes=[
                Porte(vers=fiche("martpart"), nom="martpart", carte="capital/martpart"),
            ],
        ),
    ]


def portes_gardees(q: Quartier) -> list[Porte]:
    """Les portes gardées que la ville jouable pose sur la carte de ce quartier (`capital.json`)."""
    ville = json.loads(VILLE_JOUABLE.read_text(encoding="utf-8"))
    return [Porte(vers=d["id"], nom=d["id"][len(VILLE) + 1:], gardee=True)
            for d in ville["districts"]
            if d.get("guard", {}).get("map") == "capital/%s" % q.ident]


def main() -> int:
    analyseur = argparse.ArgumentParser(
        description="Trace les cartes des quartiers (LOT-96) ; retiré au LOT-EDITOR-06, trace seulement.")
    analyseur.add_argument("--sortie", type=Path, required=True,
                           help="dossier où écrire les tracés ; jamais celui des cartes du jeu, "
                                "que l'éditeur tient")
    arguments = analyseur.parse_args()

    sortie = arguments.sortie.resolve()
    if sortie == NIVEAUX.resolve():
        print("carte_quartiers : retiré au LOT-EDITOR-06 — les cartes du jeu se modifient dans "
              "LevelEditor, le script n'écrit plus dans Source/Elements/Levels.")
        return 1

    points = points_du_plan()
    for q in quartiers():
        q.portes += portes_gardees(q)
        texte = en_v4(json.dumps(tracer(q, points), ensure_ascii=False, indent=2) + "\n")
        chemin = sortie / ("%s.json" % q.ident)
        chemin.parent.mkdir(parents=True, exist_ok=True)
        chemin.write_text(texte, encoding="utf-8", newline="\n")
        marche = sum(1 for c in q.sol if c not in q.obstacles)
        print("carte_quartiers : %s écrite — %d x %d cases, %d franchissables, portes %s."
              % (chemin, q.largeur, q.hauteur, marche,
                 ", ".join("%s %s(%d,%d)" % (p.nom, p.bord, p.x, p.y) for p in q.portes)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
