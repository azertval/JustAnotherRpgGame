# Les cartes de la Capitale

Les cartes **jouables** suivent le même découpage que les assets :
`Levels/<région>/<ville>/<zone>.json`, et la sous-zone sous son dossier de zone.

La table rase du `LOT-102` a emporté celles de l'ancien style. Depuis le `LOT-146`, la démo se joue
sur **quatre cartes de principe** (décision D-25), dessinées dans l'éditeur sans une seule pièce,
à l'échelle des plans du planning (`Planning/versions/v0.1.0/v0.0.1-demo/maquettes/plan-*.svg`) ;
les gestes qui les ont dessinées sont l'annexe du lot
(`Planning/versions/v0.1.0/v0.0.1-demo/annexes/LOT-146-cartes-de-principe/gestes/`) :

| Carte | Cases | Ce qu'elle contient |
|---|---|---|
| `martpart.json` | 24 × 11 | Market Gate (l'entrée), la place des étals et la mère, Stravian Avenue → Arenarea |
| `arenarea.json` | 24 × 13 | Herofate Avenue ← Martpart, le parvis (garde, enfant, zone déclencheuse), l'escalier → le niveau −1 de l'arène, la façade du casino (portail condamné) |
| `arenarea/arena-of-fate.json` | 34 × 24 | l'ovale de sable et sa zone de combat (22 × 14), le maître d'arène et le combattant sous `condamne`, podium, coursive et gradins en décor |
| `arenarea/arena-of-fate/undercroft.json` | 15 × 6 | le vestiaire A où arrive le condamné, le couloir et sa porte close sous `condamne` (`prop`), l'escalier de la porte du triomphe |

La carte d'Arenarea du `LOT-109` (le quartier entier, 128 × 88) a cédé son identifiant à la carte
de principe ; elle reste dans l'historique et revient, reprise, au `LOT-147`. Les cartes
définitives sont à la `0.0.3` (`LOT-107`, `LOT-111`, `LOT-147`).
