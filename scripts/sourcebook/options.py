#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Les quatre catalogues d'options de personnage (LOT-43).

Compétences, langues, dons, multiclassage — les quatre catalogues que la fiche de personnage
suppose sans que rien ne dise d'où ils viennent. Ce module les produit depuis le corpus, et
**chaque source a été choisie pour une raison écrite ici** : sur ce lot, le choix de la source est
la moitié du travail.

| Catalogue | Source | Pourquoi celle-là |
|---|---|---|
| compétences | *Basic Rules* p. 64 | texte natif propre, liste complète en cinq lignes |
| langues | *Basic Rules* p. 38 | texte natif propre, table à trois colonnes |
| dons | **lexique** (`LOT-30`) pour les noms, *Manuel des Joueurs* pour les prérequis | voir plus bas |
| multiclassage | *Manuel des Joueurs* p. 165-166 | **seule** source complète : les *Basic Rules* y renvoient |

**Pourquoi les noms de dons viennent du lexique et non du livre.** Les titres de dons du *Manuel
des Joueurs* sont des scans mutilés — `DouÉ`, `E(PLOR.) __ TEUR DE DONJONS_`, `INITIÉ
----------------- À LA MAGIE`. Un détecteur de titres en attrape 38 sur 42, et les quatre qu'il
rate sont précisément les plus abîmés. Le lexique, lui, porte les 42 dons proprement, en paires
anglais ↔ français. C'est exactement ce pour quoi il a été construit.

Il tranche du même coup une divergence que personne n'avait vue : **les deux sources françaises ne
traduisent pas les mêmes dons pareil.** Le *Manuel des Joueurs* écrit « Adepte des éléments »,
« Spécialiste des boucliers », « Ritualiste » ; le lexique (aidedd) écrit « Adepte élémentaire »,
« Maître des boucliers », « Magie rituelle ». Les deux sont défendables ; ce qui ne l'est pas,
c'est d'en avoir deux dans le même jeu. Le lexique fait autorité (`LOT-30`), et le nom du livre est
conservé comme **variante connue** pour que la correspondance reste traçable.

**Pourquoi la table du multiclassage est recoupée.** L'OCR du *Manuel des Joueurs* **efface les
cellules valant `1`** : onze lignes sur vingt sont amputées, et un lanceur de niveau 20 y perd ses
emplacements de niveau 8 et 9. Une valeur absente ne se relit pas. La table est donc prise sur la
progression du **magicien** des *Basic Rules* — identique, et en texte natif — puis confrontée
cellule à cellule à celle du *Manuel des Joueurs* : toute cellule que l'OCR a bien lue doit
coïncider, et une divergence qui ne serait pas un `1` manquant **arrête la génération**.
"""
from __future__ import annotations

import json
import re
import unicodedata
from dataclasses import dataclass, field

from .corpus import Corpus
from .extraction import Extracteur
from .glossaire import normaliser, normaliser_cle
from .mise_en_page import colonnes_de_page

SORTIE_RPG = 'Source/Elements/Rpg'

# -- Compétences ---------------------------------------------------------------------------------
# Basic Rules p. 64 : « • Force : Athlétisme », « • Dextérité : Acrobaties, Discrétion,
# Escamotage »… Cinq lignes, dix-huit compétences. La page est le POINT D'ATTESTATION : le module
# vérifie que chaque nom s'y trouve avant de l'écrire.
PAGE_COMPETENCES = 64
# La liste tient dans la colonne DROITE de la page ; prendre la page entière entrelacerait le
# texte des deux colonnes, et quatre compétences en fin de ligne s'y perdraient.
REGION_COMPETENCES = (305, 560, 560, 665)

CARACTERISTIQUES = {
    'Force': 'strength',
    'Dextérité': 'dexterity',
    'Constitution': 'constitution',
    'Intelligence': 'intelligence',
    'Sagesse': 'wisdom',
    'Charisme': 'charisma',
}

# -- Langues -------------------------------------------------------------------------------------
PAGE_LANGUES = 38
REGION_LANGUES = (300, 130, 570, 420)

# -- Degres de difficulte -------------------------------------------------------------------------
# Basic Rules p. 64, table << Tache / DD >>. EX-REG-021 interdit qu'un nombre de difficulte
# apparaisse litteralement dans le code : ces six paliers sont une donnee, extraite comme le reste.
PAGE_DIFFICULTE = 64
REGION_DIFFICULTE = (85, 500, 240, 600)

# -- Multiclassage -------------------------------------------------------------------------------
# Toutes ces constantes sont des pages IMPRIMÉES, converties par `Document.index_pdf` : le
# Manuel des Joueurs est décalé de 1 et les confondre vise la page voisine, silencieusement.
PAGE_MULTICLASSAGE_REGLE = 165      # prérequis (colonne droite)
PAGE_MULTICLASSAGE_MAITRISES = 166  # maîtrises obtenues
PAGE_EMPLACEMENTS = 167             # table des emplacements de sorts
PAGE_MAGICIEN = 31                  # Basic Rules : la progression du magicien, en texte natif

REGION_PREREQUIS = (300, 230, 570, 410)
REGION_MAITRISES = (35, 175, 285, 455)
REGION_EMPLACEMENTS = (40, 300, 285, 555)
BORNES_EMPLACEMENTS = [75, 100, 117, 138, 160, 184, 207, 230, 252]


class OptionsError(Exception):
    """Un catalogue d'options n'a pas pu être produit tel qu'il est déclaré."""


