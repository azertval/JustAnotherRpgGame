#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Espèces, historiques et classes provisoires (LOT-36).

De quoi construire un personnage jouable au plus tôt, en trois catalogues tirés de **trois
documents et deux langues**. C'est ce mélange qui fait la difficulté du lot, et chaque source a
été choisie pour une raison écrite ici.

| Catalogue | Source | Pourquoi celle-là |
|---|---|---|
| 4 races + 6 sous-races | *Basic Rules* p. 12-22 | texte natif propre, blocs `TRAITS` réguliers |
| 5 races | *Manuel des Joueurs* | **seule** source française de leurs mécaniques |
| 4 sous-espèces elfiques, 4 espèces | *Player's Guide* | propres au monde de Tanares |
| 6 historiques | *Basic Rules* p. 41-46 | texte natif propre |
| 7 historiques | *Player's Guide* p. 260-273 | propres à Tanares |
| 4 classes simplifiées | *Player's Guide* p. 192-207 | socle **provisoire** du premier combat |

Le *Manuel des Joueurs* est un scan, et sa graisse ment
--------------------------------------------------------

La méthode du `LOT-33` — la graisse porte la structure — **ne s'applique pas** au *Manuel*. Son
OCR attribue les polices au hasard : sur le bloc du tieffelin, « Vitesse. » ne porte aucune
graisse, « Âge. » en porte sur deux fragments non contigus, et les titres eux-mêmes sont mutilés —
`TaiJJe` pour « Taille », `Vision dans Je noir` pour « Vision dans le noir », `tliaumaturgie`,
`d'wie`. Un détecteur de titres n'y a rien à détecter.

Deux parades, et aucune n'est une relecture :

1. **Les mécaniques se lisent par leur phrase, pas par leur titre.** « votre valeur de Charisme
   augmente de 2 » se trouve quel que soit l'état du mot « Augmentation » qui l'introduit. Les
   quatre champs qui comptent — augmentation de caractéristique, taille, vitesse, langues — ont
   chacun une formulation stable que le module cherche dans **tout** le bloc.
2. **Les noms de traits viennent du lexique** (`LOT-30`), qui porte les capacités raciales sous
   ses catégories `capacité (nain)`, `capacité (tieffelin)`… — proprement, en paires anglais ↔
   français. C'est exactement ce pour quoi il a été construit, et c'est la parade que le `LOT-43`
   avait déjà employée pour les titres de dons.

**Et les augmentations de caractéristique sont recoupées.** Elles figurent deux fois dans le
*Manuel* : dans le bloc de la race, et dans la table « Augmentations raciales » de la page 12. Les
deux doivent coïncider, sinon la génération s'arrête. L'OCR y écrit systématiquement le chiffre 1
en lettre — `(+l)`, `(+ l)`, `{+l)` — ce qui est une **corruption visible** et non une disparition
(`LOT-43`) : aucun bonus racial ne vaut « +l », la normalisation est donc sûre, et elle est
comptée et annoncée à chaque exécution.

Ce que le corpus dit, et que la feuille de route disait autrement
------------------------------------------------------------------

Le sommaire du *Player's Guide* porte **douze** espèces, pas treize : nain, elfe, halfelin,
humain, drakéide, gnome, orc, tieffelin, cirrus, gloomfolk, soulborn, taii'maku — plus les quatre
sous-espèces elfiques, comptées à part. Le chiffre est relevé du sommaire, pas estimé.

**Tanares ne porte aucune mécanique pour les huit espèces classiques.** Ses chapitres leur donnent
de l'histoire, du peuplement, des variantes optionnelles — jamais un bloc de traits. « La version
de Tanares fait foi sur le fond » se lit donc comme le corpus le permet : le **fond narratif**
vient de Tanares, la **mécanique** des livres français, et chaque espèce déclare les deux dans son
champ `source`.

Les quatre classes sont provisoires, et le déclarent
------------------------------------------------------

