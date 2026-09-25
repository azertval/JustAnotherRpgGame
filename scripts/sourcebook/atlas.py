#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Atlas des régions de Tanares (LOT-37).

Le chapitre 5 du `Tanares_Sourcebook.pdf` vers `Source/Elements/World/` : les **régions**, leur
gouvernement, leur faction, leur population par espèce, leurs **sept statistiques régionales**, et
les **lieux** de leurs « Places of Interest ».

Ce que ce module refuse de faire
--------------------------------

Il ne lit pas ce chapitre comme du texte. Une page du livre porte **deux pages imprimées** côte à
côte, chacune en **deux colonnes** : un mode en flux y recolle la fin d'une ligne de la colonne 1
avec le début de la colonne 2, et le résultat se lit comme une phrase. C'est la panne la plus
coûteuse du corpus, parce qu'elle produit du texte *plausible*. Tout ce module lit donc par
**coordonnée** — les lignes par ordonnée, les colonnes par projection horizontale (`EX-CNT-021`).

L'encart statistique est délimité par son cadre, pas à l'estime
---------------------------------------------------------------

Les sept statistiques vivent dans un encart dont la colonne de droite (les valeurs) s'aligne, à
l'ordonnée près, sur le corps de texte de la colonne voisine. Découper à une abscisse choisie à la
main donne « Citizen Freedom Very Low allows for ample military funding » : la valeur est juste, et
la phrase qui la suit vient d'un autre paragraphe. Rien ne le signale.

L'encart est donc trouvé par son **rectangle plein**, celui que le PDF dessine derrière lui : le
seul rempli, haut de plus de 50 points, qui encadre horizontalement le titre « Regional
Statistics » et commence à son ordonnée. Le clip est alors exact, et le bloc d'introduction
(`Government`, `Faction`, `Population`) se lit dans la même colonne, au-dessus du titre.

Une statistique est une **liste**, jamais une chaîne
-----------------------------------------------------

Six régions sur treize donnent une valeur uniforme — `High` — mais le Benênet impérial écrit
« Low (south), High (north) » et les Domaines straviens « Low underground and High on the surface ».
Aplatir ces deux-là sur une seule valeur invente une donnée ; les laisser en texte libre rend les
sept axes inexploitables, ce que l'acceptation du lot interdit explicitement.

Chaque axe est donc une **liste d'appréciations**, chacune avec sa note fermée et sa portée
optionnelle. Une région uniforme en porte une seule, sans portée. Le moteur qui ne modélise pas les
portées lit la première ; celui qui les modélise les a. Aucune valeur n'est inventée, aucune n'est
perdue.

Ce que le module ne tire pas du texte : le voisinage
-----------------------------------------------------