@dataclass
class Catalogue:
    """Un dossier de sortie et les entrées qu'il porte."""

    dossier: str
    entrees: list = field(default_factory=list)


def _sans_accent(texte: str) -> str:
    decompose = unicodedata.normalize('NFD', texte)
    return ''.join(c for c in decompose if unicodedata.category(c) != 'Mn')


def identifiant(anglais: str) -> str:
    """Un identifiant kebab-case ASCII, tiré du terme **anglais** du lexique.

    L'anglais, et non le français : c'est la clé stable du projet, et la seule qui ne change pas
    quand une traduction est arbitrée autrement.
    """
    brut = _sans_accent(normaliser(anglais)).lower()
    brut = re.sub(r"[''’]", '', brut)
    return re.sub(r'-+', '-', re.sub(r'[^a-z0-9]+', '-', brut)).strip('-')


# Ce que le livre nomme autrement que le lexique. Le lexique fait autorité (`LOT-30`) ; cette
# table est la correspondance vers la graphie du livre, et elle est **déclarée** plutôt que
# devinée : un rapprochement approximatif finirait par apparier deux termes voisins et différents.
ALIAS_DU_LIVRE = {
    'elfique': 'Elvish',   # table des langues des Basic Rules ; le lexique dit « elfe »
    # FAUX AMI, et le plus dangereux du corpus : le Manuel des Joueurs appelle « Sorcier » la
    # classe que le lexique nomme « occultiste » (warlock). « Sorcier » ressemble à *sorcerer*,
    # qui est l'ENSORCELEUR — une autre classe, présente dans la même table, deux lignes plus
    # haut. Apparier ces deux-là par ressemblance intervertirait leurs prérequis en silence.
    'sorcier': 'warlock',
}


def index_par_francais(lexique: list, categorie: str) -> dict:
    """{forme française → entrée}, **une clé par variante**.

    Le lexique sépare les traductions concurrentes par « / » — `Deception = Tromperie /
    Supercherie`. Indexer sur la chaîne entière ne reconnaîtrait ni l'une ni l'autre, et la
    compétence sortirait manquante alors qu'elle est là, sous son autre nom.
    """
    index = {}
    for entree in lexique:
        if normaliser_cle(entree.categorie) != normaliser_cle(categorie):
            continue
        for variante in entree.francais.split('/'):
            index[normaliser_cle(variante)] = entree
    par_anglais = {normaliser_cle(e.anglais): e for e in index.values()}
    for forme, anglais in ALIAS_DU_LIVRE.items():
        cible = par_anglais.get(normaliser_cle(anglais))
        if cible is not None:
            index.setdefault(forme, cible)
    return index


# -- Compétences ---------------------------------------------------------------------------------