Brawler, mage, priest et scoundrel sont le socle du premier modèle de combat : leur intérêt est
d'être simples, et elles font tourner attaques, dégâts et tours sans exiger d'abord le système
complet de ressources de classe. Elles seront **retirées** au profit des seize classes complètes
du `LOT-47` et des `LOT-51` à `LOT-65`, et portent donc `status.provisoire` avec son critère de
retrait écrit d'avance (`EX-CNT-032`). Un test vérifie qu'aucune donnée définitive ne les
référence : le jour du retrait, supprimer ces quatre fichiers ne doit rien casser.
"""
from __future__ import annotations

import json
import re
from dataclasses import dataclass, field

from .corpus import Corpus
from .extraction import Extracteur
from .catalogues import catalogue_francais, identifiant, index_avec_lexique
from .glossaire import normaliser, normaliser_cle
from .mise_en_page import colonnes_de_page, paragraphes, verifier_gouttiere

SORTIE_RPG = 'Source/Elements/Rpg'
PROVENANCE_SRD = 'srd'
PROVENANCE_PHB = 'phb-fr'
PROVENANCE_TANARES = 'tanares'

# -- Basic Rules ---------------------------------------------------------------------------------
# Pages IMPRIMEES. Le chapitre RACES ouvre p. 12 et CLASSES p. 23 ; les historiques vont d'ACOLYTE
# p. 41 a SOLDAT p. 46, EQUIPEMENT ouvrant p. 47.
PAGES_RACES_BR = range(12, 23)
PAGES_HISTORIQUES_BR = range(41, 47)

# Corps des titres du document, mesures : 16 pour une race ou un historique, 11 pour une section
# (`TRAITS`), 9,5 pour une sous-section -- dont les sous-races. Les seuils sont poses SOUS la
# valeur mesuree, jamais dessus : le corps rendu par le PDF est un flottant, et 16 s'y lit parfois
# 15,999998. Un seuil pose a l'egalite laisse alors passer le titre de chapitre et rate les quatre
# races -- ce qui ne produit aucune erreur, seulement un catalogue a une entree.
CORPS_RACE = 15.5
CORPS_SECTION = 10.5
CORPS_SOUS_SECTION = 9.4
POLICE_TITRE_BR = 'Cambria'

# Tolerance de comparaison de deux corps de caractere. Le corps rendu par le PDF est un flottant :
# 9,5 s'y lit 9,4599... et 16 s'y lit 15,999998. La tolerance doit rester SOUS l'ecart le plus
# petit qui separe deux niveaux de titre du corpus -- un demi-point, entre le 9,5 d'un titre de
# capacite et le 9,0 de son texte.
TOLERANCE_CORPS = 0.25

# La section qui porte les mecaniques. Tout ce qui precede est du recit.
SECTION_TRAITS = 'TRAITS'

# -- Manuel des Joueurs --------------------------------------------------------------------------
# La page de la table << Augmentations raciales >>, qui sert de recoupement.
PAGE_AUGMENTATIONS = 12

# -- Player's Guide ------------------------------------------------------------------------------
# Pages IMPRIMEES, relevees au sommaire (p. 4).
PAGES_ESPECES_PG = range(11, 56)
PAGES_HISTORIQUES_PG = range(260, 274)

# Corps des titres du Player's Guide : 18 pour une espece, une classe ou un chapitre, 14,5 pour une
# section, 17,5 pour le groupe des sous-especes elfiques.
CORPS_TITRE_PG = 17.4
CORPS_SECTION_PG = 14.0
POLICE_TITRE_PG = 'Heuristica'

# -- Vocabulaire ---------------------------------------------------------------------------------
CARACTERISTIQUES = {
    'force': 'strength',
    'dexterite': 'dexterity',
    'constitution': 'constitution',
    'intelligence': 'intelligence',
    'sagesse': 'wisdom',
    'charisme': 'charisma',
}
CARACTERISTIQUES_EN = {
    'strength': 'strength',
    'dexterity': 'dexterity',
    'constitution': 'constitution',
    'intelligence': 'intelligence',
    'wisdom': 'wisdom',
    'charisma': 'charisma',
}

# Taille ecrite en toutes lettres dans les blocs francais. Le lexique n'en porte que cinq --
# << Moyenne >> a echappe a l'extraction du glossaire (LOT-33) -- d'ou cette table declaree.
TAILLES = {
    'tres petite': 'tiny', 'petite': 'small', 'moyenne': 'medium', 'grande': 'large',
    'tres grande': 'huge', 'gigantesque': 'gargantuan',
}
TAILLES_EN = {
    'tiny': 'tiny', 'small': 'small', 'medium': 'medium', 'large': 'large', 'huge': 'huge',
    'gargantuan': 'gargantuan',
}

# Les etiquettes des blocs d'historique, par langue. Elles sont DECLAREES et non devinees : les
# deux livres n'annoncent pas la capacite de la meme facon -- les Basic Rules par un titre
# << CAPACITE : ABRI DU FIDELE >>, le Player's Guide par une etiquette << Feature: ... >> en gras.
ETIQUETTES_HISTORIQUE_FR = {
    'competences': r'Comp[ée]tences?\s+ma[îi]tris[ée]es?',
    'langues': r'Langues?',
    'outils': r'Outils?\s+ma[îi]tris[ée]s?',
    'equipement': r'[ÉE]quipement',
    'capacite': r'Capacit[ée]\s*:',
}
ETIQUETTES_HISTORIQUE_EN = {
    'competences': r'Skill\s+Proficiencies',
    'langues': r'Languages?',
    'outils': r'Tool\s+Proficiencies',
    'equipement': r'Equipment',
    'capacite': r'Feature\s*:',
}

# Les livres ecrivent le nombre de langues EN TOUTES LETTRES -- << deux de votre choix >>, << two
# of your choice >>. Chercher un chiffre ne trouve rien, et l'historique sort sans langue : le
# champ est facultatif, et son absence ne se signale nulle part.
NOMBRES_EN_LETTRES = {
    'zero': 0, 'un': 1, 'une': 1, 'one': 1, 'deux': 2, 'two': 2, 'trois': 3, 'three': 3,
    'quatre': 4, 'four': 4,
}

# Les huit especes que Tanares apporte, avec le titre du bloc qui porte leur mecanique et, pour les
# sous-especes, l'espece dont elles derivent. La liste est DECLAREE et attestee par le sommaire du
# livre (p. 4) : le chapitre melange, sous le meme corps de titre, des especes, des sections
# ("Six Colors of Magical Clones") et des variantes optionnelles ("Inclination to Engineering"),
# qui portent toutes une augmentation de caracteristique. Aucun critere typographique ni semantique
# ne les separe ; le sommaire, lui, les separe.
#
# Les huit espeeces CLASSIQUES du chapitre -- nain, elfe, halfelin, humain, drakeide, gnome, orc,
# tieffelin -- n'y figurent pas : Tanares leur donne de l'histoire et des variantes, jamais un bloc
# de traits. Leur mecanique vient des livres francais. L'ORC est le seul nom que ceux-la n'ont pas,
# et il reste hors du catalogue faute de mecanique : lui inventer une taille et une vitesse serait
# pire que son absence.
ESPECES_DE_TANARES = (
    ('Cirrus', 'Cirrus Traits', None),
    ('Gloomfolk', 'Gloomfolk Traits', None),
    ('Soulborn', 'Soulborn Traits', None),
    ("Taii'maku", "Taii'maku Traits", None),
    ('Elfe d\'été', 'Summer Elves', 'elfe'),
    ('Elfe de printemps', 'Spring Elves', 'elfe'),
    ('Elfe d\'automne', 'Autumn Elves', 'elfe'),
    ('Elfe d\'hiver', 'Winter Elves And Kemets', 'elfe'),
)

# Conversion des pieds du Player's Guide en metres. Le corpus francais compte deja en metres, et
# les deux unites dans un meme catalogue donneraient des elfes trois fois plus rapides que des
# nains. Le rapport est celui du livre : une case vaut 5 pieds ou 1,50 metre.
PIEDS_PAR_CASE = 5
METRES_PAR_CASE = 1.5

# Etiquettes de profil des blocs anglais : leur contenu part dans un champ TYPE.
ETIQUETTES_DE_PROFIL_EN = {
    'ability score increase', 'age', 'alignment', 'size', 'speed', 'languages', 'creature type',
    'subspecies', 'sub-species',
}

# Mecanisme declare quand le livre laisse un choix au joueur que la table du schema ne sait pas
# porter (EX-CNT-030).
MECANISME_CHOIX = 'augmentation-de-caracteristique-au-choix'

# Le << Manuel des Joueurs >> nomme le drakeide << Sangdragon >>. Le lexique fait autorite (LOT-30)
# et dit << drakeide >> ; la graphie du livre est conservee comme variante connue, exactement comme
# le LOT-43 l'a fait pour les douze dons que les deux sources francaises traduisent differemment.
ALIAS_DU_MANUEL = {
    'sangdragon': 'drakéide',
    'demi-ore': 'demi-orc',        # OCR : le `c` de << orc >> ressort en `e`, partout.
    'demi-ores': 'demi-orc',
}

# Les corruptions d'OCR du << Manuel des Joueurs >> qui touchent un mot que le module doit
# RECONNAITRE -- un nom de langue, un article. Elles sont declarees ici, une par une, avec ce que
# le scan a fait : une substitution non declaree serait indiscernable d'une regle du jeu.
#
# Toutes les autres corruptions -- `TaiJJe`, `tliaumaturgie`, `d'wie` -- sont laissees telles
# quelles : elles tombent dans du texte narratif, que le module ne compare a rien.
OCR_DU_MANUEL = (
    ("!'", "l'"),            # l' -> !' : le `l` bas de casse ressort en point d'exclamation
    ('commwi', 'commun'),    # le groupe `un` lu `wi`
    ('ore', 'orc'),          # `orc` lu `ore` PARTOUT : << demi-ore >>, << l'ore >>, << les ores >>
)

# Les cinq races que seul le << Manuel des Joueurs >> porte, avec leur page IMPRIMEE. Les pages
# sont DECLAREES et non cherchees : les titres du Manuel sont des scans mutiles, et celui du
# demi-elfe ne se retrouve par aucune expression raisonnable. Une page se verifie en ouvrant le
# livre ; un titre mutile ne se verifie pas.
RACES_DU_MANUEL = (
    ('drakéide', 41),
    ('demi-elfe', 33),
    ('demi-orc', 35),
    ('gnome', 37),
    ('tieffelin', 43),
)

# La table recapitulative << New Simplified Classes >>, page IMPRIMEE. Elle porte les quatre
# colonnes qu'aucun bloc de classe ne repete : de de vie, caracteristique principale, jets de
# sauvegarde, maitrises d'armes et d'armures.
PAGE_TABLE_CLASSES = 57

# Les tables de progression, une par classe, page IMPRIMEE. Vingt niveaux chacune.
ZIP_CLASSES = (
    ('Brawler', 193),
    ('Mage', 197),
    ('Priest', 201),
    ('Scoundrel', 205),
)
NIVEAUX_ATTENDUS = 20

# Les quatre classes simplifiees, et leur ordre dans le livre.
CLASSES_SIMPLIFIEES = ('Brawler', 'Mage', 'Priest', 'Scoundrel')

# Critere de retrait, ecrit d'avance (EX-CNT-032). Il est ici et pas dans quatre fichiers : quatre
# copies d'un critere divergent, et celle qu'on lirait ne serait pas celle qui vaut.
RETRAIT_CLASSES = ('les seize classes completes sont livrees (LOT-47, LOT-51 a LOT-65) et aucune '
                   'donnee definitive ne reference ces quatre classes')
RAISON_CLASSES = ('socle provisoire du premier modele de combat : simple a jouer, suffisant pour '
                  'faire tourner attaques, degats et tours avant le systeme complet de classes')


# Les etiquettes de profil d'un bloc de race. Leur contenu part dans un champ TYPE ; les reprendre
# aussi en trait ferait deux sources pour la meme regle, et la seconde vieillirait sans qu'on la
# relise.
ETIQUETTES_DE_PROFIL = {
    'augmentation de caracteristiques', 'augmentation de caracteristique', 'age', 'alignement',
    'taille', 'vitesse', 'langues', 'sous-race',
}


class PersonnageError(Exception):
    """Un catalogue de personnage n'a pas pu être produit tel qu'il est déclaré."""


@dataclass
class Bloc:
    """Un bloc titré et ses lignes, en ordre de lecture, avec leur segment de colonne."""

    titre: str
    corps: float
    lignes: list = field(default_factory=list)

    def paragraphes(self) -> list:
        return paragraphes(self.lignes)

    def texte(self) -> str:
        return normaliser(' '.join(l.texte for _, l in self.lignes))


# -- Outils communs ------------------------------------------------------------------------------

def decouper_en_blocs(extracteur: Extracteur, pages, police: str, corps_titre: float,
                      moities=(None,)) -> list[Bloc]:
    """Découpe une plage de pages en blocs, un par titre au corps demandé.

    Les lignes sont lues **colonne par colonne**, dans l'ordre de lecture, et chacune garde le
    segment (page, moitié, colonne) dont elle vient : c'est ce qui permet ensuite de regrouper les
    paragraphes sans comparer les ordonnées de deux colonnes différentes.
    """
    blocs: list[Bloc] = []
    for imprimee in pages:
        index = extracteur.document.index_pdf(imprimee)
        if not 0 <= index < extracteur.document.pages:
            continue
        for moitie in moities:
            rect = extracteur.rectangle(index, moitie)
            milieu = (rect.x0 + rect.x1) / 2
            regions = ((rect.x0, rect.y0, milieu, rect.y1),
                       (milieu, rect.y0, rect.x1, rect.y1))
            for numero, region in enumerate(regions):
                segment = (index, moitie, numero)
                for ligne in extracteur.lignes(index, moitie, region):
                    titre = (ligne.fragments[0].police.startswith(police)
                             and ligne.corps >= corps_titre)
                    if titre:
                        blocs.append(Bloc(titre=normaliser(ligne.texte), corps=ligne.corps))
                    elif blocs:
                        blocs[-1].lignes.append((segment, ligne))
    return blocs


def lire_augmentations(texte: str) -> dict:
    """« Votre Charisme augmente de 2 et votre valeur d'Intelligence de 1 » → la table du schéma.

    Une **table**, jamais une phrase : c'est la seule forme que le moteur puisse appliquer, et
    c'est ce que `species.schema.json` exige. La recherche porte sur les six noms de
    caractéristique, ce qui la rend indifférente à l'état du titre qui introduit la phrase — le
    point qui fait tenir l'extraction sur un scan (voir l'en-tête du module).
    """
    # << Toutes vos caracteristiques augmentent de 1 >> : l'humain est la seule race dont
    # l'augmentation est ecrite COLLECTIVEMENT. Chercher les six noms un a un n'en trouve aucun,
    # et l'humain sortirait sans augmentation -- une race valide au schema, et fausse.
    collectif = re.search(r'toutes\s+vos\s+caracteristiques\s+augmentent\s+de\s+(\d+)',
                          normaliser_cle(texte))
    if collectif:
        return {champ: int(collectif.group(1)) for champ in CARACTERISTIQUES.values()}

    valeurs = {}
    for cle, champ in CARACTERISTIQUES.items():
        # « votre Force augmente de 2 », « votre valeur de Force augmente de 2 », « et votre
        # valeur d'Intelligence de 1 » : le verbe est parfois sous-entendu dans la seconde moitié
        # de la phrase, ce que le livre fait systématiquement.
        motif = (r'(?:valeur\s+d[eu\']?\s*)?%s\s*(?:augmente|de\s+base)?\s*'
                 r'(?:augmente\s+)?de\s+(\d+)' % cle)
        trouve = re.search(motif, normaliser_cle(texte))
        if trouve:
            valeurs[champ] = int(trouve.group(1))
    return valeurs


def lire_taille(texte: str) -> str | None:
    """« Votre taille est Moyenne (M) », « Ils sont de taille Moyenne » → la valeur du schéma."""
    bas = normaliser_cle(texte)
    for mot, valeur in sorted(TAILLES.items(), key=lambda x: -len(x[0])):
        if re.search(r'taille\s+(?:est\s+)?%s' % mot, bas) or re.search(r'de\s+taille\s+%s' % mot,
                                                                       bas):
            return valeur
    return None


def lire_vitesse(texte: str) -> float | None:
    """« Votre vitesse de base est de 7,50 mètres » → 7.5, en mètres."""
    trouve = re.search(r'vitesse\s+de\s+base[^.]{0,40}?est\s+de\s+([\d,.]+)\s*m',
                       normaliser_cle(texte))
    if not trouve:
        return None
    valeur = float(trouve.group(1).replace(',', '.'))
    return int(valeur) if valeur == int(valeur) else valeur


def lire_langues(texte: str, langues: dict) -> list[str]:
    """Les langues du catalogue citées par un bloc, dans l'ordre du texte.

    Seules celles que le catalogue du `LOT-43` porte sont retenues, et la comparaison se fait sur
    ses graphies — nom, identifiant, alias du livre. Inventer une entrée de catalogue depuis une
    phrase serait exactement ce que le `LOT-33` a refusé de faire pour « l'aérien ».
    """
    citees = []
    # `!'ore` : l'OCR du Manuel rend le `l` de l'article par un point d'exclamation, et le `c`
    # d'`orc` par un `e`. Sans les deux corrections, le demi-orc sort ne parlant que le commun.
    texte = texte.replace("!'", "l'")
    for mot in re.findall(r"\b(?:le|la|les|du|l'|d')\s*([\wÀ-ÿ-]+)", texte):
        cle = normaliser_cle(corriger_ocr(mot))
        if cle in langues and langues[cle] not in citees:
            citees.append(langues[cle])
    return citees


def index_lexique(lexique: list, prefixe: str) -> dict:
    """{français normalisé → français accentué} pour les catégories du préfixe.

    La clé est **sans accent** parce que la recherche se fait sur un texte normalisé ; la valeur
    les **garde**, parce que c'est elle qui s'affichera. Rendre la clé des deux côtés livrerait
    « Resistance aux degats » comme nom de trait, dans un jeu en français.
    """
    index = {}
    for entree in lexique:
        if not normaliser_cle(entree.categorie).startswith(normaliser_cle(prefixe)):
            continue
        for variante in entree.francais.split('/'):
            propre = normaliser(variante).strip()
            index.setdefault(normaliser_cle(propre), propre)
    return index


# -- Especes des Basic Rules ---------------------------------------------------------------------

def especes_basic_rules(corpus: Corpus, tables: dict, cache=None) -> list[dict]:
    """Les 4 races des *Basic Rules* et leurs 6 sous-races.

    Chaque race ouvre par un titre au corps 16 ; sa section ``TRAITS`` (corps 11) porte les
    mécaniques, tout ce qui précède étant du récit. Les sous-races sont des titres au corps 9,5
    **dans** cette section — mais tous les titres de ce corps n'en sont pas : « DUERGAR » et
    « VARIANTE » sont des encadrés. Le départage est sémantique et non typographique : une
    sous-race porte une augmentation de caractéristique, un encadré n'en porte pas.
    """
    especes: list[dict] = []
    with Extracteur(corpus['basic-rules'], cache=cache) as extracteur:
        pages = [extracteur.document.index_pdf(p) for p in PAGES_RACES_BR]
        verifier_gouttiere(extracteur, pages)
        blocs = decouper_en_blocs(extracteur, PAGES_RACES_BR, POLICE_TITRE_BR, CORPS_SOUS_SECTION)

    race = None
    dans_traits = False
    for bloc in blocs:
        if bloc.corps >= CORPS_RACE:
            # Le titre de chapitre << RACES >> et la section << CHOISIR UNE RACE >> sont composes
            # au meme corps qu'une race. Ce qui les distingue n'est pas typographique : une race a
            # une section TRAITS, eux n'en ont pas. L'espece n'est donc retenue qu'apres l'avoir
            # trouvee.
            race = _espece_depuis(bloc, tables, PROVENANCE_SRD)
            dans_traits = False
            continue
        if race is None:
            continue
        if bloc.corps >= CORPS_SECTION:
            dans_traits = normaliser_cle(bloc.titre) == normaliser_cle(SECTION_TRAITS)
            if dans_traits:
                _remplir_mecaniques(race, bloc, tables)
                especes.append(race)
            continue
        # Corps 9,5 : sous-race ou encadre. Seule une sous-race porte une augmentation.
        if not dans_traits:
            continue
        augmentations = lire_augmentations(bloc.texte())
        if not augmentations:
            continue
        sous = _espece_depuis(bloc, tables, PROVENANCE_SRD)
        sous['parentSpecies'] = race['id']
        sous['size'] = race['size']
        sous['speed'] = race['speed']
        _remplir_mecaniques(sous, bloc, tables)
        especes.append(sous)
    return especes


def _espece_depuis(bloc: Bloc, tables: dict, provenance: str) -> dict:
    """L'ossature d'une espèce : identifiant, nom d'affichage, provenance.

    Le titre du livre est en capitales — « NAIN DES MONTAGNES ». Le nom d'affichage vient du
    **lexique**, qui porte la casse de référence ; à défaut, le titre est mis en bas de casse avec
    une capitale initiale, et le module le signale.
    """
    cle = normaliser_cle(bloc.titre)
    cle = normaliser_cle(ALIAS_DU_MANUEL.get(cle, cle))
    nom = tables['noms_races'].get(cle)
    if nom is None:
        nom = normaliser(bloc.titre).lower()
    # Le lexique ecrit les races en bas de casse (<< nain >>, << haut-elfe >>) et le livre en
    # capitales (<< NAIN DES MONTAGNES >>). Ni l'un ni l'autre n'est une casse d'affichage : la
    # capitale initiale est posee ici, une fois, plutot que par chaque ecran qui affichera un nom.
    nom = nom[:1].upper() + nom[1:]
    return {'id': identifiant(nom), 'name': nom, 'source': provenance}


def _remplir_mecaniques(espece: dict, bloc: Bloc, tables: dict) -> None:
    """Augmentations, taille, vitesse, langues et traits nommés, lus dans le bloc."""
    texte = bloc.texte()
    augmentations = lire_augmentations(texte)
    if augmentations:
        espece['abilityScoreIncrease'] = augmentations
    taille = lire_taille(texte)
    if taille:
        espece['size'] = taille
    vitesse = lire_vitesse(texte)
    if vitesse is not None:
        espece['speed'] = vitesse
    langues = _langues_du_bloc(bloc, tables)
    if langues:
        espece['languages'] = langues
    traits = _traits_du_bloc(bloc)
    if traits:
        espece['traits'] = traits


def _langues_du_bloc(bloc: Bloc, tables: dict) -> list[str]:
    """Les langues citées par le paragraphe « Langues » du bloc, et par lui seul.

    Chercher dans le bloc entier ramasserait « le nain » de « la langue naine comporte de
    nombreuses consonnes dures » — vrai ici, faux ailleurs, et faux sans prévenir.
    """
    for groupe in bloc.paragraphes():
        entier = normaliser(' '.join(l.texte for l in groupe))
        if re.match(r'^Langues?\s*\.', entier):
            return lire_langues(entier, tables['langues'])
    return []


def _est_etiquette(paragraphe: str) -> bool:
    """Vrai si le paragraphe ouvre sur une des étiquettes de profil, quelle que soit sa graisse."""
    bas = normaliser_cle(paragraphe)
    return any(bas.startswith(etiquette) for etiquette in ETIQUETTES_DE_PROFIL)


def _traits_du_bloc(bloc: Bloc) -> list[dict]:
    """Les traits nommés du bloc : un paragraphe ouvert par un fragment gras.

    Les six étiquettes de profil — augmentation, âge, alignement, taille, vitesse, langues — sont
    écartées : leur contenu est déjà dans un champ typé, et le répéter en trait ferait deux
    sources pour la même règle.
    """
    traits: list[dict] = []
    for groupe in bloc.paragraphes():
        if not groupe[0].fragments[0].gras:
            continue
        nom = ''
        for fragment in groupe[0].fragments:
            if not fragment.gras:
                break
            nom += fragment.texte
        nom = normaliser(nom).rstrip('.').strip()
        texte = normaliser(' '.join(l.texte for l in groupe))
        # La graisse s'arrete parfois AU MILIEU du titre -- << **Augmentation** de
        # caracteristiques. >> --, et le nom retenu serait alors << Augmentation >>. L'etiquette se
        # reconnait donc sur le debut du PARAGRAPHE, ou le titre est entier quel que soit l'endroit
        # ou la graisse s'interrompt.
        if not nom or _est_etiquette(texte):
            continue
        if normaliser_cle(texte).startswith(normaliser_cle(nom)):
            texte = normaliser(texte[len(nom):].lstrip(' .'))
        if texte:
            traits.append({'name': nom, 'text': texte})
    return traits


# -- Especes du Manuel des Joueurs ---------------------------------------------------------------

def augmentations_raciales(extracteur: Extracteur) -> dict:
    """La table « Augmentations raciales » de la page 12, lue par coordonnée.

    Elle est le **recoupement** des cinq blocs de race tirés du *Manuel* — la seule chose qui
    empêche une valeur mal lue sur un scan de passer pour une règle. Le nom de la caractéristique
    ouvre chaque groupe et les races suivent jusqu'au nom suivant : c'est l'ordonnée qui les
    rattache, pas la lecture en flux, qui les mélangerait avec le corps de texte de la colonne
    voisine.

    Renvoie ``{race normalisée: {caractéristique: bonus}}``.
    """
    index = extracteur.document.index_pdf(PAGE_AUGMENTATIONS)
    caracteristique = None
    table: dict = {}
    for ligne in extracteur.lignes(index):
        texte = normaliser(ligne.texte)
        cle = normaliser_cle(texte)
        if cle in CARACTERISTIQUES:
            caracteristique = CARACTERISTIQUES[cle]
            continue
        if caracteristique is None:
            continue
        for nom, bonus in re.findall(r'([A-Za-zÀ-ÿ][\wÀ-ÿ \'-]*?)\s*[({]\s*\+\s*([\dl])\s*[)}]',
                                     texte):
            table.setdefault(_cle_de_race(nom), {})[caracteristique] = _chiffre(bonus)
    return table


def _chiffre(brut: str) -> int:
    """« l » est le chiffre 1. L'OCR du *Manuel* écrit systématiquement la lettre à sa place.

    C'est une **corruption visible**, non une disparition (`LOT-43`) : aucun bonus racial ne vaut
    « +l », la lecture est donc sûre. Elle est comptée et annoncée, jamais silencieuse.
    """
    return 1 if brut.lower() == 'l' else int(brut)


def _cle_de_race(nom: str) -> str:
    """Le nom d'une race, ramené à la graphie du lexique.

    Le *Manuel* écrit « Sangdragon » là où le lexique dit « drakéide », et son OCR rend le `c` de
    « orc » par un `e` — « demi-ore » — partout, table et blocs compris. Les deux sont déclarés
    plutôt que devinés : un rapprochement approximatif apparierait « demi-ore » et « demi-elfe ».
    Le pluriel est retiré parce que la table écrit « Nains » et les blocs « nain ».
    """
    cle = normaliser_cle(nom).strip()
    cle = ALIAS_DU_MANUEL.get(cle, cle)
    cle = normaliser_cle(cle)
    return re.sub(r's$', '', cle)


def corriger_ocr(mot: str) -> str:
    """Un mot du *Manuel*, ses corruptions d'OCR déclarées corrigées. Voir ``OCR_DU_MANUEL``."""
    bas = normaliser_cle(mot)
    for faux, juste in OCR_DU_MANUEL:
        if bas == normaliser_cle(faux):
            return juste
    return mot


