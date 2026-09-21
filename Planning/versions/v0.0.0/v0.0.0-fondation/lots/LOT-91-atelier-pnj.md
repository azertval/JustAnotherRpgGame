+++
id = "LOT-91"
titre = "Atelier des PNJ : les figurines du Character Compendium"
version = "0.0.0"
filiere = "pnj"
statut = "livre"
taille = "L"
resume = "Une méthode éprouvée sur cinq PNJ donne à toute fiche du Character Compendium une figurine animée au style du jeu, avec l'atelier versionné pour la rejouer."
prerequis = ["LOT-50", "LOT-39"]
livrables = [
  "L'atelier versionné dans le dossier du lot : `atelier/prompts/`, `atelier/ancres/`, `atelier/pnj/index.json` (les 160 fiches), `atelier/pnj/<slug>/`, `atelier/scripts/`.",
  "Les scripts de la chaîne : `references.py`, `chatgpt.py`, `normalise_portrait.py`, `palette.py`, `normalise.py`, `compose.py`, `integre.py`, `mesure.py`, `prompts.py`, `tour.py`, `portrait.py`.",
  "Le jeu d'animations figé (`idle` 6, `walk` 8, `hit` 4, `death` 6, `attack` 8, `cast` 8) et ses cellules 48 × 64 / 96 × 64.",
  "Cinq figurines intégrées sous `Source/Elements/Assets/Npc/<slug>/` (Anariel, Jade, Lizz, Nakral, Xorius) : six bandes, leurs `.anim.json`, `portrait.png`, et `Npc/manifest.json`.",
  "La [preuve de concept](@ref lot-91-poc) (`poc.md`) : son journal et son verdict.",
]
criteres = [
  "Chaque PNJ coché a, dans `Source/Elements/Assets/Npc/<slug>/`, ses six bandes aux dimensions du jeu d'animations, leurs `.anim.json` et `portrait.png`, et figure dans `Npc/manifest.json`.",
  "Chaque PNJ coché a ses références dans `atelier/pnj/<slug>/` (`prompt_b.txt`, `portrait.png`, `palette.txt`).",
  "Une disposition modifiée garde sa version précédente et dit ce qui a changé.",
]
sources = ["Character Compendium, p. 4-163 (les 160 fiches nommées)"]
+++

## Pourquoi

Donner une **figurine animée au style du jeu** aux 160 fiches nommées du *Character Compendium*
(`Documentation/SourceBook/VTT/Character Compendium - High.pdf`, pages 4 à 163), une par une, par la
même méthode. Le PoC a montré que la méthode produit un modèle fidèle pour un PNJ donné, mais qu'on ne
code pas une chaîne automatique là où chaque PNJ demande encore quelques choix (arme, main, marche,
effet). **La suite fait confiance à la méthode** : un modèle qui passe ses contrôles est intégré, sans
essai en jeu de chaque PNJ.

## Périmètre

### Hors du lot, nommément

- Les **héros du Colisée** (Kaelith, Elira, Darin, Bram) : sans fiche au Compendium, leur reproduction
  au style commun demandera leurs propres références. Aujourd'hui, `Npc/manifest.json` (`replaces`)
  fait jouer Anariel, Jade, Xorius et Nakral à leur place au Colisée.
- La **classe 96 × 96** (*Large*, *Huge*) : décidée, pas écrite ; sa disposition s'écrit au premier PNJ
  qui la demande.
- Les créatures *Gargantuan*, et les fiches sans caractéristiques (Hell Dragon, p. 61).
- Les **PNJ génériques** (`LOT-82`) et le **bestiaire** (`LOT-46`) : ils pourront réemployer la méthode,
  pas ce lot.
- L'**automatisation** de la boucle (génération par l'API, revue par vision) : le PoC a montré qu'elle
  demanderait encore trop de choix au cas par cas ; les scripts restent des outils manuels.

## Les décisions

