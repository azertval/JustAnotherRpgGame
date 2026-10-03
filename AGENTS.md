# Consignes persistantes du projet

## Lots d’assets 3D

Avant de produire ou retoucher un lot d’assets, lire `Planning/standards/style-3d.md` — notamment « Critères de qualité validés par l’auteur » et « Ce qui reste ouvert » — et, pour un personnage, `Planning/standards/personnages-3d.md`.

Un asset de scène est un maillage (`.glb`) ou, pour ce que le standard tolère encore, une image : le §7 de `style-3d.md` dit lesquelles, et jusqu’à quand. Un personnage est un maillage qui lui est propre, généré par Meshy depuis une vue de face en pose neutre (de trois quarts pour un quadrupède), puis lié au squelette commun de sa silhouette par script ; il ne se commande plus en bandes peintes. Aucune retouche à la main d’un maillage ni d’une texture : la chaîne se rejoue, ou la pièce se recommande. Les articulations et les clips d’un personnage se règlent dans Blender par l’atelier des assets de l’éditeur (D-44, `retouch_character.py`) : ce qui en revient est une donnée — fiche de liaison, fiche de retouche — que la chaîne rejoue ; jamais un maillage ni un `.glb` exporté par Blender.

Le standard n’écrit que des valeurs mesurées ou des décisions datées de l’auteur. Ne pas combler une question ouverte par une valeur devinée : la poser à l’auteur, ou la laisser au lot que le standard nomme.

Le résultat de référence accepté le 23 septembre 2026 pour la facture du décor reste le LOT-105, V4 après la reprise du relief des sols, des balustres et des haies : `Tools/AssetsHD/Regions/central-empire/capital/Common/V4/README.md`. Appliquer ses critères aux prochains lots, en adaptant la palette et les emblèmes au lieu. Préserver les pièces déjà validées lorsque la demande porte sur une retouche ciblée.

Un lot qui remplace un asset, un script ou un document le supprime dans sa propre PR (D-32) : `scripts/checks/check_orphans.py` le vérifie.
