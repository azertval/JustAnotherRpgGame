#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Lint du graphe de lots de JustAnotherRpgGame.

La feuille de route ``Documentation/Lot/roadmap.md`` est l'**unique source de vérité** des
lots à venir. Un document de cette taille dérive : les comptes vieillissent, un lien de dépendance
s'écrit d'un seul côté, un lot disparaît du tableau d'ordre, deux lots revendiquent la même
exigence, le diagramme trace une flèche que plus rien ne justifie. L'audit qui a produit la version
courante de cette page y a trouvé six incohérences de cette forme — toutes mécaniquement
détectables, aucune détectée.

Ce lint les refuse en CI. Il vérifie :

1. chaque lot de la filière porte une ligne ``*Prérequis*`` et une ligne d'acceptation ;
2. chaque lot de la filière porte une **ancre Doxygen** ``{#lot-NN}``, unique ;
3. le graphe de prérequis est **acyclique** ;
4. tout prérequis désigne un lot **existant** ;
5. les liens sont **symétriques** : si A déclare « Alimente B », alors B déclare A en prérequis ;
6. chaque lot de la filière apparaît dans le **regroupement d'intention** de la section 6 ;
7. les **numéros manquants** de la plage sont soit retirés par fusion et recensés comme tels, soit
   livrés et pourvus de leur dossier ; un numéro retiré n'est plus cité comme prérequis ;
8. les **comptes annoncés en toutes lettres** correspondent au décompte réel ;
9. aucune **exigence** n'est revendiquée en retrait par deux lots à la fois ;
10. le **tableau récapitulatif** de la section 6 correspond au graphe déclaré ;
11. toute arête du **diagramme** de la section 6 correspond à un lien déclaré ;
12. tout ``LOT-NN`` cité dans une **spécification** désigne un lot existant — de cette page, ou
    d'une fiche de ``Planning/`` pour les numéros à trois chiffres (``LOT-100`` et au-delà, depuis
    le ``LOT-100``) — et aucune spécification n'emploie plus l'ancienne notation ``LOT-H-NN``
    (retirée au ``LOT-88``) ;
13. le **tableau d'avancement** en tête de feuille de route est exactement la suite que produit la
    règle d'ordre — *à chaque pas, parmi les lots dont tous les prérequis sont faits, celui du
    jalon de version le plus proche ; à jalon égal, celui qui en débloque le plus* ;
14. chaque lot restant figure dans **un jalon de version** du tableau en tête de page, et un seul.

Les lots livrés (``LOT-01`` à ``LOT-07``) et absorbés (``LOT-08`` à ``LOT-29``) sont exclus des
contrôles 1, 2 et 5 : leur texte est repris tel quel de leurs epics d'origine, et les sections 5, 9
et 10 de la feuille de route font foi sur eux — c'est écrit en tête de la section 11.

Sortie : liste des violations, code de retour 1 si au moins une. Aucune dépendance externe.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

# La console Windows est en cp1252 : les messages contiennent des flèches et des guillemets.
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

RACINE = Path(__file__).resolve().parent.parent
DOSSIER_LOTS = RACINE / 'Documentation' / 'Lot'
ROADMAP = DOSSIER_LOTS / 'roadmap.md'
SPECIFICATIONS = RACINE / 'Documentation' / 'Specification'

# Un renvoi de spécification LOT-NN désigne un lot de ce programme, et doit donc en désigner un qui
# existe. L'ancienne notation LOT-H-NN, qui renvoyait à un programme de lots aujourd'hui retiré du
# dépôt, n'a plus rien à désigner (LOT-88).
RENVOI_SPEC_RE = re.compile(r'LOT-(\d+)')
ANCIEN_RENVOI_RE = re.compile(r'LOT-H-\d+')

# Depuis le LOT-100, les lots à venir se numérotent à partir de 100 et vivent dans `Planning/`, une
# fiche par fichier ; cette page est figée et ne les porte pas. Une spécification qui en cite un ne
# cite donc rien d'inexistant -- mais il faut que la fiche existe, sans quoi la règle 12 ne garderait
# plus rien du jour où le premier renvoi à trois chiffres est apparu.
PLANIFICATION = RACINE / 'Planning' / 'versions'
FICHE_PLANIFICATION_RE = re.compile(r'^(LOT-\d+)-')