def competences(corpus: Corpus, lexique: list, cache=None) -> Catalogue:
    """Les dix-huit compétences et leur caractéristique associée."""
    document = corpus['basic-rules']
    with Extracteur(document, cache=cache) as extracteur:
        texte = normaliser(extracteur.texte(document.index_pdf(PAGE_COMPETENCES),
                                            region=REGION_COMPETENCES, tri=False))

    par_francais = index_par_francais(lexique, 'compétence')

    entrees = []
    for libelle, cle in CARACTERISTIQUES.items():
        motif = re.compile(r'•\s*%s\s*:\s*([^•]+?)(?=•|$)' % libelle)
        trouve = motif.search(texte)
        if trouve is None:
            continue
        # La ligne court sur la largeur de la colonne et se mêle au texte voisin : on ne retient
        # que les fragments qui sont des compétences connues du lexique.
        for morceau in re.split(r'[,\n]', trouve.group(1)):
            nom = normaliser(morceau).strip(' .')
            entree = par_francais.get(normaliser_cle(nom))
            if entree is None:
                continue
            entrees.append({
                'id': identifiant(entree.anglais),
                'name': entree.francais,
                'source': 'srd',
                'ability': cle,
            })
    vus = {e['id'] for e in entrees}
    attendues = {identifiant(e.anglais) for e in set(par_francais.values())}
    manquantes = sorted(attendues - vus)
    if manquantes:
        raise OptionsError(
            'compétences : %s figure(nt) au lexique et pas sur la page %d des Basic Rules. '
            'La page est le point d\'attestation : une compétence qui ne s\'y trouve pas serait '
            'une donnée inventée.' % (', '.join(manquantes), PAGE_COMPETENCES))
    return Catalogue('skills', sorted(entrees, key=lambda e: e['id']))


# -- Langues -------------------------------------------------------------------------------------

def langues(corpus: Corpus, lexique: list, cache=None) -> Catalogue:
    """Les seize langues, avec leur écriture et leurs races typiques.

    Deux entrées sont coupées par la largeur de colonne — « Commun des / profondeurs » et ses
    locuteurs « Beholders, flagelleurs / mentaux ». Les lignes sont donc **accumulées** jusqu'à ce
    que le nom obtenu figure au lexique, plutôt que lues une par une : c'est le lexique qui dit où
    une entrée se termine, et non la mise en page.
    """
    document = corpus['basic-rules']
    with Extracteur(document, cache=cache) as extracteur:
        lignes = extracteur.tableau(document.index_pdf(PAGE_LANGUES), region=REGION_LANGUES)

    par_francais = index_par_francais(lexique, 'langue')

    entrees: list = []
    exotique = False
    # Une entrée coupée par la largeur de colonne s'étale sur deux lignes, et les TROIS colonnes
    # sont coupées ensemble : « Commun des / profondeurs », « Créatures de / l'Outreterre ». On
    # accumule donc les trois de front, et c'est le lexique qui dit quand l'entrée est complète.
    attente = ['', '', '']
    for ligne in lignes:
        nom, races, ecriture = (normaliser(c) for c in (list(ligne) + ['', '', ''])[:3])
        if nom.startswith(('Langues exotiques', 'Langues standards')):
            exotique = nom.startswith('Langues exotiques')
            attente = ['', '', '']
            continue
        if not nom:
            # Ligne sans nom : elle prolonge les locuteurs de l'entrée DÉJÀ ÉMISE
            # (« Beholders, flagelleurs » / « mentaux »).
            if entrees and races:
                entrees[-1]['typicalSpeakers'] = normaliser(
                    entrees[-1]['typicalSpeakers'] + ' ' + races)
            continue

        attente = [normaliser(' '.join(x for x in (attente[i], valeur) if x))
                   for i, valeur in enumerate((nom, races, ecriture))]
        entree = par_francais.get(normaliser_cle(attente[0]))
        if entree is None:
            # Au-delà de quatre mots, ce n'est plus un nom coupé mais du texte voisin.
            if len(attente[0].split()) > 4:
                attente = ['', '', '']
            continue

        donnee = {
            'id': identifiant(entree.anglais),
            'name': entree.francais.split('/')[0].strip(),
            'source': 'srd',
            'exotic': exotique,
            'typicalSpeakers': attente[1],
        }
        if attente[2] and attente[2] != '-':
            donnee['script'] = attente[2]
        entrees.append(donnee)
        attente = ['', '', '']

    if len(entrees) != 16:
        raise OptionsError(
            'langues : %d extraites, 16 attendues (8 standards, 8 exotiques). La table de la '
            "page %d n'a pas été lue en entier." % (len(entrees), PAGE_LANGUES))
    return Catalogue('languages', sorted(entrees, key=lambda e: e['id']))


# -- Dons ----------------------------------------------------------------------------------------