def _phrase_des_langues(bloc: str) -> str:
    """La phrase « Langues. … » d'un bloc, et elle seule.

    Chercher les langues dans le bloc entier ramasse celles que la prose cite au passage : le
    demi-orc en ressortait parlant gnome, parce que le bloc du gnome commence à la page suivante.
    """
    trouve = re.search(r'Langues?\s*\.\s*(.{0,300})', bloc)
    return trouve.group(1) if trouve else ''


def _texte_des_pages(extracteur: Extracteur, imprimees) -> str:
    """Le texte de plusieurs pages, colonne par colonne, dans l'ordre de lecture."""
    morceaux = []
    for imprimee in imprimees:
        index = extracteur.document.index_pdf(imprimee)
        if not 0 <= index < extracteur.document.pages:
            continue
        for region in colonnes_de_page(extracteur, index):
            morceaux.append(extracteur.texte(index, region=region))
    return normaliser(' '.join(morceaux))


def especes_manuel(corpus: Corpus, tables: dict) -> tuple[list[dict], list[str]]:
    """Les cinq races que seul le *Manuel des Joueurs* porte. Renvoie (espèces, signalements).

    Leurs pages sont **déclarées**, pas cherchées : les titres du *Manuel* sont des scans mutilés
    — `TaiJJe`, `Vision dans Je noir` — et le titre du demi-elfe ne se retrouve par aucune
    expression raisonnable. La page, elle, est vérifiable en ouvrant le livre, et le module
    contrôle qu'elle ne porte qu'**un seul** bloc de traits : deux blocs sur la même page
    mélangeraient deux races sans rien casser.
    """
    especes: list[dict] = []
    signalements: list[str] = []
    with Extracteur(corpus['manuel-des-joueurs']) as extracteur:
        recoupement = augmentations_raciales(extracteur)
        for nom_lexique, imprimee in RACES_DU_MANUEL:
            # La page declaree ET la suivante : un bloc de traits deborde regulierement sur la
            # page d'apres -- celui du gnome y laisse sa vitesse et ses langues. Ne lire que la
            # page declaree amputerait la race de ses derniers traits, silencieusement.
            texte = _texte_des_pages(extracteur, (imprimee, imprimee + 1))

            # Le bloc commence au premier repere mecanique, et s'arrete au suivant : l'augmentation
            # de caracteristique n'apparait qu'une fois par race, si bien que la deuxieme annonce
            # la SOUS-RACE ou la race suivante. Les quatre reperes sont cherches parce qu'aucun
            # n'est garanti -- l'OCR a entierement efface l'augmentation ET l'age du demi-elfe.
            reperes = [m.start() for motif in (r'Augmentation\s+de\s+caract', r'Alignement\s*\.',
                                               r'Taille\s*\.', r'vitesse\s+de\s+base')
                       for m in re.finditer(motif, texte, re.I)]
            if not reperes:
                raise PersonnageError(
                    'Manuel des Joueurs p. %d : aucun repère de bloc de traits pour « %s ». La '
                    'page est déclarée dans RACES_DU_MANUEL et doit en porter un.'
                    % (imprimee, nom_lexique))
            bloc = texte[min(reperes):]
            augmentations = [m.start() for m in re.finditer(r'Augmentation\s+de\s+caract', bloc,
                                                            re.I)]
            if len(augmentations) > 1:
                bloc = bloc[:augmentations[1]]

            espece = {
                'id': identifiant(nom_lexique),
                'name': nom_lexique[:1].upper() + nom_lexique[1:],
                'source': PROVENANCE_PHB,
            }
            lues = lire_augmentations(bloc)
            attendues = recoupement.get(_cle_de_race(nom_lexique), {})
            # Le recoupement peut COMPLETER, pas seulement contredire : le bloc du demi-elfe a
            # perdu sa ligne d'augmentation a l'OCR, et la table de la page 12 la restitue.
            signalements.extend(_recouper(nom_lexique, imprimee, lues, attendues))
            if not lues:
                raise PersonnageError(
                    'Manuel des Joueurs p. %d : aucune augmentation de caractéristique pour '
                    '« %s », ni dans le bloc ni dans la table des augmentations raciales. Les '
                    'deux sources sont muettes ; il n\'y a rien à recouper.'
                    % (imprimee, nom_lexique))
            espece['abilityScoreIncrease'] = lues
            taille = lire_taille(bloc)
            if taille:
                espece['size'] = taille
            vitesse = lire_vitesse(bloc)
            if vitesse is not None:
                espece['speed'] = vitesse
            langues = lire_langues(_phrase_des_langues(bloc), tables['langues'])
            if langues:
                espece['languages'] = langues
            espece['traits'] = _traits_du_lexique(bloc, tables['traits_raciaux'])
            if not espece['traits']:
                del espece['traits']
            especes.append(espece)
    return especes, signalements


