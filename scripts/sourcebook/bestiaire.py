#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Le bestiaire de base : les 94 bêtes d'`Animaux.pdf` (LOT-33).

C'est le seul gisement du corpus qui ne demande aucun jugement humain — gabarit régulier,
français, texte natif — et c'est pour cela qu'il est seul dans ce lot. Les 82 blocs de Tanares
relèvent du `LOT-46` : mêmes gabarits, mais en anglais, donc une étape de traduction qui n'a rien
à faire ici.

**L'extraction se fait sur la typographie, pas sur des expressions régulières.** Un bloc de
statistiques n'a ni balise ni ponctuation qui sépare le nom d'un trait de sa description : seule
la graisse le fait — « **Vue aiguisée**. L'aigle a un avantage… ». Découper sur le premier point
donne « Attaque au corps à corps avec une arme : +4 au toucher, allonge 1,50 m » comme nom
d'action, et le mode texte perd en prime les espaces des titres en gras (`Vueaiguisée`,
`Sens dela toile`, `Tactiquedegroupe`). ``Extracteur.lignes()`` rend police, corps et graisse ;
tout ce module en découle :

| Élément du bloc | Ce qui le désigne |
|---|---|
| nom de créature | police `DnDMr.Eaves`, corps 14,5 — 94 occurrences, pas une de plus |
| en-tête `ACTIONS` | petites capitales `ScalySans`, corps 9 |
| étiquette de profil | premier fragment **gras** dont le texte est l'une des douze étiquettes |
| nom de trait ou d'action | premier fragment **gras** hors de ces douze |
| paragraphe d'ambiance | police `CenturySchoolbook` — une **autre** police que la règle |
| encadré « Variante » | police `Calibri` — cinq encadrés qui ne sont pas des créatures |
| fin de paragraphe | interligne : 11,2 pt dans un paragraphe, 15,2 pt et plus entre deux |

**Le sommaire est le point d'attestation.** La page 2 porte 94 entrées ; le corps du document
porte 94 titres. Les deux listes doivent coïncider, sinon la génération s'arrête : c'est le seul
contrôle qui détecte un bloc sauté par un découpage en colonnes trop étroit, panne qui ne laisse
aucune autre trace. Le sommaire donne en outre la **casse d'affichage** — « Aigle géant » — que
les petites capitales du titre ont perdue.

**La gouttière est mesurée, puis vérifiée à chaque exécution.** Les pages sont à deux colonnes ;
lire une page entière entrelace les deux et attribue une action à la créature voisine. Le blanc
central occupe `[287, 309]` sur les trente pages du document, ce que
``mise_en_page.verifier_gouttiere()`` recontrôle : le jour où une édition le déplace, la coupe
échoue au lieu de produire des blocs mélangés.

Quatre défauts du lexique mis au jour par les 94 noms
-----------------------------------------------------

Les identifiants sont **anglais** — c'est la convention des catalogues (`acrobatics`,
`dwarvish`) — et viennent du lexique du `LOT-30`, seule table d'autorité du projet. Les 94 noms
français s'y retrouvent, à quatre conditions :

- le champ français porte des **variantes séparées par `/`** — « Bec de hache / Autrache » : une
  comparaison sur la chaîne entière en rate deux ;
- « Zombi » y est écrit « Zombi Objets magiques D&D 5 » — un **titre de section happé** par
  l'extraction du glossaire ;
- « Tigre à dents de sabre » y est écrit « Tigre à dents de **sabe** » — une coquille ;
- l'entrée « Tigre » a son côté **anglais resté en français**, et l'identifiant en sortait
  « tigre » au milieu de quatre-vingt-treize identifiants anglais. Celui-là ne se détecte pas
  mécaniquement — « Quasit = Quasit » est identique des deux côtés et parfaitement juste — et
  c'est une relecture des 94 identifiants qui l'a trouvé.

Les trois derniers sont déclarés dans ``ALIAS`` plutôt que corrigés dans le lexique : le lexique
est généré, une correction à la main y serait effacée à la prochaine exécution. L'alias, lui,
porte sa raison et se relit.

Ce que le schéma ne peut pas dire, et qui n'est pas perdu pour autant
--------------------------------------------------------------------

Trois formulations du livre ne rentrent pas dans un champ typé. Aucune n'est jetée, aucune n'est
élargie en silence :

- **« résistance aux dégâts contondants… provenant d'attaques non magiques ».** Mettre
  `bludgeoning` dans `damageResistances` rendrait le diablotin résistant à une masse d'armes, ce
  que le livre ne dit pas. La clause qualifiée reste dans un trait, et la créature **déclare** le
  mécanisme `resistance-conditionnelle` (`EX-CNT-030`) : le moteur listera au chargement ce qu'il
  ne sait pas honorer plutôt que de le jouer de travers.
- **« comprend le commun mais ne peut pas le parler ».** `languages` porte les langues du
  catalogue citées ; la nuance reste dans un trait, avec le mécanisme
  `langue-comprise-non-parlee`.
