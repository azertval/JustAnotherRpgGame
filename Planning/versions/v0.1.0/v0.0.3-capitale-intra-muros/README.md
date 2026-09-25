# Version 0.0.3 — la Capitale dans ses murs

Six quartiers, par paires de voisins, les trois lieux de la démo produits pour de bon, et la
sauvegarde — parce qu'à neuf quartiers la Capitale ne se visite plus en une séance. La quête du livre que l'ancien `LOT-16` avait écrite, « Les enfants de
Martpart », revient ici : la ville a désormais de quoi la porter.

## Gabarit d'une zone

Un lot de zone livre les **trois** choses que la zone demande, dans cet ordre :

1. **Assets HD** — les dix familles du [standard](../../../standards/style-2d-hd.md) passées en revue : ce qui vient
   du commun, ce qui est propre à la zone ; installés sous `Regions/central-empire/…/<zone>/Scene/`.
2. **PNJ** — nommés (à leur place), neutres (archétypes du commun, proportions du livre), hostiles ;
   figurines, portraits, fiches, placements.
3. **Carte** — jouable, dessinée dans l'éditeur, contrôlée par `--check` ; et l'image de la zone pour
   l'onglet « Carte ».

Un lot de zone qui dépasse la taille **L** se redécoupe en trois lots (assets, PNJ, carte), comme
ceux de la `0.0.1`.

## Ce qui vient de la démo

La `0.0.1` se joue sur des **cartes de principe** ([D-25](../../../vision/decisions.md), 25 septembre
2026) : ses lots de *world building* étaient trop complexes pour elle, et s'inscrivent mieux ici,
où une zone se produit pour de bon. Ils gardent leur numéro.

| Lieu | Assets | Carte | PNJ |
|---|---|---|---|
| Martpart | [LOT-110](lots/LOT-110-assets-hd-martpart.md) | [LOT-111](lots/LOT-111-carte-martpart.md) | [LOT-115](lots/LOT-115-pnj-martpart.md) |
| Arenarea | [LOT-147](lots/LOT-147-zone-arenarea-reprise.md) — les livraisons des `LOT-108` et `LOT-109`, **reprises** au standard du jeu final | (même lot) | [LOT-114](lots/LOT-114-pnj-arenarea.md) |
| Arena of Fate | [LOT-106](lots/LOT-106-assets-hd-arena-of-fate.md) | [LOT-107](lots/LOT-107-carte-arena-of-fate.md) | [LOT-113](lots/LOT-113-pnj-arena-of-fate.md) |

Ils suivent le kit complété ([LOT-151](lots/LOT-151-kit-commun-intra-muros.md)) et le pinceau de
foule ([LOT-158](lots/LOT-158-peupler-une-zone.md)), comme les six autres quartiers. La version
se clôt quand « Des pommes pour l'arène » se rejoue sur les cartes définitives, sans changer une
ligne de la quête.