PREMIER_LOT_FILIERE = 30

# Sections de lot : "### `LOT-30` — Titre {#lot-30}" ou la plage "### `LOT-51` à `LOT-65` — …"
SECTION_RE = re.compile(r'^### `LOT-(\d+)`(?: à `LOT-(\d+)`)? — (.+)$', re.M)
# Sections des lots absorbés (LOT-08 à LOT-29) : "### LOT-08 — Titre {#lot-08}"
SECTION_ABSORBEE_RE = re.compile(r'^### LOT-(\d+) — (.+)$', re.M)
ANCRE_RE = re.compile(r'\{#lot-(\d+)\}')
LOT_RE = re.compile(r'LOT-(\d+)')
TITRE_RE = re.compile(r'— (.+?)(?: \{#lot-\d+\})?$')

# Ce qui, dans la ligne « Prérequis », introduit le sens inverse de la dépendance.
# « Prérequis de chacun » est une exception : elle déclare bien de l'amont.
SENS_INVERSE_RE = re.compile(r'Alimente|Prérequis de(?! chacun)|Débloque|Contrôlé par|Réoriente')

# Comptes écrits en toutes lettres que le lint sait vérifier.
NOMBRES_FR = {
    1: 'un', 2: 'deux', 3: 'trois', 4: 'quatre', 5: 'cinq', 6: 'six', 7: 'sept', 8: 'huit',
    9: 'neuf', 10: 'dix', 11: 'onze', 12: 'douze', 13: 'treize', 14: 'quatorze', 15: 'quinze',
    16: 'seize', 20: 'vingt', 30: 'trente', 35: 'trente-cinq', 38: 'trente-huit', 39: 'trente-neuf', 40: 'quarante', 41: 'quarante et un', 42: 'quarante-deux', 43: 'quarante-trois',
    44: 'quarante-quatre',
    50: 'cinquante',
    45: 'quarante-cinq', 46: 'quarante-six', 47: 'quarante-sept', 48: 'quarante-huit',
    49: 'quarante-neuf',
    51: 'cinquante et un', 52: 'cinquante-deux', 53: 'cinquante-trois',
    54: 'cinquante-quatre', 55: 'cinquante-cinq',
}


class Rapport:
    def __init__(self) -> None:
        self.violations: list[str] = []

    def erreur(self, message: str) -> None:
        self.violations.append(message)

    def bilan(self) -> int:
        if not self.violations:
            print('lint_lots : OK')
            return 0
        print('lint_lots : %d violation(s)\n' % len(self.violations))
        for v in self.violations:
            print('  - ' + v)
        return 1


def numero(n) -> str:
    return 'LOT-%s' % n


def lots_de_la_planification():
    """Les lots déclarés par une fiche de ``Planning/`` : ceux que cette page ne porte plus."""
    trouves = set()
    for chemin in PLANIFICATION.rglob('LOT-*.md'):
        trouve = FICHE_PLANIFICATION_RE.match(chemin.name)
        if trouve:
            trouves.add(trouve.group(1))
    return trouves


def lots_livres():
    """Les lots qui ont leur dossier : livrés, ils ont quitté la feuille de route."""
    livres = set()
    for chemin in DOSSIER_LOTS.glob('LOT-*'):
        if chemin.is_dir():
            m = re.match(r'LOT-(\d+)', chemin.name)
            if m:
                livres.add('LOT-' + m.group(1))
    return livres


