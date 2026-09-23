#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Validation des catalogues RPG contre leurs schémas (LOT-32).

Le contrat avant les données. `Source/Elements/Rpg/schema/` porte un JSON Schema par famille —
créature, objet, arme, armure, sort, espèce, classe, historique, état, type de dégâts — et ce
script valide contre eux **tout** `Source/Elements/Rpg/**/*.json`, en intégration continue.

Il vérifie cinq choses :

1. **Les schémas eux-mêmes sont valides.** Un schéma mal formé n'échoue pas : il *accepte tout*.
   C'est la panne la plus traître de cette famille d'outils, parce qu'elle se présente comme un
   succès.

2. **Chaque donnée respecte le schéma de sa famille**, déduite de son dossier. Un échec nomme le
   **fichier et la ligne** (`EX-CNT-010`) : « donnée invalide » sur un catalogue de mille entrées
   ne se corrige pas, « `wolf.json:14` » si.

3. **Les énumérations fermées des schémas coïncident avec le lexique** du `LOT-30` — les treize
   types de dégâts, les quinze états, les huit écoles de magie. Le C++ est relié à ces mêmes
   énumérations par un test (`test_rpg_enums.cpp`, `EX-CNT-011`), si bien que les trois — moteur,
   contrat, table de traduction — disent le même mot pour la même chose, ou la CI échoue. Chaque
   arête du triangle est contrôlée une fois, aucune deux fois.

4. **Les données provisoires sont énumérées** (`EX-CNT-032`). Ce n'est pas une faute : c'est un
   état d'avancement consultable. Le schéma exige en revanche que chacune écrive **d'avance** son
   critère de retrait — une donnée provisoire non marquée devient permanente par accident.

5. **Le script s'auto-teste** sur `scripts/fixtures/rpg/`, avant de se prononcer sur les vraies
   données. Il n'y en a aucune aujourd'hui : les catalogues arrivent du `LOT-33` au `LOT-84`. Un
   validateur qui n'a rien validé est un validateur dont on ne sait rien — c'est la panne du
   `LOT-78`, et la même parade que `check_glossary.py`.

Dépendance : **jsonschema** (`pip install jsonschema`), installé par le workflow de CI.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

RACINE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(RACINE / 'scripts'))

from sourcebook.glossaire import lire_csv, normaliser_cle  # noqa: E402

try:
    import jsonschema
    from referencing import Registry, Resource
except ImportError as erreur:  # pragma: no cover - dépend de l'environnement
    print('check_rpg_data : jsonschema est requis — pip install jsonschema (%s).' % erreur)
    sys.exit(1)

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

RPG = RACINE / 'Source' / 'Elements' / 'Rpg'
# L'atlas du LOT-37 vit a cote des catalogues RPG, mais partage LEUR dossier de schemas : le socle
# commun (`source`, `status`, `id`) y est deja, et un second dossier de contrats en donnerait deux
# versions qui divergeraient. Une region n'est pas une regle de jeu, son contrat l'est.
MONDE = RACINE / 'Source' / 'Elements' / 'World'
SCHEMAS = RPG / 'schema'
LEXIQUE = RACINE / 'Source' / 'Elements' / 'Localization' / 'rpg.glossary.csv'
FIXTURES = RACINE / 'scripts' / 'fixtures' / 'rpg'

# Dossier de données → famille. Un dossier absent de cette table et contenant du JSON est une
# erreur : la famille se déclare ici, elle ne se devine pas d'un nom de fichier.
FAMILLES = {
    'creatures': 'creature',
    'items': 'item',
    'weapons': 'weapon',
    'armors': 'armor',
    'spells': 'spell',
    'species': 'species',
    'classes': 'class',
    'backgrounds': 'background',
    'conditions': 'condition',
    'damage-types': 'damage-type',
    # Options de personnage (LOT-43).
    'skills': 'skill',
    'languages': 'language',
    'feats': 'feat',
    # Atlas (LOT-37).
    'regions': 'region',
    # Le Colisee (LOT-50) : les arenes sont des donnees de lieu, une par variante regionale.
    'arena': 'arena',
    'locations': 'location',
    # Fiches de personnage (LOT-38). Seule famille de ce dossier a decrire une INSTANCE et non un
    # catalogue : une espece est un modele dont on tire des personnages, une fiche EST un
    # personnage. Elle passe par le meme controle que les autres -- une fiche de demonstration mal
    # formee est un ecran vide qu'on met une heure a expliquer.
    'characters': 'character',
    'encounters': 'encounter',
    # Dialogues (LOT-15), sous World/ : une conversation est un contenu de lieu. Le schema dit la
    # forme ; les cibles, les cycles et les impasses se refusent au chargement (core::readDialogue).
    'dialogues': 'dialogue',
    # Villes jouables (LOT-96), sous World/ : le graphe des quartiers d'une ville, et la porte ou
    # << Nouvelle partie >> pose le heros.
    'cities': 'city',
}

