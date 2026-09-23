# LOT-105 — V4 propre

38 pièces installées : 34 Capitale et 4 Empire. Grille commune 256 × 159 px ; sols de cette taille exacte, murs droits de deux cases, angles d’une case. Les objets conservent leurs proportions et leurs ancres de pose.

## Fabrication

19 nouvelles sources dessinées avec l’outil imagegen intégré, puis neuf reprises pour supprimer grain, patine et textures parasites. Sources dans `Sources/`, premiers essais conservés dans `Sources/FirstPass/`. Instructions complètes : [prompts initiaux](prompts.json) et [prompts de nettoyage et bannière](cleanup-prompts.json).

Le recalage par script a été autorisé par l’utilisateur. Les images de matériaux sont projetées sur des surfaces communes : union des volumes aux angles, suppression des faces internes, même phase de texture à chaque case. Aucune peinture corrective locale des coins. Murs, balustrades et haies disposent d’angles rentrants et sortants. Les variantes de sol gardent les mêmes joints et ne varient que légèrement en teinte. L’emblème de la colonne et des bannières est une tête de lion couronnée de profil, avec laurier.

## Contrôles effectués

- 38 images installées identiques pixel par pixel aux images chargées dans le moteur ; ancres identiques.
- 4 tests ciblés passent : dimensions et ancres, douze raccords aux angles, cohérence des variantes de sol, présence des sources calibrées.
- Les deux installations sont à jour ; contrôle des budgets HD conforme.
- Carte de 359 placements, couvrant les 38 pièces, appliquée par LevelEditor puis rendue aux échelles 1 et 2. Inspection visuelle des sols et des assemblages.
- Vérification du moteur : zéro erreur. Avertissements existants concernant les cellules difficult/cover, encore traitées comme ouvertes par le gameplay. Il s’agit d’une validation du chargement et du rendu, pas d’un essai de déplacement en partie.

[Rendu du moteur](engine-validation.png) · [Rendu ×2](engine-validation-2x.png) · [Journal des commandes](engine-check.json) · [Map interactive](../apercus/validation.html)

Le dossier `Engine/` est une copie de test isolée adaptée au chargeur actuel du moteur. Les assets de production restent dans `Source/Elements/Assets/Regions/central-empire/`. La version précédente est sauvegardée dans les dossiers `Versions/before-v4` des deux kits.

## Reconstruction depuis la racine du dépôt

```text
python scripts/build_capital_v4.py
python scripts/tests/test_capital_v4.py
python scripts/validate_capital_engine.py
python scripts/build_capital_v4.py --install
python scripts/build_capital_validation.py
python scripts/build_capital_gallery.py
```

Python nécessite Pillow et NumPy. La validation moteur nécessite `build/ninja/bin/LevelEditor.exe`. Ne pas réinstaller les anciens calibrages V3 pour modifier cette version.


## Reprise ciblée du relief — 23 septembre 2026

À la demande de l’utilisateur, reprise des sols, des petits piliers des balustrades, puis des buissons. Les piliers isolés et le reste du kit sont conservés.

Les deux matériaux de sol ont été retouchés avec imagegen intégré : facettes de pierre taillée, biseaux et joints creux, sans ajout de grain. Prompts : [relief-prompts.json](relief-prompts.json). Sources actives : `Sources/paving-surface.png` et `Sources/flagstone-surface.png`.

Les balustres sont maintenant des volumes tournés à seize faces, avec pieds et chapiteaux en volume. Le dessus des haies est arrondi sur l’union des axes, avec extrémités fermées et éclairage des surfaces. Le rendu final reste constitué de PNG isométriques pour le moteur. Même grille, mêmes raccords et mêmes hauteurs générales. Les fichiers précédents et le script de fabrication sont conservés dans `BeforeRelief/`.

Contrôle après installation : 18 pièces modifiées (10 sols et bordures, 4 balustrades, 4 haies), les 20 autres restent identiques pixel par pixel. Quatre tests réussis ; nouveaux rendus moteur ×1 et ×2, PNG installés identiques à ceux testés.