def lire_sections(texte: str) -> dict:
    """{lot: {titre, ancre, corps}} ; une plage « LOT-51 à LOT-65 » couvre ses intermédiaires."""
    sections: dict = {}
    bornes = []
    for m in SECTION_RE.finditer(texte):
        bornes.append((m.start(), m.group(0), m.group(1), m.group(2)))
    for m in SECTION_ABSORBEE_RE.finditer(texte):
        bornes.append((m.start(), m.group(0), m.group(1), None))
    bornes.sort()

    for i, (debut, entete, premier, dernier) in enumerate(bornes):
        fin = bornes[i + 1][0] if i + 1 < len(bornes) else len(texte)
        ancres = ANCRE_RE.findall(entete)
        lot = numero(premier)
        sections[lot] = {
            'titre': entete,
            'ancre': ancres[0] if ancres else None,
            'corps': texte[debut:fin],
        }
        if dernier:
            for n in range(int(premier) + 1, int(dernier) + 1):
                sections[numero(n)] = dict(sections[lot], couvert_par=lot)
    return sections


def bloc_prerequis(corps: str):
    """Le paragraphe « Prérequis », qui peut courir sur plusieurs lignes.

    Deux écritures coexistent, et le lint doit lire les deux. Les lots de la filière (§5)
    l'écrivent en italique, ``*Prérequis : …*`` ; les lots absorbés (§11) l'écrivent dans leur
    bloc de citation, ``> Prérequis : …``, parce que leur texte est repris tel quel de leurs
    epics d'origine. Ne lire que la première forme laissait **les vingt et un lots absorbés sans
    aucun prérequis connu du lint** : ni le contrôle d'acyclicité ni celui des prérequis
    existants ne les voyait, et l'ordre d'exécution s'en serait trouvé faux.
    """
    m = re.search(r'^\*Prérequis[^\n]*(?:\n(?!\n)[^\n]*)*', corps, re.M)
    if m:
        return m.group(0)
    m = re.search(r'^> Prérequis[^\n]*(?:\n> [^\n]*)*', corps, re.M)
    return m.group(0) if m else None


def prerequis_de(corps: str):
    """(prérequis déclarés, lots que ce lot déclare alimenter)."""
    bloc = bloc_prerequis(corps)
    if not bloc:
        return [], []
    coupe = SENS_INVERSE_RE.search(bloc)
    amont = bloc[:coupe.start()] if coupe else bloc
    aval = bloc[coupe.start():] if coupe else ''
    return ([numero(n) for n in LOT_RE.findall(amont)],
            [numero(n) for n in LOT_RE.findall(aval)])


def graphe(texte: str):
    """(prérequis, alimente déclaré, alimente effectif, sections)."""
    sections = lire_sections(texte)
    amont: dict = {}
    aval_declare: dict = {}
    for lot, s in sections.items():
        if s.get('couvert_par'):
            continue
        a, b = prerequis_de(s['corps'])
        amont[lot] = [x for x in a if x != lot]
        aval_declare[lot] = [x for x in b if x != lot]

    aval: dict = {lot: set() for lot in amont}
    for lot, deps in amont.items():
        for d in deps:
            aval.setdefault(d, set()).add(lot)
    for lot, cibles in aval_declare.items():
        aval.setdefault(lot, set()).update(cibles)
    return amont, aval_declare, aval, sections


def porteur_de(sections: dict, lot: str) -> str:
    """Le lot qui porte la section de ``lot`` — lui-même, ou la plage qui le couvre."""
    return sections.get(lot, {}).get('couvert_par', lot)


def prerequis_effectifs(amont: dict, aval_declare: dict, sections: dict) -> dict:
    """Ce que chaque lot attend réellement, les deux sens de déclaration réunis.

    Un lien de dépendance s'écrit **d'un côté ou de l'autre** dans cette page : soit le lot aval
    le déclare en prérequis, soit le lot amont déclare l'alimenter. Le contrôle de symétrie
    (règle 5) impose déjà que les deux se correspondent **entre lots de la filière** ; il ne peut
    rien imposer aux lots absorbés, dont le texte est repris tel quel et ne connaît pas la
    filière. Le `LOT-27` en est le cas d'espèce : il ne déclare aucun prérequis, alors que cinq
    lots de contenu déclarent l'alimenter. Ne lire que l'amont le placerait avant eux, et le
    *vertical slice* se jouerait sur des catalogues vides.
    """
    effectifs = {lot: set(porteur_de(sections, d) for d in deps) for lot, deps in amont.items()}
    for source, cibles in aval_declare.items():
        for cible in cibles:
            porteur = porteur_de(sections, cible)
            if porteur in effectifs:
                effectifs[porteur].add(porteur_de(sections, source))
    for lot in effectifs:
        effectifs[lot].discard(lot)
    return effectifs