# Les noms que le Manuel des Joueurs donne, quand ils diffèrent de ceux du lexique. Le lexique fait
# autorite ; ces variantes sont conservees pour que la correspondance livre <-> donnee reste
# tracable, et pour qu'une recherche par le nom du livre aboutisse.
VARIANTES_MANUEL = {
    'elemental-adept': 'Adepte des éléments',
    'great-weapon-master': "Grand maître d'armes",
    'heavily-armored': 'Lourdement protégé',
    'heavy-armor-master': 'Spécialiste des armures lourdes',
    'lightly-armored': 'Légèrement protégé',
    'medium-armor-master': 'Spécialiste des armures intermédiaires',
    'moderately-armored': 'Modérément protégé',
    'polearm-master': "Spécialiste des armes d'hast",
    'resilient': 'Résistant',
    'ritual-caster': 'Ritualiste',
    'shield-master': 'Spécialiste des boucliers',
    'martial-adept': 'Expert du combat',
}


def dons(corpus: Corpus, lexique: list, cache=None) -> Catalogue:
    """Les quarante-deux dons, nommés par le lexique, attestés dans le *Manuel des Joueurs*.

    Chaque don est livré **narratif** (`EX-RPG-040`, `EX-RPG-051`) et **provisoire**
    (`EX-CNT-032`) : aucun mécanisme de don n'existe encore dans le moteur, et un don qui se
    présenterait comme jouable sans l'être coûterait bien plus cher à diagnostiquer qu'un don
    déclaré non joué. Le critère de retrait est écrit d'avance.
    """
    entrees = []
    for entree in lexique:
        if normaliser_cle(entree.categorie) != 'don':
            continue
        cle = identifiant(entree.anglais)
        variantes = [v.strip() for v in entree.francais.split('/')][1:]
        if cle in VARIANTES_MANUEL and VARIANTES_MANUEL[cle] not in variantes:
            variantes.append(VARIANTES_MANUEL[cle])
        donnee = {
            'id': cle,
            'name': entree.francais.split('/')[0].strip(),
            'source': 'phb-fr',
            'narratif': True,
            'status': {
                'provisoire': True,
                'raison': "nom et provenance seulement : aucun mécanisme de don n'existe encore "
                          'dans le moteur',
                'retraitSi': "le LOT-13 a livré la fiche et les mécanismes que ce don exige, et "
                             "l'effet est décrit en `mecanismesRequis`",
            },
        }
        if variantes:
            donnee['variantes'] = variantes
        entrees.append(donnee)
    if len(entrees) != 42:
        raise OptionsError(
            'dons : %d au lexique sous la catégorie « don », 42 attendus. Le corpus ou le lexique '
            'a changé — vérifier avant de régénérer.' % len(entrees))
    return Catalogue('feats', sorted(entrees, key=lambda e: e['id']))


# -- Degres de difficulte -------------------------------------------------------------------------
# Basic Rules p. 64, table << Tache / DD >>. EX-REG-021 interdit qu'un nombre de difficulte
# apparaisse litteralement dans le code : ces six paliers sont une donnee, extraite comme le reste.
PAGE_DIFFICULTE = 64
REGION_DIFFICULTE = (85, 500, 240, 600)

# -- Creation de personnage ----------------------------------------------------------------------
# Les deux constantes de regle qu'une fiche emploie, avec la page IMPRIMEE et le motif de la phrase
# qui les atteste. Le motif capture le nombre LUI-MEME : sans cela, la valeur serait ecrite ici, et
# une constante ecrite dans le module d'extraction n'est pas plus extraite qu'une constante ecrite
# dans le C++.
CONSTANTES_DE_CREATION = (
    ('unarmoredArmorClass', 10,
     r"Sans armure[^.]{0,40}la CA de votre personnage est [ée]gale [àa] (\d+)[^.]{0,60}\."),
    ('maximumAbilityScore', 11,
     r"Vous ne pouvez pas augmenter une valeur de\s+caract[ée]ristique au-del[àa] de (\d+)\."),
)

# -- Experience --------------------------------------------------------------------------------
# Basic Rules p. 11, table << Points d'experience / Niveau / Bonus de maitrise >>, colonne droite.
# La region exclut l'en-tete (y < 200) : ses cellules ne sont pas des nombres et brouilleraient le
# controle de cardinal.
PAGE_EXPERIENCE = 11
REGION_EXPERIENCE = (320, 200, 560, 460)
NIVEAU_MAXIMAL = 20