def _recouper(nom: str, page: int, lues: dict, attendues: dict) -> list[str]:
    """Confronte le bloc de la race à la table de la page 12. Une divergence **arrête** tout.

    La règle est celle du `LOT-43`, et elle distingue deux cas que rien ne distingue à l'œil :

    - une valeur **présente des deux côtés** et différente est une lecture fausse quelque part :
      la génération s'arrête, parce que les deux sources ne décrivent plus la même règle ;
    - une valeur **présente d'un seul côté** est une ligne que l'OCR a escamotée. C'est le défaut
      documenté du *Manuel* — il efface, il ne corrompt pas —, et il est rapporté, pas fatal.
    """
    signalements = []
    for champ in set(lues) | set(attendues):
        a, b = lues.get(champ), attendues.get(champ)
        if a is not None and b is not None and a != b:
            raise PersonnageError(
                '%s (p. %d) : le bloc donne %s +%d, la table des augmentations raciales +%d. '
                'Les deux sources ne décrivent plus la même règle ; ce n\'est pas une cellule '
                'escamotée, et cela se tranche à la main.' % (nom, page, champ, a, b))
        if b is None:
            signalements.append(
                '%s : %s +%d figure dans le bloc de la page %d, pas dans la table des '
                'augmentations raciales de la page %d — une ligne escamotée par l\'OCR. La '
                'valeur du bloc est retenue.' % (nom, champ, a, page, PAGE_AUGMENTATIONS))
        elif a is None:
            signalements.append(
                '%s : %s +%d figure dans la table de la page %d, pas dans le bloc de la page %d. '
                'La valeur de la table est retenue.' % (nom, champ, b, PAGE_AUGMENTATIONS, page))
            lues[champ] = b
    return signalements