« Aucune région inatteignable » suppose un **graphe**, et le livre n'en écrit aucun : l'adjacence
n'existe que sur sa carte du monde. Elle est donc **déclarée** ici, dans `VOISINS`, relevée sur
cette carte — la seule donnée de ce lot qui ne vienne pas d'une extraction, et c'est pour cela
qu'elle est isolée dans une table nommée plutôt que noyée dans le flot.
"""
from __future__ import annotations

import json
import re
import unicodedata

from .corpus import Corpus
from .extraction import Extracteur, ExtractionError

SORTIE_MONDE = 'Source/Elements/World'

DOCUMENT = 'tanares-sourcebook'

# Le chapitre 5, « The World of Tanares », en pages PDF. La borne haute est exclusive et large :
# le module s'arrete sur la derniere region trouvee, la borne n'est qu'un garde-fou de lecture.
CHAPITRE = range(43, 104)

# -- Vocabulaire ferme ---------------------------------------------------------------------------
# Les cinq notes de l'encart, du livre vers le schema. « Normal » est bien le degre median du
# livre -- pas « Average », que la feuille de route citait de memoire.
NOTES = {
    'Very Low': 'veryLow',
    'Low': 'low',
    'Normal': 'normal',
    'High': 'high',
    'Very High': 'veryHigh',
}

# Les sept axes, dans l'ordre ou le livre les imprime, et leur nom de champ.
AXES = {
    'Citizen Freedom': 'citizenFreedom',
    'Crime and Violence': 'crimeAndViolence',
    'Economic Prosperity': 'economicProsperity',
    'Government Corruption': 'governmentCorruption',
    'Magic Access': 'magicAccess',
    'Monster Presence': 'monsterPresence',
    'Political Stability': 'politicalStability',
}

# Les trois etiquettes en gras du bloc d'introduction, et leur champ.
ETIQUETTES = {'Government': 'government', 'Faction': 'faction', 'Population': 'population'}

# Les intertitres de section, en corps 18. Les trois premiers sont presents dans les treize
# regions ; tout autre titre de ce corps ouvre une section libre, conservee sous son nom.
SECTIONS = ('Secrets', 'Threats and Conflicts', 'Places of Interest')

CORPS_SECTION = 17.0
CORPS_SOUS_TITRE = 11.5

# Tout le titrage en prose du chapitre est dans cette police. Le plan de la capitale de la
# Republique porte, lui, treize legendes numerotees en GothicUltraOT : ce sont les reperes d'une
# IMAGE, pas des intertitres, et ils nomment des lieux que la prose decrit deja quelques lignes
# plus bas. Les lire comme des intertitres cree treize doublons sans description.
POLICE_PROSE = 'Heuristica'

# -- Deux irregularites de la source, nommees plutot que contournees -----------------------------
# 1. La police de titrage du livre mappe le « e » circonflexe sur l'apostrophe typographique. Le
#    remplacer partout serait faux : la meme apostrophe est authentique dans « Fisherman's Wharf »
#    et « Taii'Maku ». Un seul nom du chapitre en souffre, et il est repare nommement.
NOMS_REPARES = {'Imperial Ben’net': 'Imperial Benênet'}

# 2. « Places of Interest » se termine, dans dix regions sur treize, par une galerie de PERSONNAGES
#    -- souverains, capitaines, grands pretres. Le livre ne l'annonce par aucun intertitre : ces
#    noms portent exactement la meme graisse, le meme corps et la meme police que les lieux qui les
#    precedent. Aucune regle typographique ne les separe, et il n'y a rien a deviner ici : la
#    premiere entree de chaque galerie est DECLAREE, et le module s'arrete dessus.
#
#    Les trois regions absentes de cette table n'ont pas de galerie ; leur section se ferme sur
#    l'intertitre suivant, comme la typographie le prevoit.
GALERIES = {
    'republic-of-freelands': 'Prime Minister Tellatius',
    'imperial-benenet': 'Governor Bonas Weyrdo',
    'kingdom-of-kolbjorn': 'Yviah, the Frost Giant Queen',
    'seashores': 'Captain Dorro',
    'sindile-forest': 'Niary',
    'storm-islands': 'Solnertha, the master of necromancy',
    'taii-maku-city-states': 'Shield, the Steel Golem',
    'theocracy-of-kepesh': 'Kalistessenâmun, the Lich Pharaoh',
    'tsvetan': 'The druid class receives a new subclass in this section.',
    'yama': 'Kitsune',
}

# Nombre de lieux attendu par region, releve a la main sur le livre. Un controle de cardinal est
# le seul garde-fou contre une derive silencieuse de l'extraction : une section qui se ferme trop
# tot ne produit pas d'erreur, elle produit moins de lieux.
LIEUX_ATTENDUS = {
    'central-empire': 25,  # 12 « Places of Interest », la Capitale et ses 12 quartiers (LOT-92)
    'republic-of-freelands': 15,
    'imperial-benenet': 7,
    'kingdom-of-kolbjorn': 6,
    'magocracy-of-mage-tower': 6,
    'seashores': 6,
    'sindile-forest': 7,
    'stravian-domains': 6,
    'storm-islands': 6,
    'taii-maku-city-states': 4,
    'theocracy-of-kepesh': 6,
    'tsvetan': 6,
    'yama': 7,
}

# Le livre ecrit ses parts de population au pluriel, et pas toujours le meme pluriel : « soulborn »
# ici, « soulborns » la ; « elves », « other elves », « summer elves ». Sans table, deux graphies
# d'un meme peuple donnent deux especes, et une somme de pourcentages qui ne se rapproche d'aucune
# realite. Le singulier retenu est celui du livre lui-meme, au singulier.
#
# Ces cles restent en ANGLAIS, et ne pointent PAS vers le catalogue d'especes du LOT-36, qui est en
# francais et n'en couvre que la moitie (ni geant, ni gobelinoide, ni kemet, ni merfolk, ni orc, ni
# soulborn n'y existent). Fabriquer ici une correspondance a moitie vide serait pire que pas de
# correspondance du tout : c'est la plomberie de cles du `LOT-39` qui la portera, en entier.
ESPECES = {
    'humans': 'human', 'gnomes': 'gnome', 'dwarves': 'dwarf', 'halflings': 'halfling',
    'elves': 'elf', 'summer elves': 'summer-elf', 'autumn elves': 'autumn-elf',
    'winter elves': 'winter-elf', 'other elves': 'other-elf', 'tieflings': 'tiefling',
    'soulborns': 'soulborn', 'soulborn': 'soulborn', 'cirrus': 'cirrus', 'giants': 'giant',
    'gloomfolks': 'gloomfolk', 'goblinoids': 'goblinoid', 'kemets': 'kemet',
    'merfolk': 'merfolk', 'orcs': 'orc', 'taii’maku': 'taii-maku',
}

# « others » n'est pas un peuple : c'est le reste de la repartition. Le ranger parmi les especes
# donnerait une espece « others » presente dans les treize regions, et le premier code qui trie les
# peuples par frequence la mettrait en tete.
RESTE = 'others'

# Largeur ajoutee au cadre de l'encart pour retrouver la colonne de texte qui le porte : le cadre
# s'arrete avant la marge de la colonne, et sans cette marge le dernier mot de chaque ligne du
# bloc d'introduction tombe -- « ... of the Tanarean Empire, [the] ».
MARGE_COLONNE = 14.0

# En dessous de cette largeur, une colonne detectee n'est pas une colonne de texte mais le numero
# de page ou une legende isolee.
LARGEUR_COLONNE_MINIMALE = 60.0

# -- La Capitale (LOT-92) ------------------------------------------------------------------------
# La capitale imperiale n'est pas sous « Places of Interest » : le livre la decrit dans un ENCART
# d'une page PDF entiere, au milieu de la region Central Empire, titre en corps 18 (« The Capital
# City, / Tanarean Empire's Capital »), puis « Districts of the Capital » et douze quartiers
# numerotes, chacun ouvert par un fragment gras italique « N- Nom. ». Rien dans la typographie ne
# distingue l'encart d'une ouverture de region, sinon l'absence d'encart statistique : il est donc
# DECLARE ici, page et region, et lu par sa propre fonction. Sans lui, Martpart et Arenarea -- les
# deux quartiers du vertical slice -- n'existent pas dans l'atlas.
CAPITALE = {'page': 49, 'region': 'central-empire', 'nom': 'The Capital City'}
QUARTIERS_ATTENDUS = 12
INTERTITRE_QUARTIERS = 'Districts of the Capital'
QUARTIER_RE = re.compile(r'^(\d+)-\s*(.+?)\.\s*$')
# Deux lignes que le rendu de page mele a la prose : le numero de page imprime (« 98 ») et le
# filigrane de la commande (« Valentin Eloy (Order #...) »). Dans l'encart de la Capitale, sans
# table, toute ligne de chiffres seuls est un numero de page ; sous « Places of Interest », non --
# une table d6 aligne « 3 », « 4 », « 5 » -- et seul le numero colle au filigrane en est un.
HORS_PROSE_RE = re.compile(r'^\d+(\s+\d+)?$|\(Order #\d+\)')
FILIGRANE_RE = re.compile(r'\(Order #\d+\)')
NUMERO_DE_PAGE_RE = re.compile(r'^\d+(\s+\d+)?$')


def sans_filigrane(lignes: list[str]) -> list[str]:
    """Les lignes d'un lieu, sans le filigrane ni le numero de page seul qui le jouxte."""
    marques = {i for i, ligne in enumerate(lignes) if FILIGRANE_RE.search(ligne)}
    marques |= {j for i in marques for j in (i - 1, i + 1)
                if 0 <= j < len(lignes) and NUMERO_DE_PAGE_RE.match(lignes[j].strip())}
    return [ligne for i, ligne in enumerate(lignes) if i not in marques]


