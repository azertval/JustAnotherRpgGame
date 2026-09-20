#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Contrôle des catalogues de traduction Qt (`.ts`) du jeu (refonte de la CI, phase 4).

Les écrans écrivent leurs textes en français dans le QML (`qsTr("Nouvelle partie")`) et
`jadg_en.ts` en porte la traduction. Rien ne vérifiait qu'une chaîne ajoutée à un écran était
traduite : `lupdate` l'ajoute en `unfinished`, `lrelease` la compile quand même, et le jeu en
anglais affiche du français sans que personne ne le voie. Ce contrôle refuse :

- une traduction **inachevée** (`type="unfinished"`, ou vide) ;
- une entrée **disparue** (`type="vanished"` ou `obsolete`) : sa source n'existe plus dans le code,
  elle ne sert qu'à faire croire le catalogue plus complet qu'il n'est ;
- des **marqueurs** différents entre source et traduction (`%1`, `%L2`, `%n`) : `arg()` remplirait
  le mauvais trou, ou aucun ;
- un **espace de bord** ajouté ou perdu : il sert presque toujours à coller deux textes à l'écran
  (sauf l'espace français avant « : ; ! ? » en tête de source, que l'anglais n'écrit pas) ;
- un catalogue sans attribut `language`, ou mal formé.

Deux usages, un même script :

- sur le catalogue **versionné** (job `lint-exigences`, sans Qt) ;
- après `cmake --build … --target update_translations` (job `build-ninja`) : `lupdate` a relu le
  code, donc une chaîne nouvelle y apparaît inachevée. C'est ce second passage qui prouve que le
  catalogue est **à jour du code**. (La cible passe `-no-obsolete` : une chaîne retirée du code
  disparaît du catalogue au lieu d'y rester en `vanished`.)

Usage :
  python scripts/check_translations.py              tous les .ts suivis par git
  python scripts/check_translations.py FICHIER.ts...
"""
import argparse
import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ElementTree

# %1 à %99, éventuellement localisés (%L1), et %n des formes plurielles.
PLACEHOLDER_RE = re.compile(r'%L?(?:[1-9][0-9]?)|%n')
DEAD_TYPES = {'vanished', 'obsolete'}


def placeholders(text):
    return sorted(PLACEHOLDER_RE.findall(text or ''))


# Typographie française : espace avant « : ; ! ? ». Une source qui commence par « : » la porte,
# que l'anglais n'écrit pas — ce n'est pas un espace perdu.
FRENCH_SPACE_BEFORE_PUNCTUATION_RE = re.compile(r'^\s+(?=[:;!?])')


def edges(text):
    """Espaces de début et de fin : le texte qu'on colle à un autre en dépend."""
    text = FRENCH_SPACE_BEFORE_PUNCTUATION_RE.sub('', text or '')
    return (text[:len(text) - len(text.lstrip())], text[len(text.rstrip()):])


def problems(raw, name='catalogue'):
    """Défauts du catalogue @p raw (texte XML) ; liste vide s'il est admis."""
    try:
        root = ElementTree.fromstring(raw)
    except ElementTree.ParseError as error:
        return ['%s : XML mal formé (%s)' % (name, error)]
    if root.tag != 'TS':
        return ['%s : racine <%s>, <TS> attendue' % (name, root.tag)]
    found = []
    if not root.get('language'):
        found.append('%s : attribut language absent de <TS>' % name)

    messages = 0
    for context in root.iter('context'):
        context_name = context.findtext('name') or '?'
        for message in context.iter('message'):
            messages += 1
            source = message.findtext('source') or ''
            where = '%s : [%s] « %s »' % (name, context_name, source)
            translation = message.find('translation')
            if translation is None:
                found.append('%s : pas de <translation>' % where)
                continue
            kind = translation.get('type')
            if kind in DEAD_TYPES:
                found.append('%s : entrée %s (sa source n\'existe plus dans le code ; '
                             'lupdate -no-obsolete la retire)' % (where, kind))
                continue
            if message.get('numerus') == 'yes':
                forms = [f.text or '' for f in translation.iter('numerusform')]
            else:
                forms = [translation.text or '']
            if kind == 'unfinished' or not forms or any(not f.strip() for f in forms):
                found.append('%s : traduction inachevée' % where)
                continue
            for form in forms:
                expected = placeholders(source)
                # Une forme plurielle peut écrire le nombre en toutes lettres (« one item ») : %n y
                # est facultatif, les autres marqueurs non.
                got = placeholders(form)
                if message.get('numerus') == 'yes':
                    expected = [p for p in expected if p != '%n']
                    got = [p for p in got if p != '%n']
                if got != expected:
                    found.append('%s : marqueurs %s dans la source, %s dans « %s »'
                                 % (where, expected or 'aucun', got or 'aucun', form))
                if edges(form) != edges(source):
                    found.append('%s : espaces de bord différents dans « %s »' % (where, form))
    if messages == 0:
        found.append('%s : aucun message ; le catalogue est vide ou sa lecture est cassée' % name)
    return found


def tracked_catalogues():
    listing = subprocess.run(['git', 'ls-files', '-z', '*.ts'], capture_output=True, check=True)
    return [p for p in listing.stdout.decode('utf-8').split('\0') if p]


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('files', nargs='*', help='catalogues .ts (défaut : tous ceux suivis)')
    arguments = parser.parse_args()

    if arguments.files:
        paths = arguments.files
    else:
        os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        paths = tracked_catalogues()
    if not paths:
        print('ERREUR : aucun catalogue .ts trouvé ; la recherche est cassée.')
        return 1

    errors = []
    for path in paths:
        with open(path, encoding='utf-8') as handle:
            errors.extend(problems(handle.read(), path.replace('\\', '/')))
    for message in errors:
        print('ERREUR : ' + message)
    if errors:
        print('\n%d défaut(s). Mettre le catalogue à jour du code : cmake --build --preset ninja '
              '--target update_translations, puis traduire dans Qt Linguist.' % len(errors))
        return 1
    print('OK : %d catalogue(s), toutes les traductions achevées et cohérentes.' % len(paths))
    return 0


if __name__ == '__main__':
    sys.exit(main())