def _traits_du_lexique(bloc: str, traits_raciaux: dict) -> list[dict]:
    """Les traits raciaux du bloc, **nommés par le lexique** et non par le livre.

    Les titres du *Manuel* sont mutilés — « Vision dans Je noir », « TaiJJe » —, et les livrer tels
    quels ferait entrer la faute d'OCR dans la donnée du jeu. Le lexique porte ces mêmes capacités
    proprement, sous ses catégories `capacité (nain)`, `capacité (tieffelin)`… ; le module y
    cherche celles que le bloc mentionne, et découpe le texte entre deux titres reconnus.

    Un trait que le lexique ignore est **perdu**, et c'est assumé : mieux vaut une espèce dont les
    traits nommés sont justes qu'une espèce dont un trait s'appelle « Vision dans Je noir ».
    """
    plat = normaliser_cle(bloc)
    positions = []
    for francais in sorted(traits_raciaux, key=len, reverse=True):
        if len(francais) < 5:
            continue
        # Un nom du lexique ne vaut TITRE que s'il ouvre une phrase et qu'un point le ferme :
        # « Ascendance féerique. Vous êtes avantagé… ». Sans cette condition, « votre Souffle, »
        # au milieu d'une phrase ouvre un faux trait, et le drakéide en sortait avec quinze traits
        # dont douze étaient des morceaux de phrase.
        #
        # La recherche porte sur la forme normalisée, indice pour indice : `normaliser_cle` retire
        # les accents sans changer les longueurs, et les positions valent donc pour les deux.
        for trouve in re.finditer(r'(?:^|(?<=\.\s))%s\s*\.' % re.escape(francais), plat):
            positions.append((trouve.start(), trouve.end(), francais))
    positions.sort()
    retenues = []
    for debut, fin, francais in positions:
        if retenues and debut < retenues[-1][1]:
            continue
        retenues.append((debut, fin, francais))

    traits = []
    for numero, (_debut, fin, francais) in enumerate(retenues):
        borne = retenues[numero + 1][0] if numero + 1 < len(retenues) else len(bloc)
        texte = normaliser(bloc[fin:borne]).lstrip(' .')
        nom = traits_raciaux[francais]
        if texte:
            traits.append({'name': nom[:1].upper() + nom[1:], 'text': texte})
    return traits


# -- Especes de Tanares --------------------------------------------------------------------------

def _blocs_tanares(extracteur: Extracteur, pages, corps_titre: float) -> list[Bloc]:
    """Découpe une plage de pages du *Player's Guide* en blocs titrés.

    Le livre est **paginé en double page** — une page PDF en porte deux — et chaque page imprimée
    est elle-même sur deux colonnes. La coupe se fait donc en deux temps : la moitié, puis la
    gouttière mesurée de cette moitié. Sauter le second temps entrelace les colonnes au milieu des
    phrases, et « Your Wisdom score is increased sense of duty. Particularly skilled in » est ce
    qu'on obtient.
    """
    blocs: list[Bloc] = []
    vues = set()
    for imprimee in pages:
        index = extracteur.document.index_pdf(imprimee)
        if index in vues or not 0 <= index < extracteur.document.pages:
            continue
        vues.add(index)
        for moitie in ('gauche', 'droite'):
            for numero, region in enumerate(colonnes_de_page(extracteur, index, moitie)):
                segment = (index, moitie, numero)
                for ligne in extracteur.lignes(index, moitie, region):
                    titre = (ligne.fragments[0].police.startswith(POLICE_TITRE_PG)
                             and ligne.corps >= corps_titre)
                    if titre:
                        blocs.append(Bloc(titre=normaliser(ligne.texte), corps=ligne.corps))
                    elif blocs:
                        blocs[-1].lignes.append((segment, ligne))
    return blocs


def especes_tanares(corpus: Corpus, parents: dict) -> tuple[list[dict], list[str]]:
    """Les huit espèces propres à Tanares : quatre nouvelles et quatre sous-espèces elfiques.

    **Tanares ne porte aucune mécanique pour les huit espèces classiques** — nain, elfe, halfelin,
    humain, drakéide, gnome, orc, tieffelin. Ses chapitres leur donnent de l'histoire et des
    variantes optionnelles, jamais un bloc de traits. « La version de Tanares fait foi sur le
    fond » se lit donc comme le corpus le permet : le fond narratif vient de Tanares, la mécanique
    des livres français.

    Les huit retenues sont **déclarées** (``ESPECES_DE_TANARES``) et non devinées, pour la raison
    écrite là-bas : le chapitre compose au même corps des espèces, des sections et des variantes
    optionnelles, et toutes portent une augmentation de caractéristique.

    ``parents`` donne la taille et la vitesse dont une sous-espèce hérite — le livre ne les répète
    pas, et les laisser vides ferait refuser l'entrée par le schéma.
    """
    especes: list[dict] = []
    signalements: list[str] = []
    with Extracteur(corpus['players-guide']) as extracteur:
        blocs = _blocs_tanares(extracteur, PAGES_ESPECES_PG, CORPS_SECTION_PG)
    par_titre = {normaliser_cle(b.titre): b for b in blocs}

    for nom, titre, parent in ESPECES_DE_TANARES:
        bloc = par_titre.get(normaliser_cle(titre))
        if bloc is None:
            raise PersonnageError(
                "Player's Guide : le bloc « %s » est introuvable. Il est déclaré dans "
                'ESPECES_DE_TANARES et atteste au sommaire ; son absence signifie que la plage '
                'de pages ou la découpe a changé.' % titre)
        texte = bloc.texte()
        espece = {'id': identifiant(nom), 'name': nom, 'source': PROVENANCE_TANARES}
        if parent:
            espece['parentSpecies'] = parent
            herite = parents.get(parent)
            if herite is None:
                raise PersonnageError(
                    '« %s » dérive de « %s », qui n\'est pas au catalogue. Une sous-espèce sans '
                    'espèce parente n\'hérite ni taille ni vitesse, et le schéma les exige.'
                    % (nom, parent))
            espece['size'], espece['speed'] = herite

        fixes, au_choix = lire_augmentations_en(texte)
        if fixes:
            espece['abilityScoreIncrease'] = fixes
        taille = lire_taille_en(texte)
        if taille:
            espece['size'] = taille
        vitesse = lire_vitesse_en(texte)
        if vitesse is not None:
            espece['speed'] = vitesse
        traits = _traits_tanares(bloc)
        if traits:
            espece['traits'] = traits
        if au_choix:
            # Le livre laisse le JOUEUR choisir : << one other ability score of your choice
            # increases by 1 >>. Le schema porte une TABLE, qui ne sait pas dire << au choix >> ;
            # inventer une caracteristique la figerait, et toutes les especes de Tanares
            # augmenteraient la meme. Le mecanisme est declare, la phrase reste dans le texte.
            espece['mecanismesRequis'] = [MECANISME_CHOIX]
            signalements.append(
                '%s : le livre laisse une augmentation AU CHOIX du joueur, que la table du schéma '
                'ne sait pas porter. Le mécanisme « %s » est déclaré ; aucune caractéristique '
                'n\'est choisie à sa place.' % (nom, MECANISME_CHOIX))
        if 'size' not in espece or 'speed' not in espece:
            # Le soulborn est le cas d'espece, au sens propre : le livre ecrit << Your size is
            # equivalent to your birth parents >> et << Your base walking speed matches that of
            # your birth parents >>. Ce n'est pas une extraction incomplete, c'est la regle -- et
            # le schema du LOT-32 exige une taille et une vitesse, ce qu'il avait toutes les
            # raisons de faire. Lui en inventer une le rendrait jouable et faux ; l'ecarter en le
            # DISANT laisse la decision a un lot qui saura modeliser l'heritage.
            signalements.append(
                '%s : le livre fait hériter sa taille et sa vitesse des parents du personnage, et '
                "ne leur donne donc aucune valeur. Le schéma les exige : l'espèce est écartée du "
                'catalogue plutôt que dotée de valeurs inventées.' % nom)
            continue
        especes.append(espece)
    return especes, signalements


