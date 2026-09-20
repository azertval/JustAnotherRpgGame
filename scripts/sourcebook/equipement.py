#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Armes, armures et équipement d'aventurier (LOT-36 → LOT-34).

Les tables d'équipement des *Basic Rules* vers `Source/Elements/Rpg/` : armes (dégâts, poids, prix,
propriétés), armures (classe d'armure, exigence de Force, discrétion), matériel, outils, montures,
marchandises et services.

**C'est le lot où le §4 de la feuille de route se paie.** Un tableau ne s'extrait pas en flux de
texte (`EX-CNT-021`), et nulle part la conséquence n'est aussi silencieuse qu'ici : une valeur de
prix décalée d'une ligne ne casse rien, ne lève aucune alerte, et déséquilibre l'économie sans que
personne ne comprenne pourquoi. Toutes les tables de ce module sont donc lues **par coordonnée** —
les lignes par ordonnée, les cellules par abscisse — et jamais par un mode en flux.

Ces tables sont **pleine largeur**, contrairement au reste du livre : elles traversent la gouttière
que le `LOT-33` avait mesurée. Les découper en colonnes y couperait chaque rangée en deux au milieu
d'un prix. Leur région est donc **déclarée**, page et bande d'ordonnées, et le module contrôle qu'il
en tire le nombre de lignes attendu.

Trois unités converties une seule fois
---------------------------------------

Le livre écrit des prix en pièces (po, pa, pc) et des poids en kilogrammes ; les schémas du
`LOT-32` veulent des **pièces de cuivre** et des **grammes**. La conversion se fait ici, à
l'extraction, pour qu'une seule unité arrive jusqu'au moteur : deux unités dans un même catalogue
donneraient une masse d'armes moins chère qu'une dague, et l'écart passerait pour de l'équilibrage.

Le **groupe** d'une arme ou d'une armure n'est pas dans sa rangée
------------------------------------------------------------------

Le livre range ses armes sous quatre intertitres — « Armes courantes de corps à corps », « Armes de
guerre à distance »… — et ses armures sous quatre autres. Ce sont ces intertitres, et eux seuls, qui
disent qu'une arme est *courante* ou *de guerre*, à distance ou au corps à corps ; la rangée, elle,
ne porte rien de tel. Une extraction qui ne lirait que les rangées produirait trente-sept armes sans
catégorie, ce qu'aucun schéma ne refuserait — `category` est requis, mais rien ne dit qu'il est
juste.

Les intertitres sont reconnus à leur **rangée à une seule cellule** : une table à cinq colonnes n'en
produit pas d'autre.
"""
from __future__ import annotations

import json
import re
import unicodedata
from dataclasses import dataclass

from .bestiaire import portees
from .corpus import Corpus
from .extraction import Extracteur
from .glossaire import normaliser, normaliser_cle

SORTIE_RPG = 'Source/Elements/Rpg'

# -- Conversions ---------------------------------------------------------------------------------
# Le livre compte en pieces, les schemas en pieces de CUIVRE. Une seule unite interne evite les
# conversions dispersees ; l'affichage en po/pa/pc est une affaire de presentation (LOT-32).
CUIVRE_PAR_PIECE = {'pc': 1, 'pa': 10, 'pe': 50, 'po': 100, 'pp': 1000}

# Le livre compte en kilogrammes et en grammes ; les schemas en GRAMMES.
GRAMMES_PAR_UNITE = {'kg': 1000, 'g': 1}

# -- Tables declarees ----------------------------------------------------------------------------
# Page IMPRIMEE et bande d'ordonnees de chaque table. Les bandes sont larges d'une marge : le
# controle de cardinal, plus bas, dit si elles ont attrape trop ou trop peu.


@dataclass(frozen=True)
class Table:
    """Une table du livre : où elle est, ce qu'elle contient, combien de rangées on en attend."""

    page: int
    y0: float
    y1: float
    colonnes: int
    attendues: int


# Les bandes commencent AVANT le premier intertitre, jamais apres : une bande qui debute sur la
# premiere rangee de donnees laisse ses dix premieres armes sans categorie -- elles sont alors
# silencieusement ecartees, et le controle de cardinal est la seule chose qui le dise.
TABLE_ARMES = Table(page=48, y0=275.0, y1=800.0, colonnes=4, attendues=37)
# TREIZE rangees, et c'est le chiffre que la feuille de route annonce : trois armures legeres,
# cinq intermediaires, quatre lourdes et le BOUCLIER, qui est une rangee de cette table sans etre
# une armure. Compter quatorze -- douze armures plus un bouclier -- ferait echouer la generation
# sur une table pourtant complete.
TABLE_ARMURES = Table(page=50, y0=550.0, y1=780.0, colonnes=7, attendues=13)

# Les tables d'objets sans mecanique attachee -- materiel, outils, montures. Elles partagent le
# schema `item` et ne se distinguent que par leur categorie.
#
# `double` dit que la table est composee en DEUX SOUS-TABLES COTE A COTE : une rangee y porte six
# cellules, donc deux objets. C'est le cas du materiel d'aventurier, et le lire d'un bloc donnerait
# un objet pesant << 500 g Billes de fronde (20) >>.
#
# `attendues` est un MINIMUM et non un compte exact : ces tables melent des rangees de donnees et
# des intertitres, et exiger un compte exact ferait echouer la generation sur une mise en page.


@dataclass(frozen=True)
class TableObjets(Table):
    """Une table d'objets : sa categorie, et si elle est composee en deux sous-tables."""

    categorie: str = 'gear'
    double: bool = False


TABLES_OBJETS = (
    TableObjets(page=52, y0=40.0, y1=800.0, colonnes=6, attendues=60, categorie='gear',
                double=True),
    TableObjets(page=55, y0=180.0, y1=420.0, colonnes=3, attendues=10, categorie='tool'),
    TableObjets(page=56, y0=40.0, y1=250.0, colonnes=3, attendues=10, categorie='mount'),
)

# Les intertitres du livre, et ce qu'ils disent de la rangee qui suit. Ils sont DECLARES parce que
# leur libelle est la seule chose qui distingue une arme courante d'une arme de guerre, et qu'un
# rapprochement approximatif ferait passer << Armes de guerre a distance >> pour du corps a corps.
GROUPES_ARMES = {
    'armes courantes de corps a corps': ('simple', False),
    'armes courantes a distance': ('simple', True),
    'armes de guerre de corps a corps': ('martial', False),
    'armes de guerre a distance': ('martial', True),
}
GROUPES_ARMURES = {
    'armures legeres': 'light',
    'armures intermediaires': 'medium',
    'armures lourdes': 'heavy',
    'bouclier': 'shield',
}

# Les types de degats du livre, en francais, tels que la colonne << Degat >> les ecrit. Ils sont
# rapproches du lexique par `types_de_degats()` ; cette table ne sert qu'a isoler le mot.
MOTIF_DEGATS = re.compile(r'^\s*(\d+d\d+|\d+)\s+([a-zA-ZÀ-ÿ]+)')


class EquipementError(Exception):
    """Une table d'équipement n'a pas pu être lue telle qu'elle est déclarée."""


# -- Outils --------------------------------------------------------------------------------------

def identifiant(nom: str) -> str:
    """Un nom en identifiant kebab-case ASCII."""
    sans_accent = unicodedata.normalize('NFD', normaliser(nom))
    sans_accent = ''.join(c for c in sans_accent if unicodedata.category(c) != 'Mn')
    return re.sub(r'-+', '-', re.sub(r'[^a-z0-9]+', '-', sans_accent.lower())).strip('-')


def prix_en_cuivre(cellule: str) -> int | None:
    """« 5 po » → 500, « 2 pa » → 20, « 5 pc » → 5. Un tiret ne vaut pas zéro, il vaut *absent*.

    La distinction compte : une fronde à « - » n'est pas gratuite, c'est une case que le livre n'a
    pas remplie. Rendre 0 en ferait un objet qu'un marchand donnerait.
    """
    texte = normaliser(cellule).replace(' ', ' ')
    trouve = re.match(r'^([\d ,.]+)\s*(pc|pa|pe|po|pp)\b', texte)
    if not trouve:
        return None
    valeur = float(trouve.group(1).replace(' ', '').replace(',', '.'))
    return int(round(valeur * CUIVRE_PAR_PIECE[trouve.group(2)]))


def poids_en_grammes(cellule: str) -> int | None:
    """« 6,5 kg » → 6500, « 500 g » → 500. Un tiret vaut *absent*, pas zéro."""
    texte = normaliser(cellule).replace(' ', ' ')
    trouve = re.match(r'^([\d ,.]+)\s*(kg|g)\b', texte)
    if not trouve:
        return None
    valeur = float(trouve.group(1).replace(' ', '').replace(',', '.'))
    return int(round(valeur * GRAMMES_PAR_UNITE[trouve.group(2)]))


def rangees(extracteur: Extracteur, table: Table) -> list[list[str]]:
    """Les rangées d'une table déclarée, groupées par **ordonnée** (`EX-CNT-021`).

    Une rangée à **une seule cellule** est un intertitre, rendu tel quel : c'est lui qui porte la
    catégorie, que la rangée de données ne connaît pas.
    """
    index = extracteur.document.index_pdf(table.page)
    par_ordonnee: dict = {}
    for ligne in extracteur.lignes(index, region=(0.0, table.y0, 596.0, table.y1)):
        par_ordonnee.setdefault(round(ligne.y, 1), []).append(ligne)
    resultat = []
    for _, cellules in sorted(par_ordonnee.items()):
        ordonnees = sorted(cellules, key=lambda l: l.fragments[0].x0)
        resultat.append([normaliser(l.texte) for l in ordonnees])
    return resultat


def types_de_degats(lexique: list) -> dict:
    """{mot français du livre → type du schéma}, depuis le lexique (`LOT-30`)."""
    index = {}
    for entree in lexique:
        if normaliser_cle(entree.categorie) != normaliser_cle('type de dégâts'):
            continue
        for variante in entree.francais.split('/'):
            index[normaliser_cle(variante).strip()] = normaliser_cle(entree.anglais)
    return index


# -- Armes ---------------------------------------------------------------------------------------

def armes(corpus: Corpus, lexique: list, cache=None) -> tuple[list[dict], list[str]]:
    """Les 37 armes de la table des *Basic Rules* p. 48. Renvoie (armes, signalements)."""
    degats = types_de_degats(lexique)
    resultat: list[dict] = []
    signalements: list[str] = []
    with Extracteur(corpus['basic-rules'], cache=cache) as extracteur:
        lignes = rangees(extracteur, TABLE_ARMES)

    categorie, distance = None, False
    for cellules in lignes:
        if len(cellules) == 1:
            cle = normaliser_cle(cellules[0])
            if cle not in GROUPES_ARMES:
                continue
            categorie, distance = GROUPES_ARMES[cle]
            continue
        if categorie is None or len(cellules) < 4:
            continue
        arme = _lire_arme(cellules, categorie, distance, degats, signalements)
        if arme is not None:
            resultat.append(arme)

    if len(resultat) != TABLE_ARMES.attendues:
        raise EquipementError(
            'armes : %d extraites page %d des Basic Rules, %d attendues. Une table amputee ne se '
            'voit qu\'au moment ou un joueur cherche l\'arme qui manque.'
            % (len(resultat), TABLE_ARMES.page, TABLE_ARMES.attendues))
    return resultat, signalements


def _lire_arme(cellules: list[str], categorie: str, distance: bool, degats: dict,
               signalements: list[str]) -> dict | None:
    """Une rangee de la table des armes. Le FILET n'a pas de degats : sa colonne est absente."""
    nom = cellules[0]
    # Le filet ne fait aucun degat, et sa rangee a donc une cellule de moins. Aligner sur la
    # premiere cellule qui ressemble a des degats plutot que sur un indice fixe : un decalage
    # d'indice mettrait le poids dans la colonne des degats, ce que rien ne refuserait.
    depart = 1
    des = MOTIF_DEGATS.match(cellules[1]) if len(cellules) > 1 else None
    if des is None:
        depart = 0
        signalements.append(
            "%s : aucune colonne de degats -- l'arme n'en inflige pas. Sa rangee a une cellule de "
            'moins que les autres, et les colonnes suivantes sont decalees d\'autant.' % nom)

    arme = {
        'id': identifiant(nom),
        'name': nom,
        'source': 'srd',
        'category': categorie,
    }
    if distance:
        arme['ranged'] = True
    if des is not None:
        arme['damage'] = des.group(1)
        mot = normaliser_cle(des.group(2))
        if mot not in degats:
            raise EquipementError(
                '%s : type de degats << %s >> absent du lexique.' % (nom, des.group(2)))
        arme['damageType'] = degats[mot]
    poids = poids_en_grammes(cellules[depart + 1]) if len(cellules) > depart + 1 else None
    prix = prix_en_cuivre(cellules[depart + 2]) if len(cellules) > depart + 2 else None
    if poids is None or prix is None:
        # `weightGrams` et `price` sont REQUIS par le schema pour une arme : une arme sans prix ne
        # se vend ni ne s'achete, et une arme sans poids ne pese rien dans un inventaire.
        signalements.append(
            '%s : poids ou prix illisible (%s | %s). La fronde du livre porte un tiret dans la '
            'colonne du poids ; le tiret vaut ABSENT, jamais zero.'
            % (nom, cellules[depart + 1] if len(cellules) > depart + 1 else '?',
               cellules[depart + 2] if len(cellules) > depart + 2 else '?'))
    arme['weightGrams'] = poids if poids is not None else 0
    arme['price'] = prix if prix is not None else 0
    if len(cellules) > depart + 3:
        proprietes = normaliser(cellules[depart + 3])
        if proprietes and proprietes != '-':
            arme['text'] = proprietes
            arme.update(proprietes_d_arme(nom, proprietes))
    return arme


# Les dix propriétés d'arme du livre, et leur nom dans le schéma (`weapon.schema.json`).
PROPRIETES_ARME = {
    'munitions': 'ammunition',
    'finesse': 'finesse',
    'lourde': 'heavy',
    'legere': 'light',
    'chargement': 'loading',
    'allonge': 'reach',
    'special': 'special',
    'lancer': 'thrown',
    'a deux mains': 'two-handed',
    'polyvalente': 'versatile',
}

# « Polyvalente (1d8) » : les dés à deux mains.
MOTIF_DEGATS_POLYVALENTS = re.compile(r'\((\d+d\d+)\)')


def proprietes_d_arme(nom: str, texte: str) -> dict:
    """La colonne des propriétés, structurée : ``properties``, ``versatileDamage``, ``rangeNormal``
    et ``rangeLong`` (`LOT-22`).

    « Munitions (portée 45 m/180 m), lourde, à deux mains » : les virgules séparent les propriétés,
    sauf entre parenthèses. Une propriété inconnue **arrête** l'extraction — la deviner ferait d'une
    coquille de la table une règle du jeu.
    """
    resultat: dict = {}
    proprietes: list[str] = []
    for morceau in re.split(r',\s*(?![^()]*\))', texte):
        cle = normaliser_cle(re.sub(r'\([^)]*\)', '', morceau)).strip()
        if not cle:
            continue
        if cle not in PROPRIETES_ARME:
            raise EquipementError('%s : propriete << %s >> inconnue.' % (nom, morceau))
        proprietes.append(PROPRIETES_ARME[cle])
        if cle == 'polyvalente' and (des := MOTIF_DEGATS_POLYVALENTS.search(morceau)):
            resultat['versatileDamage'] = des.group(1)
    resultat['properties'] = proprietes
    resultat.update(portees(texte))
    return resultat


# -- Armures -------------------------------------------------------------------------------------

def armures(corpus: Corpus, cache=None) -> list[dict]:
    """Les 13 armures et le bouclier de la table des *Basic Rules* p. 50."""
    resultat: list[dict] = []
    with Extracteur(corpus['basic-rules'], cache=cache) as extracteur:
        lignes = rangees(extracteur, TABLE_ARMURES)

    categorie = None
    for cellules in lignes:
        if len(cellules) == 1:
            cle = normaliser_cle(cellules[0])
            categorie = GROUPES_ARMURES.get(cle, categorie)
            continue
        if categorie is None or len(cellules) < 7:
            continue
        resultat.append(_lire_armure(cellules, categorie))

    if len(resultat) != TABLE_ARMURES.attendues:
        raise EquipementError(
            'armures : %d extraites page %d des Basic Rules, %d attendues.'
            % (len(resultat), TABLE_ARMURES.page, TABLE_ARMURES.attendues))
    return resultat


def _lire_armure(cellules: list[str], categorie: str) -> dict:
    """Une rangee de la table des armures : nom, VO, CA, Force, Discretion, poids, prix.

    La colonne CA porte une FORMULE, pas un nombre : << 11 + Mod.Dex >>, << 14 + Mod.Dex (max +2)
    >>, << 16 >>, << +2 >> pour le bouclier. Les trois formes disent trois regles differentes, et
    les reduire a leur premier nombre ferait porter au harnois un bonus de Dexterite qu'il n'a pas
    -- ce qui rend le personnage plus resistant, jamais moins, et ne se remarque pas.
    """
    nom, _vo, formule, force, discretion, poids, prix = cellules[:7]
    armure = {
        'id': identifiant(nom),
        'name': nom,
        'source': 'srd',
        'category': categorie,
    }
    base = re.match(r'^\+?(\d+)', normaliser(formule))
    if not base:
        raise EquipementError('%s : classe d\'armure illisible (%r).' % (nom, formule))
    armure['baseArmorClass'] = int(base.group(1))
    if re.search(r'Mod\.?\s*Dex', formule, re.I):
        armure['dexterityBonus'] = True
        plafond = re.search(r'max\s*\+?(\d+)', formule, re.I)
        if plafond:
            armure['dexterityBonusMax'] = int(plafond.group(1))
    exigence = re.search(r'For\s*(\d+)', normaliser(force), re.I)
    if exigence:
        armure['strengthRequired'] = int(exigence.group(1))
    if normaliser_cle(discretion).startswith('desavantage'):
        armure['stealthDisadvantage'] = True
    poids_g = poids_en_grammes(poids)
    prix_c = prix_en_cuivre(prix)
    if poids_g is None or prix_c is None:
        raise EquipementError('%s : poids ou prix illisible (%r | %r).' % (nom, poids, prix))
    armure['weightGrams'] = poids_g
    armure['price'] = prix_c
    return armure


# -- Production ----------------------------------------------------------------------------------

def produire(corpus: Corpus, lexique: list, racine, cache=None) -> tuple[list, list[str]]:
    """Écrit les armes et les armures sous ``racine``. Renvoie (fichiers écrits, signalements)."""
    catalogue_armes, signalements = armes(corpus, lexique, cache)
    catalogue_armures = armures(corpus, cache)
    catalogue_objets, dits = objets(corpus, cache)
    signalements.extend(dits)

    ecrits = []
    for dossier, entrees in (('weapons', catalogue_armes), ('armors', catalogue_armures),
                             ('items', catalogue_objets)):
        identifiants = [e['id'] for e in entrees]
        doublons = sorted({i for i in identifiants if identifiants.count(i) > 1})
        if doublons:
            raise EquipementError(
                '%s : identifiant(s) en double — %s. Deux entrees ecriraient le meme fichier, et '
                'la seconde effacerait la premiere sans un mot.' % (dossier, ', '.join(doublons)))
        chemin_dossier = racine / dossier
        chemin_dossier.mkdir(parents=True, exist_ok=True)
        for entree in entrees:
            chemin = chemin_dossier / ('%s.json' % entree['id'])
            chemin.write_text(json.dumps(entree, ensure_ascii=False, indent=2) + '\n',
                              encoding='utf-8')
            ecrits.append(chemin)
    return ecrits, signalements


# -- Objets : materiel, outils, montures ---------------------------------------------------------

def objets(corpus: Corpus, cache=None) -> tuple[list[dict], list[str]]:
    """Le matériel d'aventurier, les outils et les montures. Renvoie (objets, signalements).

    Ces trois familles n'ont **aucune mécanique attachée** — un nom, un prix, un poids — et
    partagent donc le schéma `item`. Ce qui les distingue est leur `category`, qui vient de la
    table dont elles sortent.
    """
    resultat: list[dict] = []
    signalements: list[str] = []
    vus: set = set()
    with Extracteur(corpus['basic-rules'], cache=cache) as extracteur:
        for table in TABLES_OBJETS:
            lues = 0
            for cellules in rangees(extracteur, table):
                for triplet in _triplets(cellules, table.double):
                    objet = _lire_objet(triplet, table.categorie, signalements)
                    if objet is None or objet['id'] in vus:
                        continue
                    vus.add(objet['id'])
                    resultat.append(objet)
                    lues += 1
            if lues < table.attendues:
                raise EquipementError(
                    '%s : %d objet(s) lus page %d des Basic Rules, au moins %d attendus. Une '
                    'table amputee ne se voit qu\'au moment ou un joueur cherche l\'objet qui '
                    'manque.' % (table.categorie, lues, table.page, table.attendues))
    return resultat, signalements


def _triplets(cellules: list[str], double: bool) -> list[list[str]]:
    """Découpe une rangée en entrées de trois cellules — nom, prix, poids.

    La table du **matériel d'aventurier** est composée en **deux sous-tables côte à côte** : une
    rangée y porte six cellules, donc deux objets. Les lire d'un bloc donnerait un objet nommé
    « Acide (fiole) » coûtant « 25 po » et pesant « 500 g Billes de fronde (20) » — une valeur qui
    n'est pas un poids, et qu'aucun schéma ne refuserait puisque le champ serait simplement absent.
    """
    if not double:
        return [cellules[:3]] if len(cellules) >= 3 else []
    morceaux = []
    for depart in range(0, len(cellules) - 2, 3):
        morceaux.append(cellules[depart:depart + 3])
    return morceaux


def _lire_objet(cellules: list[str], categorie: str, signalements: list[str]) -> dict | None:
    """Une entrée de trois cellules : nom, prix, poids."""
    nom, prix, poids = (normaliser(c) for c in cellules)
    if not nom or nom in ('Objet', 'Prix', 'Poids', 'Coût', 'Cout') or prix in ('Prix', 'Coût'):
        return None  # En-tete de table.
    valeur = prix_en_cuivre(prix)
    if valeur is None:
        # Une ligne sans prix lisible n'est pas un objet : c'est un intertitre (<< Barde x4 x2 >>)
        # ou une ligne de continuation. L'ecrire produirait un objet gratuit au nom de section.
        return None
    objet = {
        'id': identifiant(nom),
        'name': nom,
        'source': 'srd',
        'category': categorie,
        'price': valeur,
    }
    grammes = poids_en_grammes(poids)
    if grammes is not None:
        objet['weightGrams'] = grammes
    elif normaliser(poids) not in ('-', '', '—'):
        signalements.append(
            '%s : poids illisible (%r) -- ni un nombre, ni le tiret que le livre emploie pour un '
            'poids negligeable.' % (nom, poids))
    return objet