JALON_RE = re.compile(r'^\| \*{0,2}`(0\.\d+\.\d+)`\*{0,2} \|[^|]*\|([^|]*)\|', re.M)


def jalons(texte: str) -> dict:
    """{lot: rang du jalon de version}, lu dans le tableau « Version » en tête de page.

    Les jalons sont des états jouables successifs (0.0.1 le slice, 0.0.2 les régions…). Un lot
    d'un jalon ultérieur ne démarre pas tant que le jalon courant a un lot prêt : c'est la seule
    priorité que le graphe ne dit pas, et la seule que l'auteur fixe à la main. Une plage
    « `LOT-51` → `LOT-65` » couvre ses intermédiaires.
    """
    rangs: dict = {}
    for rang, m in enumerate(JALON_RE.finditer(texte), 1):
        cellule = m.group(2)
        for a, b in re.findall(r'`LOT-(\d+)` → `LOT-(\d+)`', cellule):
            for n in range(int(a), int(b) + 1):
                rangs.setdefault(numero(n), rang)
        for n in LOT_RE.findall(cellule):
            rangs.setdefault(numero(n), rang)
    return rangs


def descendants(lot: str, enfants: dict, restants: set) -> set:
    """Les lots restants qu'un lot débloque, directement ou en cascade."""
    vus: set = set()
    pile = [lot]
    while pile:
        for enfant in enfants.get(pile.pop(), ()):
            if enfant in restants and enfant not in vus:
                vus.add(enfant)
                pile.append(enfant)
    return vus


def ordre_execution(texte: str):
    """La suite d'exécution des lots restants, ce que chacun attend et ce qu'il débloque.

    **La règle, en une phrase :** à chaque pas, on prend, parmi les lots dont tous les prérequis
    sont faits, **celui du jalon de version le plus proche** ; à jalon égal, **celui qui en
    débloque le plus** — à égalité, le plus petit numéro ; un lot qui en couvre d'autres passe en
    dernier, parce que c'est une série et non un lot. Le jalon prime : sans lui, le calcul
    plaçait les factions du bac à sable (`0.0.2`) avant le contenu du slice (`0.0.1`), parce
    qu'elles débloquaient davantage.

    Le critère n'est pas le numéro. Il l'a été un temps, et il donnait une suite déterministe mais
    bête : elle plaçait le `LOT-10` et le `LOT-12` devant le `LOT-30`, alors que ce dernier
    débloque à lui seul quarante-neuf des lots restants et que les deux premiers n'en débloquent
    que vingt-cinq chacun. « Outillage et contrats ; le plus tôt est le mieux » cesse ainsi d'être
    un avis éditorial pour devenir ce que le graphe dit — le `LOT-30` sort premier parce qu'il
    débloque le plus, pas parce qu'on l'a décidé.

    Ce que la règle **ne fait pas** : elle ne raccourcit pas le programme, et ne prétend pas
    minimiser une durée que rien ne mesure. Elle maximise, à chaque pas, le nombre de lots qui
    deviennent démarrables — c'est-à-dire qu'elle repousse le plus tard possible le moment où il
    ne reste qu'un seul chemin.

    Le numéro ne sert qu'à départager, et il faut un départage : sans lui, deux lots de même
    portée sortiraient dans l'ordre du dictionnaire Python, et la suite changerait d'une exécution
    à l'autre.

    Renvoie ``(suite, effectifs, portees, sections)``.
    """
    amont, aval_declare, _, sections = graphe(texte)
    effectifs = prerequis_effectifs(amont, aval_declare, sections)

    couverts_par: dict = {}
    for lot, s in sections.items():
        porteur = s.get('couvert_par')
        if porteur:
            couverts_par.setdefault(porteur, []).append(lot)

    enfants: dict = {lot: set() for lot in effectifs}
    for lot, deps in effectifs.items():
        for dep in deps:
            if dep in enfants:
                enfants[dep].add(lot)

    rang_jalon = jalons(texte)
    faits = set(lots_livres())
    restants = set(effectifs)
    suite: list = []
    portees: dict = {}
    while restants:
        prets = [lot for lot in restants if effectifs[lot] <= faits]
        if not prets:
            # Un blocage ici est un cycle, que la règle 3 signale déjà, ou un prérequis vers un
            # lot inexistant, que signale la règle 4. On rend la suite partielle : le lint
            # rapporte la cause exacte, pas ce symptôme.
            break
        portee = {lot: len(descendants(lot, enfants, restants)) for lot in prets}
        # Un lot QUI EN COUVRE D'AUTRES (`LOT-51` → `LOT-65` : une classe par lot) passe en
        # dernier, quelle que soit sa portée. Ce n'est pas une exception à la règle mais une
        # lecture de ce qu'il est : ce n'est pas un lot, c'est une série, et la placer au milieu
        # de la suite ferait croire qu'on la traverse d'un bloc avant de reprendre le programme.
        # Le départage ne la choisit donc que lorsqu'elle est seule en lice — jamais de blocage,
        # puisque `prets` finit par ne plus contenir qu'elle.
        lot = max(prets, key=lambda x: (not couverts_par.get(x), -rang_jalon.get(x, 99),
                                        portee[x], -int(x[4:])))
        suite.append(lot)
        portees[lot] = portee[lot]
        faits.add(lot)
        faits.update(couverts_par.get(lot, []))
        restants.discard(lot)
    return suite, effectifs, portees, sections


