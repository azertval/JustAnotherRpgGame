+++
id = "LOT-127"
titre = "La recette de l'éditeur, à la main"
version = "0.0.1"
filiere = "editeur"
statut = "a-faire"
taille = "S"
resume = "L'auteur passe l'éditeur à la souris et au clavier, d'un bout à l'autre, avant d'y dessiner la première carte HD : la vérification due depuis huit lots est faite."
prerequis = ["LOT-104", "LOT-124", "LOT-125"]
livrables = [
  "Un **cahier de recette** de l'éditeur dans `Documentation/Editeur/` : une ligne par geste des lots `LOT-EDITOR-03` à `10`, `13` et `14`, cochée par l'auteur.",
  "Les anomalies trouvées : corrigées dans ce lot, ou inscrites au [LOT-166](../../v0.0.4-faubourgs/lots/LOT-166-dettes-de-l-atelier.md).",
  "Les mentions « vérification à la souris due » retirées des dossiers de lot de l'éditeur.",
]
criteres = [
  "L'auteur a monté, à la souris seule, une salle fermée avec les pièces installées de `Tools/AssetsHD/Colisee/` : sols, murs, angles, une porte, une entité, une zone — puis l'a essayée dans le jeu (`F5`).",
  "Chaque ligne du cahier est cochée ou renvoie à une anomalie numérotée.",
]
+++

## Pourquoi

Huit lots de l'éditeur ont été livrés « vérification à la souris due » : les clics postés par un
script n'atteignent pas Qt, et seuls les scénarios `--apply` ont été joués. La
[définition de « livré »](../../../../standards/definition-de-livre.md) a changé : ce qui demande une main est un
critère d'acceptation. La dette se règle **une fois**, ici, sur la pré-carte de l'Arena of Fate —
avant que trois cartes en dépendent.