# -- Voisinage -----------------------------------------------------------------------------------
# Releve sur la carte du monde (Tanares_Sourcebook.pdf, chapitre 5). Symetrique par construction :
# le controle de coherence le verifie plutot que de le supposer.
VOISINS = {
    'central-empire': ['stravian-domains', 'imperial-benenet', 'sindile-forest',
                       'theocracy-of-kepesh', 'seashores', 'tsvetan'],
    'republic-of-freelands': ['stravian-domains', 'seashores', 'tsvetan', 'taii-maku-city-states'],
    'imperial-benenet': ['central-empire', 'kingdom-of-kolbjorn', 'magocracy-of-mage-tower'],
    'kingdom-of-kolbjorn': ['imperial-benenet', 'storm-islands', 'magocracy-of-mage-tower'],
    'magocracy-of-mage-tower': ['imperial-benenet', 'kingdom-of-kolbjorn', 'stravian-domains'],
    'seashores': ['central-empire', 'republic-of-freelands', 'theocracy-of-kepesh',
                  'storm-islands', 'yama'],
    'sindile-forest': ['central-empire', 'theocracy-of-kepesh'],
    'stravian-domains': ['central-empire', 'republic-of-freelands', 'magocracy-of-mage-tower'],
    'storm-islands': ['kingdom-of-kolbjorn', 'seashores'],
    'taii-maku-city-states': ['republic-of-freelands', 'tsvetan'],
    'theocracy-of-kepesh': ['central-empire', 'sindile-forest', 'seashores'],
    'tsvetan': ['central-empire', 'republic-of-freelands', 'taii-maku-city-states'],
    'yama': ['seashores'],
}


