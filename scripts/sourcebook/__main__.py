#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Ligne de commande de la chaîne d'extraction du corpus (LOT-30).

    python scripts/sourcebook info
    python scripts/sourcebook verifier
    python scripts/sourcebook texte basic-rules --page-imprimee 48
    python scripts/sourcebook tableau basic-rules --page-imprimee 48 --region 40,270,560,760 \\
                                                  --bornes 140,215,260,300
    python scripts/sourcebook image tanares-sourcebook --page 50 --moitie gauche -o carte.png
    python scripts/sourcebook regions tanares-sourcebook --page 50
    python scripts/sourcebook glossaire

**Rien ici ne tourne en intégration continue** : les PDF ne sont pas sur le runner, et ne le seront
pas (`EX-CNT-023`). Ce sont les **données produites** qui sont versionnées et validées en CI. Cet
outil se rejoue à la demande, sur un poste qui a le corpus.

Toute commande commence par **vérifier l'empreinte** du document. C'est délibérément non
contournable : une extraction menée sur une autre édition produit des données décalées et muettes,
ce qu'`EX-CNT-020` interdit. Le seul cas où ce contrôle gêne — relever une empreinte qui a
légitimement changé — a sa commande dédiée, ``verifier --regenerer``.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

if __package__ in (None, ''):  # pragma: no cover - dépend du mode d'invocation
    # Lancé par « python scripts/sourcebook », l'interpréteur met scripts/sourcebook/ en tête de
    # sys.path, pas scripts/ : les imports relatifs du paquet n'ont alors aucun paquet parent.
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    from sourcebook.corpus import Corpus, CorpusError
    from sourcebook.extraction import PPP_DEFAUT, ExtractionError, Extracteur
    from sourcebook import glossaire as mod_glossaire
    from sourcebook import options as mod_options
    from sourcebook import bestiaire as mod_bestiaire
    from sourcebook import personnage as mod_personnage
    from sourcebook import equipement as mod_equipement
    from sourcebook import atlas as mod_atlas
else:
    from .corpus import Corpus, CorpusError
    from .extraction import PPP_DEFAUT, ExtractionError, Extracteur
    from . import glossaire as mod_glossaire
    from . import options as mod_options
    from . import bestiaire as mod_bestiaire
    from . import personnage as mod_personnage
    from . import equipement as mod_equipement
    from . import atlas as mod_atlas

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')

RACINE = Path(__file__).resolve().parent.parent.parent


def quadruplet(valeur: str) -> tuple:
    """Type argparse d'une région `x0,y0,x1,y1` : quatre flottants, ni plus ni moins."""
    morceaux = valeur.split(',')
    if len(morceaux) != 4:
        raise argparse.ArgumentTypeError('région attendue sous la forme x0,y0,x1,y1')
    return tuple(float(m) for m in morceaux)


def flottants(valeur: str) -> list:
    """Type argparse d'une liste de flottants séparés par des virgules."""
    return [float(m) for m in valeur.split(',') if m.strip()]