# `rules/` porte des REGLES, pas une collection d'entrees semblables : chaque fichier y a son
# propre schema, resolu par son nom. Un dossier par regle serait un dossier a un fichier.
REGLES = {
    'multiclassing': 'multiclassing',
    'difficulty': 'difficulty',
    'experience': 'experience',
    'character-creation': 'character-creation',
    'encumbrance': 'encumbrance',
    'heroic-marks': 'heroic-marks',
    # L'IA tactique (LOT-23) : les poids des profils sont des donnees (EX-VIS-007).
    'behaviors': 'behaviors',
}

# Énumération fermée d'un schéma ↔ catégorie du lexique. `equivalence` exige l'égalité des deux
# ensembles ; `inclusion` exige seulement que le schéma soit couvert par le lexique — c'est le cas
# des propriétés d'arme, dont la catégorie du glossaire porte aussi « armes de guerre » et
# « armes improvisées », qui ne sont pas des propriétés.
CORRESPONDANCES = (
    ('common.schema.json', 'damageType', 'type de dégâts', 'equivalence'),
    ('common.schema.json', 'conditionRef', 'état', 'equivalence'),
    ('common.schema.json', 'magicSchool', 'école de magie', 'equivalence'),
    ('weapon.schema.json', None, 'propriété', 'inclusion'),
)


def charger_schemas() -> tuple[dict, list[str]]:
    """Tous les schémas du dossier, et les violations de leur propre validité."""
    schemas: dict = {}
    violations = []
    for chemin in sorted(SCHEMAS.glob('*.schema.json')):
        try:
            contenu = json.loads(chemin.read_text(encoding='utf-8'))
        except json.JSONDecodeError as erreur:
            violations.append('%s:%d:%d : JSON mal formé — %s'
                              % (chemin.name, erreur.lineno, erreur.colno, erreur.msg))
            continue
        try:
            jsonschema.Draft202012Validator.check_schema(contenu)
        except jsonschema.SchemaError as erreur:
            violations.append('%s : schéma invalide — %s. Un schéma mal formé n\'échoue pas, il '
                              'accepte tout.' % (chemin.name, erreur.message))
            continue
        schemas[chemin.name] = contenu
    if not schemas and not violations:
        violations.append('%s : aucun schéma. Le contrat précède les données ; sans lui, ce '
                          'contrôle ne contrôle rien.' % SCHEMAS)
    return schemas, violations


def validateur(schemas: dict, famille: str):
    """Un validateur pour une famille, capable de résoudre les `$ref` vers `common.schema.json`.

    Le registre est construit depuis les schémas déjà chargés, sans accès réseau : les `$id` sont
    des URI d'espace de noms, pas des adresses à résoudre. Un validateur qui tenterait de les
    télécharger échouerait sur un runner sans réseau — ou pire, réussirait en récupérant autre
    chose.
    """
    registre = Registry().with_resources(
        [(s['$id'], Resource.from_contents(s)) for s in schemas.values() if '$id' in s])
    return jsonschema.Draft202012Validator(
        schemas['%s.schema.json' % famille], registry=registre)


def ligne_du_pointeur(texte: str, chemin) -> int:
    """Ligne approximative d'un chemin d'erreur JSON, pour que le message situe la faute.

    `jsonschema` rapporte un **chemin logique** (`actions/0/damageType`), pas une position. On le
    retrouve dans le texte brut en cherchant chaque clé successivement, sans jamais revenir en
    arrière. C'est exact sur une donnée mise en forme normalement, et au pire pessimiste d'une
    poignée de lignes sur un fichier écrit en une seule ligne — dans les deux cas très au-dessus de
    « donnée invalide », qui est ce que ce message remplace.
    """
    position = 0
    for segment in chemin:
        if isinstance(segment, int):
            continue
        trouve = texte.find('"%s"' % segment, position)
        if trouve < 0:
            break
        position = trouve
    return texte.count('\n', 0, position) + 1