class AtlasError(ExtractionError):
    """Le chapitre ne se lit pas comme le module l'attend."""


# -- Outils de texte -----------------------------------------------------------------------------

def cle(nom: str) -> str:
    """Identifiant stable d'un nom propre : sans accent, sans apostrophe, en minuscules-tirets."""
    sans_accent = unicodedata.normalize('NFKD', nom)
    sans_accent = ''.join(c for c in sans_accent if not unicodedata.combining(c))
    # Le livre coupe « Taii'maku » d'une apostrophe typographique et « Taii-Maku » d'un tiret
    # demi-cadratin : les deux doivent tomber sur la meme cle.
    sans_accent = re.sub(r'[‘’ʼ–—]', '-', sans_accent)
    sans_accent = re.sub(r"[^A-Za-z0-9]+", '-', sans_accent)
    return sans_accent.strip('-').lower()


def recoller(lignes: list[str]) -> str:
    """Un paragraphe depuis ses lignes, cesures recollees.

    Le livre coupe ses mots en fin de ligne (« sum-\nmer elves »). Recoller sans regarder produit
    « sum mer elves » ; ne pas recoller du tout produit « sum- mer elves ». Les deux passent tous
    les controles de schema, et aucun humain ne relit 13 000 mots.
    """
    texte = ''
    for ligne in lignes:
        ligne = ligne.strip()
        if not ligne:
            continue
        if texte.endswith('-'):
            texte = texte[:-1] + ligne
        elif texte:
            texte += ' ' + ligne
        else:
            texte = ligne
    return re.sub(r'\s+', ' ', texte).strip()


def _titre_double(texte: str) -> str:
    """Le livre imprime ses titres deux fois, l'un sur l'autre, pour l'ombrage.

    « Central Central Empire Empire » n'est pas une faute d'extraction : ce sont deux calques
    superposes, que le mode mots rend l'un apres l'autre. Le titre est la moitie de la sequence.
    """
    mots = texte.split()
    if len(mots) % 2 == 0:
        moitie = len(mots) // 2
        # Les calques alternent mot a mot (« A A B B »), pas bloc a bloc (« A B A B »).
        if all(mots[2 * i] == mots[2 * i + 1] for i in range(moitie)):
            return ' '.join(mots[2 * i] for i in range(moitie))
    return texte.strip()


# -- Lecture de page -----------------------------------------------------------------------------

def _colonnes(ex: Extracteur, index: int, moitie: str) -> list[tuple[float, float]]:
    """Bornes des colonnes de texte d'une demi-page, numero de page ecarte."""
    mots = ex.mots(index, moitie=moitie)
    if not mots:
        return []
    bornes = ex._bornes_colonnes(mots, 6.0)
    return [b for b in bornes if b[1] - b[0] >= LARGEUR_COLONNE_MINIMALE]


def _lignes_ordonnees(ex: Extracteur, index: int, moitie: str) -> list:
    """Les lignes d'une demi-page **dans l'ordre de lecture** : colonne par colonne.

    `Extracteur.lignes` groupe par ordonnee sur toute la region qu'on lui donne ; sur une page a
    deux colonnes, cela entrelace les deux. Le decoupage prealable en colonnes est ce qui rend
    l'ordre de lecture juste.
    """
    rect = ex.rectangle(index, moitie)
    sortie = []
    for x0, x1 in _colonnes(ex, index, moitie) or [(rect.x0, rect.x1)]:
        sortie.extend(ex.lignes(index, region=(x0 - 1, rect.y0, x1 + 1, rect.y1)))
    return sortie


def _cadre_statistiques(ex: Extracteur, index: int):
    """Le mot « Regional » de l'encart, et le rectangle plein qui porte ses sept lignes.

    Rend `None` si la page n'ouvre pas de region.
    """
    page = ex._page(index)
    titres = [m for m in page.get_text('words') if m[4] == 'Regional']
    for titre in titres:
        cadres = [d['rect'] for d in page.get_drawings()
                  if d['type'] == 'f' and d['rect'].height > 50
                  and d['rect'].x0 <= titre[0] and d['rect'].x1 >= titre[2]
                  and d['rect'].y0 >= titre[1] - 2]
        if cadres:
            return titre, max(cadres, key=lambda r: r.height)
    return None