def lire_augmentations_en(texte: str) -> tuple[dict, bool]:
    """« Your Constitution score increases by 2 » → (table, « il reste un choix au joueur »).

    Deux moitiés dans presque toutes les espèces de Tanares : une augmentation **fixe**, qui entre
    dans la table, et une **au choix**, qui n'y entre pas. Renvoyer la première et signaler la
    seconde est la seule lecture qui ne mente pas.
    """
    trouve = re.search(r'Ability\s+Score\s+Increase\s*\.\s*(.{0,300})', texte, re.S)
    if not trouve:
        return {}, False
    phrase = trouve.group(1)
    valeurs = {}
    for nom, montant in re.findall(
            r'\b(Strength|Dexterity|Constitution|Intelligence|Wisdom|Charisma)\b'
            r'[^.]{0,40}?(?:increases?|increased)\s*(?:by\s*)?(\d+)', phrase, re.I):
        valeurs.setdefault(CARACTERISTIQUES_EN[nom.lower()], int(montant))
    # Deux formulations du choix, et la seconde ne dit pas << of your choice >>. Le cirrus ecrit
    # << increase one ability score by 2 and another ability score by 1 >> : aucune caracteristique
    # nommee, donc rien de fixe. Une augmentation dont on ne tire AUCUNE valeur est, par
    # construction, entierement laissee au joueur -- la traiter comme une absence ferait un cirrus
    # sans augmentation du tout.
    au_choix = bool(re.search(r'of\s+your\s+choice', phrase, re.I)) or not valeurs
    return valeurs, au_choix


def lire_taille_en(texte: str) -> str | None:
    """« Your size is Medium » → ``medium``."""
    trouve = re.search(r'[Yy]our\s+size\s+is\s+(\w+)', texte)
    if not trouve:
        return None
    return TAILLES_EN.get(trouve.group(1).lower())


def lire_vitesse_en(texte: str) -> float | None:
    """« Your base walking speed is 30 feet » → 9, en **mètres**.

    Le *Player's Guide* compte en pieds, les catalogues en mètres — les livres français, eux,
    écrivent déjà des mètres. La conversion est celle du corpus lui-même : 5 pieds valent 1,50 m,
    ce qui est aussi le côté d'une case (`core::METERS_PER_TILE`). Mélanger les deux unités dans
    un même catalogue donnerait des elfes trois fois plus rapides que des nains.
    """
    trouve = re.search(r'base\s+walking\s+speed\s+is\s+(\d+)\s*(?:feet|ft)', texte, re.I)
    if not trouve:
        return None
    metres = int(trouve.group(1)) / PIEDS_PAR_CASE * METRES_PAR_CASE
    return int(metres) if metres == int(metres) else round(metres, 2)


def _traits_tanares(bloc: Bloc) -> list[dict]:
    """Les traits d'un bloc de Tanares : un paragraphe ouvert par un fragment gras."""
    traits = []
    for groupe in bloc.paragraphes():
        premier = groupe[0].fragments[0]
        if not premier.gras:
            continue
        nom = ''
        for fragment in groupe[0].fragments:
            if not fragment.gras:
                break
            nom += fragment.texte
        nom = normaliser(nom).rstrip('.').strip()
        texte = normaliser(' '.join(l.texte for l in groupe))
        if not nom or normaliser_cle(nom) in ETIQUETTES_DE_PROFIL_EN:
            continue
        if normaliser_cle(texte).startswith(normaliser_cle(nom)):
            texte = normaliser(texte[len(nom):].lstrip(' .'))
        if texte:
            traits.append({'name': nom, 'text': texte})
    return traits


# -- Historiques ---------------------------------------------------------------------------------

def historiques_basic_rules(corpus: Corpus, tables: dict) -> tuple[list[dict], list[str]]:
    """Les six historiques des *Basic Rules* — acolyte, criminel, héros du peuple, noble, sage,
    soldat.

    Même grammaire que les races : un titre au corps 16, des paragraphes narratifs, puis les
    étiquettes en gras — « Compétences maîtrisées : », « Langues : », « Équipement : » — et la
    capacité, dont le titre est au corps 9,5.
    """
    with Extracteur(corpus['basic-rules']) as extracteur:
        # Coupe au corps du TITRE d'historique (16), pas a celui des sous-sections (9,5) : le
        # titre de la capacite -- << CAPACITE : ABRI DU FIDELE >> -- est compose a 9,5, et couper
        # dessus le sortirait du bloc de l'historique auquel il appartient.
        blocs = decouper_en_blocs(extracteur, PAGES_HISTORIQUES_BR, POLICE_TITRE_BR, CORPS_RACE)
    return _historiques_depuis(blocs, tables, PROVENANCE_SRD, ETIQUETTES_HISTORIQUE_FR)


def historiques_tanares(corpus: Corpus, tables: dict) -> tuple[list[dict], list[str]]:
    """Les sept historiques du *Player's Guide*, chapitre 3.

    **Le livre en annonce six et en porte sept.** Son paragraphe d'ouverture énumère
    « a cartographer, a community leader, a penumbral survivor, a dragon hunter, an occultist, or
    an undercover agent » et oublie l'*imperial servant*, que le sommaire de la page 4 liste
    pourtant p. 266. C'est le sommaire qui fait foi : il indexe ce que le livre contient, la prose
    d'ouverture décrit ce que l'auteur avait en tête.
    """
    with Extracteur(corpus['players-guide']) as extracteur:
        blocs = _blocs_tanares(extracteur, PAGES_HISTORIQUES_PG, CORPS_TITRE_PG)
    return _historiques_depuis(blocs, tables, PROVENANCE_TANARES, ETIQUETTES_HISTORIQUE_EN)


def valeur_etiquette(texte: str, motif: str, etiquettes: dict) -> str | None:
    """La valeur qui suit une étiquette, **bornée par l'étiquette suivante**.

    Sans cette borne, la capture court jusqu'au quota de caractères et avale la section d'après :
    le criminel ressortait avec la compétence « Tromperie Outils maîtrisés : un type de jeu », et
    l'acolyte avec une seule de ses deux compétences, la seconde étant noyée dans l'équipement.
    Les blocs n'ont aucune ponctuation qui sépare deux sections — seule l'étiquette suivante le
    fait.
    """
    debut = re.search(motif + r'\s*:?\s*', texte, re.I)
    if not debut:
        return None
    reste = texte[debut.end():]
    fins = [m.start() for autre in etiquettes.values()
            for m in re.finditer(autre + r'\s*:', reste, re.I)]
    return normaliser(reste[:min(fins)] if fins else reste[:200])


def _historiques_depuis(blocs, tables: dict, provenance: str,
                        etiquettes: dict) -> tuple[list[dict], list[str]]:
    """Assemble les historiques d'une liste de blocs. Renvoie (historiques, signalements)."""
    historiques: list[dict] = []
    signalements: list[str] = []
    for bloc in blocs:
        texte = bloc.texte()
        phrase_competences = valeur_etiquette(texte, etiquettes['competences'], etiquettes)
        if phrase_competences is None:
            continue
        nom = normaliser(bloc.titre)
        nom = nom[:1].upper() + nom[1:].lower() if nom.isupper() else nom
        historique = {'id': identifiant(nom), 'name': nom, 'source': provenance}

        competences, inconnues = _competences(phrase_competences, tables['competences'])
        if competences:
            historique['skillProficiencies'] = competences
        for mot in inconnues:
            signalements.append(
                '%s : la compétence « %s » ne figure pas au catalogue du LOT-43 ; elle est '
                'signalée, pas inventée.' % (nom, mot))

        phrase_langues = valeur_etiquette(texte, etiquettes['langues'], etiquettes)
        if phrase_langues:
            nombre = _compter_langues(phrase_langues)
            if nombre is not None:
                historique['languageCount'] = nombre

        capacite = _capacite(bloc, etiquettes)
        if capacite:
            historique['feature'] = capacite
        recit = _recit(bloc, etiquettes)
        if recit:
            historique['text'] = recit
        historiques.append(historique)
    return historiques, signalements