- **« l'aérien »**, que l'aigle géant comprend, n'est **pas** au catalogue des seize langues du
  `LOT-43`, et la table des *Basic Rules* p. 38 ne le porte pas davantage. Le rapprocher du
  primordial serait un élargissement muet ; le module le **signale** et ne l'écrit pas.

Enfin, chaque bloc se termine souvent par un paragraphe d'ambiance — « Les araignées-loups
géantes chassent en terrain découvert… ». Il va dans `description`, champ ajouté au schéma par ce
lot : le premier remplissage d'une famille est le moment où le contrat rencontre la réalité, et
laisser tomber ce paragraphe obligerait à réextraire le document le jour où le bestiaire s'affiche.
"""
from __future__ import annotations

import json
import re
from dataclasses import dataclass, field

from .catalogues import catalogue_francais, identifiant, index_avec_lexique
from .corpus import Corpus
from .extraction import Extracteur
from .glossaire import normaliser, normaliser_cle
from .mise_en_page import colonnes, paragraphes, verifier_gouttiere

SORTIE_RPG = 'Source/Elements/Rpg'
DOSSIER = 'creatures'
DOCUMENT = 'animaux'
PROVENANCE = 'srd'

# Le sommaire, puis le corps. Les pages 0 et 1 sont la couverture et son verso : elles ne portent
# aucun bloc, et les inclure ferait passer le titre de couverture pour un nom de créature.
PAGE_SOMMAIRE = 2
PAGES_BLOCS = range(3, 32)

# Corps du titre d'un bloc, et sa police. Les deux ensemble : `DnDMr.Eaves` sert aussi aux
# en-têtes de section, à un corps plus petit.
POLICE_TITRE = 'DnDMr.Eaves'
CORPS_TITRE = 12.0

# Polices du bloc de statistiques. Tout ce qui n'en est pas — `Calibri` des encadrés « Variante »,
# `CenturySchoolbook` des numéros de page — est écarté avant toute analyse.
POLICES_BLOC = ('ScalySans', POLICE_TITRE)

# Le paragraphe d'ambiance qui clôt un bloc — « Pour piéger sa proie, une araignée géante tisse
# des toiles élaborées… » — est composé dans une AUTRE police que la règle, et c'est ce qui le
# distingue : rien dans sa ponctuation ne le sépare de l'action qui le précède. Le corps minimal
# écarte le pied de page, composé dans la même police à 7 et 8 points.
POLICE_AMBIANCE = 'CenturySchoolbook'
CORPS_AMBIANCE = 9.5

# En-tête de section, en petites capitales. Deux créatures — la grenouille et l'hippocampe — n'en
# ont aucune : elles n'ont pas d'attaque, ce qui est la donnée et non un manque.
SECTION_ACTIONS = 'ACTIONS'

# Titre de la section qui suit les 88 bêtes. Ce n'est pas une créature, mais il est composé dans la
# même police et au même corps qu'un nom de bloc.
SECTION_AUTRES = 'AUTRES CRÉATURES'

# Les douze étiquettes du profil, dans l'ordre du livre. Fermée : un fragment gras qui n'y figure
# pas est un nom de trait ou d'action. La liste est vérifiée à l'extraction — les sept premières
# sont attendues sur les 94 blocs.
ETIQUETTES = {
    "Classe d'armure": 'armorClass',
    'Points de vie': 'hitPoints',
    'Vitesse': 'speed',
    'Compétences': 'skills',
    'Jets de sauvegarde': 'saves',
    'Résistances aux dégâts': 'damageResistances',
    'Immunités aux dégâts': 'damageImmunities',
    'Vulnérabilités aux dégâts': 'damageVulnerabilities',
    'Immunités aux conditions': 'conditionImmunities',
    'Sens': 'senses',
    'Langues': 'languages',
    'Puissance': 'challengeRating',
}
ETIQUETTES_REQUISES = ("Classe d'armure", 'Points de vie', 'Vitesse', 'Sens', 'Langues',
                       'Puissance')

# En-têtes des six caractéristiques, dans l'ordre de la table. Chacun est un fragment gras isolé,
# et les six valeurs suivent sur la ligne d'en dessous, dans le même ordre d'abscisse.
ABREVIATIONS = ('FOR', 'DEX', 'CON', 'INT', 'SAG', 'CHA')
CARACTERISTIQUES = ('strength', 'dexterity', 'constitution', 'intelligence', 'wisdom', 'charisma')

# Abréviation de taille du livre → valeur du schéma. Le lexique porte les cinq autres sous la
# catégorie « taille », avec l'abréviation entre parenthèses ; il ne porte PAS « Moyenne (M) », la
# seule que l'extraction du glossaire ait manquée. Elle est déclarée ici plutôt que devinée : une
# taille absente ferait échouer 32 des 94 blocs sans dire pourquoi.
TAILLES = {
    'TP': 'tiny', 'P': 'small', 'M': 'medium', 'G': 'large', 'TG': 'huge', 'Gig': 'gargantuan',
}

# Modes de déplacement du livre → clés du schéma. `walk` n'a pas de mot : c'est la valeur nue qui
# ouvre la ligne, « Vitesse 9 m, escalade 9 m ». Une créature sans vitesse de marche porte `0 m`
# — l'épaulard, l'hippocampe, le requin géant — et le schéma exige `walk` pour cette raison :
# l'absence signalerait une extraction incomplète, pas une créature immobile.
DEPLACEMENTS = {
    'vol': 'fly', 'nage': 'swim', 'escalade': 'climb', 'creusement': 'burrow',
}

# Noms français des créatures que le lexique ne rend pas tels quels, et pourquoi. Corriger le
# lexique serait sans effet : il est GÉNÉRÉ, et la correction sauterait à la prochaine exécution.
ALIAS = {
    'zombi': ('Zombie', "le lexique écrit « Zombi Objets magiques D&D 5 » : un titre de section "
                        "happé par l'extraction du glossaire."),
    'tigre a dents de sabre': ('Saber-Toothed Tiger',
                               "le lexique écrit « Tigre à dents de sabe » — coquille."),
    # Ce troisième-là ne se détecte pas mécaniquement, et c'est ce qui le rend intéressant : le
    # lexique porte l'entrée « Tigre = Tigre », dont le côté ANGLAIS est resté en français.
    # L'identifiant en sortait « tigre » au milieu de quatre-vingt-treize identifiants anglais.
    # Aucune règle ne peut le trouver seule — « Quasit = Quasit » et « Pseudodragon =
    # Pseudodragon » sont identiques des deux côtés et parfaitement justes. Les 94 identifiants
    # ont donc été relus à l'œil ; c'est le seul de la liste qui ne tenait pas.
    'tigre': ('Tiger', "le lexique porte « Tigre = Tigre » : le côté anglais n'a pas été "
                       "traduit."),
}

# Mécanismes qu'une donnée déclare lorsque le champ typé est sciemment plus étroit que le livre
# (`EX-CNT-030`).
MECANISME_RESISTANCE = 'resistance-conditionnelle'
MECANISME_LANGUE = 'langue-comprise-non-parlee'

# Qualificatifs qui rendent une résistance conditionnelle. Le livre n'en emploie qu'un, mais le
# détecter par la présence d'un mot de plus que les types de dégâts serait fragile : la liste est
# explicite, et une clause qu'elle ne reconnaît pas arrête la génération.
QUALIFICATIFS = ('non magique', 'non magiques', 'magiques')


class BestiaireError(Exception):
    """Le bestiaire n'a pas pu être produit tel qu'il est déclaré."""


@dataclass
class Bloc:
    """Un bloc de statistiques : son titre en petites capitales et ses lignes, en ordre de lecture.

    Les lignes portent leur **segment** — page et colonne — parce qu'un bloc court d'une colonne à
    la suivante et que l'ordonnée y repart de zéro : comparer les ordonnées de deux segments pour
    en déduire un saut de paragraphe donnerait un écart de plusieurs centaines de points à chaque
    changement de colonne.
    """

    titre: str
    lignes: list = field(default_factory=list)


# -- Découpe du document -------------------------------------------------------------------------

def est_du_bloc(fragment) -> bool:
    """Vrai si le fragment appartient au bloc de statistiques ou à son texte d'ambiance.

    Tout le reste est écarté avant analyse : les cinq encadrés « Variante » du document, composés
    en `Calibri`, et le pied de page. Absorber un encadré ajouterait à la créature qui précède des
    traits que le livre donne comme facultatifs.
    """
    if fragment.police.startswith(POLICES_BLOC):
        return True
    return fragment.police.startswith(POLICE_AMBIANCE) and fragment.corps >= CORPS_AMBIANCE


def lire_sommaire(extracteur: Extracteur) -> list[str]:
    """Les 94 noms du sommaire, dans leur casse d'affichage.

    C'est la liste de référence : les titres du corps sont en petites capitales et ont perdu
    « Aigle géant » au profit de « AIGLE GÉANT ». Le rapprochement des deux listes est ce qui
    prouve qu'aucun bloc n'a été sauté.
    """
    noms = []
    for region in colonnes():
        for ligne in extracteur.lignes(PAGE_SOMMAIRE, region=region):
            trouve = re.match(r'^(.+?)\s*\.{4,}\s*\d+\s*$', normaliser(ligne.texte))
            if trouve and normaliser_cle(trouve.group(1)) != normaliser_cle(SECTION_AUTRES):
                noms.append(normaliser(trouve.group(1)))
    return noms


def decouper(extracteur: Extracteur) -> list[Bloc]:
    """Découpe le corps du document en blocs, un par titre de créature.

    Seules les lignes composées dans les polices du bloc sont retenues : les cinq encadrés
    « Variante » du document sont en `Calibri`, et les absorber ajouterait à la créature qui
    précède des traits que le livre présente comme facultatifs.
    """
    blocs: list[Bloc] = []
    for index in PAGES_BLOCS:
        for numero, region in enumerate(colonnes()):
            segment = (index, numero)
            for ligne in extracteur.lignes(index, region=region):
                if not any(est_du_bloc(f) for f in ligne.fragments):
                    continue
                titre = ligne.fragments[0].police.startswith(POLICE_TITRE) \
                    and ligne.corps >= CORPS_TITRE
                texte = normaliser(ligne.texte)
                if titre and normaliser_cle(texte) == normaliser_cle(SECTION_AUTRES):
                    continue
                if titre:
                    blocs.append(Bloc(titre=texte))
                elif blocs:
                    blocs[-1].lignes.append((segment, ligne))
    return blocs


# -- Lecture d'un bloc ---------------------------------------------------------------------------

def etiquette_de(ligne) -> str | None:
    """L'étiquette de profil qui ouvre la ligne, ou ``None`` si elle n'en porte pas."""
    premier = ligne.fragments[0]
    if not premier.gras:
        return None
    return normaliser(premier.texte) if normaliser(premier.texte) in ETIQUETTES else None


def valeur_apres(ligne, etiquette: str) -> str:
    """Le texte d'une ligne, son étiquette retirée."""
    return normaliser(normaliser(ligne.texte)[len(etiquette):])