| Sujet | Décision |
|---|---|
| **Générateur** | Génération d'images d'OpenAI, envois faits **à la main** dans l'interface ChatGPT (l'API reste possible : `tour.py`, `portrait.py`, même prompt). Pas de graine : la stabilité vient des ancres, du contrôle de réception et du prompt corrigé. Claude ne dessine pas : il rédige, assemble, mesure et relit. |
| **Jeu d'animations** | Figé : `idle` 6, `walk` 8, `hit` 4, `death` 6, `attack` 8, `cast` 8. Tout PNJ a un `cast` (un barde joue de son instrument). |
| **Cellules** | 48 × 64 pour `idle`, `walk`, `hit` (pieds à x = 24) ; **96 × 64** pour `death` (pieds à x = 48), `attack` et `cast` (pieds à x = 32, effets vers la droite). Personnage de **45 pixels d'art**, comme les héros du Colisée. Tailles *Tiny* à *Medium* seulement ; *Large* et *Huge* attendent une classe 96 × 96 à écrire au premier d'entre eux ; *Gargantuan* hors périmètre. |
| **Fond** | Transparent ; le halo semi-transparent du générateur est binarisé à 127 à la réception. |
| **Orientation** | De face, légèrement plongeante ; la main droite du personnage à gauche du spectateur ; l'arme dans la main que nomme le bloc B. |
| **Emplacement** | `Source/Elements/Assets/Npc/<slug>/` : six bandes, leurs `.anim.json` et `portrait.png` (le portrait pixel art) ; `Npc/manifest.json` liste les PNJ. |
| **Références** | Versionnées dans `atelier/` (ci-dessous), sauf les rendus du corpus, reproductibles depuis le numéro de page. |

### Le jeu d'animations

| Action | Images | Cellule | Durée / image | Boucle | Pose |
|---|---|---|---|---|---|
| `idle` | 6 | 48 × 64 | 0,15 s | oui | Garde, respiration ; l'image 6 revient vers la 1. |
| `walk` | 8 | 48 × 64 | 0,10 s | oui | Quatre silhouettes de jambes, deux fois en miroir (ou reptation, ondulation…). |
| `hit` | 4 | 48 × 64 | 0,08 s | non | Choc, recul, reprise, garde. |
| `death` | 6 | 96 × 64 | 0,12 s | non, tenue | Chancelle 1-3, chute 4, couché 5-6 dans deux hauteurs. |
| `attack` | 8 | 96 × 64 | 0,08 s | non | Armé 1-3, frappe 4-5 avec la traînée de la seule arme, retour 6-8. |
| `cast` | 8 | 96 × 64 | 0,10 s | non | Concentre 1-4, libère vers la droite 5-6, retour 7-8. |

## L'atelier

Tout ce qu'il faut pour produire un PNJ est rangé dans ce dossier de lot, sous `atelier/` :

