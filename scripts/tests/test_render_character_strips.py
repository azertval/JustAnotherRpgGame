# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Le rendu des personnages en bandes (LOT-1000) : la caméra du jeu, l'échantillonnage, l'assemblage.

Blender n'est pas en CI : ce qui s'éprouve ici est la géométrie qui fait tomber le rendu au pixel
du moteur (le losange de 0,62, 256 px pour une case, le pied sur la ligne de sol), et ce que le
script fait des images rendues. Le rendu lui-même se juge sur le poste.
"""
import json
import math

import pytest

pytest.importorskip('numpy')
pytest.importorskip('PIL.Image')

import numpy as np  # noqa: E402

import install_hd_asset  # noqa: E402
import render_character_strips as R  # noqa: E402


def ecran(vecteur):
    """Un vecteur du monde en pixels d'écran (x à droite, y vers le bas)."""
    axes = R.repere_camera()
    k = R.pixels_par_metre()
    x = sum(a * b for a, b in zip(vecteur, axes['droite'])) * k
    y = -sum(a * b for a, b in zip(vecteur, axes['haut'])) * k
    return x, y


def test_la_camera_est_orthonormee():
    axes = R.repere_camera()
    for a in axes.values():
        assert math.isclose(sum(c * c for c in a), 1.0)
    for a, b in (('vue', 'droite'), ('vue', 'haut'), ('droite', 'haut')):
        assert abs(sum(x * y for x, y in zip(axes[a], axes[b]))) < 1e-12


def test_une_case_se_projette_en_losange_du_moteur():
    # Une colonne de plus (+X) descend à droite, une rangée de plus (-Y) descend à gauche : le
    # demi-losange de `IsoProjection`, 128 × 79,4 px pour une case de 1,5 m.
    cx, cy = ecran((R.CASE_M, 0, 0))
    rx, ry = ecran((0, -R.CASE_M, 0))
    assert cx == pytest.approx(128.0) and cy == pytest.approx(128.0 * R.RAPPORT)
    assert rx == pytest.approx(-128.0) and ry == pytest.approx(128.0 * R.RAPPORT)


def test_un_corps_de_1_80_m_mesure_la_hauteur_du_standard():
    _, y = ecran((0, 0, 1.8))
    assert round(-y) == install_hd_asset.HAUTEUR_FIGURINE


@pytest.mark.parametrize('facing, attendu', [('se', (1, 1)), ('sw', (-1, 1)), ('ne', (1, -1)), ('nw', (-1, -1))])
def test_chaque_orientation_regarde_ou_le_moteur_la_fait_marcher(facing, attendu):
    # Le personnage est modelé face à -Y ; tourné, son avant doit aller vers `attendu` à l'écran
    # (les signes de `check_figure_walk.FACINGS`).
    a = R.lacet_orientation(facing)
    avant = (math.sin(a), -math.cos(a), 0.0)
    x, y = ecran(avant)
    assert (math.copysign(1, x), math.copysign(1, y)) == attendu


@pytest.mark.parametrize('cellule', [R.CELLULE, R.CELLULE_LARGE])
def test_le_pied_tombe_sur_la_ligne_de_sol_au_milieu_de_la_cellule(cellule):
    largeur, hauteur = R.toile(cellule)
    _, dy = R.cible_camera(cellule)
    # La caméra vise `dy` au-dessus du pied : le pied est à `dy` sous le centre de la toile.
    ligne = hauteur / 2 + dy * R.pixels_par_metre()
    assert ligne - R.DEBORD == pytest.approx(R.SOL)
    assert largeur / 2 - R.DEBORD == cellule[0] / 2


def test_une_boucle_s_arrete_avant_de_revenir_une_fin_sur_sa_derniere_pose():
    assert R.instants(0, 32, True) == [0, 4, 8, 12, 16, 20, 24, 28]
    assert R.instants(0, 56, False) == [0, 8, 16, 24, 32, 40, 48, 56]


def test_la_reduction_ne_noircit_pas_le_bord():
    image = np.zeros((4, 4, 4), np.uint8)
    image[:, :2] = (200, 100, 50, 255)
    reduite = R.reduire(image, 2)
    assert reduite.shape == (2, 2, 4)
    assert tuple(reduite[0, 0]) == (200, 100, 50, 255)
    bord = np.zeros((2, 2, 4), np.uint8)
    bord[0, 0] = (200, 100, 50, 255)
    (pixel,) = R.reduire(bord, 2).reshape(-1, 4)
    assert tuple(pixel[:3]) == (200, 100, 50) and pixel[3] == 64


def toile(cellule, x0, x1, y0, y1):
    largeur, hauteur = R.toile(cellule)
    image = np.zeros((hauteur, largeur, 4), np.uint8)
    image[y0 + R.DEBORD:y1 + R.DEBORD, x0 + R.DEBORD:x1 + R.DEBORD] = 255
    return image


def test_la_cellule_se_decoupe_et_donne_ce_qui_passe_sous_le_sol():
    image, bas = R.cellule_depuis_toile(toile(R.CELLULE, 60, 130, 80, 270), R.CELLULE, 'essai')
    assert bas == 270
    assert image[80, 60, 3] == 255 and image.shape[1] == 192
    bande = R.assembler([image, image], R.CELLULE, bas)
    assert bande.shape == (272, 384, 4)


def test_le_corps_massif_couche_reste_entier_sous_la_ligne_de_sol():
    # La mort du brawler remodelé dépasse la réserve initiale de 128 px.
    image, bas = R.cellule_depuis_toile(toile(R.CELLULE_LARGE, 60, 320, 180, 410),
                                      R.CELLULE_LARGE, 'death-ne')
    bande = R.assembler([image] * 8, R.CELLULE_LARGE, bas)
    assert bande.shape == (416, 3072, 4)
    assert bande[409, 60, 3] == 255
    assert not bande[410:, :, 3].any()


@pytest.mark.parametrize('boite, message', [
    ((60, 130, -3, 250), 'haut'),
    ((2, 130, 80, 250), 'transparents'),
    ((60, 190, 80, 250), 'transparents'),
])
def test_un_rendu_qui_deborde_est_refuse(boite, message):
    with pytest.raises(R.RenduError, match=message):
        R.cellule_depuis_toile(toile(R.CELLULE, *boite), R.CELLULE, 'essai')


def test_le_descripteur_s_installe_tel_quel(tmp_path):
    texte = json.dumps(R.descripteur('Heroes/brawler', 'portrait.png', [('walk', 'se'), ('death', 'nw')],
                                     'Common/Characters'))
    chemin = tmp_path / 'install.json'
    chemin.write_text(texte, encoding='utf-8')
    (figure,) = install_hd_asset.read_descriptor(chemin).figures
    marche, mort = figure.strips
    assert marche.placed and marche.frame_duration == 0.0625 and marche.loop and not marche.wide
    assert mort.placed and mort.wide and not mort.loop
    assert figure.portrait == 'portrait.png'