def valider_dossier(schemas: dict, racine: Path) -> tuple[list[str], list[str], int]:
    """Valide les données d'une arborescence. Renvoie (violations, provisoires, fichiers lus)."""
    violations: list[str] = []
    provisoires: list[str] = []
    lus = 0
    for chemin in sorted(racine.rglob('*.json')):
        if SCHEMAS in chemin.parents or chemin.parent == SCHEMAS:
            continue
        famille = (REGLES.get(chemin.stem) if chemin.parent.name == 'rules'
                   else FAMILLES.get(chemin.parent.name))
        if famille is None:
            violations.append(
                '%s : dossier « %s » sans famille déclarée. La famille se déclare dans '
                'FAMILLES, elle ne se devine pas d\'un nom de fichier.'
                % (chemin.relative_to(RACINE).as_posix(), chemin.parent.name))
            continue
        if '%s.schema.json' % famille not in schemas:
            violations.append('%s : la famille « %s » n\'a pas de schéma.'
                              % (chemin.relative_to(RACINE).as_posix(), famille))
            continue

        texte = chemin.read_text(encoding='utf-8')
        try:
            donnee = json.loads(texte)
        except json.JSONDecodeError as erreur:
            violations.append('%s:%d:%d : JSON mal formé — %s'
                              % (chemin.relative_to(RACINE).as_posix(), erreur.lineno,
                                 erreur.colno, erreur.msg))
            continue
        lus += 1

        # Dedupliquees : le socle commun est applique par `allOf` ET repris dans le `required`
        # de chaque famille, si bien qu'un champ obligatoire manquant se signale deux fois. Deux
        # lignes identiques pour une seule faute font douter qu'il n'y en ait qu'une.
        deja_vues = set()
        for erreur in sorted(validateur(schemas, famille).iter_errors(donnee),
                             key=lambda e: (list(e.absolute_path), e.message)):
            message = (
                '%s:%d : %s%s'
                % (chemin.relative_to(RACINE).as_posix(),
                   ligne_du_pointeur(texte, erreur.absolute_path),
                   ('/'.join(str(p) for p in erreur.absolute_path) + ' : ')
                   if erreur.absolute_path else '',
                   erreur.message))
            if message not in deja_vues:
                deja_vues.add(message)
                violations.append(message)

        statut = donnee.get('status') if isinstance(donnee, dict) else None
        if isinstance(statut, dict) and statut.get('provisoire'):
            provisoires.append('%s — %s (retrait si : %s)'
                               % (chemin.relative_to(RACINE).as_posix(),
                                  statut.get('raison', '?'), statut.get('retraitSi', '?')))
    return violations, provisoires, lus


def controler_references(racine: Path) -> list[str]:
    """Toute langue citée par une créature ou une espèce existe au catalogue (`LOT-43`).

    Le schéma vérifie qu'une langue est une *chaîne* ; il ne peut pas vérifier qu'elle **existe**.
    Sans ce contrôle, une créature déclarant parler le « draconien » — pour « draconique » —
    passerait, et le [LOT-15](@ref lot-15) refuserait un dialogue pour une langue qui n'existe
    pas, ce qui est indiscernable d'un bogue de dialogue.

    La comparaison se fait sur le nom **français** aussi bien que sur l'identifiant : les blocs de
    créature du corpus écrivent « nain », pas « dwarvish ».
    """
    dossier = racine / 'languages'
    if not dossier.is_dir():
        return []
    connues = set()
    for chemin in sorted(dossier.glob('*.json')):
        donnee = json.loads(chemin.read_text(encoding='utf-8'))
        connues.add(normaliser_cle(donnee.get('id', '')))
        connues.add(normaliser_cle(donnee.get('name', '')))

    violations = []
    for famille in ('creatures', 'species'):
        for chemin in sorted((racine / famille).glob('*.json')):
            donnee = json.loads(chemin.read_text(encoding='utf-8'))
            for citee in donnee.get('languages', []):
                if normaliser_cle(citee) not in connues:
                    violations.append(
                        "%s : la langue « %s » ne figure pas au catalogue. Une langue inventée "
                        'fait refuser un dialogue sans que rien ne dise pourquoi.'
                        % (chemin.relative_to(RACINE).as_posix(), citee))
    return violations


