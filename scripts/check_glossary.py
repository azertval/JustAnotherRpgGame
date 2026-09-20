#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Contrôle du lexique de règles et de son respect par les catalogues de traduction (LOT-30).

`Source/Elements/Localization/rpg.glossary.csv` est la **table d'autorité** des termes de règle :
une seule traduction par terme, dans tout le jeu. Sans contrôle, cette promesse ne tient pas une
semaine — *saving throw* devient « jet de sauvegarde » dans la fiche, « JdS » dans le journal de
combat et « sauvegarde » dans l'infobulle, et le joueur croit à trois mécaniques différentes.

Ce script vérifie trois choses, et tourne **en CI** : le lexique est une donnée versionnée, les PDF
dont il sort ne le sont pas (`EX-CNT-023`). Le runner n'a donc rien à extraire.

0. **Le corpus source est bien exclu du dépôt.** La feuille de route affirme depuis son écriture
   que `Documentation/SourceBook/` est « exclu en entier par le `.gitignore` » — et la règle n'y
   était pas : elle vivait comme modification locale non commitée sur un seul poste. Sur un clone
   neuf, un `git add -A` aurait embarqué 280 Mo de PDF, dont un fichier de 83 Mo. Une affirmation
   que rien ne vérifie finit par devenir fausse ; celle-ci l'était déjà.

1. **Le lexique est bien formé** — trois colonnes, aucun terme vide, aucun couple
   (anglais, catégorie) en double, et les trois ensembles **fermés** au complet : huit écoles de
   magie, quinze conditions, treize types de dégâts. Ces trois nombres sont fixés par les règles,
   pas par le corpus ; s'ils bougent, c'est l'extraction qui a régressé, pas D&D qui a changé.

2. **Les clés de règle des catalogues respectent le lexique.** Une clé de règle est une clé de
   `fr.lang` / `en.lang` dont l'espace de noms figure dans `NAMESPACES_DE_REGLE` ci-dessous. Pour
   chacune, la valeur anglaise doit **être** un terme du lexique, et la valeur française doit être
   **une de ses traductions admises** (celles séparées par « / »).

   Le premier volet de cette règle est le plus exigeant, et c'est voulu : une clé de règle porte un
   *terme*, pas une phrase. « Vous êtes empoisonné » n'a rien à faire sous `condition.` ; le terme
   `poisoned` y a sa place, et il doit sortir « empoisonné » et rien d'autre.

**Aucune clé de règle n'existe encore** — les lots qui les créeront (`LOT-13` et suivants) ne sont
pas faits. Un contrôle sans entrée est un contrôle vert par vacuité, dont personne ne sait s'il
fonctionne : c'est exactement la panne du `LOT-78`, où une règle de lint contenait un caractère
invisible qui l'empêchait de jamais correspondre, sans que rien ne le signale. D'où l'**auto-test**,
exécuté à chaque appel et avant tout le reste : six catalogues fictifs — deux conformes, quatre
fautifs — que le contrôle doit classer correctement avant d'avoir le droit de se prononcer sur les
vrais. Si l'auto-test échoue, le script s'arrête là : un contrôle cassé ne rend pas de verdict.