def tableau_ordre(texte: str) -> str:
    """Le tableau d'avancement en tête de page, dérivé du graphe comme le récapitulatif.

    Une ligne par lot restant, dans l'ordre d'exécution, avec son statut :

    - ``prochain`` — le premier de la suite, celui à démarrer ;
    - ``prêt`` — tous ses prérequis sont **livrés**, il pourrait démarrer aujourd'hui ; la règle
      lui préfère seulement un lot qui débloque davantage ;
    - ``en attente`` — il attend au moins un lot non livré.

    Le statut est la seule colonne qui bouge sans que la page change : livrer un lot en fait
    passer d'autres de « en attente » à « prêt ». C'est la raison pour laquelle ce tableau est
    calculé et non écrit.
    """
    suite, effectifs, portees, sections = ordre_execution(texte)
    livres = lots_livres()

    def titre(lot: str) -> str:
        m = TITRE_RE.search(sections[lot]['titre'])
        return m.group(1) if m else ''

    def nom(lot: str) -> str:
        couverts = [l for l, s in sections.items() if s.get('couvert_par') == lot]
        if not couverts:
            return '`%s`' % lot
        return '`%s` → `%s`' % (lot, max(couverts, key=lambda x: int(x[4:])))

    def statut(rang: int, lot: str) -> str:
        if rang == 1:
            return '**prochain**'
        return 'prêt' if effectifs[lot] <= livres else 'en attente'

    return '\n'.join(
        '| %d | %s | %s | %d | %s |'
        % (rang, nom(lot), titre(lot), portees[lot], statut(rang, lot))
        for rang, lot in enumerate(suite, 1))


def tableau_recapitulatif(texte: str) -> str:
    """Le tableau de la section 6, dérivé du graphe : une ligne par lot de la filière."""
    amont, _, aval, sections = graphe(texte)
    filiere = sorted(
        (l for l in sections
         if int(l[4:]) >= PREMIER_LOT_FILIERE and not sections[l].get('couvert_par')),
        key=lambda x: int(x[4:]))

    def titre(lot: str) -> str:
        m = TITRE_RE.search(sections[lot]['titre'])
        return m.group(1) if m else ''

    def liste(valeurs) -> str:
        ordonnees = sorted(valeurs, key=lambda x: int(x[4:]))
        return ', '.join('`%s`' % v for v in ordonnees) if ordonnees else '—'

    return '\n'.join(
        '| `%s` | %s | %s | %s |' % (lot, titre(lot), liste(amont.get(lot, [])),
                                     liste(aval.get(lot, set())))
        for lot in filiere)