def controler_atlas(racine: Path) -> list[str]:
    """L'acceptation du LOT-37 (`EX-CNT-062`), verifiee sur les donnees ECRITES.

    Le module d'extraction controle deja ces trois points, mais il ne tourne qu'a la demande et
    exige les PDF du corpus, absents du depot. Les donnees, elles, sont versionnees : une edition
    a la main qui casserait le graphe ne rencontrerait sinon aucun garde-fou jusqu'au jeu.
    """
    regions_dir, lieux_dir = racine / 'regions', racine / 'locations'
    if not regions_dir.is_dir():
        return []
    regions = {}
    for chemin in sorted(regions_dir.glob('*.json')):
        donnee = json.loads(chemin.read_text(encoding='utf-8'))
        regions[donnee['id']] = donnee

    violations = []
    for chemin in sorted(lieux_dir.glob('*.json')):
        lieu = json.loads(chemin.read_text(encoding='utf-8'))
        if lieu['region'] not in regions:
            violations.append(
                "%s : region « %s » inconnue. Un lieu sans region n'est atteignable par aucun "
                'trajet.' % (chemin.relative_to(RACINE).as_posix(), lieu['region']))
        elif lieu['id'] not in regions[lieu['region']]['locations']:
            violations.append(
                '%s : la region « %s » ne le cite pas en retour.'
                % (chemin.relative_to(RACINE).as_posix(), lieu['region']))

    for identifiant, region in regions.items():
        for voisin in region['neighbors']:
            if voisin not in regions:
                violations.append('regions/%s.json : voisin « %s » inconnu.'
                                  % (identifiant, voisin))
            elif identifiant not in regions[voisin]['neighbors']:
                violations.append(
                    'regions/%s.json : voisinage non symetrique avec « %s ». Une arete a sens '
                    'unique laisse une region sans retour.' % (identifiant, voisin))

    if regions:
        depart = sorted(regions)[0]
        vus, pile = {depart}, [depart]
        while pile:
            for voisin in regions[pile.pop()]['neighbors']:
                if voisin in regions and voisin not in vus:
                    vus.add(voisin)
                    pile.append(voisin)
        isolees = sorted(set(regions) - vus)
        if isolees:
            violations.append(
                'regions : %s inatteignable(s) depuis « %s ». Une region injoignable est du '
                'contenu qui ne sera jamais vu.' % (', '.join(isolees), depart))
    return violations


def points_d_arrivee(carte: Path) -> set[str]:
    """Les noms des points d'arrivee d'une carte de niveau, vide si elle est illisible."""
    try:
        niveau = json.loads(carte.read_text(encoding='utf-8'))
    except (OSError, json.JSONDecodeError):
        return set()
    return {e.get('name', '') for e in niveau.get('entities', []) if e.get('type') == 'spawnPoint'}


def pnj_de(carte: Path) -> list[dict]:
    """Les entites `npc` d'une carte de niveau, vide si elle est illisible."""
    try:
        niveau = json.loads(carte.read_text(encoding='utf-8'))
    except (OSError, json.JSONDecodeError):
        return []
    return [e for e in niveau.get('entities', []) if e.get('type') == 'npc']