def _mots_dans(page, x0: float, y0: float, x1: float, y1: float) -> list:
    return sorted((m for m in page.get_text('words')
                   if x0 <= m[0] and m[2] <= x1 and y0 <= m[1] and m[3] <= y1),
                  key=lambda m: (m[1], m[0]))


def _lignes_de(ex: Extracteur, mots: list) -> list[str]:
    return [' '.join(m[4] for m in sorted(groupe, key=lambda m: m[0]))
            for groupe in ex._grouper_lignes(mots)]


# -- Statistiques régionales ---------------------------------------------------------------------

def _note(valeur: str) -> tuple[str, str] | None:
    """Découpe « Very High » ou « High » en (note fermée, reste)."""
    for libelle in ('Very Low', 'Very High', 'Low', 'Normal', 'High'):
        if valeur == libelle or valeur.startswith(libelle + ' ') or valeur.startswith(libelle + ','):
            return NOTES[libelle], valeur[len(libelle):].lstrip(' ,')
    return None


def _appreciations(axe: str, valeur: str) -> list[dict]:
    """Une valeur d'encart vers ses appréciations typées.

    Trois formes, et rien d'autre. Toute quatrieme leve : une valeur qu'on ne sait pas typer doit
    se voir a l'extraction, pas se retrouver en texte libre dans le catalogue.

      « High »                                      -> une appreciation sans portee
      « Low (south), High (north) »                 -> deux appreciations, portee entre parentheses
      « Low underground and High on the surface. »  -> deux appreciations, portee en toutes lettres
    """
    valeur = valeur.strip().rstrip('.')
    decoupe = _note(valeur)
    if decoupe is None:
        raise AtlasError(f'{axe} : « {valeur} » ne commence par aucune des cinq notes.')
    note, reste = decoupe
    if not reste:
        return [{'grade': note}]

    entre_parentheses = re.fullmatch(r'\((?P<p1>[^)]+)\),\s*(?P<suite>.+)', reste)
    if entre_parentheses:
        suite = _note(entre_parentheses.group('suite'))
        if suite is None:
            raise AtlasError(f'{axe} : « {valeur} » : seconde note illisible.')
        note2, reste2 = suite
        portee2 = re.fullmatch(r'\((?P<p2>[^)]+)\)', reste2.strip())
        if portee2 is None:
            raise AtlasError(f'{axe} : « {valeur} » : seconde portée illisible.')
        return [{'grade': note, 'scope': entre_parentheses.group('p1').strip()},
                {'grade': note2, 'scope': portee2.group('p2').strip()}]

    en_toutes_lettres = re.fullmatch(r'(?P<p1>.+?)\s+and\s+(?P<suite>.+)', reste)
    if en_toutes_lettres:
        suite = _note(en_toutes_lettres.group('suite'))
        if suite is None:
            raise AtlasError(f'{axe} : « {valeur} » : seconde note illisible.')
        note2, reste2 = suite
        portee2 = re.sub(r'^(on|in|at)\s+(the\s+)?', '', reste2.strip())
        if not portee2:
            raise AtlasError(f'{axe} : « {valeur} » : seconde portée absente.')
        return [{'grade': note, 'scope': en_toutes_lettres.group('p1').strip()},
                {'grade': note2, 'scope': portee2}]

    raise AtlasError(f'{axe} : « {valeur} » : forme inconnue.')


def _statistiques(ex: Extracteur, index: int, titre, cadre) -> dict:
    """Les sept axes de l'encart, typés.

    Une valeur peut deborder sur la ligne suivante DANS l'encart (« Low underground and High on / the
    surface. ») : toute ligne qui n'ouvre pas un axe prolonge le precedent.
    """
    mots = _mots_dans(ex._page(index), cadre.x0, cadre.y0, cadre.x1, cadre.y1)
    brutes: dict[str, list[str]] = {}
    courant = None
    for ligne in _lignes_de(ex, mots):
        ouvert = next((a for a in AXES if ligne.startswith(a)), None)
        if ouvert:
            courant = ouvert
            brutes[courant] = [ligne[len(ouvert):].strip()]
        elif courant:
            brutes[courant].append(ligne.strip())
    manquants = [a for a in AXES if a not in brutes]
    if manquants:
        raise AtlasError(f'page {index} : encart incomplet, axes absents : {manquants}.')
    return {AXES[axe]: _appreciations(axe, recoller(brutes[axe])) for axe in AXES}


# -- Introduction --------------------------------------------------------------------------------

