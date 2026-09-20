#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Le lexique bilingue : première sortie de la chaîne d'extraction.

Ce module produit `Source/Elements/Localization/rpg.glossary.csv` depuis `Glossaire.pdf`, complété
par le vocabulaire des *Basic Rules*. Il est ici, dans le paquet d'extraction, parce qu'il **éprouve
la chaîne** autant qu'il en dépend : le glossaire est le gisement le plus simple du corpus — texte
natif, deux colonnes, une entrée par ligne — et si l'outillage ne sait pas le lire proprement, il ne
saura rien lire.

**Ce que le lexique n'est pas.** Ce n'est pas une traduction d'interface : `fr.lang` et `en.lang`
font déjà cela. C'est une **table d'autorité** — une seule traduction par terme de règle, dans tout
le jeu. Sans elle, *saving throw* devient « jet de sauvegarde » dans la fiche, « JdS » dans le
journal de combat et « sauvegarde » dans l'infobulle, et le joueur croit à trois mécaniques
différentes. Elle sert en outre à traduire les données anglaises du Sourcebook et du *Player's
Guide* de façon cohérente avec celles venues du français.

**Le format d'une entrée du PDF** est ``anglais = français ; catégorie``, la catégorie étant
facultative et le français pouvant porter plusieurs variantes séparées par ``/`` :

    saving throw = jet de sauvegarde
    charmed = charmé ; état
    incapacitated = incapable d'agir / neutralisé ; état

Une entrée peut **courir sur plusieurs lignes** : le PDF coupe à la largeur de colonne, et une
ligne sans ``=`` est la suite de la précédente. Sur les 2 231 lignes utiles du document, 153 sont
des continuations — les ignorer amputerait autant de catégories, silencieusement.

**Le complément des *Basic Rules*** couvre ce que le glossaire ne catégorise pas. Trois manques,
tous sur des ensembles **fermés** que le moteur énumérera :