# -- Multiclassage -------------------------------------------------------------------------------

def _table_magicien(corpus: Corpus, cache=None) -> dict:
    """La progression du magicien des *Basic Rules*, en texte natif : neuf colonnes par niveau."""
    document = corpus['basic-rules']
    with Extracteur(document, cache=cache) as extracteur:
        table = {}
        for ligne in extracteur._grouper_lignes(
                extracteur.mots(document.index_pdf(PAGE_MAGICIEN))):
            valeurs = [mot[4] for mot in ligne]
            if not valeurs or not valeurs[0].isdigit():
                continue
            nombres = [v for v in valeurs if v.isdigit() or v == '-']
            if len(nombres) < 10:
                continue
            table[int(valeurs[0])] = [0 if v == '-' else int(v) for v in nombres[-9:]]
    if sorted(table) != list(range(1, 21)):
        raise OptionsError(
            'multiclassage : la progression du magicien (Basic Rules p. %d) donne %d niveaux, 20 '
            'attendus.' % (PAGE_MAGICIEN, len(table)))
    return table


def _table_manuel(corpus: Corpus, cache=None) -> dict:
    """La table du multiclassage du *Manuel des Joueurs* — lacunaire, sert de contrôle."""
    document = corpus['manuel-des-joueurs']
    with Extracteur(document, cache=cache) as extracteur:
        table = {}
        for ligne in extracteur.tableau(document.index_pdf(PAGE_EMPLACEMENTS),
                                        region=REGION_EMPLACEMENTS,
                                        bornes=BORNES_EMPLACEMENTS):
            niveau = ligne[0].strip().replace('l', '1')
            if not niveau.isdigit():
                continue
            table[int(niveau)] = [
                int(c.strip().replace('l', '1')) if c.strip().replace('l', '1').isdigit() else 0
                for c in ligne[1:10]]
    return table


def _recouper_emplacements(reference: dict, controle: dict) -> list[str]:
    """Confronte les deux tables cellule à cellule. Renvoie les écarts *attendus* (des `1` perdus).

    Toute cellule que l'OCR a **lue** doit coïncider avec la référence. Une cellule lue et
    différente n'est pas une perte de `1` : c'est une divergence de fond, et elle arrête la
    génération. Une cellule absente côté OCR n'est tolérée que si la référence y met `1` — le seul
    chiffre que ce scan escamote.
    """
    pertes = []
    for niveau, attendus in sorted(reference.items()):
        lus = controle.get(niveau)
        if lus is None:
            continue
        for colonne, (attendu, lu) in enumerate(zip(attendus, lus), start=1):
            if lu == attendu:
                continue
            if lu == 0 and attendu == 1:
                pertes.append('niveau %d, emplacement de niveau %d' % (niveau, colonne))
                continue
            raise OptionsError(
                'multiclassage : au niveau %d, emplacement de niveau %d, la progression du '
                'magicien donne %d et le Manuel des Joueurs %d. Ce n\'est pas un « 1 » escamoté '
                'par l\'OCR mais une divergence de fond : les deux tables ne décrivent pas la '
                'même chose, et il faut trancher à la main avant de régénérer.'
                % (niveau, colonne, attendu, lu))
    return pertes