def _population(texte: str) -> dict:
    """« Around 2,300,000. Humans 84%, gnomes 5%, ... others 2%. » vers un total et des parts.

    Le total est **approximatif dans le livre** (« Around ») et le reste : arrondir en silence
    donnerait une precision que la source n'a pas.
    """
    total = re.search(r'([\d][\d,\.]*)\s*(?:,|\.)?\s', texte)
    parts = []
    reste = None
    for espece, pourcent in re.findall(r'([A-Za-z’\'\- ]+?)\s+(\d+)\s*%', texte):
        nom = espece.strip().strip(',').strip()
        nom = re.sub(r'^(and|around|approximately)\s+', '', nom, flags=re.I).strip().lower()
        if not nom:
            continue
        if nom == RESTE:
            reste = int(pourcent)
            continue
        if nom not in ESPECES:
            raise AtlasError(f'peuple « {nom} » absent de la table des espèces.')
        parts.append({'species': ESPECES[nom], 'label': nom, 'percent': int(pourcent)})
    if not parts:
        raise AtlasError(f'population sans répartition : « {texte[:80]} »')
    somme = sum(p['percent'] for p in parts) + (reste or 0)
    if not 97 <= somme <= 103:
        # Le livre arrondit ; il ne se trompe pas de dix points. Un ecart plus large signale une
        # part perdue en fin de ligne, pas une coquille de l'auteur.
        raise AtlasError(f'répartition à {somme} % : une part manque ou a été lue deux fois.')
    return {
        'total': int(total.group(1).replace(',', '').replace('.', '')) if total else None,
        'species': parts,
        'otherPercent': reste,
    }


def _introduction(ex: Extracteur, index: int, titre, cadre) -> dict:
    """Le bloc `Government` / `Faction` / `Population`, au-dessus de l'encart, même colonne."""
    mots = _mots_dans(ex._page(index), cadre.x0 - 3, 0.0, cadre.x1 + MARGE_COLONNE, titre[1])
    champs: dict[str, list[str]] = {}
    courant = None
    for ligne in _lignes_de(ex, mots):
        ouvert = next((e for e in ETIQUETTES if ligne.startswith(e + ':')), None)
        if ouvert:
            courant = ouvert
            champs[courant] = [ligne[len(ouvert) + 1:].strip()]
        elif courant:
            champs[courant].append(ligne)
    manquants = [e for e in ETIQUETTES if e not in champs]
    if manquants:
        raise AtlasError(f'page {index} : introduction incomplète, étiquettes absentes '
                         f': {manquants}.')
    sortie = {ETIQUETTES[e]: recoller(champs[e]) for e in ETIQUETTES}
    sortie['population'] = _population(sortie['population'])
    return sortie


# -- Découpage du chapitre -----------------------------------------------------------------------

def _sections(ex: Extracteur, index: int, moitie: str) -> list[tuple[float, str]]:
    """Les intertitres d'une demi-page : (corps, texte), dans l'ordre de lecture."""
    sortie = []
    for ligne in _lignes_ordonnees(ex, index, moitie):
        if ligne.corps and ligne.corps >= CORPS_SOUS_TITRE:
            sortie.append((ligne.corps, _titre_double(ligne.texte.strip())))
    return sortie


def _regions(ex: Extracteur) -> list[dict]:
    """Les régions du chapitre : nom, page d'ouverture, encart, introduction.

    Une region s'ouvre a la demi-page qui porte un encart `Regional Statistics` ; son nom est le
    seul intertitre de corps 18 de cette demi-page qui ne soit pas un intertitre de section.
    """
    ouvertures = []
    for index in CHAPITRE:
        cadre = _cadre_statistiques(ex, index)
        if cadre is None:
            continue
        titre, rect = cadre
        moitie = 'gauche' if rect.x1 <= ex._page(index).rect.width / 2 else 'droite'
        noms = [t for corps, t in _sections(ex, index, moitie)
                if corps >= CORPS_SECTION and t not in SECTIONS]
        if len(noms) != 1:
            raise AtlasError(f'page {index} : {len(noms)} titre(s) de région candidat(s) '
                             f'({noms}), un seul attendu.')
        ouvertures.append({
            'nom': noms[0],
            'page': index,
            'moitie': moitie,
            'titre': titre,
            'cadre': rect,
        })
    if not ouvertures:
        raise AtlasError('aucune région trouvée : le chapitre a changé de forme.')
    return ouvertures