def caracteristiques(lignes: list) -> dict:
    """Les six valeurs de caractéristique, lues sur la table `FOR DEX CON INT SAG CHA`.

    Les en-têtes et les valeurs sont douze lignes distinctes — chaque cellule a sa propre boîte —
    rendues en ordre de lecture : les six en-têtes, puis les six valeurs, dans le même ordre
    d'abscisse. Le contrôle de l'ordre des en-têtes est ce qui garantit que la deuxième moitié
    correspond bien à la première ; sans lui, une colonne déplacée intervertirait deux
    caractéristiques sans rien casser.
    """
    entetes = [i for i, l in enumerate(lignes) if normaliser(l.texte) in ABREVIATIONS]
    if len(entetes) != len(ABREVIATIONS):
        raise BestiaireError('table de caractéristiques introuvable (%d en-têtes sur 6).'
                             % len(entetes))
    debut = entetes[0]
    lues = [normaliser(l.texte) for l in lignes[debut:debut + 12]]
    if tuple(lues[:6]) != ABREVIATIONS:
        raise BestiaireError("en-têtes de caractéristiques dans l'ordre %s, %s attendu."
                             % (lues[:6], list(ABREVIATIONS)))
    valeurs = {}
    for nom, brut in zip(CARACTERISTIQUES, lues[6:12]):
        trouve = re.match(r'^(\d+)\s*\([+\-–−]?\d+\)$', brut)
        if not trouve:
            raise BestiaireError('valeur de caractéristique illisible : %r.' % brut)
        valeurs[nom] = int(trouve.group(1))
    return valeurs


