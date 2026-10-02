# Mentions de tiers

Ce fichier recense les composants tiers redistribués avec JustAnotherRpgGame, ou dont il dépend au
build, avec leur licence. Il ne remplace pas les fichiers de licence livrés à côté des ressources
concernées : il y renvoie.

JustAnotherRpgGame est un **fan game non commercial**. Son code est sous **PolyForm Noncommercial
1.0.0** ([`LICENSE`](LICENSE)), ses contenus originaux sous **CC BY-NC-SA 4.0**
([`LICENSE-CONTENT`](LICENSE-CONTENT)).

> **À ne pas confondre.** Les en-têtes `SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0`
> marquent le **code du projet** — `Source/**/*.h`, `Source/**/*.cpp`, `scripts/*.py`, les
> `CMakeLists.txt` — et **rien d'autre**. Les images, les polices, les bibliothèques
> tierces et les univers dont le jeu s'inspire gardent chacun leur propre régime, listé ci-dessous.
> Voir un fichier `.cpp` marqué PolyForm ne dit **rien** de la licence d'un `.png` du même dépôt.

Ces mentions sont également affichées **dans le jeu**, à l'écran *Crédits* : un utilisateur qui
n'ouvrira jamais ce fichier doit tout de même savoir que le jeu embarque Qt sous LGPLv3 et des
polices sous SIL OFL, et qu'il emprunte à Dungeons & Dragons et à Tanares. La LGPLv3 et la SIL OFL
l'exigent.

Compatibilité vérifiée : la **LGPLv3** n'impose rien à la licence d'une application **liée
dynamiquement** à la bibliothèque, donc Qt et une licence non commerciale cohabitent sans conflit.
La licence des polices (SIL OFL) et celles des bibliothèques (MIT, BSD 3-Clause, Apache 2.0) sont
permissives : elles autorisent leur usage dans un projet sous une autre licence, à charge d'en
conserver les mentions.

## Univers, règles et marques

Le jeu s'inspire de deux univers de jeu de rôle **qui ne lui appartiennent pas**. Aucune licence du
projet ne s'étend à eux, et le projet n'est **ni affilié, ni approuvé, ni soutenu** par leurs ayants
droit. Les livres ne sont pas versionnés (`Documentation/SourceBook/`, hors dépôt) ; les données
qui en sont extraites portent leur provenance dans un champ `"source"` imposé par schéma.