def _lieux(ex: Extracteur, debut: int, fin: int, galerie: str | None) -> list[dict]:
    """Les lieux d'une région : les intertitres de corps 12 sous « Places of Interest ».

    La section court jusqu'au prochain intertitre de corps 18, ou jusqu'a `galerie` -- le premier
    nom de la galerie de personnages, quand la region en porte une.

    Un intertitre qui suit immediatement un autre intertitre, sans texte entre les deux, en est la
    **suite** et non un nouveau lieu : le livre coupe ses titres longs sur deux lignes
    (« Tengoku Palace and Kinshi, / the forbidden city »). Les separer donnerait deux lieux, dont
    un sans description et un au nom tronque -- deux fautes qu'aucun schema ne voit.
    """
    lieux: list[dict] = []
    dedans = False
    courant = None
    for index in range(debut, fin):
        for moitie in ('gauche', 'droite'):
            for ligne in _lignes_ordonnees(ex, index, moitie):
                texte = _titre_double(ligne.texte.strip())
                corps = ligne.corps or 0.0
                if corps >= CORPS_SECTION:
                    dedans = texte == 'Places of Interest'
                    courant = None
                    continue
                if not dedans:
                    continue
                if corps >= CORPS_SOUS_TITRE:
                    if not ligne.fragments[0].police.startswith(POLICE_PROSE):
                        continue
                    if courant is not None and not courant['lignes']:
                        courant['name'] = recoller([courant['name'], texte])
                    else:
                        courant = {'name': texte, 'page': index, 'lignes': []}
                        lieux.append(courant)
                    if galerie and courant['name'] == galerie:
                        lieux.pop()
                        return lieux
                elif courant is not None and texte:
                    courant['lignes'].append(texte)
    return lieux


def _capitale(ex: Extracteur) -> list[dict]:
    """La Capitale puis ses quartiers, dans l'ordre du livre : (name, lignes).

    La Capitale porte le texte qui precede « Districts of the Capital » (population, gouvernance,
    armee comprises : c'est sa fiche) ; chaque quartier, le texte qui suit son fragment d'ouverture
    jusqu'au quartier suivant. Un compte different de douze, ou une numerotation qui saute, est
    une erreur : un quartier fondu dans le precedent passerait tous les autres controles.
    """
    index = CAPITALE['page']
    capitale = {'name': CAPITALE['nom'], 'lignes': []}
    lieux = [capitale]
    courant = capitale
    titre_vu = quartiers = False
    for moitie in ('gauche', 'droite'):
        for ligne in _lignes_ordonnees(ex, index, moitie):
            texte = ligne.texte.strip()
            corps = ligne.corps or 0.0
            if not texte or HORS_PROSE_RE.search(texte):
                continue
            if corps >= CORPS_SECTION:
                titre_vu = titre_vu or texte.startswith(CAPITALE['nom'])
                continue
            if corps >= CORPS_SOUS_TITRE:
                quartiers = quartiers or texte == INTERTITRE_QUARTIERS
                continue
            ouverture = ligne.fragments[0]
            m = QUARTIER_RE.match(ouverture.texte.strip()) if ouverture.gras else None
            if quartiers and m:
                if int(m.group(1)) != len(lieux):
                    raise AtlasError(f'page {index} : quartier « {m.group(2)} » numéroté '
                                     f'{m.group(1)}, {len(lieux)} attendu.')
                courant = {'name': m.group(2), 'lignes': []}
                lieux.append(courant)
                texte = ''.join(f.texte for f in ligne.fragments[1:]).strip()
            if texte:
                courant['lignes'].append(texte)
    if not titre_vu:
        raise AtlasError(f"page {index} : titre « {CAPITALE['nom']} » absent, l'encart a bougé.")
    if len(lieux) - 1 != QUARTIERS_ATTENDUS:
        raise AtlasError(f'page {index} : {len(lieux) - 1} quartier(s), '
                         f'{QUARTIERS_ATTENDUS} attendus.')
    return lieux


# -- Production ----------------------------------------------------------------------------------