def nombre(texte: str):
    """Un nombre du livre — « 1,50 » — en nombre. La virgule décimale est française.

    Un nombre entier ressort **entier**. `9.0` et `9` sont la même vitesse pour le schéma, et la
    seconde forme est celle qu'un relecteur compare au livre sans hésiter.
    """
    valeur = float(texte.replace(',', '.'))
    return int(valeur) if valeur == int(valeur) else valeur


MOTIF_PORTEE = re.compile(r'portée\s+([\d,]+)\s*(?:m\s*)?(?:/\s*([\d,]+)\s*)?m\b', re.IGNORECASE)


def portees(texte: str) -> dict:
    """Les portées d'une attaque à distance, en mètres : ``rangeNormal`` et ``rangeLong``.

    Le livre les écrit de quatre façons — « portée 24/96 m » dans un bloc de créature, « portée
    45 m/180 m » et « portée 1,50 m/ 4,50 m » dans la table des armes, « portée 18 m » pour une
    portée unique, qui donne deux nombres égaux (`LOT-22`). Le moteur ne lit jamais cette prose :
    c'est ici qu'elle devient des nombres.
    """
    trouve = MOTIF_PORTEE.search(texte)
    if trouve is None:
        return {}
    normale = nombre(trouve.group(1))
    longue = nombre(trouve.group(2)) if trouve.group(2) else normale
    return {'rangeNormal': normale, 'rangeLong': longue}


def des(texte: str) -> str:
    """Une expression de dés du livre en forme du schéma : « 4d10 + 4 » → ``4d10+4``.

    Le livre emploie trois tirets pour le signe négatif — `-`, `–`, `−` — parfois dans le même
    bloc. Les confondre est sans conséquence visible : le schéma refuse la forme, et c'est lui qui
    signale la faute plutôt qu'un jet de dés impossible en cours de partie.
    """
    texte = normaliser(texte).replace('–', '-').replace('−', '-').replace('—', '-')
    return re.sub(r'\s+', '', texte)


# -- Champs du profil ----------------------------------------------------------------------------

