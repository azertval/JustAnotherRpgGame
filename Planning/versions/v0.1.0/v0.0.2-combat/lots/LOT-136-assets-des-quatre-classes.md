+++
id = "LOT-136"
titre = "Assets des quatre classes"
version = "0.0.2"
filiere = "assets"
statut = "a-faire"
taille = "L"
resume = "Les quatre héros ont leur figurine HD, leur portrait, leurs effets de sort."
prerequis = ["LOT-130"]
livrables = [
  "`Common/Characters/Heroes/{brawler,mage,priest,scoundrel}/` : figurine animée (repos, marche, attaque, sort, touché, mort), portrait, jeton.",
  "`Common/Fx/` : *fire bolt*, *magic missile*, *sacred flame*, *cure wounds*, *bless*, *fireball*, l'impact et le raté.",
]
criteres = [
  "Les quatre héros se distinguent **à la silhouette**, à la taille du jeu.",
  "Chaque sort des niveaux 1 à 5 a un effet visible ; aucun n'est un simple texte.",
]
sources = [
  "Player's Guide to Tanares, p. 195, 199, 203, 207 : les quatre illustrations — **référence de costume seulement**",
]
+++

## Les quatre silhouettes

| Classe | Fiche du livre | Silhouette |
|---|---|---|
| Brawler | Half-Orc, sans armure, grande hache | massive, torse nu, arme à deux mains |
| Mage | Autumn Elf, bâton | élancée, robe, bâton |
| Priest | Hill Dwarf, écailles, bouclier, marteau | trapue, bouclier, symbole sacré |
| Scoundrel | Human, cuir, rapière, arc court | fine, capuche, deux lames |

Le héros de la démo **est** le Brawler : le [LOT-112](../../v0.0.1-demo/lots/LOT-112-heros-de-la-demo.md)
livre `Heroes/brawler/` (sans l'animation de sort, qu'un Brawler n'a pas). Ce lot produit les trois
autres.