Sortie : liste des violations, code de retour 1 si au moins une. Aucune dépendance externe.
"""
from __future__ import annotations

import sys
from pathlib import Path

RACINE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(RACINE / 'scripts'))

from sourcebook.glossaire import (  # noqa: E402  (après l'ajustement de sys.path)
    Entree, GlossaireError, lire_csv, normaliser, normaliser_cle,
)

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

LEXIQUE = RACINE / 'Source' / 'Elements' / 'Localization' / 'rpg.glossary.csv'
CATALOGUES = RACINE / 'Source' / 'Elements' / 'Localization'
GITIGNORE = RACINE / '.gitignore'

# Les chemins qu'`EX-CNT-023` interdit de versionner : le corpus source, et le cache intermédiaire
# de l'extraction. Chacun doit figurer tel quel dans le .gitignore.
EXCLUSIONS_REQUISES = ('Documentation/SourceBook/', '/.cache/')

# Les espaces de noms dont les valeurs sont des **termes de règle**, et non des phrases
# d'interface. Y ajouter un espace de noms revient à promettre que toutes ses valeurs sont des
# termes du lexique : ne l'y mettre que si c'est vrai.
NAMESPACES_DE_REGLE = (
    'condition.',
    # Ajoutes au LOT-38 : la fiche de personnage affiche les dix-huit competences et les six
    # caracteristiques, et ce sont des TERMES de regle -- « Escamotage », pas « Tour de main ».
    # Les ecrire une seconde fois dans un catalogue d'interface, sans lien avec le lexique, etait
    # exactement le defaut que ce controle existe pour empecher.
    'rpg.skill.',
    'rpg.ability.',
    'damage.',
    'school.',
    'weapon_property.',
    'ability.',
    'skill.',
)

# Ensembles fermés par les règles du jeu, pas par le corpus. Un écart signale une régression de
# l'extraction — ou un complément perdu — et jamais une évolution légitime.
ENSEMBLES_FERMES = {
    'école de magie': 8,
    'état': 15,
    'type de dégâts': 13,
}


def controler_exclusion_du_corpus() -> list[str]:
    """Le corpus et le cache d'extraction sont exclus du dépôt (`EX-CNT-023`)."""
    if not GITIGNORE.is_file():
        return ["`.gitignore` absent : rien n'exclut le corpus source (EX-CNT-023)."]
    lignes = {l.strip() for l in GITIGNORE.read_text(encoding='utf-8').splitlines()}
    return [
        "%s n'est pas exclu par le .gitignore. EX-CNT-023 l'interdit au dépôt : 280 Mo de "
        'binaires que git compresse mal et que chaque clone traînerait.' % chemin
        for chemin in EXCLUSIONS_REQUISES if chemin not in lignes
    ]


def lire_catalogue(chemin: Path) -> dict:
    """Un fichier ``.lang`` : ``clé = valeur``, ``#`` en commentaire, premier ``=`` séparateur."""
    valeurs = {}
    for ligne in chemin.read_text(encoding='utf-8').splitlines():
        ligne = ligne.strip()
        if not ligne or ligne.startswith('#') or '=' not in ligne:
            continue
        cle, valeur = ligne.split('=', 1)
        valeurs[cle.strip()] = valeur.strip()
    return valeurs


def forme_francaise(texte: str) -> str:
    """Forme de comparaison du **français** : casse ignorée, accents **significatifs**.

    ``normaliser_cle`` retire les accents, ce qui convient pour retrouver un terme anglais mais
    jamais pour valider une traduction française : il ferait passer « etourdi » pour « étourdi ».
    Une table d'autorité qui accepte les deux graphies n'impose plus rien.
    """
    return normaliser(texte).casefold()


def traductions_admises(entree: Entree) -> list[str]:
    """Les formes acceptables d'une entrée : « a / b » en admet deux, la première faisant foi."""
    return [forme_francaise(v) for v in entree.francais.split('/') if v.strip()]


def index_par_terme(lexique: list[Entree]) -> dict:
    """{terme anglais normalisé: [entrées]} — plusieurs si le terme est un homonyme."""
    index: dict = {}
    for entree in lexique:
        index.setdefault(normaliser_cle(entree.anglais), []).append(entree)
    return index


def controler_lexique(lexique: list[Entree]) -> list[str]:
    violations = []
    vus: dict = {}
    for entree in lexique:
        if not entree.anglais.strip() or not entree.francais.strip():
            violations.append('entrée incomplète : %r' % (entree,))
            continue
        cle = (normaliser_cle(entree.anglais), normaliser_cle(entree.categorie))
        if cle in vus:
            violations.append(
                '« %s » (%s) apparaît deux fois : « %s » et « %s ». Une table d\'autorité ne peut '
                'pas porter les deux.'
                % (entree.anglais, entree.categorie or 'sans catégorie',
                   vus[cle].francais, entree.francais))
        vus[cle] = entree

    for categorie, attendu in sorted(ENSEMBLES_FERMES.items()):
        reel = sum(1 for e in lexique if normaliser_cle(e.categorie) == normaliser_cle(categorie))
        if reel != attendu:
            violations.append(
                "%d entrée(s) de catégorie « %s », %d attendue(s). Cet ensemble est fermé par les "
                'règles du jeu : un écart est une régression de l\'extraction.'
                % (reel, categorie, attendu))
    return violations