def lire_type(texte: str) -> tuple[str, str, str]:
    """« Bête de taille P, sans alignement » → (type, taille, alignement).

    Le type n'est pas normalisé en anglais : il est ouvert par le schéma, et le Sourcebook en
    introduira que le SRD ne connaît pas. Les quinze formes du document sont régulières —
    « Nuée de bêtes TP », « Fiélon (diable, métamorphe) » — et la seule partie contrainte est la
    taille.
    """
    trouve = re.match(r'^(.+?)\s+de\s+taille\s+(\w+)\s*,\s*(.+)$', normaliser(texte))
    if not trouve:
        raise BestiaireError('ligne de type illisible : %r.' % texte)
    type_, abreviation, alignement = trouve.groups()
    if abreviation not in TAILLES:
        raise BestiaireError('taille « %s » inconnue (attendu : %s).'
                             % (abreviation, ', '.join(TAILLES)))
    # Seule l'initiale passe en bas de casse : « Nuée de bêtes TP » garde son abréviation de
    # taille, qu'un `lower()` entier rendrait illisible.
    return type_[:1].lower() + type_[1:], TAILLES[abreviation], alignement.strip()


def lire_classe_armure(texte: str) -> int:
    trouve = re.match(r'^(\d+)', normaliser(texte))
    if not trouve:
        raise BestiaireError("classe d'armure illisible : %r." % texte)
    return int(trouve.group(1))


def lire_points_de_vie(texte: str) -> tuple[int, str | None]:
    """« 26 (4d10 + 4) » → (26, ``4d10+4``). La moyenne est la donnée, les dés la reproduisent."""
    trouve = re.match(r'^(\d+)\s*(?:\(([^)]+)\))?', normaliser(texte))
    if not trouve:
        raise BestiaireError('points de vie illisibles : %r.' % texte)
    return int(trouve.group(1)), des(trouve.group(2)) if trouve.group(2) else None


def lire_vitesse(texte: str) -> dict:
    """« 9 m, escalade 9 m » → ``{"walk": 9, "climb": 9}``, en mètres."""
    texte = normaliser(texte)
    trouve = re.match(r'^([\d,]+)\s*m', texte)
    if not trouve:
        raise BestiaireError('vitesse illisible : %r.' % texte)
    vitesses = {'walk': nombre(trouve.group(1))}
    for mot, distance in re.findall(r'(\w+)\s+([\d,]+)\s*m', texte):
        if mot in DEPLACEMENTS:
            vitesses[DEPLACEMENTS[mot]] = nombre(distance)
        elif mot not in ('m',):
            raise BestiaireError('mode de déplacement « %s » inconnu dans %r.' % (mot, texte))
    return vitesses


def lire_puissance(texte: str) -> float:
    """« 1/4 (50 PX) » → 0.25. Le facteur de puissance est fractionnaire sous 1."""
    trouve = re.match(r'^(\d+)(?:\s*/\s*(\d+))?', normaliser(texte))
    if not trouve:
        raise BestiaireError('facteur de puissance illisible : %r.' % texte)
    if trouve.group(2):
        return int(trouve.group(1)) / int(trouve.group(2))
    return int(trouve.group(1))


def lire_competences(texte: str, competences: dict) -> dict:
    """« Discrétion +7, Perception +3 » → ``{"stealth": 7, "perception": 3}``.

    Les clés sont les identifiants du catalogue du `LOT-43`, pas les noms français : une compétence
    citée par une créature sous un nom que le catalogue ignore arrête la génération, faute de quoi
    la fiche de personnage et le bloc de créature nommeraient différemment le même jet.
    """
    valeurs = {}
    for morceau in normaliser(texte).split(','):
        trouve = re.match(r'^(.+?)\s*([+\-–−]\s*\d+)$', morceau.strip())
        if not trouve:
            raise BestiaireError('compétence illisible : %r.' % morceau)
        cle = normaliser_cle(trouve.group(1))
        if cle not in competences:
            raise BestiaireError('compétence « %s » absente du catalogue du LOT-43.'
                                 % trouve.group(1).strip())
        valeurs[competences[cle]] = int(des(trouve.group(2)).replace('+', ''))
    return valeurs


def lire_types_degats(texte: str, types: dict) -> tuple[list[str], str | None]:
    """Une ligne de résistances ou d'immunités → (types du schéma, clause qualifiée restante).

    Le livre écrit « froid ; contondant, perforant et tranchant provenant d'attaques non
    magiques ». Le point-virgule sépare l'inconditionnel du conditionnel, et seule la première
    partie entre dans le champ typé : y verser la seconde rendrait le diablotin résistant à une
    masse d'armes ordinaire.
    """
    texte = normaliser(texte)
    parties = texte.split(';')
    conditionnelle = None
    if len(parties) > 1:
        conditionnelle = normaliser(';'.join(parties[1:]))
    elif any(q in texte for q in QUALIFICATIFS):
        return [], texte
    valeurs = []
    for morceau in re.split(r',| et ', parties[0]):
        if not normaliser_cle(morceau).strip(' .'):
            continue
        type_ = type_de_degats(morceau, types)
        if type_ is None:
            raise BestiaireError('type de dégâts « %s » absent du lexique.' % morceau.strip())
        valeurs.append(type_)
    return valeurs, conditionnelle