def multiclassage(corpus: Corpus, lexique: list, cache=None) -> tuple[dict, list[str]]:
    """La règle complète : prérequis, maîtrises, table d'emplacements. Et les pertes recoupées."""
    manuel = corpus['manuel-des-joueurs']

    with Extracteur(manuel, cache=cache) as extracteur:
        prerequis_bruts = extracteur.tableau(manuel.index_pdf(PAGE_MULTICLASSAGE_REGLE),
                                             region=REGION_PREREQUIS)
        maitrises_brutes = extracteur.tableau(manuel.index_pdf(PAGE_MULTICLASSAGE_MAITRISES),
                                              region=REGION_MAITRISES, bornes=[92])

    par_francais = index_par_francais(lexique, 'classe')

    def classe_connue(cellule: str):
        """L'entrée de lexique d'une cellule qui commence par un nom de classe, sinon None."""
        mots = normaliser(cellule).split()
        return par_francais.get(normaliser_cle(mots[0])) if mots else None

    prerequis = []
    for ligne in prerequis_bruts:
        cellule = normaliser(ligne[0])
        entree = classe_connue(cellule)
        if entree is None:
            continue  # en-tête, titre, ou texte de la colonne voisine
        exigence = cellule.split(' ', 1)[1] if ' ' in cellule else ''
        # « 1 ntelligence » : le seul artefact OCR de cette table, et il est visible à l'oeil.
        exigence = exigence.replace('1 ntelligence', 'Intelligence')
        prerequis.append({
            'class': identifiant(entree.anglais),
            'className': entree.francais,
            'requirement': exigence,
        })

    maitrises = []
    courante = None
    for ligne in maitrises_brutes:
        cellule, texte = (normaliser(c) for c in (list(ligne) + ['', ''])[:2])
        entree = classe_connue(cellule) if cellule else None
        if entree is not None:
            courante = {
                'class': identifiant(entree.anglais),
                'className': entree.francais,
                'proficiencies': texte,
            }
            maitrises.append(courante)
        elif courante is not None and texte and not cellule:
            # Ligne de continuation : la cellule de gauche est vide, la maîtrise se poursuit.
            courante['proficiencies'] = normaliser(courante['proficiencies'] + ' ' + texte)

    _verifier_progressions(corpus, cache)
    reference = _table_magicien(corpus, cache)
    pertes = _recouper_emplacements(reference, _table_manuel(corpus, cache))

    if len(prerequis) != 12 or len(maitrises) != 12:
        raise OptionsError(
            'multiclassage : %d prérequis et %d lignes de maîtrises, 12 de chaque attendus.'
            % (len(prerequis), len(maitrises)))

    return {
        'id': 'multiclassing',
        'name': 'Multiclassage',
        'source': 'phb-fr',
        'casterProgression': [
            {'class': cle, 'progression': valeur}
            for cle, valeur in sorted(PROGRESSIONS.items())
        ],
        'prerequisites': prerequis,
        'proficienciesGained': maitrises,
        'spellSlots': [{'casterLevel': n, 'slots': reference[n]} for n in sorted(reference)],
    }, pertes


# La progression de lanceur de chaque classe, telle que la phrase de la page 166 l'enonce :
#
#   « additionnez tous les niveaux dans vos classes de barde, de druide, de clerc, d'ensorceleur
#    et de magicien, la moitie de vos niveaux (arrondis a l'entier inferieur) dans les classes de
#    paladin et rodeur, et un tiers de vos niveaux de guerrier et de roublard (arrondis a l'entier
#    inferieur) si vous avez choisi l'archetype de chevalier occulte ou d'arnaqueur arcanique. »
#
# La phrase est le POINT D'ATTESTATION : `_verifier_progressions` verifie que chaque classe citee
# ci-dessous y figure bien, et que chaque classe qu'elle nomme est citee ici. Une table de
# fractions saisie de memoire est indetectablement fausse -- un lanceur multiclasse mal calcule
# reste jouable, simplement faux (EX-RPG-041).
PROGRESSIONS = {
    'bard': 'full', 'druid': 'full', 'cleric': 'full', 'sorcerer': 'full', 'wizard': 'full',
    'paladin': 'half', 'ranger': 'half',
    'fighter': 'third', 'rogue': 'third',
    # Le sorcier (occultiste) a la MAGIE DE PACTE, qui n'entre pas dans cette somme : ses
    # emplacements sont peu nombreux, toujours au niveau maximal, et recuperes au repos COURT
    # (EX-RPG-052). Les additionner aux autres serait la faute la plus couteuse de cette regle.
    'warlock': 'pact',
    'barbarian': 'none', 'monk': 'none',
}

# Les classes que la phrase cite, dans leur graphie francaise, par progression attendue.
CITEES = {
    'full': ('barde', 'druide', 'clerc', "d'ensorceleur", 'magicien'),
    'half': ('paladin', 'rodeur'),
    'third': ('guerrier', 'roublard'),
}


def _verifier_progressions(corpus: Corpus, cache=None) -> None:
    """La phrase de la page 166 cite bien les classes que `PROGRESSIONS` declare."""
    document = corpus['manuel-des-joueurs']
    with Extracteur(document, cache=cache) as extracteur:
        texte = normaliser_cle(
            extracteur.texte(document.index_pdf(PAGE_MULTICLASSAGE_MAITRISES), tri=False))
    absentes = [nom for noms in CITEES.values() for nom in noms
                if normaliser_cle(nom) not in texte]
    if absentes:
        raise OptionsError(
            'multiclassage : %s ne figure(nt) pas dans la regle de la page %d. La table des '
            'progressions de lanceur serait alors saisie de memoire, et une fraction fausse ne '
            "se voit pas -- le personnage reste jouable, simplement faux."
            % (', '.join(absentes), PAGE_MULTICLASSAGE_MAITRISES))