def _competences(phrase: str, catalogue: dict) -> tuple[list[str], list[str]]:
    """« Intuition, Religion » → les identifiants du catalogue, et ce qui n'y figure pas."""
    connues, inconnues = [], []
    for morceau in re.split(r',|\bet\b|\band\b', phrase):
        mot = normaliser(morceau).strip(" .;:")
        if not mot or len(mot) < 3:
            continue
        cle = normaliser_cle(mot)
        if cle in catalogue:
            if catalogue[cle] not in connues:
                connues.append(catalogue[cle])
        elif not re.search(r'\d|choix|choice|votre|your', cle):
            inconnues.append(mot)
    return connues, inconnues


def _compter_langues(phrase: str) -> int | None:
    """« deux de votre choix », « two of your choice » → 2.

    Le livre écrit le nombre **en toutes lettres**, jamais en chiffres. Chercher un chiffre ne
    trouve rien, et l'historique sort sans langue — ce qu'aucun contrôle ne signale, puisque le
    champ est facultatif.
    """
    bas = normaliser_cle(phrase)
    for mot, valeur in NOMBRES_EN_LETTRES.items():
        if re.search(r'\b%s\b' % mot, bas):
            return valeur
    trouve = re.search(r'\b(\d)\b', bas)
    return int(trouve.group(1)) if trouve else None


def _capacite(bloc: Bloc, etiquettes: dict) -> dict | None:
    """La capacité d'historique : son titre et son texte.

    Les deux livres l'annoncent différemment — les *Basic Rules* par un titre
    « CAPACITÉ : ABRI DU FIDÈLE », le *Player's Guide* par « Feature: … » — et le motif est donc
    déclaré par langue plutôt que deviné.

    Le titre peut **courir sur deux lignes**, et l'interligne des titres du *Player's Guide* est
    plus grand que le seuil de paragraphe : « Feature: Cherished » et « By The People » sont deux
    paragraphes pour ``paragraphes()``, et un seul titre pour le lecteur. C'est le **corps** qui
    les réunit — 14,5 contre 9,8 pour le texte —, jamais la ponctuation : il n'y en a aucune.
    """
    groupes = bloc.paragraphes()
    for position, groupe in enumerate(groupes):
        premiere = normaliser(groupe[0].texte)
        trouve = re.match(etiquettes['capacite'] + r'\s*:?\s*(.*)$', premiere, re.I)
        if not trouve:
            continue
        # Le corps du titre separe le nom du texte. La tolerance est SERREE : le corps rendu par
        # le PDF est un flottant, et les Basic Rules composent le titre a 9,5 et le texte a 9,0 --
        # un demi-point d'ecart. Une tolerance de 0,5 avalait tout l'historique dans le nom.
        corps_titre = groupe[0].corps
        morceaux = [normaliser(trouve.group(1))]
        reste = []
        for ligne in [l for g in groupes[position:] for l in g][1:]:
            if not reste and abs(ligne.corps - corps_titre) < TOLERANCE_CORPS:
                morceaux.append(normaliser(ligne.texte))
            elif ligne.corps >= corps_titre - TOLERANCE_CORPS:
                # Le titre SUIVANT -- << PERSONNALITES PROPOSEES >>, << VARIANTE : ESPION >> --
                # ferme la capacite. Sans cette borne, elle absorbe les tables de personnalite,
                # les quatre d6 et le pied de page de la fin de l'historique.
                break
            else:
                reste.append(ligne)
        nom = normaliser(' '.join(m for m in morceaux if m)).strip(' .:')
        texte = normaliser(' '.join(l.texte for l in reste))
        if nom and texte:
            return {'name': nom[:1].upper() + nom[1:].lower() if nom.isupper() else nom,
                    'text': texte}
    return None


def _recit(bloc: Bloc, etiquettes: dict) -> str:
    """Les paragraphes narratifs d'un historique : ceux qui précèdent la première étiquette."""
    morceaux = []
    for groupe in bloc.paragraphes():
        entier = normaliser(' '.join(l.texte for l in groupe))
        if any(re.match(motif, entier, re.I) for motif in etiquettes.values()):
            break
        if groupe[0].fragments[0].gras:
            break
        morceaux.append(entier)
    return ' '.join(morceaux)


# -- Classes simplifiees -------------------------------------------------------------------------

def classes_simplifiees(corpus: Corpus) -> tuple[list[dict], list[str]]:
    """Les quatre classes simplifiées du *Player's Guide* : brawler, mage, priest, scoundrel.

    Deux sources dans le même livre, et il en faut deux :

    - la **table récapitulative** de la page 57 donne dé de vie, caractéristique principale et
      maîtrises de sauvegarde — trois colonnes qu'aucun bloc de classe ne répète ;
    - la **table de progression** de chaque classe (p. 193, 197, 201, 205) donne les vingt niveaux,
      leur bonus de maîtrise et leurs capacités.

    Les deux sont des **tableaux**, lus par coordonnée et jamais en flux (`EX-CNT-021`) : chaque
    cellule y est une ligne à part entière, et l'ordonnée les rassemble en rangées.

    **Deux colonnes ne sont pas extraites**, et c'est délibéré. La *description* et les *maîtrises
    d'armes et d'armures* courent sur trois lignes chacune et s'entrelacent avec leurs voisines ;
    on en tire « A crafty rogue specialized in coordinat- » et « Light and medium armor, shields,
    Constitution simple and martial weapons » — une phrase coupée au milieu d'un mot, et un
    mélange de deux colonnes qu'aucun schéma ne refuserait. Les maîtrises relèvent de toute façon
    du `LOT-34`, qui livre les armes et les armures et pourra les rattacher à de vrais objets.
    """
    signalements: list[str] = []
    with Extracteur(corpus['players-guide']) as extracteur:
        recapitulatif = _table_des_classes(extracteur)
        progressions = _progressions(extracteur)

    classes: list[dict] = []
    for nom in CLASSES_SIMPLIFIEES:
        resume = recapitulatif.get(normaliser_cle(nom))
        if resume is None:
            raise PersonnageError(
                "Player's Guide p. %d : la classe « %s » manque à la table récapitulative « New "
                'Simplified Classes ». Sans elle, ni dé de vie ni jets de sauvegarde.'
                % (PAGE_TABLE_CLASSES, nom))
        progression = progressions.get(normaliser_cle(nom))
        if not progression:
            raise PersonnageError(
                "Player's Guide : la table de progression de « %s » est introuvable. Une classe "
                'sans progression ne monte pas de niveau, et rien ne le signalerait.' % nom)

        classe = {
            'id': identifiant(nom),
            'name': nom,
            'source': PROVENANCE_TANARES,
            'hitDie': resume['hitDie'],
            'primaryAbility': resume['primaryAbility'],
            'savingThrowProficiencies': resume['savingThrowProficiencies'],
            'progression': progression,
            # Provisoire, et qui le DIT (EX-CNT-032). Le critere de retrait est ecrit d'avance et
            # vit a un seul endroit : quatre copies d'un critere divergent, et celle qu'on lirait
            # ne serait pas celle qui vaut.
            'status': {'provisoire': True, 'raison': RAISON_CLASSES, 'retraitSi': RETRAIT_CLASSES},
        }
        if len(progression) != NIVEAUX_ATTENDUS:
            signalements.append(
                '%s : %d niveaux extraits, %d attendus. Une table amputée ne se voit qu\'au '
                'niveau où elle manque.' % (nom, len(progression), NIVEAUX_ATTENDUS))
        classes.append(classe)
    return classes, signalements


def _rangees(extracteur: Extracteur, index: int, moitie: str, region) -> list[list]:
    """Les rangées d'un tableau, groupées par **ordonnée** (`EX-CNT-021`).

    Ces tableaux n'ont ni filet ni séparateur : chaque cellule est une ligne à part entière, et
    seule sa position la rattache à sa rangée et à sa colonne. Les grouper par ordonnée et les
    ordonner par abscisse est exactement la lecture que ``tableau()`` fait des mots ; ici les
    cellules sont déjà découpées, et la projection en colonnes n'a plus rien à trancher.
    """
    rangees: dict = {}
    for ligne in extracteur.lignes(index, moitie, region):
        rangees.setdefault(round(ligne.y, 1), []).append(ligne)
    return [[l for l in sorted(cellules, key=lambda x: x.fragments[0].x0)]
            for _, cellules in sorted(rangees.items())]


