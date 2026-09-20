# T00 — Socle et reprise des cinq PNJ du PoC

> Statut : en cours, premier essai non validé. Famille : PNJ nommés. Série pilote de cinq unités métier, sous le plafond de dix ; calibration technique avant production régionale.
> Contrat commun : [epic.md](epic.md). Exécution : chat local et outils intégrés.
> 39 tours générés et reçus ; zéro PNJ autorisé à intégrer. Bilan et rapport détaillé : `Tools/AssetFactory/bilan-poc.md` et `rapport-poc.md` (hors dépôt).

## Assets

- [ ] `anariel` — Anariel, the Swordmage — Character Compendium, page PDF 10.
- [ ] `jade` — Jade, the Bard — Character Compendium, page PDF 68.
- [ ] `lizz` — Lizz, the Medusa — Character Compendium, page PDF 83.
- [ ] `nakral` — Nakral, the Death Knight — Character Compendium, page PDF 92.
- [ ] `xorius` — Xorius, the Archers’ General — Character Compendium, page PDF 148.

## Étapes du socle

- [x] Figer le commit et les empreintes des résultats manuels, pour comparaison sans écrasement.
- [ ] Préparer les profils de famille, sources, ancres et formats ; implémenter seulement PNJ.
- [ ] Extraire les cinq fiches depuis texte et illustrations ; relire les anciens blocs B. Les références visuelles du corpus sont autorisées au générateur et conservées hors Git avec localisateurs/empreintes versionnés.
- [x] Écrire l'identité versionnée, l'index d'états et le journal permettant la reprise après coupure.
- [x] Prévoir les commandes de préparation, réception, QC, rapport, état et intégration.
      Le chat effectue les appels de génération et la relecture visuelle.
- [x] Adapter normalisation, palette et assemblage : retirer la substitution d'une image d'idle
      dans hit, les comptes artificiellement complétés et les tolérances de rognage utile.
      L'extraction de palette ignore l'alpha transparent, au lieu de supposer un fond navy.
- [ ] Étalonner les mesures de palette et d'animation sur les références manuelles et des défauts
      connus, puis écrire les seuils chiffrés avant d'évaluer les nouvelles productions.
- [ ] Vérifier reprise, réponse incertaine, invalidation des verdicts, exclusion Git des PDF/rendus/crops et autres images d’entrée du corpus,
      comptes incorrects et installation interrompue, avec tests locaux sans génération en CI.

## Production et acceptation

- [ ] Refaire les cinq portraits et les trente animations dans ce chat, depuis les fiches sourcées, les références visuelles autorisées hors Git,
      les ancres de style déclarées et les nouveaux portraits retenus.
- [ ] Refaire chaque animation rejetée par une passe X ; journaliser le prompt corrigé.
- [ ] Produire les planches-contact et GIF, ouvrir les images et consigner la revue visuelle.
- [ ] Comparer anciennes et nouvelles sorties côte à côte : identité, style, alpha, palette,
      silhouette, mouvement, nombre de tours et interventions. Aucun coût en dollars à calculer.
- [ ] Intégrer uniquement après validation, sans perte de la table `replaces` du manifeste.
- [ ] Vérifier cinq portraits, trente bandes et trente descripteurs dans la galerie et dans
      le lecteur d'animations du jeu ; attendre les textures avant capture.
- [ ] Mettre à jour crédits, rapport et changelog ; build via `scripts/build.ps1`, CI verte.
- [ ] Ouvrir la PR de la branche `lot/LOT-CREATION-ASSETS-pnj-pilote`.

La tâche n'est terminée que si les cinq PNJ sont validés, comparés et intégrés. Une limite
d'usage ou une ambiguïté produit un état reprenable, pas une validation implicite.
Les séries suivantes attendent le verdict de ce pilote.