| Univers ou ouvrage | Ayant droit | Régime | Ce que le jeu en tire |
|---|---|---|---|
| **System Reference Document 5.1** | Wizards of the Coast LLC | **CC BY 4.0** — seule partie ouverte | mécaniques du d20 (`"source": "srd"`) |
| *Dungeons & Dragons*, *D&D* | Wizards of the Coast LLC | **marques déposées** | inspiration ; aucune marque dans le nom ni le logo du jeu |
| *Manuel des Joueurs*, *Guide du Maître*, *Manuel des Monstres* (5ᵉ éd., VF) | © Wizards of the Coast LLC, VF Black Book Éditions | tous droits réservés | règles, vocabulaire et bestiaire (`"source": "phb-fr"`) |
| *Basic Rules* en français | © Wizards of the Coast LLC, traduction communautaire [AideDD](https://www.aidedd.org/) | tous droits réservés | règles de base |
| *Player's Guide to Tanares*, *Tanares Sourcebook*, feuilles de personnage, cartes VTT | © **Dragori Games, Inc.** | tous droits réservés ; « Tanares », « Penumbral Plane » et les noms réservés sont *Product Identity* | monde, régions, espèces, dialogues (`"source": "tanares"`) ; **aucune image** : les deux cartes du monde extraites ont été retirées (`LOT-94`), et [`illustrations.json`](Source/Elements/Assets/UI/illustrations.json) refuse cette provenance ; les cartes de l'écran « Carte » ([`Assets/Maps/`](Source/Elements/Assets/Maps/manifest.json)) sont **peintes par l'auteur** d'après la géographie de ce monde, sans lettrage, et aucune n'est une image du livre |

Attribution exigée par la CC BY 4.0 du SRD :

> This work includes material taken from the System Reference Document 5.1 (“SRD 5.1”) by Wizards of
> the Coast LLC and available at <https://dnd.wizards.com/resources/systems-reference-document>. The
> SRD 5.1 is licensed under the Creative Commons Attribution 4.0 International License available at
> <https://creativecommons.org/licenses/by/4.0/legalcode>.

**Pourquoi c'est tenable, et jusqu'où.** Le projet est privé, gratuit et non commercial. Ce statut ne
vaut **pas** autorisation : la politique de *fan content* de Wizards of the Coast exclut
explicitement les jeux, même gratuits, et Dragori Games ne publie aucune politique équivalente.
Toute diffusion publique supposerait donc l'accord écrit de Dragori Games pour Tanares, et de ne
garder de D&D que le SRD. Un ayant droit qui demande le retrait d'un élément l'obtient.

## Bibliothèques

| Composant | Version | Licence | Mode | Redistribué ? |
|---|---|---|---|---|
| [Qt](https://www.qt.io/) | 6.11.2 (`win64_msvc2022_64`) | **LGPLv3** | Provisionné hors dépôt | **Oui** — DLL déployées à côté de l'exécutable (`windeployqt`) |
| [GoogleTest](https://github.com/google/googletest) | v1.15.2 | BSD 3-Clause | FetchContent | Non — tests seulement |
| [nlohmann/json](https://github.com/nlohmann/json) | v3.11.3 | MIT | FetchContent | Non — en-têtes compilés dans l'exécutable |
| [Google Benchmark](https://github.com/google/benchmark) | v1.9.5 | Apache 2.0 | FetchContent, avec `BUILD_BENCHMARKS` | Non — mesures de performance seulement |
| [aqtinstall](https://github.com/miurahr/aqtinstall) | commit épinglé | MIT | Outil de build | Non |

Le **chargeur de maillages** (`core::readMeshFile`, `LOT-1003`) lit le format glTF 2.0 binaire
(`.glb`), spécification ouverte du Khronos Group, sans bibliothèque dédiée : l'enveloppe binaire est
lue par le code du projet, son bloc JSON par **nlohmann/json**, déjà lié, et l'image incorporée est
décodée par **Qt** (`QImage`). Qt Quick 3D n'est pas utilisé — il n'est distribué que sous GPLv3 ou
licence commerciale.

`DirectX` ne figure pas ici : il provient du **Windows SDK** et relève de sa licence, comme tout
composant du système d'exploitation.

### Qt et la LGPLv3 — la seule obligation qui pèse réellement

Qt est utilisé sous **LGPLv3**, en **lien dynamique**. C'est ce qui permet de ne pas publier le
source du jeu. En contrepartie, trois obligations doivent rester vraies, et elles le sont :

1. **Le lien reste dynamique.** Les DLL Qt sont déployées à côté de l'exécutable par `windeployqt`,
   jamais liées statiquement — un utilisateur peut donc les remplacer par sa propre version de Qt.
2. **L'usage de Qt et sa licence sont mentionnés** — c'est l'objet de ce fichier.
3. **Aucune modification de Qt n'est distribuée.** Le projet consomme Qt tel quel ; si cela changeait
   un jour, les sources modifiées devraient être publiées sous LGPLv3.

Passer Qt en lien **statique** invaliderait le point 1 et changerait les obligations du projet : ce
n'est pas un détail d'optimisation, c'est une décision de licence.

## Ressources

Toutes les ressources tierces sont déjà créditées à l'endroit où elles vivent — ce tableau n'en est
que l'index.

| Type | Emplacement | Licence | Détail |
|---|---|---|---|
| Graphismes | `Source/Elements/Assets/` | aucune ressource tierce : images propres au projet | [`Assets/CREDITS.md`](Source/Elements/Assets/CREDITS.md) |
| Police *Cinzel* | `Source/Elements/Assets/Fonts/` | SIL Open Font License 1.1 | `Cinzel-LICENSE.txt` |
| Police *IM Fell English* | `Source/Elements/Assets/Fonts/` | SIL Open Font License 1.1 | `IMFellEnglish-LICENSE.txt` |
| Police *Pinyon Script* | `Source/Elements/Assets/Fonts/` | SIL Open Font License 1.1 | `PinyonScript-LICENSE.txt` |

La **SIL OFL** exige une attribution : les fichiers de licence doivent accompagner les polices
partout où elles sont redistribuées.

## Entretien

Ce fichier se met à jour **avec** la dépendance qu'il décrit, jamais après coup : ajouter une
bibliothèque ou une ressource tierce sans l'inscrire ici, c'est perdre l'information au moment où
elle est encore connue. Le tableau des dépendances de build vit en double dans
[`External/README.md`](External/README.md), qui en donne le *mode de provisionnement* ; ici, seule
la **licence** compte.