def difficulte(corpus: Corpus, cache=None) -> dict:
    """Les six degres de difficulte nommes, de « tres facile » a « quasi impossible »."""
    document = corpus['basic-rules']
    with Extracteur(document, cache=cache) as extracteur:
        lignes = extracteur.tableau(document.index_pdf(PAGE_DIFFICULTE),
                                    region=REGION_DIFFICULTE)

    paliers = []
    for ligne in lignes:
        nom, valeur = (normaliser(c) for c in (list(ligne) + ['', ''])[:2])
        if not valeur.isdigit():
            continue  # en-tete « Tache | DD »
        paliers.append({'id': identifiant(nom), 'name': nom, 'dc': int(valeur)})

    if len(paliers) != 6:
        raise OptionsError(
            "difficulte : %d paliers extraits page %d des Basic Rules, 6 attendus. Une echelle "
            'amputee ferait retomber les seuils manquants sur une valeur ecrite en dur, ce que '
            "EX-REG-021 interdit." % (len(paliers), PAGE_DIFFICULTE))
    if [p['dc'] for p in paliers] != sorted(p['dc'] for p in paliers):
        raise OptionsError(
            'difficulte : les paliers ne sont pas croissants — %s. '
            "L'ordre du tableau est significatif." % [p['dc'] for p in paliers])

    return {
        'id': 'difficulty',
        'name': 'Degres de difficulte',
        'source': 'srd',
        'tiers': paliers,
    }


# -- Production ------------------------------------------------------------------------------

def produire(corpus: Corpus, lexique: list, racine, cache=None) -> tuple[list, list[str]]:
    """Écrit les quatre catalogues sous ``racine``. Renvoie (fichiers écrits, pertes recoupées).

    **Un fichier par entrée**, nommé par son identifiant. C'est la forme que
    `scripts/checks/check_rpg_data.py` valide, et celle qui rend un conflit de fusion lisible : un
    catalogue en un seul tableau JSON ferait tenir quarante-deux dons dans un fichier, où deux
    ajouts simultanés se marchent dessus. Le multiclassage fait exception — c'est **une** règle,
    pas une collection, et la scinder n'aurait rien à séparer.
    """
    ecrits = []
    for catalogue in (competences(corpus, lexique, cache),
                      langues(corpus, lexique, cache),
                      dons(corpus, lexique, cache)):
        dossier = racine / catalogue.dossier
        dossier.mkdir(parents=True, exist_ok=True)
        for entree in catalogue.entrees:
            chemin = dossier / ('%s.json' % entree['id'])
            chemin.write_text(json.dumps(entree, ensure_ascii=False, indent=2) + '\n',
                              encoding='utf-8')
            ecrits.append(chemin)

    dossier = racine / 'rules'
    dossier.mkdir(parents=True, exist_ok=True)

    regle, pertes = multiclassage(corpus, lexique, cache)
    for nom, contenu in (('multiclassing', regle), ('difficulty', difficulte(corpus, cache)),
                         ('experience', experience(corpus, cache)),
                         ('character-creation', creation_de_personnage(corpus, cache))):
        chemin = dossier / ('%s.json' % nom)
        chemin.write_text(json.dumps(contenu, ensure_ascii=False, indent=2) + '\n',
                          encoding='utf-8')
        ecrits.append(chemin)

    return ecrits, pertes


