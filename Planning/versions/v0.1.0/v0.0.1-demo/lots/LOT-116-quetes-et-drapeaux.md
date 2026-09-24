+++
id = "LOT-116"
titre = "Quêtes et drapeaux de monde"
version = "0.0.1"
filiere = "moteur"
statut = "en-cours"
taille = "L"
resume = "Le jeu se souvient de ce que le joueur a fait : des drapeaux, un journal de quêtes, des PNJ présents ou absents selon l'avancement."
prerequis = ["LOT-100"]
reprend = ["LOT-16"]
livrables = [
  "`core` : drapeaux de monde typés, quêtes décrites en données (étapes, conditions, effets), chargées et validées au démarrage.",
  "Les dialogues posent et lisent des drapeaux ; une entité de carte a une **condition de présence**.",
  "Le journal de quêtes à l'écran.",
  "Le contrôle de l'éditeur (`--check`) qui refuse un drapeau lu mais jamais posé.",
]
criteres = [
  "Une quête de test à trois étapes se joue sans fenêtre, en test d'intégration.",
  "Un PNJ conditionné paraît et disparaît quand le drapeau change, sans recharger la carte.",
  "Un fichier de quête mal formé est refusé au chargement avec un message qui nomme la ligne.",
]
+++

## Périmètre

Le **mécanisme**. La quête de la démo est au LOT-120. La **sauvegarde** des drapeaux n'est pas ici :
la démo se joue d'une traite, la sauvegarde arrive en `0.0.3`.

L'ancien LOT-16 portait aussi la quête « Les enfants de Martpart », tirée du livre (Myr, p. 101).
Elle n'est pas perdue : elle revient avec les quartiers de la `0.0.3`, quand la Capitale a de quoi
la porter.

## Décisions de réalisation

Réalisé le 24 septembre 2026.

1. **« Typé » veut dire « à valeurs déclarées ».** La quête de la démo n'a qu'une mémoire à cinq
   valeurs ([fiche](../quete-demo.md)) : un drapeau reste un fait présent ou absent, sauf s'il est
   **déclaré** par une quête (liste fermée, initiale). `core::WorldFlags::setValue` refuse une
   valeur hors liste ; `set` refuse de poser sans valeur un drapeau déclaré. Pas d'entier ni de
   compteur : rien ne les demande en `0.0.1`.
2. **Une étape se lit dans le monde, elle ne s'ordonne pas.** Elle est atteinte dès que toutes ses
   conditions tiennent, une fois, et le reste ; son fait est un drapeau fabriqué
   (`quest/<quête>/step/<étape>`). L'avancement se **lit** dans les drapeaux
   (`core::questProgress`), jamais stocké à part : la sauvegarde de la `0.0.3` n'aura que les
   drapeaux à écrire. Les embranchements (persuader ou endosser) sont deux étapes qu'atteignent
   deux valeurs du même drapeau — pas de graphe de quête.
3. **Une seule condition pour trois lecteurs.** `core::FlagCondition` sort de `Dialogue.h` et sert
   les dialogues, les étapes et la présence : `isSet`, `equals`, `notEquals` (une valeur ou une
   liste). Les dialogues du `LOT-15` restent valides tels quels.
4. **La ligne d'une erreur de sens.** nlohmann 3.11 ne garde pas les positions : `core::positionOfPointer`
   relit le texte en suivant le chemin JSON de la valeur fautive. Le chargeur de quête lit le texte
   d'origine, sans le réécrire (celui des dialogues le réécrit et perd ses lignes : non repris ici).
5. **La présence en trois propriétés plates** (`presenceFlag`, `presenceTest`, `presenceValue`
   `a|b`) : une propriété de carte ne tient qu'un scalaire, un objet serait jeté sans un mot au
   chargement. Elle vaut pour **toute** famille et se contrôle dans `validateMapEntities` sans code
   par famille. Mal formée, l'entité reste présente.
6. **Sans rechargement, par révision.** `WorldFlags::revision()` avance à chaque changement ; la
   session relit ses interactifs et fait avancer les quêtes quand elle bouge (`refreshFromFlags`,
   même gelée), `WorldPlay` recompose la scène. Pas de signal dans le cœur.
7. **Un seul ensemble de drapeaux par partie.** `DialogueModel` écrivait dans un ensemble statique
   à lui : un drapeau posé en parlant n'atteignait pas la carte. Il écrit désormais dans ceux de
   `WorldModel::current()` (repli statique sans partie, pour le designer).
8. **Le jeton jaune** (D-22) est le PNJ qui porte un dialogue **ou** une condition de présence :
   l'enfant de la démo ne parle pas, la quête le fait paraître.
9. **Le journal** garde son formulaire (et sa capture de référence) : la quête choisie porte la
   marque `›` sur sa ligne, plutôt qu'un état de sélection que le formulaire n'a pas.

## Ce qui n'est pas ici

- Déclarer la condition de présence dans `EntityKinds` (l'inspecteur), refuser sur une carte une
  valeur qu'aucune quête ne déclare, suivre les `carte#id` cités : [LOT-126](LOT-126-ce-que-la-quete-demande-aux-cartes.md).
- La sauvegarde des drapeaux : `0.0.3`.
- Le panneau « Problems » de la fenêtre de l'éditeur ne montre que les constats par carte ; le
  contrôle du récit (quêtes, dialogues) passe par `LevelEditor --check`.