def controler_villes(racine: Path, niveaux: Path, plans: Path) -> list[str]:
    """Chaque quartier d'une ville jouable mene quelque part, et existe sur le plan (`LOT-96`).

    Le schema dit la forme ; il ne peut pas dire qu'une carte **existe**, qu'une fiche d'atlas est
    celle d'un quartier de CETTE ville, ni qu'un point du plan n'a ete oublie. Un quartier qui ne
    mene nulle part est un point du plan qu'aucun chemin n'atteint ; un quartier du plan absent de
    la ville, un point qu'on ne pourra jamais visiter.
    """
    dossier = racine / 'cities'
    if not dossier.is_dir():
        return []
    lieux = {c.stem for c in (racine / 'locations').glob('*.json')}
    plan_villes = json.loads(plans.read_text(encoding='utf-8')).get('cities', {})
    violations = []
    for chemin in sorted(dossier.glob('*.json')):
        nom = chemin.relative_to(RACINE).as_posix()
        ville = json.loads(chemin.read_text(encoding='utf-8'))
        lieu = ville.get('location', '')
        if lieu not in lieux:
            violations.append("%s : la ville « %s » n'a pas de fiche d'atlas." % (nom, lieu))
        places = set(plan_villes.get(lieu, {}).get('places', {}))
        if not places:
            violations.append('%s : aucun plan « %s » dans world-maps.json.' % (nom, lieu))

        cartes = {}
        for quartier in ville.get('districts', []):
            ident = quartier.get('id', '')
            if ident not in lieux:
                violations.append("%s : le quartier « %s » n'a pas de fiche d'atlas."
                                  % (nom, ident))
            if places and ident not in places:
                violations.append("%s : le quartier « %s » n'est pas place sur le plan de la "
                                  'ville.' % (nom, ident))
            if 'map' in quartier:
                cartes[ident] = quartier['map']
                if not (niveaux / (quartier['map'] + '.json')).is_file():
                    violations.append("%s : la carte « %s » du quartier « %s » n'existe pas."
                                      % (nom, quartier['map'], ident))
        dialogues = {c.stem for c in (racine / 'dialogues').glob('*.json')}
        for quartier in ville.get('districts', []):
            garde = quartier.get('guard')
            if garde and garde['map'] in cartes.values():
                # La porte gardee est une sentinelle posee sur la carte voisine : un PNJ qui
                # nomme le quartier qu'il ferme, et qui a un dialogue a tenir.
                sentinelles = [e for e in pnj_de(niveaux / (garde['map'] + '.json'))
                               if e.get('guards') == quartier.get('id')]
                if len(sentinelles) != 1:
                    violations.append(
                        "%s : la porte gardee de « %s » doit avoir une sentinelle sur « %s », "
                        "elle en a %d." % (nom, quartier.get('id', ''), garde['map'],
                                           len(sentinelles)))
                for sentinelle in sentinelles:
                    if sentinelle.get('dialogue') not in dialogues:
                        violations.append(
                            "%s : la sentinelle de « %s » ouvre le dialogue « %s », qui n'existe "
                            "pas." % (nom, quartier.get('id', ''), sentinelle.get('dialogue')))
            if garde and garde['map'] not in cartes.values():
                violations.append(
                    "%s : la porte gardee de « %s » se tient sur « %s », qui n'est la carte "
                    "d'aucun quartier de la ville." % (nom, quartier.get('id', ''), garde['map']))
        oublies = sorted(places - {q.get('id', '') for q in ville.get('districts', [])})
        if oublies:
            violations.append('%s : quartier(s) du plan absent(s) de la ville : %s.'
                              % (nom, ', '.join(oublies)))

        depart = ville.get('start', {})
        carte_depart = cartes.get(depart.get('district', ''))
        if carte_depart is None:
            violations.append("%s : le quartier de depart « %s » n'a pas de carte."
                              % (nom, depart.get('district', '')))
        elif depart.get('arrival') not in points_d_arrivee(niveaux / (carte_depart + '.json')):
            violations.append("%s : la carte « %s » n'a pas de point d'arrivee « %s »."
                              % (nom, carte_depart, depart.get('arrival')))
    return violations


def controler_enumerations(schemas: dict) -> list[str]:
    """Les énumérations fermées des schémas coïncident avec les catégories du lexique (LOT-30)."""
    if not LEXIQUE.is_file():
        return ['%s absent : les énumérations ne peuvent pas être confrontées au lexique.'
                % LEXIQUE.name]
    lexique = lire_csv(LEXIQUE.read_text(encoding='utf-8'))
    violations = []
    for fichier, definition, categorie, mode in CORRESPONDANCES:
        schema = schemas.get(fichier)
        if schema is None:
            violations.append('%s : absent, énumération « %s » non contrôlée.'
                              % (fichier, definition or 'properties'))
            continue
        if definition is not None:
            valeurs = set(schema['$defs'][definition]['enum'])
        else:
            valeurs = set(schema['properties']['properties']['items']['enum'])
        attendues = {normaliser_cle(e.anglais) for e in lexique
                     if normaliser_cle(e.categorie) == normaliser_cle(categorie)}
        obtenues = {normaliser_cle(v) for v in valeurs}

        inconnues = sorted(obtenues - attendues)
        if inconnues:
            violations.append(
                '%s / %s : %s ne figure(nt) pas au lexique sous « %s ». Le moteur nommerait une '
                'chose que la table de traduction ignore.'
                % (fichier, definition or 'properties', ', '.join(inconnues), categorie))
        if mode == 'equivalence':
            manquantes = sorted(attendues - obtenues)
            if manquantes:
                violations.append(
                    '%s / %s : le lexique porte %s sous « %s », que le schéma ne connaît pas. '
                    'Cet ensemble est fermé : les deux listes doivent être identiques.'
                    % (fichier, definition, ', '.join(manquantes), categorie))
    return violations