def _table_des_classes(extracteur: Extracteur) -> dict:
    """La table « New Simplified Classes » de la page 57 : dé de vie, caractéristiques, maîtrises.

    Les cellules d'une même rangée débordent sur plusieurs lignes — la description tient sur trois
    lignes, les maîtrises sur deux — et la rangée d'une classe est donc tout ce qui suit son nom
    jusqu'au nom suivant. Le nom, lui, est l'un des quatre déclarés : le chercher plutôt que le
    déduire évite d'avoir à décider si « Daggers, darts, slings » ouvre une classe.
    """
    index = extracteur.document.index_pdf(PAGE_TABLE_CLASSES)
    lignes = [l for region in colonnes_de_page(extracteur, index, 'droite')
              for l in extracteur.lignes(index, 'droite', region)]
    noms = {normaliser_cle(n): n for n in CLASSES_SIMPLIFIEES}

    debuts = [(numero, noms[normaliser_cle(l.texte)])
              for numero, l in enumerate(lignes) if normaliser_cle(l.texte) in noms]
    table: dict = {}
    for position, (numero, nom) in enumerate(debuts):
        fin = debuts[position + 1][0] if position + 1 < len(debuts) else len(lignes)
        entier = normaliser(' '.join(l.texte for l in lignes[numero:fin]))
        table[normaliser_cle(nom)] = _lire_resume(nom, entier)
    return table


def _lire_resume(nom: str, texte: str) -> dict:
    """Une rangée de la table récapitulative → dé de vie, caractéristiques, maîtrises."""
    de = re.search(r'\bd(4|6|8|10|12)\b', texte)
    if not de:
        raise PersonnageError(
            '« %s » : dé de vie illisible dans la table récapitulative. Le dé de vie décide des '
            'points de vie à chaque niveau ; le deviner fausserait toute la classe.' % nom)
    caracteristiques = [CARACTERISTIQUES_EN[m.lower()] for m in re.findall(
        r'\b(Strength|Dexterity|Constitution|Intelligence|Wisdom|Charisma)\b', texte)]
    if len(caracteristiques) < 3:
        raise PersonnageError(
            '« %s » : %d caractéristique(s) dans la table récapitulative, au moins 3 attendues '
            '(une principale, deux de sauvegarde).' % (nom, len(caracteristiques)))
    # L'ordre des colonnes est celui du livre : caracteristique principale, puis les DEUX jets de
    # sauvegarde. Le schema exige exactement deux sauvegardes, ce qui rend l'erreur de colonne
    # visible : trois valeurs, ou une, seraient refusees.
    resume = {
        'hitDie': int(de.group(1)),
        'primaryAbility': [caracteristiques[0]],
        'savingThrowProficiencies': caracteristiques[1:3],
    }
    # Les maitrises d'armes et d'armures de la cinquieme colonne ne sont PAS extraites. Leurs
    # cellules courent sur trois lignes et s'entrelacent avec celles de la description, si bien
    # qu'on en tire << Light and medium armor, shields, Constitution simple and martial weapons >>
    # -- un melange de deux colonnes, qu'aucun schema ne refuserait. Elles relevent de toute facon
    # du LOT-34, qui livre les armes et les armures et pourra les rattacher a de vrais objets.
    return resume


def _progressions(extracteur: Extracteur) -> dict:
    """Les tables de progression des quatre classes : vingt niveaux, bonus de maîtrise, capacités.

    La table est reconnue à sa **première colonne** : vingt cellules « 1st », « 2nd », … « 20th »,
    dans l'ordre. Aucun autre tableau du livre n'a cette colonne, et un tableau qui l'aurait
    perdue — parce que la page a changé — se signale par un compte de niveaux qui n'est plus vingt.
    """
    progressions: dict = {}
    for nom, imprimee in ZIP_CLASSES:
        index = extracteur.document.index_pdf(imprimee)
        niveaux: list[dict] = []
        for moitie in ('gauche', 'droite'):
            for region in colonnes_de_page(extracteur, index, moitie):
                for rangee in _rangees(extracteur, index, moitie, region):
                    textes = [normaliser(l.texte) for l in rangee]
                    if len(textes) < 2:
                        continue
                    niveau = re.match(r'^(\d{1,2})(?:st|nd|rd|th)$', textes[0])
                    bonus = re.match(r'^\+(\d)$', textes[1])
                    if not niveau or not bonus:
                        continue
                    entree = {'level': int(niveau.group(1)),
                              'proficiencyBonus': int(bonus.group(1))}
                    # La table du scoundrel a une QUATRIEME colonne -- les des de l'attaque
                    # sournoise, << 1d8 >>, << 2d8 >>. Ce n'est pas une capacite, et l'ecrire
                    # comme telle donnerait une capacite nommee `1d8` que rien ne definirait.
                    capacites = [identifiant(t) for t in textes[2:]
                                 if len(t) > 2 and not re.fullmatch(r'[+-]?\d+d\d+', t)]
                    if capacites:
                        entree['features'] = capacites
                    niveaux.append(entree)
        niveaux.sort(key=lambda e: e['level'])
        progressions[normaliser_cle(nom)] = niveaux
    return progressions


# -- Production ----------------------------------------------------------------------------------

def tables_de_reference(racine, lexique: list) -> dict:
    """Les tables que l'extraction interroge : langues, compétences, noms de race, traits raciaux.

    Toutes sont construites **depuis ce qui est déjà livré** — le lexique du `LOT-30`, les
    compétences et les langues du `LOT-43`. Aucune n'est écrite ici : un nom de compétence recopié
    dans ce module divergerait de son catalogue le jour où l'un des deux change, et c'est le genre
    d'écart qui ne se voit qu'au moment où un historique cesse d'accorder une maîtrise.
    """
    competences = catalogue_francais(racine, 'skills')
    # Le Player's Guide est en ANGLAIS et le catalogue en francais : le lexique fait le pont.
    for entree in lexique:
        if normaliser_cle(entree.categorie) != 'competence':
            continue
        for variante in entree.francais.split('/'):
            cible = competences.get(normaliser_cle(variante).strip())
            if cible:
                competences.setdefault(normaliser_cle(entree.anglais), cible)

    noms_races = {}
    for entree in lexique:
        if not normaliser_cle(entree.categorie).startswith(('race', 'sous-race')):
            continue
        for variante in entree.francais.split('/'):
            propre = normaliser(variante).strip()
            noms_races.setdefault(normaliser_cle(propre), propre)

    return {
        'langues': index_avec_lexique(racine, 'languages', lexique),
        'competences': competences,
        'noms_races': noms_races,
        'traits_raciaux': index_lexique(lexique, 'capacité ('),
    }


def produire(corpus: Corpus, lexique: list, racine, cache=None) -> tuple[list, list[str]]:
    """Écrit les trois catalogues sous ``racine``. Renvoie (fichiers écrits, signalements).

    **Un fichier par entrée**, nommé par son identifiant : c'est la forme que
    `scripts/checks/check_rpg_data.py` valide, et celle qui rend un conflit de fusion lisible.

    L'ordre compte : les espèces françaises d'abord, parce que les sous-espèces de Tanares
    **héritent** de leur taille et de leur vitesse, que le *Player's Guide* ne répète pas.
    """
    tables = tables_de_reference(racine, lexique)
    signalements: list[str] = []

    especes = especes_basic_rules(corpus, tables, cache)
    du_manuel, dits = especes_manuel(corpus, tables)
    signalements.extend(dits)
    especes.extend(du_manuel)

    parents = {e['id']: (e['size'], e['speed']) for e in especes
               if 'size' in e and 'speed' in e}
    de_tanares, dits = especes_tanares(corpus, parents)
    signalements.extend(dits)
    especes.extend(de_tanares)

    historiques = []
    for produire_historiques in (historiques_basic_rules, historiques_tanares):
        lus, dits = produire_historiques(corpus, tables)
        signalements.extend(dits)
        historiques.extend(lus)

    classes, dits = classes_simplifiees(corpus)
    signalements.extend(dits)

    ecrits = []
    for dossier, entrees in (('species', especes), ('backgrounds', historiques),
                             ('classes', classes)):
        identifiants = [e['id'] for e in entrees]
        doublons = sorted({i for i in identifiants if identifiants.count(i) > 1})
        if doublons:
            raise PersonnageError(
                '%s : identifiant(s) en double — %s. Deux entrées écriraient le même fichier, et '
                'la seconde effacerait la première sans un mot.' % (dossier, ', '.join(doublons)))
        chemin_dossier = racine / dossier
        chemin_dossier.mkdir(parents=True, exist_ok=True)
        for entree in entrees:
            chemin = chemin_dossier / ('%s.json' % entree['id'])
            chemin.write_text(json.dumps(entree, ensure_ascii=False, indent=2) + '\n',
                              encoding='utf-8')
            ecrits.append(chemin)
    return ecrits, signalements