- les **huit écoles de magie** n'y figurent que sous leur forme « voie de magicien »
  (``School of Abjuration = École d'abjuration``), jamais comme écoles nues ;
- ``exhaustion = épuisement`` y est, mais **sans catégorie** — ce serait la quinzième condition ;
- la propriété d'arme ``special`` manque entièrement.

Chacun de ces compléments **déclare la page où il est attesté**, et le module vérifie que le terme
français s'y trouve réellement avant de l'écrire. Un complément tapé de mémoire est une donnée
inventée qui a l'air d'une donnée extraite ; c'est précisément ce que ce projet refuse.
"""
from __future__ import annotations

import csv
import io
import re
import unicodedata
from dataclasses import dataclass

from .corpus import Corpus
from .extraction import Extracteur

SORTIE = 'Source/Elements/Localization/rpg.glossary.csv'

# Les lignes d'habillage du PDF : en-tête de page, pied, mention de version.
HABILLAGE_RE = re.compile(r'^(www\.aidedd\.org|Glossaire D&D 5|Page \d+|Version du .*)$')

# Le séparateur d'une entrée. L'espace de part et d'autre est significatif : « = » sans espaces
# apparaît à l'intérieur de certaines traductions, et couper dessus les casserait.
SEPARATEUR = ' = '


@dataclass(frozen=True)
class Entree:
    anglais: str
    francais: str
    categorie: str

    @property
    def cle(self) -> str:
        """La clé de tri et d'unicité : l'anglais, insensible à la casse et aux accents."""
        return normaliser_cle(self.anglais)


@dataclass(frozen=True)
class Complement:
    """Un terme ajouté ou catégorisé depuis les *Basic Rules*, avec sa page d'attestation."""

    anglais: str
    francais: str
    categorie: str
    document: str
    page_imprimee: int


# Les trois manques du glossaire, chacun attesté sur une page des Basic Rules françaises.
#
# Les écoles : « … à travers une des huit écoles : abjuration, invocation, divination,
# enchantement, évocation, illusion, nécromancie ou transmutation » (p. 32). Noter que l'anglais
# `conjuration` se traduit par « invocation » et non par « conjuration » — c'est exactement le
# genre de faux ami qu'une table d'autorité existe pour figer.
COMPLEMENTS = (
    Complement('abjuration', 'abjuration', 'école de magie', 'basic-rules', 32),
    Complement('conjuration', 'invocation', 'école de magie', 'basic-rules', 32),
    Complement('divination', 'divination', 'école de magie', 'basic-rules', 32),
    Complement('enchantment', 'enchantement', 'école de magie', 'basic-rules', 32),
    Complement('evocation', 'évocation', 'école de magie', 'basic-rules', 32),
    Complement('illusion', 'illusion', 'école de magie', 'basic-rules', 32),
    Complement('necromancy', 'nécromancie', 'école de magie', 'basic-rules', 32),
    Complement('transmutation', 'transmutation', 'école de magie', 'basic-rules', 32),
    Complement('special', 'spéciale', 'propriété', 'basic-rules', 49),
    Complement('exhaustion', 'épuisement', 'état', 'basic-rules', 134),
)

ENTETE = ('anglais', 'français', 'catégorie')


class GlossaireError(Exception):
    """Le lexique n'a pas pu être produit tel qu'il est déclaré."""


def normaliser(texte: str) -> str:
    """Apostrophes typographiques, espaces insécables, espaces multiples — une seule forme.

    Le PDF mélange ``'`` et ``’`` dans les mêmes catégories, et sème des espaces insécables. Sans
    normalisation, ``jet d'attaque`` et ``jet d’attaque`` sont deux termes différents pour toute
    comparaison, et la table d'autorité cesse d'en être une.
    """
    texte = texte.replace('’', "'").replace('ʼ', "'")
    texte = texte.replace(' ', ' ').replace(' ', ' ')
    return re.sub(r'\s+', ' ', texte).strip()


def normaliser_cle(texte: str) -> str:
    """Forme de comparaison : normalisée, sans accent, en minuscules."""
    sans_accent = unicodedata.normalize('NFD', normaliser(texte))
    sans_accent = ''.join(c for c in sans_accent if unicodedata.category(c) != 'Mn')
    return sans_accent.lower()


def lignes_utiles(extracteur: Extracteur, pages: int) -> list[str]:
    """Les lignes du PDF, habillage retiré, dans l'ordre de lecture des deux colonnes.

    Le tri par coordonnée est **désactivé** ici : sur une page à deux colonnes, trier les blocs par
    ordonnée entrelace les colonnes, et le lexique sortirait alterné une entrée sur deux. L'ordre
    du document, lui, suit les colonnes — ce que l'ordre alphabétique du résultat permet de
    vérifier d'un coup d'œil.
    """
    lignes = []
    for index in range(pages):
        for ligne in extracteur.texte(index, tri=False).splitlines():
            ligne = normaliser(ligne)
            if ligne and not HABILLAGE_RE.match(ligne):
                lignes.append(ligne)
    return lignes


def assembler_entrees(lignes: list[str]) -> list[str]:
    """Recolle les entrées coupées en fin de colonne : une ligne sans ``=`` suit la précédente.

    Une entrée coupée **après un trait d'union** se recolle sans espace : la colonne a coupé au
    milieu d'un mot composé, et l'espace inséré produirait « demi- orc », « outre- monde »,
    « anti- détection ». Cinq entrées sur 2 084 sont dans ce cas ; aucune n'a de raison légitime
    de porter un trait d'union suivi d'une espace, en anglais comme en français.
    """
    entrees: list[str] = []
    for ligne in lignes:
        if SEPARATEUR in ligne:
            entrees.append(ligne)
        elif entrees:
            liaison = '' if entrees[-1].endswith('-') else ' '
            entrees[-1] = entrees[-1] + liaison + ligne
        # Une continuation sans entrée précédente n'existe pas dans ce document ; si elle
        # apparaissait, elle serait du texte d'introduction, à ignorer.
    return entrees


def decouper(entree: str) -> Entree:
    """``anglais = français ; catégorie`` → une entrée.

    La catégorie se coupe au **dernier** point-virgule, pas au premier : quelques traductions en
    contiennent un (``Avachissement; manifestation occulte``), et couper au premier attribuerait
    la moitié de la traduction à la catégorie.
    """
    anglais, reste = entree.split(SEPARATEUR, 1)
    if ';' in reste:
        francais, categorie = reste.rsplit(';', 1)
    else:
        francais, categorie = reste, ''
    return Entree(normaliser(anglais), normaliser(francais), normaliser(categorie))


def construire(corpus: Corpus, cache=None) -> list[Entree]:
    """Le lexique complet : le glossaire, puis les compléments attestés des *Basic Rules*."""
    document = corpus['glossaire']
    with Extracteur(document, cache=cache) as extracteur:
        entrees = [decouper(e) for e in assembler_entrees(
            lignes_utiles(extracteur, document.pages))]

    # La clé d'unicité est le couple (anglais, catégorie), et non l'anglais seul. Le glossaire
    # porte de vrais **homonymes** : `light` vaut « légère » comme propriété d'arme et « Lumière »
    # comme sort, `bane` vaut « Fléau / Imprécation » comme sort et « Baine » comme divinité.
    # Écraser l'un par l'autre perdrait la donnée sans rien dire — et pour `light`, ferait
    # traduire un sort par un adjectif d'arme.
    par_cle: dict[tuple[str, str], Entree] = {}
    for entree in entrees:
        if not entree.anglais or not entree.francais:
            raise GlossaireError('entrée incomplète extraite du glossaire : %r' % (entree,))
        cle = (entree.cle, normaliser_cle(entree.categorie))
        ancienne = par_cle.get(cle)
        if ancienne is None:
            par_cle[cle] = entree
        elif normaliser_cle(ancienne.francais) != normaliser_cle(entree.francais):
            raise GlossaireError(
                "« %s » (%s) est traduit deux fois différemment dans le glossaire : « %s » et "
                "« %s ». Une table d'autorité ne peut pas porter les deux."
                % (entree.anglais, entree.categorie or 'sans catégorie',
                   ancienne.francais, entree.francais))

    appliquer_complements(corpus, par_cle, cache)
    return sorted(par_cle.values(), key=lambda e: (e.cle, normaliser_cle(e.categorie)))


def appliquer_complements(corpus: Corpus, par_cle: dict, cache=None) -> None:
    """Ajoute ou catégorise les termes des *Basic Rules*, après les avoir vérifiés sur leur page.

    Trois issues, et une seule est silencieuse :

    - le terme est **absent** du glossaire → il est ajouté ;
    - il y est **sans catégorie**, avec la même traduction → la catégorie lui est donnée ;
    - il y est **autrement** → c'est un désaccord entre deux sources, et le lexique refuse de
      trancher tout seul. Une table d'autorité qui arbitre en silence n'en est plus une.
    """
    pages: dict[tuple[str, int], str] = {}
    for complement in COMPLEMENTS:
        document = corpus[complement.document]
        reference = (complement.document, complement.page_imprimee)
        if reference not in pages:
            with Extracteur(document, cache=cache) as extracteur:
                index = document.index_pdf(complement.page_imprimee)
                pages[reference] = normaliser_cle(extracteur.texte(index))
        if normaliser_cle(complement.francais) not in pages[reference]:
            raise GlossaireError(
                "« %s » est déclaré attesté page %d de %s, et ne s'y trouve pas. Le complément "
                "serait alors une donnée inventée présentée comme extraite."
                % (complement.francais, complement.page_imprimee, complement.document))

        anglais = normaliser_cle(complement.anglais)
        categorie = normaliser_cle(complement.categorie)
        deja_categorise = par_cle.get((anglais, categorie))
        sans_categorie = par_cle.get((anglais, ''))

        if deja_categorise is not None:
            if normaliser_cle(deja_categorise.francais) != normaliser_cle(complement.francais):
                raise GlossaireError(
                    "« %s » (%s) : le glossaire dit « %s », les Basic Rules « %s ». Deux sources "
                    "en désaccord — trancher est une décision humaine, pas une ligne de script."
                    % (complement.anglais, complement.categorie,
                       deja_categorise.francais, complement.francais))
        elif sans_categorie is not None and normaliser_cle(
                sans_categorie.francais) == normaliser_cle(complement.francais):
            # Le terme y était, sans catégorie : on la lui donne. La graphie du glossaire est
            # conservée, c'est celle sous laquelle le terme y est classé.
            del par_cle[(anglais, '')]
            par_cle[(anglais, categorie)] = Entree(
                sans_categorie.anglais, sans_categorie.francais, complement.categorie)
        else:
            par_cle[(anglais, categorie)] = Entree(
                complement.anglais, complement.francais, complement.categorie)


def ecrire_csv(entrees: list[Entree]) -> str:
    """Le fichier, en-tête et note de provenance comprises. Chaîne, pour être comparable."""
    tampon = io.StringIO(newline='')
    tampon.write(
        "# Lexique de traduction des termes de règle -- table d'autorité du projet (LOT-30).\n"
        '#\n'
        '# GÉNÉRÉ. Ne pas éditer à la main :\n'
        '#     python scripts/sourcebook glossaire\n'
        '#\n'
        "# Une seule traduction par terme de règle, dans tout le jeu. Ce n'est pas un catalogue\n"
        "# d'interface -- fr.lang et en.lang font cela ; c'est ce qui empêche « saving throw » de\n"
        "# devenir « jet de sauvegarde » dans la fiche et « JdS » dans le journal de combat.\n"
        '#\n'
        '# Source : Glossaire.pdf (aidedd.org), complété par Basic-Rules-FR.pdf. Provenance `srd`\n'
        '# pour la totalité du fichier (EX-CNT-001), OGL 1.0a -- inutile de la répéter par ligne.\n'
        '#\n'
        "# Plusieurs traductions d'un même terme sont séparées par « / » ; la première est la\n"
        '# forme retenue, les suivantes sont admises en lecture (check_glossary.py).\n')
    ecrivain = csv.writer(tampon, delimiter=';', lineterminator='\n')
    ecrivain.writerow(ENTETE)
    for entree in entrees:
        ecrivain.writerow((entree.anglais, entree.francais, entree.categorie))
    return tampon.getvalue()


def lire_csv(contenu: str) -> list[Entree]:
    """Relit le lexique produit — utilisé par ``scripts/check_glossary.py``."""
    lignes = [l for l in contenu.splitlines() if not l.startswith('#')]
    lecteur = csv.reader(lignes, delimiter=';')
    entrees = []
    for numero, colonnes in enumerate(lecteur, 1):
        if not colonnes or tuple(colonnes) == ENTETE:
            continue
        if len(colonnes) != 3:
            raise GlossaireError(
                '%s:%d : %d colonnes, 3 attendues (%s).'
                % (SORTIE, numero, len(colonnes), ';'.join(ENTETE)))
        entrees.append(Entree(*colonnes))
    return entrees


def statistiques(entrees: list[Entree]) -> dict:
    categories: dict[str, int] = {}
    for entree in entrees:
        categories[entree.categorie or '(sans catégorie)'] = \
            categories.get(entree.categorie or '(sans catégorie)', 0) + 1
    return {'entrees': len(entrees), 'categories': categories}