def auto_test(schemas: dict) -> list[str]:
    """Éprouve le validateur sur des fixtures, avant de se prononcer sur les vraies données.

    `scripts/fixtures/rpg/valide/` doit passer sans une violation ; chaque fichier de
    `invalide/` doit en produire **au moins une**. Un fichier de fixture qu'aucun contrôle ne
    rejette ne protège de rien — même discipline que les fixtures JSON du `LOT-79`.
    """
    if not FIXTURES.is_dir():
        return ['%s absent : le validateur ne serait éprouvé par rien.' % FIXTURES]
    echecs = []

    violations, _, lus = valider_dossier(schemas, FIXTURES / 'valide')
    if violations:
        echecs.append('auto-test : les fixtures valides sont refusées — %s' % violations[0])
    if lus == 0:
        echecs.append('auto-test : aucune fixture valide lue.')

    fichiers_invalides = sorted((FIXTURES / 'invalide').rglob('*.json'))
    if not fichiers_invalides:
        echecs.append('auto-test : aucune fixture invalide ; rien ne prouve que le validateur '
                      'refuse quoi que ce soit.')
    for chemin in fichiers_invalides:
        violations, _, _ = valider_dossier(schemas, chemin.parent)
        concernees = [v for v in violations if chemin.name in v]
        if not concernees:
            echecs.append('auto-test : %s est acceptée alors qu\'elle doit être refusée.'
                          % chemin.relative_to(FIXTURES).as_posix())
    return echecs


def main() -> int:
    schemas, violations = charger_schemas()
    if violations:
        print('check_rpg_data : %d schéma(s) en défaut\n' % len(violations))
        for v in violations:
            print('  - ' + v)
        return 1

    echecs = auto_test(schemas)
    if echecs:
        print('check_rpg_data : AUTO-TEST EN ÉCHEC — le validateur lui-même est cassé.\n')
        for e in echecs:
            print('  - ' + e)
        return 1

    violations = controler_enumerations(schemas)
    donnees, provisoires, lus = valider_dossier(schemas, RPG)
    violations += donnees
    violations += controler_references(RPG)

    monde, provisoires_monde, lus_monde = valider_dossier(schemas, MONDE)
    violations += monde
    violations += controler_atlas(MONDE)
    violations += controler_villes(MONDE, RACINE / 'Source' / 'Elements' / 'Levels',
                                   RACINE / 'Source' / 'Elements' / 'Maps' / 'world-maps.json')
    provisoires += provisoires_monde
    lus += lus_monde

    if provisoires:
        # Groupées par motif : quarante-deux lignes identiques ne se lisent pas, et la question
        # qu'on se pose devant cette liste est « combien, et pourquoi », pas « lesquelles ».
        par_motif: dict = {}
        for entree in provisoires:
            chemin, _, motif = entree.partition(' — ')
            par_motif.setdefault(motif, []).append(chemin)
        print("Données provisoires (EX-CNT-032) — état d'avancement, pas une faute :")
        for motif, chemins in sorted(par_motif.items()):
            print('  · %d entrée(s) : %s' % (len(chemins), motif))
            print('    %s%s' % (', '.join(chemins[:3]), ', …' if len(chemins) > 3 else ''))
        print()

    if violations:
        print('check_rpg_data : %d violation(s)\n' % len(violations))
        for v in violations:
            print('  - ' + v)
        return 1

    print('check_rpg_data : OK (%d schéma(s), %d donnée(s) validée(s), %d provisoire(s), '
          'auto-test vert).' % (len(schemas), lus, len(provisoires)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