def controler_catalogues(lexique: list[Entree], fr: dict, en: dict) -> list[str]:
    """Chaque clé de règle porte un terme du lexique, traduit comme le lexique le dit."""
    violations = []
    index = index_par_terme(lexique)
    for cle in sorted(fr):
        if not cle.startswith(NAMESPACES_DE_REGLE):
            continue
        if cle not in en:
            violations.append(
                "%s : clé de règle absente de en.lang. Le contrôle a besoin des deux faces — sans "
                'terme anglais, aucun rapprochement avec le lexique n\'est possible.' % cle)
            continue
        terme = normaliser_cle(en[cle])
        entrees = index.get(terme)
        if not entrees:
            violations.append(
                '%s : « %s » ne figure pas au lexique. Une clé de règle porte un terme, pas une '
                'phrase ; si c\'en est bien un, il manque au lexique.' % (cle, en[cle]))
            continue
        admises = {forme for entree in entrees for forme in traductions_admises(entree)}
        if forme_francaise(fr[cle]) not in admises:
            violations.append(
                '%s : « %s » traduit « %s », que le lexique traduit « %s ».'
                % (cle, fr[cle], en[cle],
                   ' » ou « '.join(sorted(e.francais for e in entrees))))
    return violations


# -- Auto-test ------------------------------------------------------------------------------

LEXIQUE_FICTIF = [
    Entree('poisoned', 'empoisonné', 'état'),
    Entree('saving throw', 'jet de sauvegarde', ''),
    Entree('incapacitated', "incapable d'agir / neutralisé", 'état'),
]

CAS = (
    ('conforme',
     {'condition.poisoned': 'empoisonné', 'condition.incapacitated': 'neutralisé'},
     {'condition.poisoned': 'poisoned', 'condition.incapacitated': 'incapacitated'},
     0),
    ('traduction contredisant le lexique',
     {'condition.poisoned': 'intoxiqué'},
     {'condition.poisoned': 'poisoned'},
     1),
    ('accent manquant dans la traduction française',
     {'condition.poisoned': 'empoisonne'},
     {'condition.poisoned': 'poisoned'},
     1),
    ('terme absent du lexique',
     {'condition.stunned': 'étourdi'},
     {'condition.stunned': 'stunned'},
     1),
    ('clé de règle sans face anglaise',
     {'condition.poisoned': 'empoisonné'},
     {},
     1),
    ("hors espace de noms de règle : non contrôlé",
     {'menu.quit': 'Quitter'},
     {'menu.quit': 'Quit'},
     0),
)


def auto_test() -> list[str]:
    """Éprouve le contrôle sur des catalogues fictifs, avant de le lancer sur les vrais.

    Sans cela, ce script serait vert parce qu'aucune clé de règle n'existe encore — et il le
    resterait s'il était cassé.
    """
    echecs = []
    for nom, fr, en, attendu in CAS:
        obtenu = len(controler_catalogues(LEXIQUE_FICTIF, fr, en))
        if obtenu != attendu:
            echecs.append("auto-test « %s » : %d violation(s), %d attendue(s)"
                          % (nom, obtenu, attendu))
    # La variante conforme doit aussi accepter la SECONDE traduction admise (« neutralisé »),
    # sans quoi le « / » du lexique ne servirait à rien.
    if controler_catalogues(LEXIQUE_FICTIF,
                            {'condition.incapacitated': "incapable d'agir"},
                            {'condition.incapacitated': 'incapacitated'}):
        echecs.append('auto-test : la première traduction admise est refusée')
    return echecs


def main() -> int:
    violations = auto_test()
    if violations:
        print('check_glossary : AUTO-TEST EN ÉCHEC — le contrôle lui-même est cassé.\n')
        for v in violations:
            print('  - ' + v)
        return 1

    if not LEXIQUE.is_file():
        print('check_glossary : %s absent. Le produire :\n'
              '    python scripts/sourcebook glossaire' % LEXIQUE)
        return 1

    try:
        lexique = lire_csv(LEXIQUE.read_text(encoding='utf-8'))
    except GlossaireError as erreur:
        print('check_glossary : %s' % erreur)
        return 1

    violations = controler_exclusion_du_corpus()
    violations += controler_lexique(lexique)
    fr = lire_catalogue(CATALOGUES / 'fr.lang')
    en = lire_catalogue(CATALOGUES / 'en.lang')
    violations += controler_catalogues(lexique, fr, en)

    cles_de_regle = sum(1 for c in fr if c.startswith(NAMESPACES_DE_REGLE))
    if violations:
        print('check_glossary : %d violation(s)\n' % len(violations))
        for v in violations:
            print('  - ' + v)
        return 1

    print('check_glossary : OK (%d termes au lexique, %d clé(s) de règle contrôlée(s), '
          'auto-test vert).' % (len(lexique), cles_de_regle))
    return 0


if __name__ == '__main__':
    sys.exit(main())