def main() -> int:
    r = Rapport()
    texte = ROADMAP.read_text(encoding='utf-8')
    amont, aval_declare, aval, sections = graphe(texte)

    filiere = set(l for l in sections if int(l[4:]) >= PREMIER_LOT_FILIERE)
    livres = lots_livres()
    tous = set(sections) | livres

    # ---- 1, 2 : rubriques obligatoires et ancres ----
    ancres_vues: dict = {}
    for lot in sorted(filiere):
        s = sections[lot]
        if s.get('couvert_par'):
            continue  # couvert par la plage « LOT-51 à LOT-65 », qui porte les rubriques
        if not bloc_prerequis(s['corps']):
            r.erreur('%s : aucune ligne « Prérequis »' % lot)
        if 'cceptation' not in s['corps']:
            r.erreur('%s : aucune ligne « Acceptation »' % lot)
        if not s['ancre']:
            r.erreur('%s : aucune ancre Doxygen {#lot-NN} dans le titre' % lot)
        else:
            if s['ancre'] in ancres_vues:
                r.erreur('ancre {#lot-%s} en double : %s et %s'
                         % (s['ancre'], ancres_vues[s['ancre']], lot))
            ancres_vues[s['ancre']] = lot

    # ---- 4 : prérequis vers un lot existant ----
    for lot, deps in sorted(amont.items()):
        for d in deps:
            if d not in tous:
                r.erreur("%s déclare le prérequis %s, qui n'existe pas" % (lot, d))

    # ---- 3 : acyclicité ----
    etat: dict = {}

    def visiter(n: str, chemin: list) -> None:
        if etat.get(n) == 1:
            r.erreur('cycle de prérequis : %s' % ' → '.join(chemin + [n]))
            return
        if etat.get(n) == 2:
            return
        etat[n] = 1
        for d in amont.get(n, []):
            visiter(d, chemin + [n])
        etat[n] = 2

    for n in sorted(amont):
        visiter(n, [])

    # ---- 5 : symétrie des liens, entre lots de la filière ----
    for lot in sorted(filiere):
        for cible in aval_declare.get(lot, []):
            if cible not in filiere:
                continue
            # Un lot couvert par une plage (« LOT-51 à LOT-65 ») n'a pas de ligne propre :
            # ses prérequis sont ceux de la section qui le couvre.
            porteur = sections[cible].get('couvert_par', cible)
            if lot not in amont.get(porteur, []):
                r.erreur('%s déclare alimenter %s, mais %s ne le cite pas en prérequis'
                         % (lot, cible, porteur))

    # ---- 6 : présence au tableau d'avancement en tête de page ----
    au_tableau = set()
    bloc6 = texte.split("## État d'avancement")[1].split('## 1. Le corpus')[0]
    for ligne in bloc6.split('\n'):
        if ligne.startswith('|') and 'Pourquoi' not in ligne:
            cellules = ligne.split('|')
            if len(cellules) > 3:
                nums = [int(n) for n in LOT_RE.findall(cellules[2])]
                if '→' in cellules[2] and len(nums) == 2:
                    nums = list(range(nums[0], nums[1] + 1))
                au_tableau |= set(numero(n) for n in nums)
    for lot in sorted(filiere):
        if lot not in au_tableau:
            r.erreur("%s n'apparaît dans aucune ligne du tableau d'avancement "
                     "(en tête de page)" % lot)

    # ---- 7 : numéros retirés ----
    presents = set(int(l[4:]) for l in filiere)
    numeros_livres = set(int(l[4:]) for l in livres)
    # La plage court jusqu'au plus haut numéro de la filière, sur la page ou dans son dossier :
    # un lot qui quitte la page par le haut ne rogne pas la plage, sans quoi les numéros retirés
    # sous lui en sortiraient aussi.
    borne_haute = max(presents | set(n for n in numeros_livres if n > min(presents)))
    manquants = set(range(min(presents), borne_haute + 1)) - presents
    # Un numéro manquant est légitime de deux façons : il est retiré par fusion, ou il est livré
    # et a donc quitté cette page pour son dossier.
    retires_reels = manquants - numeros_livres
    bloc_retires = texte.split('numéros retirés.**')[1].split('---')[0]
    retires_declares = set(int(n) for n in LOT_RE.findall(bloc_retires))
    if retires_reels != retires_declares:
        r.erreur('numéros retirés : le tableau déclare %s, la plage en manque %s '
                 '(hors lots livrés %s)'
                 % (sorted(retires_declares), sorted(retires_reels),
                    sorted(numeros_livres & manquants)))
    for lot, deps in sorted(amont.items()):
        for d in deps:
            if int(d[4:]) in retires_reels:
                r.erreur('%s cite %s, un numéro retiré par fusion' % (lot, d))

    # ---- 8 : comptes annoncés ----
    mot = NOMBRES_FR.get(len(filiere))
    if mot:
        attendu = '%s lots, `LOT-%d` à `LOT-%d`' % (mot.capitalize(), min(presents), borne_haute)
        if attendu not in texte:
            r.erreur('le compte annoncé en §5 ne correspond pas : attendu « %s »' % attendu)
    mot_retires = NOMBRES_FR.get(len(retires_reels))
    if mot_retires and ('**%s numéros retirés.**' % mot_retires.capitalize()) not in texte:
        r.erreur('le nombre de numéros retirés annoncé ne correspond pas : %d attendu'
                 % len(retires_reels))

    # ---- 9 : une exigence retirée par un seul lot ----
    proprietaire: dict = {}
    for lot in sorted(filiere):
        for phrase in re.findall(r'[^.]*\*\*retire\*\*[^.]*\.', sections[lot]['corps']):
            if 'ne touche pas' in phrase or 'ne se les approprie' in phrase:
                continue
            for ex in re.findall(r'EX-[A-Z]+-\d+', phrase):
                if ex in proprietaire and proprietaire[ex] != lot:
                    r.erreur('%s est déclarée retirée par %s et par %s'
                             % (ex, proprietaire[ex], lot))
                proprietaire[ex] = lot

    # ---- 10 : tableau récapitulatif à jour ----
    attendu_tab = tableau_recapitulatif(texte)
    if attendu_tab not in texte:
        manquantes = [l for l in attendu_tab.split('\n') if l not in texte]
        r.erreur("le tableau récapitulatif (§6) ne correspond plus au graphe déclaré ; "
                 "%d ligne(s) à corriger, à commencer par : %s"
                 % (len(manquantes), manquantes[0] if manquantes else '(ordre des lignes)'))

    # ---- 11 : le diagramme ne trace que des liens déclarés ----
    m_dot = re.search(r'```dot\n(.*?)\n```', texte, re.S)
    if m_dot:
        corps_dot = m_dot.group(1)
        etiquettes = dict(re.findall(r'(L\d+)\s*\[label="(LOT-\d+)', corps_dot))
        for a, b in re.findall(r'(L\d+)\s*->\s*(L\d+)', corps_dot):
            src, dst = etiquettes.get(a), etiquettes.get(b)
            if not src or not dst:
                continue
            if src not in amont.get(dst, []) and dst not in aval.get(src, set()):
                r.erreur('le diagramme (§6) trace %s → %s, que rien ne déclare' % (src, dst))

    # ---- 13 : le tableau d'avancement est bien la suite que la règle produit ----
    # Le tableau d'ordre était jusqu'ici écrit à la main, en cinq lignes de « quand » flous
    # (« démarrables maintenant », « avec LOT-09 », « avant LOT-13 ») dont aucune ne disait par
    # quoi commencer. Il est désormais calculé ; ce contrôle est ce qui l'empêche de redevenir un
    # avis. Il subsume la règle 6, qu'on garde parce qu'elle nomme le lot manquant.
    attendu_ordre = tableau_ordre(texte)
    if attendu_ordre not in texte:
        lignes_attendues = attendu_ordre.splitlines()
        manquantes = [l for l in lignes_attendues if l not in texte]
        r.erreur("le tableau d'avancement (en tête de page) n'est pas la suite que produit la "
                 "règle d'ordre (§6) ; %d ligne(s) à corriger, à commencer par : %s"
                 % (len(manquantes) or 1,
                    manquantes[0] if manquantes else "(l'ordre des lignes)"))

    # ---- 14 : chaque lot restant a un jalon de version, et un seul ----
    rang_jalon = jalons(texte)
    restants_tous = [l for l in sections if l not in livres]
    for lot in sorted(restants_tous):
        if lot not in rang_jalon:
            r.erreur("%s ne figure dans aucun jalon de version (tableau « Version » en tête de page)" % lot)
    for lot in sorted(rang_jalon):
        if lot not in sections and lot not in livres:
            r.erreur("le tableau des jalons cite %s, qui n'existe pas" % lot)

    # ---- 12 : les renvois des spécifications désignent un lot de ce programme ----
    connus = set(sections) | livres | lots_de_la_planification()
    for chemin in sorted(SPECIFICATIONS.glob('*.md')):
        lignes = chemin.read_text(encoding='utf-8').splitlines()
        for numero_ligne, ligne in enumerate(lignes, 1):
            for n in RENVOI_SPEC_RE.findall(ligne):
                lot = numero(n)
                if lot not in connus:
                    r.erreur("%s:%d cite %s, qui n'existe pas dans ce programme"
                             % (chemin.name, numero_ligne, lot))
            for ancien in ANCIEN_RENVOI_RE.findall(ligne):
                r.erreur("%s:%d cite %s : l'ancienne numérotation de lots est retirée (LOT-88)"
                         % (chemin.name, numero_ligne, ancien))

    print('lots de la filière : %d (LOT-%d à LOT-%d ; %d retiré(s), %d livré(s) hors page)'
          % (len(filiere), min(presents), borne_haute, len(retires_reels),
             len(numeros_livres & manquants)))
    return r.bilan()


