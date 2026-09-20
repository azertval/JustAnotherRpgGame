+++
id = "LOT-116"
titre = "Quêtes et drapeaux de monde"
version = "0.0.1"
filiere = "moteur"
statut = "a-faire"
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