def lire_etats(texte: str, etats: dict) -> list[str]:
    """« à terre, agrippé, charmé » → les identifiants d'état du schéma, via le lexique."""
    valeurs = []
    for morceau in re.split(r',| et ', normaliser(texte)):
        mot = normaliser_cle(morceau).strip(" .'")
        if not mot:
            continue
        if mot not in etats:
            raise BestiaireError('état « %s » absent du lexique.' % morceau.strip())
        valeurs.append(etats[mot])
    return valeurs


def lire_langues(texte: str, langues: dict) -> tuple[list[str], str | None, list[str]]:
    """Une ligne « Langues » → (langues du catalogue, phrase à conserver, mots non reconnus).

    Trois formes dans le document : le tiret des quatre-vingt-cinq bêtes muettes, une liste
    (« infernal, commun ») et une phrase (« comprend le commun et l'aérien mais ne peut pas les
    parler »). La phrase est conservée telle quelle : la nuance *comprend mais ne parle pas* est
    une règle, et un dialogue refusé par le `LOT-15` ne doit pas l'être pour une langue que la
    créature comprenait.
    """
    texte = normaliser(texte)
    if texte in ('-', '–', '—', ''):
        return [], None, []
    liste = [m.strip(" .'’") for m in re.split(r',| et ', texte)]
    if all(normaliser_cle(m) in langues for m in liste if m):
        return [langues[normaliser_cle(m)] for m in liste if m], None, []

    # Une phrase : on y cherche les langues du catalogue, mot à mot. Les mots précédés d'un
    # article élidé — « l'aérien » — sont candidats au même titre.
    citees, inconnues = [], []
    for candidat in re.findall(r"\b(?:les?|du|l')\s*([\wÀ-ÿ-]+)", texte):
        cle = normaliser_cle(candidat)
        if cle in langues and langues[cle] not in citees:
            citees.append(langues[cle])
        elif cle in ('aerien', 'aquatique', 'igne', 'terreux'):
            inconnues.append(candidat)
    return citees, texte, inconnues


# -- Traits et actions ---------------------------------------------------------------------------

def decouper_nomme(groupe: list) -> tuple[str, str]:
    """Un paragraphe ouvert par un fragment gras → (nom, texte).

    Le nom est **exactement** l'étendue de la graisse, point final retiré. C'est la seule lecture
    juste : « Toile d'araignée (Recharge 5-6) » et « Nuage d'encre (Recharge après un repos court
    ou long) » sont des noms d'action entiers, et tout découpage à la ponctuation les tronque.
    """
    nom = ''
    for fragment in groupe[0].fragments:
        if not fragment.gras:
            break
        nom += fragment.texte
    nom = normaliser(nom).rstrip('.').strip()
    texte = normaliser(' '.join(l.texte for l in groupe))
    if normaliser_cle(texte).startswith(normaliser_cle(nom)):
        texte = normaliser(texte[len(nom):].lstrip(' .'))
    return nom, texte


def lire_action(nom: str, texte: str, types: dict) -> dict:
    """Une action, avec ce que le moteur peut en jouer : bonus, allonge, dégâts.

    Seule la **première** clause de dégâts est typée. Une morsure qui inflige « 7 (1d10 + 2)
    dégâts perforants + 5 (1d10) dégâts de poison » en a deux, et le champ `damage` n'en porte
    qu'une ; le texte intégral reste, et c'est lui qui fait foi jusqu'à ce que le `LOT-21` sache
    composer plusieurs clauses.
    """
    action = {'name': nom, 'text': texte}
    if (trouve := re.search(r'([+\-–−]\s*\d+)\s+au\s+toucher', texte)):
        action['attackBonus'] = int(des(trouve.group(1)).replace('+', ''))
    if (trouve := re.search(r'allonge\s+([\d,]+)\s*m', texte)):
        action['reach'] = nombre(trouve.group(1))
    action.update(portees(texte))
    trouve =re.search(r'(?:Touché|Touche)\s*:\s*(?:(\d+)\s*\(([^)]+)\)|(\d+))\s*dégâts?\s+'
                       r'((?:de\s+|d\')?[\wÀ-ÿ]+)', texte)
    if trouve:
        action['damage'] = des(trouve.group(2)) if trouve.group(2) else trouve.group(3)
        if (type_ := type_de_degats(trouve.group(4), types)):
            action['damageType'] = type_
    return action


# -- Lexique -------------------------------------------------------------------------------------

def type_de_degats(mot: str, types: dict) -> str | None:
    """« dégâts perforants », « dégâts de poison » → l'entrée du lexique, qui est au SINGULIER.

    Le livre accorde l'adjectif au pluriel de « dégâts » et introduit les noms par un article —
    « de poison », « d'acide ». Le lexique, lui, porte « perforant » et « poison ». Comparer sans
    réduire échoue sur les treize types à la fois, ce qui ne se voit pas : le champ `damageType`
    est facultatif, et son absence passe la validation.
    """
    cle = normaliser_cle(mot)
    cle = re.sub(r"^(?:de\s+|d')", '', cle).strip(" .'")
    return types.get(cle) or types.get(cle.rstrip('s'))


