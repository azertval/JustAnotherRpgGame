#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Trace la carte du Colisée, version finale (`LOT-09`).

> **Retiré le 19 septembre 2026 (`LOT-EDITOR-06`).** L'éditeur fait foi (décision D4 de la
> feuille de route de l'éditeur) : `coliseum.json` se modifie dans `LevelEditor`, à la souris ou
> par `LevelEditor --apply`, et `LevelEditor --check` le garde en CI. Le script reste ici comme
> trace de la façon dont la carte a été posée. Sa dernière génération a été commitée telle quelle,
> puis retouchée dans l'éditeur ; il n'écrit donc plus jamais dans `Source/Elements` : il trace
> dans le dossier que nomme `--sortie`, et son `--check` est parti.

*Décision de l'auteur, 17 septembre 2026* : la carte se **pose par script**, puis se retouche dans
l'éditeur (`LOT-11`). Le fichier produit est un fichier de niveau ordinaire, sans marque d'origine :
l'éditeur l'ouvre, le modifie et le réenregistre sans rien savoir de ce script. Écrire mille cases à
la main n'était ni relisible ni rejouable ; les tracer entièrement à la souris n'était pas un usage
du temps de l'auteur.

## Ce que le script décide, et ce qu'il ne décide pas

Il décide la **géométrie** : où est le sable, où sont les couloirs, les vestiaires, les tribunes, la
loge et le hall, et quelle pièce de la planche du `LOT-92` va sur quelle case. Il ne décide ni les
règles, ni les dialogues, ni ce que la zone de combat fait — tout cela est ailleurs.

## Les trois grilles