def experience(corpus: Corpus, cache=None) -> dict:
    """La table d'expérience : seuil de PX et bonus de maîtrise, du niveau 1 au niveau 20.

    `EX-VIS-007` interdit qu'une valeur de règle vive dans le C++, et celle-ci est la plus tentante
    de toutes : vingt seuils et vingt bonus qu'on écrirait en trois lignes de code, et qui
    demanderaient alors une recompilation à chaque ajustement d'équilibrage. Sans équilibrage, un
    RPG n'est pas jouable.

    Le tableau est lu **par coordonnée** (`EX-CNT-021`) : ses trois colonnes n'ont ni filet ni
    séparateur, et un mode en flux y mêlerait les seuils aux numéros de niveau.

    Deux contrôles, parce que cette table a deux façons silencieuses de se tromper :

    - les vingt niveaux doivent être présents **et dans l'ordre** — une table amputée ne se voit
      qu'au niveau où elle manque, c'est-à-dire tard et en cours de partie ;
    - les seuils doivent être **strictement croissants** — deux seuils inversés rendraient une
      montée de niveau impossible à franchir, ou franchissable deux fois.
    """
    document = corpus['basic-rules']
    with Extracteur(document, cache=cache) as extracteur:
        lignes = extracteur.tableau(document.index_pdf(PAGE_EXPERIENCE),
                                    region=REGION_EXPERIENCE)

    niveaux = []
    for ligne in lignes:
        cellules = [normaliser(c) for c in (list(ligne) + ['', '', ''])[:3]]
        # Le livre separe les milliers par une espace fine -- << 2 700 >>, << 355 000 >>. La
        # retirer est sans risque : aucune cellule de cette table ne porte deux nombres.
        px, niveau, bonus = (c.replace(' ', '') for c in cellules)
        if not px.isdigit() or not niveau.isdigit():
            continue  # en-tete << Points d'experience | Niveau | Bonus de maitrise >>
        niveaux.append({'level': int(niveau), 'experience': int(px),
                        'proficiencyBonus': int(bonus.lstrip('+'))})

    if [n['level'] for n in niveaux] != list(range(1, NIVEAU_MAXIMAL + 1)):
        raise OptionsError(
            'experience : les niveaux extraits page %d des Basic Rules sont %s, 1 a %d attendus. '
            'Une table amputee ne se voit qu\'au niveau ou elle manque -- en cours de partie.'
            % (PAGE_EXPERIENCE, [n['level'] for n in niveaux], NIVEAU_MAXIMAL))
    seuils = [n['experience'] for n in niveaux]
    if seuils != sorted(set(seuils)):
        raise OptionsError(
            'experience : les seuils ne sont pas strictement croissants — %s. Deux seuils '
            'inverses rendent une montee de niveau infranchissable, ou franchissable deux fois.'
            % seuils)
    bonus = [n['proficiencyBonus'] for n in niveaux]
    if bonus != sorted(bonus):
        raise OptionsError(
            'experience : le bonus de maitrise decroit quelque part — %s.' % bonus)

    return {
        'id': 'experience',
        'name': "Table d'experience",
        'source': 'srd',
        'levels': niveaux,
    }


def creation_de_personnage(corpus: Corpus, cache=None) -> dict:
    """Les deux constantes de règle que la construction d'une fiche emploie (`LOT-13`).

    La classe d'armure sans armure et le plafond d'une valeur de caractéristique. Deux nombres, et
    ils valaient d'être extraits : un `10` nu dans un calcul de CA ne dit pas ce qu'il représente,
    et un `20` écrit dans le C++ ferait qu'ajuster le plafond d'un personnage demanderait une
    recompilation (`EX-VIS-007`).

    Chacun est **cherché dans la phrase qui l'atteste**, et la phrase est écrite dans la donnée
    produite. C'est ce qui distingue une constante extraite d'une constante tapée de mémoire : la
    seconde a l'air de la première, et rien ne les sépare une fois écrites.
    """
    document = corpus['basic-rules']
    with Extracteur(document, cache=cache) as extracteur:
        attestations = {}
        valeurs = {}
        for champ, page, motif in CONSTANTES_DE_CREATION:
            index = document.index_pdf(page)
            texte = normaliser(' '.join(
                extracteur.texte(index, region=region)
                for region in colonnes_de_page(extracteur, index)))
            trouve = re.search(motif, texte)
            if not trouve:
                raise OptionsError(
                    "creation de personnage : la phrase qui atteste « %s » est introuvable page "
                    '%d des Basic Rules. Sans elle, la valeur serait tapée de mémoire, ce qui a '
                    "l'air d'une extraction et n'en est pas une." % (champ, page))
            valeurs[champ] = int(trouve.group(1))
            attestations[champ] = {'page': page,
                                   'text': normaliser(trouve.group(0))}

    return {
        'id': 'character-creation',
        'name': 'Creation de personnage',
        'source': 'srd',
        'unarmoredArmorClass': valeurs['unarmoredArmorClass'],
        'maximumAbilityScore': valeurs['maximumAbilityScore'],
        'attestations': attestations,
    }
