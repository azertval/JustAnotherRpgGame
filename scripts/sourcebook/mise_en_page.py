#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""La mise en page à deux colonnes des documents aidedd, mesurée une seule fois.

`Animaux.pdf`, les *Basic Rules* et le *Manuel des Joueurs* sortent du même atelier — aidedd.org —
et partagent la même grille : deux colonnes séparées par un blanc central de **[287, 309]** points,
sur toutes leurs pages. Ce module porte cette mesure et les deux opérations qui en dépendent.

**Pourquoi ici, et pas dans chaque module d'extraction.** Le `LOT-33` a mesuré cette gouttière pour
le bestiaire ; le `LOT-36` en a besoin pour les races et les historiques. Deux mesures écrites
séparément coïncident le premier jour et divergent le jour où l'une est corrigée — et la divergence
ne se voit pas : elle produit des blocs où une moitié de paragraphe appartient à la colonne
voisine, ce qu'aucun schéma ne refuse.

Trois faits mesurés vivent donc ici :

- la **gouttière**, avec le contrôle qui la revérifie à chaque exécution ;
- l'**interligne** qui sépare deux paragraphes — 11,2 pt à l'intérieur d'un paragraphe, 15,2 pt et
  plus entre deux, sans aucune valeur intermédiaire sur les 1 258 intervalles mesurés ;
- le regroupement des lignes en paragraphes, qui dépend des deux.
"""
from __future__ import annotations

# Coupe des deux colonnes, en points PDF. Mesurée par projection horizontale des mots : le blanc
# central occupe [287, 309] sur toutes les pages contrôlées. La coupe est au milieu de la bande.
GOUTTIERE = 298.0
BANDE_GOUTTIERE = (287.0, 309.0)
LARGEUR_PAGE = 596.0
HAUTEUR_UTILE = 900.0

# Interligne : 11,2 pt dans un paragraphe, 15,2 pt et plus entre deux. Le seuil est au milieu d'un
# vide, non au bord d'une distribution : aucune valeur mesurée ne tombe entre 11,3 et 15,2.
INTERLIGNE_PARAGRAPHE = 13.0

# Largeur minimale d'un blanc pour qu'il vaille gouttiere. En dessous, c'est l'espace
# entre deux mots d'une ligne pleine largeur, et couper la page dessus la casserait en
# deux au milieu d'une phrase.
LARGEUR_MINIMALE_GOUTTIERE = 8


class MiseEnPageError(Exception):
    """La page ne présente pas la grille que l'extraction suppose."""


def colonnes() -> tuple:
    """Les deux régions de colonne, de part et d'autre de la gouttière."""
    return ((0.0, 0.0, GOUTTIERE, HAUTEUR_UTILE),
            (GOUTTIERE, 0.0, LARGEUR_PAGE, HAUTEUR_UTILE))


def verifier_gouttiere(extracteur, pages) -> None:
    """Contrôle que le blanc central est bien là où la coupe le suppose, sur chaque page.

    Sans ce contrôle, une édition dont la mise en page a bougé produirait des blocs dont la moitié
    du contenu appartient au voisin — une donnée fausse, complète et muette. La projection est
    celle qui a servi à mesurer la bande : les abscisses que ne couvre aucun mot.
    """
    for index in pages:
        couvert = [False] * int(LARGEUR_PAGE)
        for mot in extracteur.mots(index):
            for abscisse in range(int(mot[0]), min(int(LARGEUR_PAGE), int(mot[2]) + 1)):
                couvert[abscisse] = True
        blanc = all(not couvert[x] for x in range(int(BANDE_GOUTTIERE[0]),
                                                  int(BANDE_GOUTTIERE[1])))
        if not blanc:
            raise MiseEnPageError(
                '%s page %d : la gouttière %.0f–%.0f n\'est plus blanche. La coupe en deux '
                'colonnes suppose ce blanc ; sans lui, les deux colonnes s\'entrelacent et une '
                'moitié de bloc se retrouve attribuée au voisin, sans autre signe.'
                % (extracteur.document.cle, index, *BANDE_GOUTTIERE))


def paragraphes(lignes: list) -> list[list]:
    """Regroupe des lignes ``(segment, Ligne)`` en paragraphes, par interligne et par segment.

    Un changement de colonne ouvre un paragraphe : c'est la seule lecture possible d'ordonnées qui
    repartent de zéro. Le ``segment`` identifie la colonne — page et numéro de colonne — et
    l'appelant le choisit ; comparer les ordonnées de deux colonnes donnerait un écart de plusieurs
    centaines de points à chaque changement.
    """
    groupes: list[list] = []
    precedent = None
    for segment, ligne in lignes:
        neuf = (precedent is None or precedent[0] != segment
                or ligne.y - precedent[1] >= INTERLIGNE_PARAGRAPHE)
        if neuf:
            groupes.append([])
        groupes[-1].append(ligne)
        precedent = (segment, ligne.y)
    return groupes


def bande_blanche(extracteur, index: int, moitie: str | None = None) -> tuple | None:
    """La bande verticale sans un seul mot qui contient le milieu de la page, ou ``None``.

    Mesurée, pas supposée. Les documents aidedd ont une gouttière fixe — `[287, 309]` — mais le
    *Manuel des Joueurs* est un **scan**, et sa gouttière bouge d'une page à l'autre : `[288, 309]`
    page 41, `[272, 296]` page 42, rien du tout page 44. Un blanc figé y couperait tantôt dans la
    colonne de gauche, tantôt dans celle de droite, et le texte des deux se mêlerait au milieu
    d'une phrase — sans qu'aucun contrôle en aval ne s'en aperçoive.

    Renvoie ``None`` quand le milieu tombe dans du texte : la page est alors sur une seule colonne,
    ou sur une grille que ce module ne sait pas lire, et l'appelant doit la traiter comme telle
    plutôt que la couper au hasard.
    """
    rect = extracteur.rectangle(index, moitie)
    largeur = int(rect.x1 - rect.x0) + 2
    couvert = [False] * largeur
    for mot in extracteur.mots(index, moitie):
        debut = max(0, int(mot[0] - rect.x0))
        for abscisse in range(debut, min(largeur, int(mot[2] - rect.x0) + 1)):
            couvert[abscisse] = True
    milieu = largeur // 2
    if couvert[milieu]:
        return None
    gauche = milieu
    while gauche > 0 and not couvert[gauche - 1]:
        gauche -= 1
    droite = milieu
    while droite < largeur - 1 and not couvert[droite + 1]:
        droite += 1
    if droite - gauche < LARGEUR_MINIMALE_GOUTTIERE:
        return None
    return (rect.x0 + gauche, rect.x0 + droite)


def colonnes_de_page(extracteur, index: int, moitie: str | None = None) -> tuple:
    """Les régions de colonne d'une page : deux si une gouttière la traverse, une sinon."""
    rect = extracteur.rectangle(index, moitie)
    bande = bande_blanche(extracteur, index, moitie)
    if bande is None:
        return ((rect.x0, rect.y0, rect.x1, rect.y1),)
    coupe = (bande[0] + bande[1]) / 2
    return ((rect.x0, rect.y0, coupe, rect.y1),
            (coupe, rect.y0, rect.x1, rect.y1))
