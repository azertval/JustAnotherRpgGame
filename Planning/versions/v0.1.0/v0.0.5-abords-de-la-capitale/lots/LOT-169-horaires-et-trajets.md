+++
id = "LOT-169"
titre = "Éditeur — horaires et trajets"
version = "0.0.5"
filiere = "editeur"
statut = "a-faire"
taille = "S"
resume = "Ce que l'horloge du monde change sur une carte se règle et se voit dans l'éditeur : qui est où à quelle heure, ce qui ferme la nuit."
prerequis = ["LOT-171"]
livrables = [
  "Les **horaires** déclarés au contrat d'extension : une plage d'heures dans la condition de présence, des étapes horodatées sur une `route`.",
  "Un **curseur d'heure** au canevas, à côté du sélecteur d'état de partie du LOT-126 : il montre la carte à l'heure choisie, et l'essai en part.",
  "`--check` : un PNJ dont l'horaire laisse un trou, une route dont une étape tombe sur une case bloquée.",
]
criteres = [
  "Un garde relevé à la tombée de la nuit : au canevas, le curseur passé de midi à minuit échange les deux entités ; l'essai dans le jeu fait de même.",
  "Aucun code par famille dans l'éditeur.",
]
+++

## Pourquoi

La famille `route` attend ses horaires depuis le `LOT-EDITOR-05` ; le
[LOT-171](LOT-171-voyage-et-carte-de-region.md) apporte l'horloge. Petit lot : le mécanisme de
condition et le sélecteur d'état existent depuis le LOT-126, il n'y a qu'une dimension à ajouter.