| Chemin | Contenu |
|---|---|
| `atelier/prompts/` | `style.txt` (v1), `portrait_style.txt` (v1), `disposition_planche.txt` (**v2**), `disposition_marche.txt`, `disposition_reprise.txt`, `marche_jambes.txt` : les textes envoyés au générateur, en anglais. |
| `atelier/ancres/` | `planche_anariel.png` (la planche de référence, 1 536 × 2 048), `portrait_anariel.png` (l'ancre de portrait), `echelle.png` (les quatre héros du Colisée à 2 px). |
| `atelier/pnj/index.json` | Les 160 fiches : slug, nom, page. |
| `atelier/pnj/<slug>/` | Les références propres au PNJ, écrites quand on le traite : `prompt_b.txt` (le bloc B : personnage, tenue, arme, palette, jeu d'animations), `portrait.png` (portrait pixel art retenu), `palette.txt`, et au besoin `marche.txt` (une marche sans jambes). |
| `atelier/scripts/` | `references.py`, `chatgpt.py`, `normalise_portrait.py`, `palette.py`, `normalise.py`, `compose.py`, `integre.py`, `mesure.py`, `prompts.py` (chemins et assemblage du prompt), `tour.py` et `portrait.py` (par l'API). |

Le **dossier de travail**, hors dépôt (variable `NPC_ATELIER`, par défaut `D:\JustAnotherDnDGame-npc-poc`),
reçoit les rendus du corpus (`<slug>\ref\`), les envois préparés (`chatgpt\`), les images générées et
leurs normalisations (`<slug>\token\`, `<slug>\tourK\`, `<slug>\final\`). Rien n'en sort vers le dépôt
sauf par `palette.py` et `integre.py`.

Les scripts se lancent depuis `atelier/scripts/`, avec `py -3.13` (`numpy`, `Pillow`, `pymupdf`).

## La méthode, pour un PNJ

Les PNJ d'une série passent **ensemble, étape par étape** : on prépare les envois de tous, on les
génère à la main, on reçoit tout, puis on passe à l'étape suivante.

| Étape | Ce qu'on fait | Contrôle pour passer |
|---|---|---|
| **E — Références** | `references.py rendus <slug>` rend le portrait peint et la moitié droite de la page ; recadrer la figurine dorée en `ref4_figurine.png`. Lire la fiche (taille, arme, tenue) et écrire `pnj/<slug>/prompt_b.txt` sur le modèle des cinq existants. | Taille *Tiny* à *Medium* ; sinon le PNJ attend sa classe de cellule. |
| **P — Portrait** | `chatgpt.py portrait <K> <slugs>` ; générer ; `normalise_portrait.py <candidat> <travail>\<slug>\token\portrait.png` ; `palette.py <slug> <ce portrait>` (copie le portrait dans `pnj/<slug>/`, écrit la palette et la ligne `PALETTE` du bloc B). | Visage, coiffe, arme et couleurs du portrait peint ; un fichier d'image valide (un envoi raté rend un message d'erreur de 190 octets). |
| **S — Planche** | `chatgpt.py planche <K> <slugs>` ; générer ; `normalise.py <candidat> <slug>\tourK\norm planche --hauteur 45`. | `normalise.py` rend 0, **et** la revue ci-dessous a neuf « oui ». |
| **M — Marche** *(si besoin)* | Si `normalise.py` avertit « walk bouge peu » (rapport walk/idle sous 4) ou si l'œil la juge figée : `chatgpt.py marche <K> <slugs>` (la planche normalisée du PNJ en référence 1) ; `normalise.py … marche --hauteur 45`. | Code 0 ; huit silhouettes distinctes ; le même personnage que la planche. |
| **X — Passe par animation** *(si besoin)* | Une seule animation fausse (compte, arme dédoublée, effet hors cellule) : `chatgpt.py <anim> <K> <slugs>` avec `idle`, `hit`, `death`, `attack` ou `cast` ; `normalise.py … <anim> --hauteur 45`. | Code 0 ; revue de cette animation. |
| **A — Assemblage** | `compose.py <slug> <tour S> [<tour M>] [<anim>=<tour> …]`. | Code 0. |
| **I — Intégration** | `integre.py <slug>` : bandes, `.anim.json` et `portrait.png` dans `Assets/Npc/<slug>/`, slug au manifeste, **case cochée ci-dessous**. | — |

### Les règles

1. **On corrige le prompt, jamais l'image.** Un défaut commun à plusieurs PNJ corrige la disposition
   (nouvelle version, l'ancienne gardée) ; un défaut propre au PNJ corrige son bloc B. Chaque tour
   est une génération complète.
2. **Une animation fausse se refait seule**, par une passe X, jamais en empruntant les images d'une
   autre planche : d'un tour à l'autre, le générateur redessine le personnage.
3. **Au plus trois tours par étape** ; au-delà de deux passes (M comprise), la planche est à refaire.
   Un PNJ qui n'aboutit pas reste décoché, avec sa raison en fin de ligne, et l'on passe au suivant.
4. **Un code 0 ne dispense pas de la revue** : le dédoublement d'une arme ne se voit qu'à l'œil.
5. **Pas de vérification en jeu** par PNJ : un modèle qui passe A est intégré.

### La revue (neuf « oui »)

1. Visage, coiffure et couleurs du portrait.
2. Silhouette, arme et garde de la figurine dorée.
3. Le même personnage sur les quarante images (tenue, proportions, **une seule arme**, même main).
4. Le style de la planche d'Anariel : contour, ombrage, densité, pas d'anticrénelage.
5. La lumière du même côté partout.
6. `idle` boucle ; `attack`, `hit`, `cast` finissent près de la garde.
7. `death` 5-6 couchés sur la ligne.
8. Les effets restent dans leur cellule et dans la palette.
9. Aucun texte, cadre, ombre au sol, décor.

## Les PNJ

Cochés par `integre.py` quand le modèle est intégré. Nom et page de la fiche du Compendium ; le slug
nomme les dossiers `atelier/pnj/<slug>/` et `Assets/Npc/<slug>/`.

- [ ] A´Laafia, The one In Equilibrium — p. 4 — `a_laafia`
- [ ] A´Laafia's Companion: Dynamo — p. 5 — `a_laafia_s_companion_dynamo`
- [ ] Abhorroth, the Dark Titan — p. 6 — `abhorroth`
- [ ] Ahr’Arthra, The Desert Broodmother — p. 7 — `ahrarthra`
- [ ] Aldo Cotton-Hair — p. 8 — `aldo_cotton_hair`
- [ ] Ananab, the Community Sage — p. 9 — `ananab`
- [x] Anariel, the Swordmage — p. 10 — `anariel`
- [ ] Ardilog, the Everspawning Chimera — p. 11 — `ardilog`
- [ ] Ardnold Bak, The Harvester — p. 12 — `ardnold_bak`
- [ ] Ascaran, the Archangel — p. 13 — `ascaran`
- [ ] Auros (dragonborn) — p. 14 — `auros_dragonborn`
- [ ] Avelum, the Master Wizard — p. 15 — `avelum`
- [ ] Azriel, the Water Elementalist — p. 16 — `azriel`
- [ ] Azymor, the Red Avatar Dragon — p. 17 — `azymor`
- [ ] Baolmu, the Augur Madwalker — p. 18 — `baolmu`
- [ ] Baraelmer, the Battlemaster — p. 19 — `baraelmer`
- [ ] Bauis, the Kemet Traitor — p. 20 — `bauis`
- [ ] Becca — p. 21 — `becca`
- [ ] Bellara, The Daughter of C’thraxis — p. 22 — `bellara`
- [ ] Black Soul — p. 23 — `black_soul`
- [ ] Bonas Weyrdo — p. 24 — `bonas_weyrdo`
- [ ] Bretrar Brakaan — p. 25 — `bretrar_brakaan`
- [ ] Bridgit, the Penumbral Wanderer — p. 26 — `bridgit`
- [ ] Bromeliad, the Copycat — p. 27 — `bromeliad`
- [ ] Brook, The Ironhand Leader — p. 28 — `brook`
- [ ] Bruld — p. 29 — `bruld`
- [ ] Byot — p. 30 — `byot`
- [ ] Catharina, the Purple Witch Queen — p. 31 — `catharina`
- [ ] Cheiron, the  Doomspeaker Redeemer — p. 32 — `cheiron`
- [ ] Cildroly — p. 33 — `cildroly`
- [ ] Count Blake, the Vampire — p. 34 — `count_blake`
- [ ] D’craxis, the Regent of Doom — p. 35 — `dcraxis`
- [ ] Dernas, Magic Researcher — p. 36 — `dernas`
- [ ] Dillsgar — p. 37 — `dillsgar`
- [ ] Dorro — p. 38 — `dorro`
- [ ] Elarine, the Telepath — p. 39 — `elarine`
- [ ] Ellen Gideoni — p. 40 — `ellen_gideoni`
- [ ] Emperor, the Supreme — p. 41 — `emperor`
- [ ] Emperor's Companion: Imperial Tiger — p. 42 — `emperor_s_companion_imperial_tiger`
- [ ] Erithad, the Wise — p. 43 — `erithad`
- [ ] First Queen, The Supreme Hivemother Of The Kikokus — p. 44 — `first_queen`
- [ ] Fordak, the Master Awakened — p. 45 — `fordak`
- [ ] Fumetsu Tenshikin, The Eternal One — p. 46 — `fumetsu_tenshikin`
- [ ] Gaknak, the Dragonblade — p. 47 — `gaknak`
- [ ] Galender, the Weapon Master — p. 48 — `galender`
- [ ] Ganona — p. 49 — `ganona`
- [ ] Garion, the Rogue — p. 50 — `garion`
- [ ] Gazini, the Blood Drinker — p. 51 — `gazini`
- [ ] Gearot Michelline — p. 52 — `gearot_michelline`
- [ ] Gideoni, the High Hand of Krynnethoth — p. 53 — `gideoni`
- [ ] Golgöggoth, the Augur — p. 54 — `golgoggoth`
- [ ] Grace Sung, Counselor Abjurer — p. 55 — `grace_sung`
- [ ] Grizza, the Red Mother — p. 56 — `grizza`
- [ ] Gullog, the Deathbringer — p. 57 — `gullog`
- [ ] Gustigh Redfield, the Bloodbound Boss — p. 58 — `gustigh_redfield`
- [ ] Halthidon — p. 59 — `halthidon`
- [ ] Harun, the Doppleganger — p. 60 — `harun`
- [ ] Hell Dragon — p. 61 — `hell_dragon`
- [ ] Hoccugius, Counselor Transmuter — p. 62 — `hoccugius`
- [ ] Huradrin, the Dragon Clan King — p. 63 — `huradrin`
- [ ] Huradrin's Companion: Dungeon Eater — p. 64 — `huradrin_s_companion_dungeon_eater`
- [ ] Illindan, the Greenkeeper — p. 65 — `illindan`
- [ ] Isendden, the Gold Avatar Dragon — p. 66 — `isendden`
- [ ] Isumi — p. 67 — `isumi`
- [x] Jade, the Bard — p. 68 — `jade`
- [ ] Jessa, the Sheppard Redeemer — p. 69 — `jessa`
- [ ] Jocasta, the Pure — p. 70 — `jocasta`
- [ ] Jorana, the Amazon — p. 71 — `jorana`
- [ ] Juliet, the Ice Sorcerer — p. 72 — `juliet`
- [ ] Kalistessenâmun, the Lich Pharaoh — p. 73 — `kalistessenamun`
- [ ] Katar, the Barbarian — p. 74 — `katar`
- [ ] Kelanyah, the Lost Magic Researcher — p. 75 — `kelanyah`
- [ ] Kelorth, the White Avatar Dragon — p. 76 — `kelorth`
- [ ] Khloet, the Summoner — p. 77 — `khloet`
- [ ] Khloet's Companion: The Mummy — p. 78 — `khloet_s_companion_the_mummy`
- [ ] Kor'dal, the Juggernaut — p. 79 — `kor_dal`
- [ ] Lana, the Valkyrie — p. 80 — `lana`
- [ ] Lana's Companion: Sky High, The Pegasus — p. 81 — `lana_s_companion_sky_high`
- [ ] Liana, the Pirate — p. 82 — `liana`
- [x] Lizz, the Medusa — p. 83 — `lizz`
- [ ] Lypoec, the Penumbral Avatar Dragon — p. 84 — `lypoec`
- [ ] M'Bollo, the Warpriest — p. 85 — `m_bollo`
- [ ] Maryne, the Iron Maiden of the Seas — p. 86 — `maryne`
- [ ] Mavras, Captain of the Dragonheart Voyager — p. 87 — `mavras`
- [ ] Melantha, the Dragon Seeker — p. 88 — `melantha`
- [ ] Merlara, the Warden — p. 89 — `merlara`
- [ ] Morlogh, the Minotaur — p. 90 — `morlogh`
- [ ] Myr, The Death Merchants’ Leader — p. 91 — `myr`
- [x] Nakral, the Death Knight — p. 92 — `nakral`
- [ ] Niary, the Hunter — p. 93 — `niary`
- [ ] Oguemir, the Beast Master — p. 94 — `oguemir`
- [ ] Oguemir's Companion: Beast — p. 95 — `oguemir_s_companion_beast`
- [ ] Ohris, the Monk — p. 96 — `ohris`
- [ ] Oraelus — p. 97 — `oraelus`
- [ ] Orthus, the Warlock — p. 98 — `orthus`
- [ ] Orthus' Companion: Cerberus — p. 99 — `orthus_companion_cerberus`
- [ ] Oz Stormwind — p. 100 — `oz_stormwind`
- [ ] Paru-Haka, the High Priest of Ba-Ka — p. 101 — `paru_haka`
- [ ] Penumbral Tarrasque (Avatar of Droggath) — p. 102 — `penumbral_tarrasque_avatar_of_droggath`
- [ ] Pyraxis Stormcaller, Lord of Elemental Chaos — p. 103 — `pyraxis_stormcaller`
- [ ] Quizzarian, Prince of the Autumn Elves — p. 104 — `quizzarian`
- [ ] Ravel, the Arachne Madwalker — p. 105 — `ravel`
- [ ] Renkyr, the Dragon Avenger — p. 106 — `renkyr`
- [ ] Rigilia Choween, Minister of Magic — p. 107 — `rigilia_choween`
- [ ] Rokaru, the Samurai — p. 108 — `rokaru`
- [ ] Rurik, the Lion Clan King — p. 109 — `rurik`
- [ ] Saezz and Hizzas Skyborn — p. 110 — `saezz_and_hizzas_skyborn`
- [ ] Salevras, the Dark Collector — p. 111 — `salevras`
- [ ] Sara, the Seer — p. 112 — `sara`
- [ ] Se-Nâmum, the Vampiress High Priest — p. 113 — `se_namum`
- [ ] Sedrik, the Werewolf — p. 114 — `sedrik`
- [ ] Sedura, the Blue Dragonblade — p. 115 — `sedura`
- [ ] Shield, the Steel Golem — p. 116 — `shield`
- [ ] Simpson, the Republic Major — p. 117 — `simpson`
- [ ] Sir Erick, the Paladin — p. 118 — `sir_erick`
- [ ] Solnertha, the master of necromancy — p. 119 — `solnertha`
- [ ] Sundaryll, King of the Autumn Elves — p. 120 — `sundaryll`
- [ ] Sylithia Silverblade — p. 121 — `sylithia_silverblade`
- [ ] T´ssir, the Visionary — p. 122 — `t_ssir`
- [ ] Talessa, the Dragon Queen — p. 123 — `talessa`
- [ ] Taram, the High Cardinal — p. 124 — `taram`
- [ ] Tellatius, the Prime Minister — p. 125 — `tellatius`
- [ ] Teraphas, the Tactical Specialist — p. 126 — `teraphas`
- [ ] Thalia, the Druid — p. 127 — `thalia`
- [ ] Thidexius High Imperial Wizard — p. 128 — `thidexius_high_imperial_wizard`
- [ ] Thyra, the Blue Avatar Dragon — p. 129 — `thyra`
- [ ] Tiwilade, Hajal Financial Tycoon — p. 130 — `tiwilade`
- [ ] Tribin & Ironsnout, the War Boar (Tribin’s companion) — p. 131 — `tribin_ironsnout`
- [ ] Trygve, the Werebear King of Kolbjörn — p. 132 — `trygve`
- [ ] Tsuyoko, the Ninja — p. 133 — `tsuyoko`
- [ ] Tuani — p. 134 — `tuani`
- [ ] Twin Tigers, Asineus & Diamus — p. 135 — `twin_tigers`
- [ ] Tyreen, the Last Queen of Mystical — p. 136 — `tyreen`
- [ ] U´Tibam, the Inventor — p. 137 — `u_tibam`
- [ ] Ukhumlim Stronghope, the Goat Clan King — p. 138 — `ukhumlim_stronghope`
- [ ] Uster, the Necromancer — p. 139 — `uster`
- [ ] Vaeraunt, the Mastermind — p. 140 — `vaeraunt`
- [ ] Vaklav, the Judge — p. 141 — `vaklav`
- [ ] Vanarus, the Demon — p. 142 — `vanarus`
- [ ] Velord, the Barbarian Warchief — p. 143 — `velord`
- [ ] Vilani, the Air Elementalist — p. 144 — `vilani`
- [ ] Vradok — p. 145 — `vradok`
- [ ] Wynna Jannis, the Relic Hunters Guild Master — p. 146 — `wynna_jannis`
- [ ] Xateri, the Black Mother — p. 147 — `xateri`
- [x] Xorius, the Archers’ General — p. 148 — `xorius`
- [ ] Yemi, the Succubus of Envy — p. 149 — `yemi`
- [ ] Yokensha, the Bauronite Dragoness — p. 150 — `yokensha`
- [ ] Yrizard, the Spy General — p. 151 — `yrizard`
- [ ] Yviah, the Frost Giant Queen — p. 152 — `yviah`
- [ ] Zafara, the Fallen Angel — p. 153 — `zafara`
- [ ] Zaladrix, the Silver Dragon — p. 154 — `zaladrix`
- [ ] Zaldrus, the Black Avatar Dragon — p. 155 — `zaldrus`
- [ ] Zalir Draconis — p. 156 — `zalir_draconis`
- [ ] Zanac, the Fire Sorcerer — p. 157 — `zanac`
- [ ] Zarumag, the Fallen Avatar Dragon — p. 158 — `zarumag`
- [ ] Zhaeral, the High Priestess of Bás — p. 159 — `zhaeral`
- [ ] Zhelahra, the Merfolk Queen — p. 160 — `zhelahra`
- [ ] Zimmess, The Dragon Executioner — p. 161 — `zimmess`
- [ ] Zisenuh, the Infantry Leader — p. 162 — `zisenuh`
- [ ] Zund, the Illusionist — p. 163 — `zund`

## Bilan

Statut : **ouvert le 16 septembre 2026, hors feuille de route.** Ce lot ne figure pas dans la
[feuille de route](@ref roadmap) et n'entre dans aucun jalon : c'est un chantier long, alimenté
au fil de l'eau, au rythme du budget de génération. Cinq PNJ sur 160 sont cochés.
Exigences : `EX-CNT-023` (le corpus reste hors dépôt), `EX-VIS-007` (les tailles en donnée).
Aucune exigence ajoutée.

La méthode a été établie par la [preuve de concept](@ref lot-91-poc) (T0, close le
16 septembre 2026) sur cinq PNJ : Anariel, Lizz, Xorius, Nakral et Jade. Le journal du PoC garde
chaque tour, chaque défaut et chaque décision ; cette fiche n'en garde que la méthode.

- @subpage lot-91-poc — la preuve de concept, son journal et son verdict.
