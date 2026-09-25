# La racine de données d'essai

Une racine à la forme de `Source/Elements/`, mais **sans un seul asset ni une seule carte du jeu**.
Les tests de l'éditeur (`LOT-123`) puis ceux du jeu (`LOT-102`) la lisent à la place du contenu
livré : la table rase du `LOT-102` a vidé `Assets/Scene/`, `Assets/Coliseum/`, `Assets/Npc/`,
`Assets/Monsters/` et `Levels/`, et tous ces tests seraient tombés avec eux.

Ce qu'ils prouvent n'est pas du **contenu** — une carte livrée, une pièce dessinée — mais des
**mécanismes** : un manifeste se lit, une pièce se résout, un portail se traverse, une zone de
combat se découpe, une figurine s'anime, le rendu GPU et celui de l'éditeur tombent d'accord. Ces
mécanismes valent d'être gardés quel que soit le contenu du jour ; c'est pourquoi ils vivent ici.

Le chemin est `JADG_TEST_DATA_DIR` (`Source/Test/CMakeLists.txt`), et
`LevelEditor --data <cette racine>` l'ouvre à la main. Le jeu la joue de même
(`JustAnotherRpgGame.exe --data=<cette racine> --map=donjon@sable --at=24,19`, `LOT-118`) : le
héros paraît devant le maître d'arène d'essai, dont le dialogue engage les rats sur la carte.

## Ce qu'elle contient

| Dossier | Contenu |
|---|---|
| `Levels/` | `bourg/place.json` (une place de ville, cinq îlots), `cave.json`, `donjon.json` (une salle de 20 × 14 en zone de combat, huit entrées d'arène) |
| `Assets/Scene/bourg`, `.../hameau` | deux planches de lieu : `manifest.json` (clé, emprise, ancre), `appearance.json` (ce que le sol et le relief posent sur une case) |
| `Assets/Arena/` | un kit d'arène : `manifest.json` (dont `scene`, le lieu d'où il tire ses pièces), deux héros, deux gladiateurs |
| `Assets/Npc/`, `Assets/Monsters/` | une figurine chacun (`figurant`, `sentinelle`), aux cadences de l'atelier |
| `World/` | une ville (`bourg`), une région, des lieux, des dialogues — dont le maître d'arène d'essai (`maitre-d-essai`), qui engage la rencontre sur la carte (`LOT-118`), et le héraut d'essai (`heraut-d-essai`), le graphe de quatorze nœuds — deux conditions, un jet de Persuasion, une quête démarrée — que les tests de dialogue parcourent réplique par réplique |
| `Rpg/`, `Localization/`, `Maps/`, `Editor/` | une rencontre, des objets, les textes des cartes, trois modèles de carte |

## Comment son art est fait

Ce ne sont **pas** des assets : ni style, ni charte, ni revue. Les pièces de scène sont des
aplats à la géométrie du losange ; les figurines, des silhouettes plates de 48 × 64 (96 de large
pour une attaque ou une mort), une teinte par modèle, un témoin clair qui numérote l'image. Il en
faut juste assez pour qu'une image ne soit pas vide, qu'une bande ait plusieurs images et que deux
pièces ne se confondent pas.

Refaire une figurine : un aplat de la bonne taille et un `.anim.json` qui la décrit suffisent — le
dessin n'a rien à prouver.
