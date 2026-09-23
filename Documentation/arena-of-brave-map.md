# Arena of Brave — carte du Colisée

Branche : `feat/arena-of-brave-interface`.

La carte remplace désormais l'ancienne arène en jeu : le catalogue des combats utilise
`capital/arena-of-brave.json`, l'écran de combat compose son décor depuis cette carte, et
l'ancien identifiant de lancement `coliseum` redirige vers elle. Les arrivées `porte` et `sable`
restent utilisables. Le fichier historique reste disponible comme référence de test.

La zone de combat `sable` commence en (34, 37) et mesure 20 × 14 cases. Les quatre départs de
chaque camp conservent leurs positions relatives et leur rang. Le héraut se trouve en (46, 48),
le portier et les autres interlocuteurs sur le parvis ; le médecin est dans le vestiaire A.
L'accès au combat se fait toujours par le menu de l'arène ou le dialogue du héraut.

## Ouvrir et modifier

Lancer `LevelEditor --data Source/Elements --map=capital/arena-of-brave`. La carte s'ouvre dans
**LevelEditor** et les sauvegardes
écrivent directement dans `Source/Elements/Levels`, sans passer par une copie de test.

- Carte principale : `Source/Elements/Levels/capital/arena-of-brave.json` (88 × 88).
- Vestiaires : `arena-of-brave-camp-a.json` et `arena-of-brave-camp-b.json` dans le même dossier.
- Ressources : `Source/Elements/Assets/Scene/arena-of-brave/`.
- Kit : 366 pièces (353 composants et 13 secteurs assemblés), complétées par deux sols de base, disponibles dans la palette du lieu.

Choisir une pièce dans la palette du lieu et la peindre sur une couche visuelle avec **Brush**.
La gomme retire la pièce ; annuler et rétablir suivent les commandes natives de l'éditeur.
Peindre les obstacles séparément sur **Collision**. Les cartes sont livrées au format v4, avec les pièces nommées sur leurs couches.

Les 13 secteurs `territory-00` à `territory-12` conservent l'ordre de superposition vérifié
entre architecture, foule, bannières et statues. Les composants individuels sont également
disponibles. Leurs ancrages, leur orientation et leurs interfaces doivent rester compatibles ;
ne pas retourner ou faire pivoter arbitrairement leurs PNG. `assembly-reference.json` conserve
les positions d'origine et la composition détaillée de chaque secteur.

Les points d'ancrage, la profondeur et le rapport isométrique 68 × 42 sont lus de la même
façon par l'éditeur et le jeu. **Playtest** lance la carte en cours, modifications comprises.

## Parcours

Dans Arenarea, le portail en **(32, 26)** mène au parvis neutre. L'arène offre :

| Passage | Case du portail | Destination |
|---|---|---|
| Entrée du parvis | (44, 72) | Sable de l'arène |
| Sortie du sable | (44, 52) | Parvis |
| Accès camp A | (25, 44) | Vestiaire A |
| Accès camp B | (62, 44) | Vestiaire B |
| Retour au quartier | (44, 82) | Arenarea |

Les passages couverts utilisent les portails du moteur, avec des arrivées décalées pour éviter
les boucles de téléportation. Les vestiaires sont représentés en coupe, avec le mobilier et les
murs du fond. Le sable, le parvis et les vestiaires sont navigables ; les gradins et la couronne
sont des décors bloqués, sans navigation en hauteur.

Les 13 bannières territoriales restent à l'intérieur, groupées selon leurs alliances, et les
18 monuments religieux restent sur la couronne supérieure. Le parvis et ses gardiens n'ont
aucune bannière. Anariel est conservée au centre comme référence d'échelle.

## Lancement du jeu

Après `scripts/build.ps1 -Target JustAnotherRpgGame`, lancer depuis `build/ninja/bin` :

```powershell
./JustAnotherRpgGame.exe --screen=GameView --map=capital/arena-of-brave@centre --hero-figure=anariel
```

La copie des ressources vers le jeu est assurée par `CopyGameData` à chaque construction du jeu
ou de l'éditeur. Pour conserver une modification dans le dépôt, utiliser le lanceur de l'éditeur
fourni ci-dessus ; l’option native `--data <dossier>` permet de choisir explicitement la racine des données.

## Vérifications

- Chargement natif des trois cartes et sauvegarde/relecture du brouillon au format v4.
- Comparaison des ancrages et profondeurs entre éditeur et rendu GPU ; absence de textures manquantes.
- Pose, retrait, annulation et rétablissement d'une pièce sans modifier la collision.
- Traversée des portails avec `WorldTravel` et parcours des cases accessibles.
- Test d’intégration autonome : `python -m unittest scripts.tests.test_arena_map_integration`.

Les cartes et toutes leurs textures sont livrées dans le dépôt. Aucune campagne AssetFactory
ni aucun script de génération externe n’est nécessaire pour les charger ou les modifier.
L’essai de textures HD ne fait pas partie de cette intégration.