def index_lexique(lexique: list, categorie: str) -> dict:
    """Les entrées d'une catégorie du lexique, indexées par chacune de leurs variantes françaises.

    Un champ français porte plusieurs graphies séparées par `/` — « incapable d'agir / neutralisé »
    — et comparer la chaîne entière en rate systématiquement la seconde.
    """
    index = {}
    for entree in lexique:
        if normaliser_cle(entree.categorie) != normaliser_cle(categorie):
            continue
        for variante in entree.francais.split('/'):
            variante = re.sub(r'\([^)]*\)', '', variante)
            index.setdefault(normaliser_cle(variante).strip(), normaliser_cle(entree.anglais))
    return index


def index_noms(lexique: list) -> dict:
    """Tout le lexique, français → anglais, variantes comprises. Sert aux noms de créature."""
    index = {}
    for entree in lexique:
        for variante in entree.francais.split('/'):
            index.setdefault(normaliser_cle(variante).strip(), normaliser(entree.anglais))
    return index


def analyser(bloc: Bloc, nom: str, tables: dict) -> tuple[dict, list[str]]:
    """Un bloc du document en une entrée de catalogue. Renvoie (créature, signalements)."""
    groupes = paragraphes(bloc.lignes)
    if not groupes:
        raise BestiaireError('bloc vide.')

    type_, taille, alignement = lire_type(groupes[0][0].texte)
    creature = {
        'id': tables['identifiants'][normaliser_cle(nom)],
        'name': nom,
        'source': PROVENANCE,
        'size': taille,
        'creatureType': type_,
        'alignment': alignement,
    }
    # Trois etiquettes du profil produisent un trait, faute d'un champ type assez large. Elles
    # sont rassemblees ici pour garder l'ordre du livre -- jets de sauvegarde, resistances,
    # langues -- puis placees en tete des traits propres a la creature.
    du_profil: list[dict] = []
    traits: list[dict] = []
    actions: list[dict] = []
    description: list[str] = []
    mecanismes: list[str] = []
    signalements: list[str] = []
    profil: dict[str, str] = {}
    dans_actions = False
    apres_profil = False

    for groupe in groupes[1:]:
        texte_groupe = normaliser(' '.join(l.texte for l in groupe))
        if texte_groupe == SECTION_ACTIONS:
            dans_actions = True
            apres_profil = True
            continue
        if normaliser(groupe[0].texte) in ABREVIATIONS:
            creature['abilities'] = caracteristiques(groupe)
            continue
        if etiquette_de(groupe[0]) and not apres_profil:
            # Les six à douze lignes du profil se suivent sans blanc : elles forment UN paragraphe,
            # et chacune y porte son étiquette. Une ligne sans étiquette y est la suite de la
            # précédente — « Sens vision aveugle 3 m, vision dans le noir 18 m, / Perception
            # passive 10 » —, que rattacher au champ courant plutôt qu'écarter.
            courante = None
            for ligne in groupe:
                etiquette = etiquette_de(ligne)
                if etiquette:
                    courante = etiquette
                    profil[etiquette] = valeur_apres(ligne, etiquette)
                elif courante:
                    profil[courante] = normaliser('%s %s' % (profil[courante],
                                                             normaliser(ligne.texte)))
                else:
                    raise BestiaireError('ligne de profil sans étiquette : %r.' % ligne.texte)
            continue
        if groupe[0].fragments[0].gras:
            apres_profil = True
            nom_element, texte_element = decouper_nomme(groupe)
            if dans_actions:
                actions.append(lire_action(nom_element, texte_element, tables['degats']))
            else:
                traits.append({'name': nom_element, 'text': texte_element})
            continue
        if groupe[0].fragments[0].police.startswith(POLICE_AMBIANCE):
            description.append(texte_groupe)
            continue
        raise BestiaireError("paragraphe sans titre ni police d'ambiance : %r."
                             % texte_groupe[:60])

    manquantes = [e for e in ETIQUETTES_REQUISES if e not in profil]
    if manquantes:
        raise BestiaireError('étiquette(s) absente(s) du profil : %s.' % ', '.join(manquantes))
    if 'abilities' not in creature:
        raise BestiaireError('table de caractéristiques absente.')

    creature['armorClass'] = lire_classe_armure(profil["Classe d'armure"])
    points, hitDice = lire_points_de_vie(profil['Points de vie'])
    creature['hitPoints'] = points
    if hitDice:
        creature['hitDice'] = hitDice
    creature['speed'] = lire_vitesse(profil['Vitesse'])
    creature['challengeRating'] = lire_puissance(profil['Puissance'])
    creature['senses'] = [normaliser(m) for m in profil['Sens'].split(',') if normaliser(m)]

    if 'Compétences' in profil:
        creature['skills'] = lire_competences(profil['Compétences'], tables['competences'])
    for etiquette, champ in (('Résistances aux dégâts', 'damageResistances'),
                             ('Immunités aux dégâts', 'damageImmunities'),
                             ('Vulnérabilités aux dégâts', 'damageVulnerabilities')):
        if etiquette not in profil:
            continue
        valeurs, conditionnelle = lire_types_degats(profil[etiquette], tables['degats'])
        if valeurs:
            creature[champ] = valeurs
        if conditionnelle:
            du_profil.append({'name': etiquette, 'text': conditionnelle})
            if MECANISME_RESISTANCE not in mecanismes:
                mecanismes.append(MECANISME_RESISTANCE)
    if 'Immunités aux conditions' in profil:
        creature['conditionImmunities'] = lire_etats(profil['Immunités aux conditions'],
                                                     tables['etats'])
    if 'Jets de sauvegarde' in profil:
        du_profil.insert(0, {'name': 'Jets de sauvegarde',
                             'text': profil['Jets de sauvegarde']})

    citees, phrase, inconnues = lire_langues(profil['Langues'], tables['langues'])
    if citees:
        creature['languages'] = citees
    if phrase:
        du_profil.append({'name': 'Langues', 'text': phrase})
        mecanismes.append(MECANISME_LANGUE)
    for mot in inconnues:
        signalements.append(
            '%s : « %s » n\'est pas au catalogue des seize langues et ne figure pas non plus à la '
            'table des Basic Rules p. 38. La phrase est conservée ; la langue n\'est pas inventée.'
            % (nom, mot))

    traits = du_profil + traits
    if traits:
        creature['traits'] = traits
    if actions:
        creature['actions'] = actions
    if description:
        creature['description'] = ' '.join(description)
    if mecanismes:
        creature['mecanismesRequis'] = sorted(mecanismes)
    return creature, signalements


