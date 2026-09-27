+++
id = "LOT-138"
titre = "Le groupe de quatre"
version = "0.0.2"
filiere = "moteur"
statut = "en-cours"
taille = "M"
resume = "Le joueur mène jusqu'à quatre personnages : composition, meneur, suivi en exploration."
prerequis = ["LOT-130"]
reprend = ["LOT-29"]
livrables = [
  "Le **groupe** : jusqu'à quatre fiches, un meneur qu'on déplace, les autres qui suivent.",
  "L'écran de composition du groupe, depuis les quatre fiches préfabriquées.",
  "Les jets de dialogue faits par le **meilleur** du groupe, ou par le meneur — à trancher.",
]
criteres = [
  "Un groupe de quatre traverse une carte sans qu'un suiveur reste coincé.",
  "Changer de meneur change la figurine menée et le portrait du dialogue.",
]
sources = [
  "Tanares Sourcebook, p. 48-49 : pas de groupe armé de plus de six membres dans l'Empire",
]
+++

## Questions ouvertes

- Qui fait le jet de Persuasion : le meneur, ou le plus doué ? Le *Manuel* laisse le choix au
  joueur ; proposé : **le meneur**, ce qui donne un sens à son choix. **Tranché par l'auteur**
  (D-28) : **le joueur choisit** qui parle, dans un menu en bas du dialogue ; le meneur parle à
  l'ouverture.

## Décisions de réalisation

Réalisé le 27 septembre 2026 (exigences `EX-EXP-013`, `EX-EXP-014`), **PR #153**.

1. **Le groupe est un ordre.** `core::Party` est une liste ordonnée d'identifiants de fiche, de un
   à quatre : le premier **mène**, les autres suivent dans cet ordre. Pas de champ « meneur » à
   côté : c'est le rang qui fait foi. Le dernier membre ne se laisse pas, un cinquième ne se prend
   pas, un doublon non plus. Quatre, le groupe des fiches pré-tirées, reste en deçà des six que
   l'Empire tolère (*Tanares Sourcebook*, p. 48-49).
2. **Les suiveurs mettent leurs pas dans ceux du meneur.** Ni poursuite en ligne droite (elle
   bute au premier angle), ni recherche de chemin par membre et par image : `core::FollowTrail`
   retient le chemin du meneur, et chaque suiveur s'y tient à **une case** du précédent. Tout
   point de ce chemin a été tenu par le gabarit du meneur : un suiveur ne peut pas être dans un
   mur ni rester coincé — le premier critère tient **par construction**, et un test le vérifie à
   chaque pas d'une carte en U. Les suiveurs ne franchissent rien, n'entrent dans aucune zone,
   n'interagissent pas et ne se heurtent à personne.
3. **À l'arrivée, la file se range dans le dos du meneur**, par pas d'un dixième de case, tant que
   son gabarit y tiendrait ; un mur l'arrête, et les derniers attendent sur le dernier point libre
   jusqu'à ce que le meneur marche. Même chose au retour d'un combat (`placeHero`).
4. **Le meneur partout.** `hmi::playedCharacterFile` désigne sa fiche : la fiche et l'inventaire
   s'ouvrent sur lui, le combat sur la carte le met en jeu (`hmi::EncounterModel` relit son héros
   quand le meneur change), le portrait de l'affichage tête haute est le sien.
5. **Le joueur choisit qui parle** (D-28, décision de l'auteur). Le dialogue lit le groupe entier :
   un menu en bas de l'écran aligne les membres, portrait et nom, dans l'ordre de marche ; le
   meneur parle à l'ouverture, `Tab` (ou le clic) donne la parole au suivant, et celui qui parle
   **jette avec ses modificateurs** — ses langues aussi comptent. L'écouteur du runner reste le
   même et délègue au personnage qui parle.
6. **La figurine d'un membre est celle de sa classe** (`Common/Characters/Heroes/<classe>`), la
   convention de l'arborescence (`LOT-124`), sans champ nouveau dans la fiche. Seul le Brawler a
   la sienne ; les trois autres prennent le **mannequin humanoïde** (`LOT-145`) et n'ont pas de
   portrait — le cadre prend son état vide — jusqu'au `LOT-136`. `--hero-figure=` impose encore
   une figurine au meneur, quel qu'il soit.
7. **Passer la main : `Tab`**, qui met le suivant en tête et le meneur en queue — quatre appuis
   font le tour, là où échanger les deux premiers ferait un va-et-vient. La figurine menée change
   sous la main, la caméra reste où elle est.
8. **L'écran Groupe** (`G` depuis la carte, dixième écran du RPG, `RpgScreenId::Party`) montre
   l'ordre de marche et les quatre fiches (espèce, classe, niveau, PV, CA, vitesse) ; il prend et
   laisse (`Entrée`, `A`), fait mener (`M`, `X`), avance et recule dans l'ordre de marche (`Page
   préc.`/`Page suiv.`, `LB`/`RB`). « Nouvelle partie » **impose le groupe préformé** (décision
   de l'auteur, D-28) : Brawler, Priest, Scoundrel, Mage, dans cet ordre de marche
   (`hmi::WorldModel::STARTING_PARTY`) ; la démo se joue avec Grom en tête.
9. **L'affichage tête haute** remplit la case du groupe, réservée depuis le `LOT-67` : les membres,
   meneur en tête et marqué, et le portrait principal au meneur.
10. **Le groupe appartient à la partie** (`hmi::WorldModel`) : `endGame` rend le groupe de départ.
   `hmi::PartyModel` lit les fiches des quatre — les catalogues une fois
   (`hmi::loadCharacterValues`) — et passe chaque geste à la partie.

Tests : `test_party.cpp` (composition, meneur, ordre de marche, les quatre fiches du dossier),
`test_exploration_session.cpp` (trois suiveurs passent les deux angles d'un U sans entrer dans le
mur ; la file se range dans le dos du meneur ; la trace se mesure le long du chemin),
`test_party_model.cpp` (changer de meneur change la figurine menée, le portrait et la voix du
dialogue ; le joueur choisit qui parle, et le jet prend ses modificateurs ; le meneur combat ;
l'écran de groupe compose).

**Reste à la main de l'auteur** : parcourir une carte à quatre et voir les suiveurs marcher et se
tourner — la figurine du Brawler, les mannequins des trois autres ; passer la main par `Tab` ;
ouvrir l'écran Groupe au clavier et à la manette ; changer de voix dans un dialogue.

## Ce qui n'est pas ici

- Les quatre **en combat**, et ce que le combat laisse aux fiches (points de vie, lancers, un
  membre mort qui ne suit plus) : le `LOT-139`, où c'est écrit.
- La fiche **de chacun** (onglets Classe et Sorts), l'écran de groupe au complet et la montée de
  niveau : le `LOT-141`. La fiche s'ouvre sur le meneur.
- Le nom des PNJ du marché (Q-05), que le bilan de la `0.0.1` proposait de trancher « au passage »
  d'un lot de dialogue : ce lot ne touche aucun texte de dialogue, la question reste ouverte.
- Les portraits et figurines du Mage, du Priest et du Scoundrel : le `LOT-136`.