def analyser(argv) -> argparse.Namespace:
    """Le parseur de la ligne de commande, appliqué à `argv`."""
    # Les deux options globales sont déclarées dans un parseur parent, hérité par chaque
    # sous-commande : sans cela, argparse ne les accepte qu'AVANT le nom de la commande, et
    # « sourcebook info --corpus-root X » échoue là où « sourcebook --corpus-root X info »
    # fonctionne. La différence n'est visible dans aucun message d'aide.
    commun = argparse.ArgumentParser(add_help=False)
    commun.add_argument('--corpus-root', type=Path, default=None,
                        help='dossier des PDF (défaut : Documentation/SourceBook/)')
    commun.add_argument('--cache', type=Path, default=None,
                        help='dossier de cache disque, indexé par empreinte de document')

    parseur = argparse.ArgumentParser(
        prog='sourcebook', description=__doc__, parents=[commun],
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sous = parseur.add_subparsers(dest='commande', required=True)

    def commande(nom, aide):
        return sous.add_parser(nom, help=aide, parents=[commun])

    commande('info', 'les documents du manifeste et leur pagination')

    p = commande('verifier', "vérifier les empreintes (EX-CNT-020)")
    p.add_argument('document', nargs='?', help='une clé du manifeste ; tous par défaut')
    p.add_argument('--regenerer', action='store_true',
                   help='afficher les empreintes réelles, à reporter au manifeste')

    def page(sp, moitie=True):
        sp.add_argument('document')
        groupe = sp.add_mutually_exclusive_group(required=True)
        groupe.add_argument('--page', type=int, help='index de page PDF, 0-indexé')
        groupe.add_argument('--page-imprimee', type=int, help='numéro de page imprimé sur le livre')
        sp.add_argument('--region', type=quadruplet, help='x0,y0,x1,y1 en points PDF')
        if moitie:
            sp.add_argument('--moitie', choices=('gauche', 'droite'),
                            help='une moitié de page (livres en double page)')
        return sp

    p = page(commande('texte', 'texte d\'une page ou d\'une région'))
    p.add_argument('--brut', action='store_true',
                   help="ordre du document plutôt que par coordonnée (pages à deux colonnes)")

    p = page(commande('tableau', 'tableau, par regroupement de coordonnées'))
    p.add_argument('--bornes', type=flottants,
                   help='abscisses imposées des séparations de colonnes')
    p.add_argument('--ecart', type=float, default=None,
                   help='écart minimal entre deux colonnes, en points (défaut : 6)')

    p = page(commande('image', 'PNG d\'une région, par rendu (EX-CNT-022)'))
    p.add_argument('--ppp', type=int, default=PPP_DEFAUT)
    p.add_argument('-o', '--sortie', type=Path, required=True)

    page(commande('regions', 'régions à illustration proposées à l\'humain'))

    p = commande('stats', 'pages, caractères et images par document')
    p.add_argument('document', nargs='?')

    p = commande('options', "produire les catalogues d'options de personnage (LOT-43)")
    p.add_argument('-o', '--sortie', type=Path, default=None,
                   help='défaut : ' + mod_options.SORTIE_RPG)

    p = commande('bestiaire', "produire les 94 betes d'Animaux.pdf (LOT-33)")
    p.add_argument('-o', '--sortie', type=Path, default=None,
                   help='défaut : ' + mod_bestiaire.SORTIE_RPG)

    p = commande('personnage', 'produire especes, historiques et classes (LOT-36)')
    p.add_argument('-o', '--sortie', type=Path, default=None,
                   help='défaut : ' + mod_personnage.SORTIE_RPG)

    p = commande('equipement', 'produire armes et armures (LOT-34)')
    p.add_argument('-o', '--sortie', type=Path, default=None,
                   help='défaut : ' + mod_equipement.SORTIE_RPG)

    p = commande('atlas', "produire les régions et les lieux de Tanares (LOT-37)")
    p.add_argument('-o', '--sortie', type=Path, default=None,
                   help='défaut : ' + mod_atlas.SORTIE_MONDE)

    p = commande('glossaire', 'produire le lexique bilingue')
    p.add_argument('-o', '--sortie', type=Path, default=None,
                   help='défaut : ' + mod_glossaire.SORTIE)
    p.add_argument('--verifier', action='store_true',
                   help="ne rien écrire ; échouer si le fichier versionné n'est plus à jour")

    return parseur.parse_args(argv)


def index_de(document, args) -> int:
    """L'index PDF que visent `--page` ou `--page-imprimee`."""
    if args.page is not None:
        return args.page
    return document.index_pdf(args.page_imprimee)


def commande_info(corpus: Corpus) -> int:
    """`info` : la table des documents du manifeste, avec leur pagination."""
    print('%-22s %-46s %6s  %-9s %-8s' % ('clé', 'fichier', 'pages', 'pagination', 'provenance'))
    for document in sorted(corpus, key=lambda d: d.cle):
        pagination = document.pagination
        if document.pagination != 'aucune':
            imprimees = document.pages_imprimees(0)
            pagination += ' (p0 → %s)' % '/'.join(str(p) for p in imprimees)
        marque = ' [hors périmètre]' if document.hors_perimetre else ''
        print('%-22s %-46s %6d  %-9s %-8s%s'
              % (document.cle, document.fichier, document.pages, pagination,
                 document.provenance, marque))
    return 0


def commande_verifier(corpus: Corpus, args) -> int:
    """`verifier` : les empreintes contre le manifeste, ou leur relevé (`--regenerer`)."""
    documents = [corpus[args.document]] if args.document else sorted(corpus, key=lambda d: d.cle)
    echecs = 0
    for document in documents:
        if args.regenerer:
            if not document.existe():
                print('%-22s ABSENT   %s' % (document.cle, document.chemin))
                echecs += 1
                continue
            print('%-22s sha256 = "%s"' % (document.cle, document.empreinte_reelle()))
            continue
        try:
            document.verifier()
        except CorpusError as erreur:
            print('%s' % erreur)
            echecs += 1
        else:
            print('%-22s OK' % document.cle)
    if echecs:
        print('\n%d document(s) ne correspondent pas au manifeste (EX-CNT-020).' % echecs)
    return 1 if echecs else 0


def commande_stats(corpus: Corpus, args) -> int:
    """`stats` : pages, caractères et images de chaque document."""
    documents = [corpus[args.document]] if args.document else sorted(corpus, key=lambda d: d.cle)
    for document in documents:
        with Extracteur(document) as extracteur:
            s = extracteur.statistiques()
        print('%-22s %4d pages  %8d caractères (%5d/page)  %5d images dont %5d ≥ 512²'
              % (document.cle, s['pages'], s['caracteres'], s['caracteres_par_page'],
                 s['images'], s['images_512']))
    return 0


def commande_glossaire(corpus: Corpus, args) -> int:
    """`glossaire` : écrit le lexique bilingue, ou vérifie le versionné (`--verifier`)."""
    entrees = mod_glossaire.construire(corpus, cache=args.cache)
    contenu = mod_glossaire.ecrire_csv(entrees)
    chemin = args.sortie or (RACINE / mod_glossaire.SORTIE)

    stats = mod_glossaire.statistiques(entrees)
    fermees = ('école de magie', 'état', 'type de dégâts', 'propriété')
    resume = ', '.join('%s : %d' % (c, stats['categories'].get(c, 0)) for c in fermees)
    print('%d entrées — %s' % (stats['entrees'], resume))

    if args.verifier:
        if not chemin.is_file():
            print('%s : absent, alors que le lexique est une donnée versionnée.' % chemin)
            return 1
        if chemin.read_text(encoding='utf-8') != contenu:
            print("%s n'est plus ce que le corpus produit. Régénérer :\n"
                  '    python scripts/sourcebook glossaire' % chemin)
            return 1
        print('%s : à jour.' % chemin)
        return 0

    chemin.parent.mkdir(parents=True, exist_ok=True)
    chemin.write_text(contenu, encoding='utf-8')
    print('écrit : %s' % chemin)
    return 0


def commande_options(corpus: Corpus, args) -> int:
    """`options` : produit les quatre catalogues d'options de personnage (LOT-43)."""
    lexique = mod_glossaire.lire_csv(
        (RACINE / mod_glossaire.SORTIE).read_text(encoding='utf-8'))
    racine = args.sortie or (RACINE / mod_options.SORTIE_RPG)
    ecrits, pertes = mod_options.produire(corpus, lexique, racine, cache=args.cache)
    print('%d fichier(s) écrit(s) sous %s' % (len(ecrits), racine))
    if pertes:
        # Ce n'est pas une faute : c'est le rapport du recoupement, et il doit se voir. Le jour
        # où ce nombre change, le corpus ou la région d'extraction a bougé.
        print("%d cellule(s) de la table du multiclassage escamotées par l'OCR du Manuel des "
              'Joueurs, rétablies depuis la progression du magicien des Basic Rules :'
              % len(pertes))
        for perte in pertes[:5]:
            print('  · ' + perte)
        if len(pertes) > 5:
            print('  · … et %d autres' % (len(pertes) - 5))
    return 0


def commande_bestiaire(corpus: Corpus, args) -> int:
    """`bestiaire` : produit les créatures d'`Animaux.pdf` (LOT-33)."""
    lexique = mod_glossaire.lire_csv(
        (RACINE / mod_glossaire.SORTIE).read_text(encoding='utf-8'))
    racine = args.sortie or (RACINE / mod_bestiaire.SORTIE_RPG)
    ecrits, signalements = mod_bestiaire.produire(corpus, lexique, racine, cache=args.cache)
    print('%d créature(s) écrite(s) sous %s' % (len(ecrits), racine / mod_bestiaire.DOSSIER))
    for signalement in signalements:
        # Ce n'est pas une faute : c'est ce que le livre dit et que le catalogue ne sait pas
        # porter. Le taire reviendrait à choisir à la place du lecteur.
        print('  · ' + signalement)
    return 0


def commande_personnage(corpus: Corpus, args) -> int:
    """`personnage` : produit espèces, historiques et classes (LOT-36)."""
    lexique = mod_glossaire.lire_csv(
        (RACINE / mod_glossaire.SORTIE).read_text(encoding='utf-8'))
    racine = args.sortie or (RACINE / mod_personnage.SORTIE_RPG)
    ecrits, signalements = mod_personnage.produire(corpus, lexique, racine, cache=args.cache)
    print('%d fichier(s) écrit(s) sous %s' % (len(ecrits), racine))
    for signalement in signalements:
        # Ce n'est pas une faute : c'est ce que le livre dit et que le catalogue ne sait pas
        # porter, ou ce que l'OCR a escamote. Le taire reviendrait a choisir a la place du lecteur.
        print('  · ' + signalement)
    return 0


def commande_equipement(corpus: Corpus, args) -> int:
    """`equipement` : produit armes, armures et équipement d'aventurier (LOT-36)."""
    lexique = mod_glossaire.lire_csv(
        (RACINE / mod_glossaire.SORTIE).read_text(encoding='utf-8'))
    racine = args.sortie or (RACINE / mod_equipement.SORTIE_RPG)
    ecrits, signalements = mod_equipement.produire(corpus, lexique, racine, cache=args.cache)
    print('%d fichier(s) écrit(s) sous %s' % (len(ecrits), racine))
    for signalement in signalements:
        # Une case que le livre laisse vide n'est pas une faute d'extraction : c'est une case vide,
        # et la taire reviendrait a choisir a la place du lecteur.
        print('  · ' + signalement)
    return 0


def commande_atlas(corpus: Corpus, args) -> int:
    """`atlas` : produit les régions et les lieux de Tanares (LOT-37)."""
    racine = args.sortie or (RACINE / mod_atlas.SORTIE_MONDE)
    compte = mod_atlas.produire(corpus, racine, cache=args.cache)
    print('%d région(s) et %d lieu(x) écrits sous %s (%d espèce(s) citée(s))'
          % (compte['regions'], compte['lieux'], racine, compte['especes']))
    return 0


def main(argv=None) -> int:
    args = analyser(sys.argv[1:] if argv is None else argv)
    try:
        corpus = Corpus.charger(racine_corpus=args.corpus_root)

        if args.commande == 'info':
            return commande_info(corpus)
        if args.commande == 'verifier':
            return commande_verifier(corpus, args)
        if args.commande == 'stats':
            return commande_stats(corpus, args)
        if args.commande == 'glossaire':
            return commande_glossaire(corpus, args)
        if args.commande == 'options':
            return commande_options(corpus, args)
        if args.commande == 'bestiaire':
            return commande_bestiaire(corpus, args)
        if args.commande == 'personnage':
            return commande_personnage(corpus, args)
        if args.commande == 'equipement':
            return commande_equipement(corpus, args)
        if args.commande == 'atlas':
            return commande_atlas(corpus, args)

        document = corpus[args.document]
        with Extracteur(document, cache=args.cache) as extracteur:
            index = index_de(document, args)
            moitie = getattr(args, 'moitie', None)

            if args.commande == 'texte':
                print(extracteur.texte(index, moitie, args.region, tri=not args.brut))
            elif args.commande == 'tableau':
                lignes = extracteur.tableau(
                    index, moitie, args.region,
                    **({'ecart_colonne': args.ecart} if args.ecart else {}),
                    bornes=args.bornes)
                for ligne in lignes:
                    print(' | '.join(ligne))
            elif args.commande == 'image':
                donnees = extracteur.image(index, args.region, moitie, args.ppp)
                args.sortie.parent.mkdir(parents=True, exist_ok=True)
                args.sortie.write_bytes(donnees)
                print('écrit : %s (%d octets, %d ppp)'
                      % (args.sortie, len(donnees), args.ppp))
            elif args.commande == 'regions':
                candidates = extracteur.regions_candidates(index, moitie)
                imprimees = document.pages_imprimees(index)
                print('%s page PDF %d%s : %d région(s) candidate(s)'
                      % (document.cle, index,
                         ' (imprimée %s)' % '/'.join(str(p) for p in imprimees)
                         if imprimees else '', len(candidates)))
                for region in candidates:
                    print('  ' + str(region))
    except (CorpusError, ExtractionError, mod_glossaire.GlossaireError,
            mod_options.OptionsError, mod_bestiaire.BestiaireError,
            mod_personnage.PersonnageError,
            mod_equipement.EquipementError, mod_atlas.AtlasError) as erreur:
        print('%s' % erreur, file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