- la grille **racine** est la collision, et elle seule (`EX-LVL-016`) : `wall` partout où l'on ne
  passe pas, rien là où l'on passe. Une case qui porte une **assignation de texture** doit avoir un
  type non vide, sans quoi l'écrivain de niveau ne l'émet pas : une case franchissable qui porte du
  relief reçoit donc ce que sa pièce oppose (son type tactique au manifeste : `wall` pour un pilier,
  `dirt` pour un banc qu'on enjambe), et sa matière visible reste dans la couche de sol ;
- la couche **sol** porte la fente de matière de chaque case franchissable (`sand`, `dirt`,
  `solid`, `bridge`, `stairs`, `grass`, `cliff`), que la table du lieu traduit en pièce ;
- la couche **décor** porte `wall` sur chaque case de relief, et l'assignation de texture de la
  case nomme la pièce exacte (pan gauche ou droit, angle, arche, gradin, banc, torche…).

Depuis le `LOT-EDITOR-12`, ce tracé v3 passe par `LevelEditor --migrate` avant d'être écrit : la
carte commitée est en v4 (pièces sur leurs couches, collision déduite, cases forcées, entités à
identifiant). Il faut donc avoir construit l'éditeur.

*Décision de l'auteur, 19 septembre 2026 (`LOT-EDITOR-03`)* : la collision tracée est celle que
l'éditeur déduit, sans case forcée. Le **vide** autour de l'amphithéâtre — ni sol ni pièce — est un
mur : jusque-là franchissable, le héros y sortait par la porte. Les deux **piliers** du couloir
ouest, posés sur des dalles, arrêtent le pas comme ceux du couloir est.

Usage :

    py -3.13 Planning/versions/v0.0.0/v0.0.0-fondation/annexes/LOT-09-colisee-premiere-carte/atelier/carte_colisee.py --sortie DOSSIER

Le tracé s'écrit dans `DOSSIER/coliseum.json` ; le dossier des cartes du jeu est refusé.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

RACINE = Path(__file__).resolve().parents[4]
CARTE = RACINE / "Source" / "Elements" / "Levels" / "coliseum.json"

# -- Format v4 (LOT-EDITOR-12) -------------------------------------------------------------------
#
# Ce script trace une carte v3 ; le dépôt ne garde que des v4. La conversion n'est pas refaite ici :
# c'est l'éditeur qui la porte (`LevelEditor --migrate`), une seule fois pour toutes les cartes. Le
# script en dépend donc : il faut avoir construit l'éditeur (`scripts/build.ps1`). Il a vécu
# jusqu'au LOT-EDITOR-06, qui a fait de l'éditeur la source des cartes faites à la main.
EDITEUR = RACINE / "build" / "ninja" / "bin" / "LevelEditor.exe"
MANIFESTE = RACINE / "Source" / "Elements" / "Assets" / "Scene" / "coliseum" / "manifest.json"

# Ce qu'un type tactique du manifeste écrit dans la grille de collision : la règle de
# `core::collisionTileOf`. Gêne, abri et passage s'écrivent `dirt`, franchissable.
COLLISION_DU_TACTIQUE = {"solid": "wall", "obstacle": "cliff"}


def tactiques() -> dict[str, str]:
    """Le type tactique de chaque pièce du Colisée, par nom court ; à défaut, un sol passe et une
    pièce debout arrête la vue (`core::ScenePiece::tactical`)."""
    textures = json.loads(MANIFESTE.read_text(encoding="utf-8"))["textures"]
    return {
        cle.rsplit("/", 1)[-1]: piece.get("tactical", "open" if piece["class"] == "floor" else "solid")
        for cle, piece in textures.items()
    }


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


LARGEUR = 40
HAUTEUR = 34

# --- Les zones, en cases (colonnes incluses, lignes incluses) -------------------------------------
# Le sable du LOT-50 (20 x 14) est le centre de la carte ; tout le reste est autour de lui.
SABLE = (10, 10, 29, 23)
# Les quatre portes de l'enceinte : deux sur le sable (nord, sud), deux vers les vestiaires.
PORTES_SABLE = [(19, 9), (20, 9), (19, 24), (20, 24), (9, 16), (9, 17), (30, 16), (30, 17)]
# Les tribunes : quatre rangs de gradins au nord et au sud, le fond en pièces de gradin.
TRIBUNE_NORD = (10, 5, 29, 8)
TRIBUNE_SUD = (10, 25, 29, 28)
FOND_TRIBUNE_NORD = 4
FOND_TRIBUNE_SUD = 29
# La loge impériale, au milieu du rang le plus haut du nord.
LOGE = (17, 5, 22, 7)
# Les couloirs sous les gradins, et les vestiaires qui s'ouvrent dessus.
COULOIR_OUEST = (7, 6, 8, 27)
COULOIR_EST = (31, 6, 32, 27)
VESTIAIRE_OUEST = (3, 14, 6, 19)
VESTIAIRE_EST = (33, 14, 36, 19)
# Les liens : couloir -> tribunes (nord et sud), et couloir -> vestiaire (deux cases de porte).
LIENS = [(9, 7), (9, 8), (30, 7), (30, 8), (9, 25), (9, 26), (30, 25), (30, 26)]
PORTES_VESTIAIRE = [(6, 16), (6, 17), (33, 16), (33, 17)]
# Le hall et la porte du Colisée, sous les tribunes du sud.
HALL = (14, 30, 25, 32)
SEUIL = [(19, 33), (20, 33)]
# Le grand escalier : du hall aux gradins du sud.
ESCALIER = [(18, 29), (19, 29), (20, 29), (21, 29)]

# L'entrée de la carte, que le format exige en un seul exemplaire : une dalle du hall, derrière la
# porte. Sa voisine (la sortie jusqu'au `LOT-88`) reste une dalle du hall.
ENTREE = (19, 32)
SORTIE = (20, 32)

# --- Les PNJ : cinq figurines de l'atelier (LOT-91), et le héraut ---------------------------------
PNJ = [
    # (colonne, ligne, dialogue, figurine)
    (20, 22, "heraut-colisee", "nakral"),
    (16, 31, "portier-colisee", "anariel"),
    (13, 7, "parieuse-tribunes", "jade"),
    (4, 16, "medecin-vestiaire", "lizz"),
    (26, 26, "vieux-gladiateur", "xorius"),
]

# Les points d'entrée des deux camps, dans le sable : quatre par camp, face à face.
ENTREES_ARENE = [(11, 15 + rang, "allies", rang + 1) for rang in range(4)] + [
    (28, 15 + rang, "enemies", rang + 1) for rang in range(4)
]


def dans(zone: tuple[int, int, int, int], colonne: int, ligne: int) -> bool:
    """Vrai si (colonne, ligne) est dans la zone (x1, y1, x2, y2), bornes incluses."""
    x1, y1, x2, y2 = zone
    return x1 <= colonne <= x2 and y1 <= ligne <= y2


def cases(zone: tuple[int, int, int, int]):
    x1, y1, x2, y2 = zone
    for ligne in range(y1, y2 + 1):
        for colonne in range(x1, x2 + 1):
            yield colonne, ligne


def matieres() -> dict[tuple[int, int], str]:
    """La fente de matière de chaque case franchissable — la couche de sol du Colisée."""
    sol: dict[tuple[int, int], str] = {}
    for case in cases(SABLE):
        sol[case] = "sand"
    for case in cases(TRIBUNE_NORD):
        sol[case] = "stairs"
    for case in cases(TRIBUNE_SUD):
        sol[case] = "stairs"
    for case in cases(LOGE):
        sol[case] = "grass"
    for case in cases(COULOIR_OUEST):
        sol[case] = "dirt"
    for case in cases(COULOIR_EST):
        sol[case] = "dirt"
    for case in cases(VESTIAIRE_OUEST):
        sol[case] = "bridge"
    for case in cases(VESTIAIRE_EST):
        sol[case] = "bridge"
    for case in cases(HALL):
        sol[case] = "solid"
    for case in PORTES_SABLE:
        sol[case] = "cliff"
    for case in PORTES_VESTIAIRE + LIENS + ESCALIER:
        sol[case] = "dirt"
    for case in SEUIL:
        sol[case] = "cliff"
    # Les deux dalles du hall, derrière la porte, en gardent la pierre.
    sol[ENTREE] = "solid"
    sol[SORTIE] = "solid"
    return sol


def pieces_de_relief(sol: dict[tuple[int, int], str]) -> dict[tuple[int, int], str]:
    """La pièce de la planche posée sur chaque case de relief.

    Deux familles :

    - les **murs** : toute case non franchissable qui touche une case franchissable porte un pan,
      orienté par le côté d'où on le voit (l'atelier ne dessine que des pièces du fond) ; les
      angles portent l'angle, et l'on sème torches et bannières le long des grands murs ;
    - les **pièces voulues** : arches des portes, gradins du fond, loge, escalier, bancs, râteliers,
      braseros, piliers et les deux battants de la porte du Colisée.
    """
    relief: dict[tuple[int, int], str] = {}

    def franchissable(colonne: int, ligne: int) -> bool:
        return (colonne, ligne) in sol

    for ligne in range(HAUTEUR):
        for colonne in range(LARGEUR):
            if franchissable(colonne, ligne):
                continue
            nord = franchissable(colonne, ligne - 1)
            sud = franchissable(colonne, ligne + 1)
            ouest = franchissable(colonne - 1, ligne)
            est = franchissable(colonne + 1, ligne)
            if not (nord or sud or ouest or est):
                continue
            # Le bord d'une ligne court comme l'arête droite d'une case, celui d'une colonne comme
            # l'arête gauche : une pièce se dresse contre l'arête du fond parallèle au bord.
            if (nord or sud) and (ouest or est):
                relief[(colonne, ligne)] = "wall-corner"
            elif nord or sud:
                relief[(colonne, ligne)] = "wall-right"
            else:
                relief[(colonne, ligne)] = "wall-left"

    # Torches et bannières, semées le long des murs : une torche tous les quatre pas sur les murs
    # gauches, une bannière tous les cinq pas sur les murs droits. Même règle que l'enceinte du
    # LOT-50, mais sur les murs que la carte a vraiment.
    for (colonne, ligne), piece in list(relief.items()):
        if piece == "wall-left" and ligne % 4 == 0:
            relief[(colonne, ligne)] = "torch-left"
        elif piece == "wall-right" and colonne % 5 == 0:
            relief[(colonne, ligne)] = "banner-right"

    # Le fond des tribunes : des gradins, pas des murs.
    for colonne in range(TRIBUNE_NORD[0], TRIBUNE_NORD[2] + 1):
        relief[(colonne, FOND_TRIBUNE_NORD)] = "stands-right"
    for colonne in range(TRIBUNE_SUD[0], TRIBUNE_SUD[2] + 1):
        if (colonne, FOND_TRIBUNE_SUD) not in sol:
            relief[(colonne, FOND_TRIBUNE_SUD)] = "stands-right"

    # Les arches des quatre portes de l'enceinte, et les battants de la porte du Colisée.
    for colonne, ligne in PORTES_SABLE:
        relief[(colonne, ligne)] = "arch-right" if ligne in (9, 24) else "arch-left"
    relief[SEUIL[0]] = "gate-left"
    relief[SEUIL[1]] = "gate-right"

    # La loge : ses deux joues de marbre, et deux bannières au fond.
    for ligne in range(LOGE[1], LOGE[3] + 1):
        relief[(LOGE[0] - 1, ligne)] = "box-left"
        relief[(LOGE[2] + 1, ligne)] = "box-right"
    relief[(LOGE[0], FOND_TRIBUNE_NORD)] = "banner-left"
    relief[(LOGE[2], FOND_TRIBUNE_NORD)] = "banner-right"

    # Le grand escalier, du hall aux gradins.
    for colonne, ligne in ESCALIER:
        relief[(colonne, ligne)] = "stairs-right"

    # Ce que les lieux portent : bancs sur les gradins, râteliers et braseros aux vestiaires,
    # piliers dans les couloirs.
    for colonne in range(TRIBUNE_NORD[0] + 1, TRIBUNE_NORD[2], 4):
        relief[(colonne, TRIBUNE_NORD[3])] = "bench"
    for colonne in range(TRIBUNE_SUD[0] + 1, TRIBUNE_SUD[2], 4):
        relief[(colonne, TRIBUNE_SUD[1])] = "bench"
    relief[(VESTIAIRE_OUEST[0], VESTIAIRE_OUEST[1])] = "weapon-rack"
    relief[(VESTIAIRE_EST[2], VESTIAIRE_EST[1])] = "weapon-rack"
    relief[(VESTIAIRE_OUEST[0], VESTIAIRE_OUEST[3])] = "brazier"
    relief[(VESTIAIRE_EST[2], VESTIAIRE_EST[3])] = "brazier"
    for ligne in (10, 23):
        relief[(COULOIR_OUEST[0], ligne)] = "pillar"
        relief[(COULOIR_EST[1], ligne)] = "pillar"
    return relief


def tracer() -> dict:
    sol = matieres()
    relief = pieces_de_relief(sol)

    # La grille racine : la collision, et rien d'autre. `wall` partout où l'on ne passe pas, vide
    # compris ; une case franchissable qui porte du relief reçoit ce que sa pièce oppose (`dirt`,
    # non solide, pour un banc), pour que l'assignation de texture soit émise, sa matière visible
    # restant dans la couche de sol.
    tactique = tactiques()
    racine: list[dict] = []
    for ligne in range(HAUTEUR):
        for colonne in range(LARGEUR):
            case = (colonne, ligne)
            franchissable = case in sol
            piece = relief.get(case)
            if franchissable:
                if case == ENTREE:
                    type_ = "entry"
                elif piece is not None:
                    type_ = COLLISION_DU_TACTIQUE.get(tactique.get(piece, "solid"), "dirt")
                else:
                    continue
            else:
                type_ = "wall"
            tuile = {"x": colonne, "y": ligne, "type": type_}
            if piece is not None:
                tuile["texture"] = piece
            racine.append(tuile)

    couche_sol = [
        {"x": colonne, "y": ligne, "type": matiere}
        for (colonne, ligne), matiere in sorted(sol.items(), key=lambda item: (item[0][1], item[0][0]))
    ]
    couche_decor = [
        {"x": colonne, "y": ligne, "type": "wall"}
        for (colonne, ligne) in sorted(relief.keys(), key=lambda case: (case[1], case[0]))
    ]

    entites: list[dict] = []
    for colonne, ligne, dialogue, figurine in PNJ:
        entites.append(
            {"type": "npc", "x": colonne, "y": ligne, "dialogue": dialogue, "figure": figurine}
        )
    # La zone de combat : le sable, et lui seul (EX-LVL-018). `core::ArenaSession` la prend pour
    # grille tactique a la place de la carte entiere, et les cases du dehors lui sont inconnues.
    entites.append(
        {
            "type": "combatZone",
            "x": SABLE[0],
            "y": SABLE[1],
            "name": "sable",
            "width": SABLE[2] - SABLE[0] + 1,
            "height": SABLE[3] - SABLE[1] + 1,
        }
    )
    for colonne, ligne, camp, rang in ENTREES_ARENE:
        entites.append({"type": "arenaEntry", "x": colonne, "y": ligne, "side": camp, "rank": rang})
    # Le point d'arrivée de la porte : c'est par là qu'on entre quand on vient d'ailleurs, et c'est
    # ce que le `LOT-96` visera depuis Arenarea.
    entites.append({"type": "spawnPoint", "x": ENTREE[0], "y": ENTREE[1], "name": "porte"})
    entites.append({"type": "spawnPoint", "x": 20, "y": 23, "name": "sable"})

    return {
        "version": 3,
        "name": "Le Colisée",
        "width": LARGEUR,
        "height": HAUTEUR,
        "tiles": racine,
        "layers": [
            {
                "name": "sol",
                "kind": "ground",
                "scene": "coliseum",
                "note": "La matière de chaque case ; la table du lieu (Assets/Scene/coliseum/appearance.json) la traduit en pièce de la planche du LOT-92.",
                "tiles": couche_sol,
            },
            {
                "name": "relief",
                "kind": "decor",
                "note": "Une case de relief par pièce dressée ; l'assignation de texture de la grille racine nomme la pièce exacte.",
                "tiles": couche_decor,
            },
        ],
        "entities": entites,
    }


def main() -> int:
    analyseur = argparse.ArgumentParser(
        description="Trace la carte du Colisée (LOT-09) ; retiré au LOT-EDITOR-06, trace seulement."
    )
    analyseur.add_argument(
        "--sortie",
        type=Path,
        required=True,
        help="dossier où écrire le tracé ; jamais celui des cartes du jeu, que l'éditeur tient",
    )
    arguments = analyseur.parse_args()

    sortie = arguments.sortie.resolve()
    if sortie == CARTE.parent.resolve():
        print(
            "carte_colisee : retiré au LOT-EDITOR-06 — la carte du jeu se modifie dans LevelEditor, "
            "le script n'écrit plus dans Source/Elements/Levels."
        )
        return 1

    carte = tracer()
    texte = en_v4(json.dumps(carte, ensure_ascii=False, indent=2) + "\n")

    trace = sortie / CARTE.name
    trace.parent.mkdir(parents=True, exist_ok=True)
    # En LF, comme tout le depot : un fichier de donnees commite en CRLF pollue chaque diff.
    trace.write_text(texte, encoding="utf-8", newline="\n")
    franchissables = len(matieres())
    print(
        f"carte_colisee : {trace} écrite — {LARGEUR} x {HAUTEUR} cases, "
        f"{franchissables} franchissables, {len(carte['tiles'])} tuiles racine, "
        f"{len(carte['entities'])} entités."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