def extraire(corpus: Corpus, cache=None) -> tuple[list[dict], list[dict]]:
    """Les régions et les lieux du chapitre, prêts à écrire."""
    document = corpus.documents[DOCUMENT]
    regions: list[dict] = []
    lieux: list[dict] = []
    with Extracteur(document, cache=cache) as ex:
        ouvertures = _regions(ex)
        for rang, ouverture in enumerate(ouvertures):
            fin = (ouvertures[rang + 1]['page'] if rang + 1 < len(ouvertures)
                   else CHAPITRE.stop)
            nom = NOMS_REPARES.get(ouverture['nom'], ouverture['nom'])
            identifiant = cle(nom)
            region = {
                'id': identifiant,
                'name': nom,
                'source': document.provenance,
                **_introduction(ex, ouverture['page'], ouverture['titre'], ouverture['cadre']),
                'statistics': _statistiques(ex, ouverture['page'], ouverture['titre'],
                                            ouverture['cadre']),
                'neighbors': sorted(VOISINS.get(identifiant, [])),
                'locations': [],
            }
            for lieu in _lieux(ex, ouverture['page'], fin, GALERIES.get(identifiant)):
                # Une etiquette n'est pas un nom de lieu : « Effects: » ouvre la liste des effets
                # d'un flau, pas un endroit ou l'on se rend.
                if lieu['name'].endswith(':'):
                    continue
                identifiant_lieu = f"{identifiant}-{cle(lieu['name'])}"
                region['locations'].append(identifiant_lieu)
                lieux.append({
                    'id': identifiant_lieu,
                    'name': lieu['name'],
                    'region': identifiant,
                    'source': document.provenance,
                    # Le numero de page et le filigrane restent dans `lignes`, ou ils separent deux
                    # intertitres (le decoupage en depend) ; ils sortent de la description.
                    'description': recoller(sans_filigrane(lieu['lignes'])),
                })
            if identifiant == CAPITALE['region']:
                capitale, *quartiers = _capitale(ex)
                id_capitale = f"{identifiant}-{cle(capitale['name'])}"
                for lieu in [capitale] + quartiers:
                    identifiant_lieu = (id_capitale if lieu is capitale
                                        else f"{id_capitale}-{cle(lieu['name'])}")
                    region['locations'].append(identifiant_lieu)
                    lieux.append({
                        'id': identifiant_lieu,
                        'name': lieu['name'],
                        'region': identifiant,
                        'source': document.provenance,
                        'description': recoller(lieu['lignes']),
                    })
            regions.append(region)
    return regions, lieux


def controler(regions: list[dict], lieux: list[dict]) -> None:
    """Les contrôles de cohérence de l'atlas, avant écriture.

    Ce sont exactement ceux de l'acceptation du lot : aucun lieu sans region, aucun voisin qui
    n'existe pas, aucune region inatteignable.
    """
    attendus = set(LIEUX_ATTENDUS)
    obtenus = {r['id'] for r in regions}
    if attendus != obtenus:
        raise AtlasError(f'régions inattendues : {sorted(obtenus ^ attendus)}.')
    for region in regions:
        if len(region['locations']) != LIEUX_ATTENDUS[region['id']]:
            raise AtlasError(
                f"{region['id']} : {len(region['locations'])} lieu(x), "
                f"{LIEUX_ATTENDUS[region['id']]} attendu(s).")

    connues = {r['id'] for r in regions}
    if len(connues) != len(regions):
        raise AtlasError('deux régions portent le même identifiant.')

    for lieu in lieux:
        if lieu['region'] not in connues:
            raise AtlasError(f"lieu « {lieu['id']} » : région « {lieu['region']} » inconnue.")

    for region in regions:
        for voisin in region['neighbors']:
            if voisin not in connues:
                raise AtlasError(f"{region['id']} : voisin « {voisin} » inconnu.")
            if region['id'] not in dict((r['id'], r['neighbors']) for r in regions)[voisin]:
                raise AtlasError(f"voisinage non symétrique : {region['id']} → {voisin}.")

    # Atteignabilite : un parcours depuis la premiere region doit toutes les voir.
    adjacence = {r['id']: r['neighbors'] for r in regions}
    vus = {regions[0]['id']}
    pile = [regions[0]['id']]
    while pile:
        for voisin in adjacence[pile.pop()]:
            if voisin not in vus:
                vus.add(voisin)
                pile.append(voisin)
    isolees = sorted(connues - vus)
    if isolees:
        raise AtlasError(f'régions inatteignables depuis {regions[0]["id"]} : {isolees}.')


def produire(corpus: Corpus, sortie, cache=None) -> dict:
    """Écrit l'atlas et rend son décompte."""
    from pathlib import Path

    regions, lieux = extraire(corpus, cache=cache)
    controler(regions, lieux)

    racine = Path(sortie)
    for dossier, entrees in (('regions', regions), ('locations', lieux)):
        cible = racine / dossier
        cible.mkdir(parents=True, exist_ok=True)
        for ancien in cible.glob('*.json'):
            ancien.unlink()
        for entree in entrees:
            fichier = cible / f"{entree['id']}.json"
            fichier.write_text(
                json.dumps(entree, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

    return {
        'regions': len(regions),
        'lieux': len(lieux),
        'especes': len({p['species'] for r in regions for p in r['population']['species']}),
    }
