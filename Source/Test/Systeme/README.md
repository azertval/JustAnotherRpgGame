# Source/Test/Systeme/

Tests **système** (bout en bout) : un parcours d'utilisateur complet, sur les **données livrées**,
**sans la couche GPU** (fenêtre, rendu — vérifiée visuellement, cf. conventions).

- `test_parcours_edition_rpg.cpp` — le parcours d'auteur du `LOT-11` : produire une carte du RPG
  sans écrire de JSON (trois couches, un PNJ, un coffre, un portail, une rencontre), l'enregistrer,
  la recharger et la peupler comme le fait l'essai immédiat de l'éditeur.
- `test_demo_de_bout_en_bout.cpp` — la démo jouée de bout en bout par les modèles du jeu, sans
  fenêtre, depuis « Nouvelle partie » jusqu'à chacune de ses trois fins — la parole, l'arène
  gagnée, la mort sur le sable — sur le contenu livré et à hasard fixé (`LOT-120`, `LOT-146`).

Distinction : `Unit/` teste une brique isolée ; `Integration/` teste quelques briques assemblées ;
`Systeme/` rejoue un **scénario complet** sur le contenu livré.