def regenerer() -> int:
    """Réécrit dans la feuille de route les deux tableaux que ce lint calcule.

    Les règles 10 et 13 refusent un tableau qui a dérivé, mais refuser ne suffit pas : sans cette
    option, corriger une seule ligne « Prérequis » oblige à recopier à la main jusqu'à cinquante
    lignes de tableau, et c'est exactement le geste qui réintroduit l'erreur qu'on venait de
    corriger. Le lint sait produire les deux tableaux ; il doit donc savoir les poser.

    Ne touche à rien d'autre — ni au regroupement d'intention, ni au diagramme, ni au texte.
    """
    texte = ROADMAP.read_text(encoding='utf-8')
    corrige = texte
    for entete, generateur in (
        ('| # | Lot | Objet | Débloque | Statut |\n|---|---|---|---|---|\n', tableau_ordre),
        ('| Lot | Objet | Prérequis | Alimente |\n|---|---|---|---|\n', tableau_recapitulatif),
    ):
        if entete not in corrige:
            print("regeneration impossible : en-tete de tableau introuvable —\n%s" % entete)
            return 1
        debut = corrige.index(entete) + len(entete)
        fin = corrige.index('\n\n', debut)
        # Le tableau est toujours recalculé depuis le texte D'ORIGINE : régénérer le premier ne
        # doit pas changer ce que le second lit.
        corrige = corrige[:debut] + generateur(texte) + corrige[fin:]

    if corrige == texte:
        print('lint_lots --regenerer : les deux tableaux etaient deja a jour.')
        return 0
    ROADMAP.write_text(corrige, encoding='utf-8')
    print('lint_lots --regenerer : tableau d\'avancement et recapitulatif reecrits.')
    return 0


if __name__ == '__main__':
    if '--regenerer' in sys.argv[1:]:
        sys.exit(regenerer())
    sys.exit(main())