def produire(corpus: Corpus, lexique: list, racine, cache=None) -> tuple[list, list[str]]:
    """Écrit les 94 créatures sous ``racine``. Renvoie (fichiers écrits, signalements).

    **Un fichier par créature**, nommé par son identifiant : c'est la forme que
    `scripts/check_rpg_data.py` valide, et celle qui rend un conflit de fusion lisible.
    """
    tables = {
        'degats': index_lexique(lexique, 'type de dégâts'),
        'etats': index_lexique(lexique, 'état'),
        'noms': index_noms(lexique),
        'competences': catalogue_francais(racine, 'skills'),
        'langues': index_avec_lexique(racine, 'languages', lexique),
    }

    with Extracteur(corpus[DOCUMENT], cache=cache) as extracteur:
        verifier_gouttiere(extracteur, PAGES_BLOCS)
        attendus = lire_sommaire(extracteur)
        blocs = decouper(extracteur)

        if len(attendus) != len(blocs):
            raise BestiaireError(
                'le sommaire annonce %d créatures, le corps en porte %d. Un bloc sauté ne laisse '
                'aucune autre trace que cet écart.' % (len(attendus), len(blocs)))
        for nom, bloc in zip(attendus, blocs):
            if normaliser_cle(nom) != normaliser_cle(bloc.titre):
                raise BestiaireError(
                    'le sommaire et le corps divergent : « %s » contre « %s ». Les deux listes '
                    'sont dans le même ordre alphabétique ; un décalage les désaligne toutes.'
                    % (nom, bloc.titre))

        tables['identifiants'] = {}
        for nom in attendus:
            cle = normaliser_cle(nom)
            if cle in ALIAS:
                anglais = ALIAS[cle][0]
            elif cle in tables['noms']:
                anglais = tables['noms'][cle]
            else:
                raise BestiaireError(
                    '« %s » n\'a pas de nom anglais au lexique. Les identifiants des catalogues '
                    'sont anglais ; en forger un ici en ferait une convention locale, invisible '
                    'et divergente.' % nom)
            tables['identifiants'][cle] = identifiant(anglais)

        signalements: list[str] = []
        creatures = []
        for nom, bloc in zip(attendus, blocs):
            try:
                creature, dits = analyser(bloc, nom, tables)
            except BestiaireError as erreur:
                raise BestiaireError('%s : %s' % (nom, erreur)) from erreur
            creatures.append(creature)
            signalements.extend(dits)

    identifiants = [c['id'] for c in creatures]
    doublons = sorted({i for i in identifiants if identifiants.count(i) > 1})
    if doublons:
        raise BestiaireError(
            'identifiant(s) en double : %s. Deux créatures écriraient le même fichier, et la '
            'seconde effacerait la première sans un mot.' % ', '.join(doublons))

    dossier = racine / DOSSIER
    dossier.mkdir(parents=True, exist_ok=True)
    ecrits = []
    for creature in creatures:
        chemin = dossier / ('%s.json' % creature['id'])
        chemin.write_text(json.dumps(creature, ensure_ascii=False, indent=2) + '\n',
                          encoding='utf-8')
        ecrits.append(chemin)
    return ecrits, signalements
