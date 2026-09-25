// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <span>

/**
 * @file HMI/Presentation/RpgScreens.h
 * @brief Catalogue des écrans du RPG et **ossature** de leur contenu (`LOT-68`, `EX-IHM-090`,
 *        `EX-IHM-091`).
 *
 * Logique **pure** (aucune dépendance Qt), testable hors instance d'application (`EX-NFR-010`) --
 * même patron que `HMI/Presentation/ScreenFlow.h`.
 *
 * ## Une table, et non huit écrans écrits à la main
 *
 * Les huit écrans que le RPG doit à terme porter -- fiche, inventaire, journal, carte, dialogue,
 * marchand, tableau de la Guilde, ATH de combat -- n'ont, à ce lot, aucun contenu à afficher : les
 * données viendront des lots qui les remplissent (`LOT-38`, `LOT-42`, `LOT-45`, `LOT-24`). Ce qui
 * est livré ici est ce qu'ils ont **en commun** : leur cadre, leur navigation, leur parcours de
 * focus, et leur **ossature**.
 *
 * Cette ossature est décrite en **données** plutôt qu'en code d'interface, pour la raison qu'écrit
 * `EX-IHM-090` : un neuvième écran doit coûter une **ligne de table**, jamais une retouche des huit
 * autres. Les formulaires Qt Quick (`Source/Ui/Screens/*Form.ui.qml`) posent les sept genres de
 * blocs ci-dessous et rien de plus ; la table n'a aucune connaissance d'un écran en particulier.
 */

namespace hmi {

/// Les neuf écrans du RPG. L'ordre est celui du **cycle** de navigation
/// (`nextRpgScreen`/`previousRpgScreen`) et celui de la table de `rpgScreens()`.
enum class RpgScreenId {
    CharacterSheet,  ///< Fiche de personnage (remplie par le `LOT-38`).
    /// Compétences et sorts (maquette 10, `LOT-87` T3.8) : attaques, sortilèges, écoles de magie.
    /// Ouvert depuis la fiche ; rempli par le `LOT-35`.
    Skills,
    Inventory,     ///< Inventaire et équipement (`LOT-14`).
    QuestJournal,  ///< Journal de quêtes (`LOT-16`).
    WorldMap,      ///< Carte du monde (`LOT-42`).
    Dialogue,      ///< Dialogue avec un PNJ (`LOT-15`).
    Merchant,      ///< Marchand (`LOT-26`).
    /// Équipe de **mercenaires** (maquette 09, `LOT-87` T3.7) : la feuille d'équipe (`LOT-38`)
    /// et le tableau de la Guilde réunis en quatre onglets. Remplie par `LOT-45` et `LOT-83`.
    Company,
    CombatHud,  ///< Affichage tête haute de combat (`LOT-24`).
};

/// Nombre d'écrans du catalogue.
inline constexpr std::size_t RPG_SCREEN_COUNT = 9;

/**
 * @brief Règle de **superposition** d'un écran (`EX-IHM-091`) : ce qui met le jeu en pause et ce
 *        qui se consulte en marchant.
 *
 * Portée par la **description** de l'écran, jamais par le code qui l'ouvre : une règle décidée à
 * l'ouverture se contredit d'un point d'appel à l'autre, et personne ne peut la relire d'un coup
 * d'œil.
 */
enum class RpgSuperposition {
    /// La simulation est **suspendue** tant que l'écran est ouvert : on ne lit pas sa fiche en
    /// esquivant un gobelin.
    PausesGame,
    /// L'écran se consulte **en marchant** : la simulation continue derrière lui.
    WhileWalking,
};

/// Genres de blocs que le châssis sait peindre. Volontairement peu nombreux : sept formes
/// suffisent aux huit écrans, et un genre ajouté pour un seul écran est le premier pas vers huit
/// écrans écrits à la main.
enum class RpgBlockKind {
    Fields,     ///< Lignes « libellé → valeur ». `labelKeys` en donne les libellés.
    Grid,       ///< Grille de cases vides (sac, caractéristiques) : `columns` x `rows`.
    List,       ///< Liste d'entrées vides (quêtes, marchandises) : `rows` lignes.
    Prose,      ///< Bloc de texte suivi (réplique, description de contrat).
    Portrait,   ///< Cadre d'illustration carré (interlocuteur, blason).
    Track,      ///< Suite horizontale de jetons (ordre d'initiative) : `columns` jetons.
    ActionBar,  ///< Barre d'actions : `columns` cases larges, alignées en bas.
};

/**
 * @brief Une ligne « libellé → valeur » d'un bloc `Fields`.
 *
 * L'**identifiant de valeur** est ce par quoi un écran se remplit (`LOT-38`) : le châssis pose le
 * libellé traduit à gauche, et attend une valeur à droite, sous cet identifiant. Un identifiant
 * vide dit « ce champ n'a pas encore de source » — il reste au tiret cadratin, et c'est une
 * information, pas un oubli : la liste de ces champs est le périmètre restant, lisible d'un coup
 * d'œil dans la table.
 *
 * Un identifiant, jamais la clé de traduction : « Nom » se dit sur la fiche, dans un dialogue et
 * sur une cible de combat, et ces trois-là ne valent pas la même chose.
 */
struct RpgField {
    const char* labelKey = "";
    const char* valueId = "";
};

/// Un bloc de l'ossature d'un écran.
struct RpgContentBlock {
    const char* titleKey = "";  ///< Clé du titre du bloc ; vide pour un bloc sans titre.
    RpgBlockKind kind = RpgBlockKind::Fields;
    int columns = 0;  ///< `Grid`, `Track`, `ActionBar` : nombre de colonnes/jetons.
    /// `Grid` : nombre de lignes. `List` : nombre de lignes **quand** `valueIds` est vide ; sinon
    /// c'est la longueur de `valueIds` qui décide, pour que les deux ne puissent pas diverger.
    int rows = 0;
    /// `Fields` : une ligne par entrée. Vide pour les autres genres.
    std::span<const RpgField> fields{};
    /// `List` et `Prose` : l'identifiant de valeur de chaque ligne (`List`) ou du paragraphe
    /// (`Prose`). Vide tant que rien ne les alimente.
    std::span<const char* const> valueIds{};
};

/// Ossature d'un écran : deux colonnes de blocs. La colonne droite peut être vide -- l'écran
/// occupe alors toute la largeur, ce qui est le cas du dialogue.
struct RpgScreenLayout {
    std::span<const RpgContentBlock> leftColumn{};
    std::span<const RpgContentBlock> rightColumn{};
};

/**
 * @brief Comment un écran est **rendu** (`LOT-38`).
 *
 * Le `LOT-68` n'avait qu'une voie : une ossature en données, peinte par un rendu générique. C'était
 * le bon outil pour huit écrans dont aucune maquette n'existe. Un écran qui **a** une maquette
 * gravée — la fiche de personnage et ses cinq planches — mérite mieux qu'un résumé en deux
 * colonnes : il reçoit sa planche, décrite en Qt Designer, et garde le même cadre, la même
 * navigation et le même pied d'actions que les autres.
 *
 * Le champ est ici, dans la table, et non deviné par le code qui construit les écrans : c'est la
 * description de l'écran qui dit comment il se rend, exactement comme elle dit s'il suspend la
 * simulation.
 */
/// Description complète d'un écran du RPG.
struct RpgScreenDescriptor {
    RpgScreenId id = RpgScreenId::CharacterSheet;
    /// `objectName` du widget, **ciblé par le thème** (`theme-identity.qss`). Un nom, jamais un
    /// indice : insérer un écran dans l'énumération ne doit pas déplacer l'habillage d'un autre.
    const char* objectName = "";
    const char* titleKey = "";  ///< Clé du titre de l'écran (catalogue de traduction).
    RpgSuperposition superposition = RpgSuperposition::PausesGame;
    /// L'ossature de l'écran : ce que son formulaire Qt Quick en pose.
    RpgScreenLayout layout{};
};

/// @return La table des écrans, dans l'ordre du cycle de navigation.
[[nodiscard]] std::span<const RpgScreenDescriptor> rpgScreens() noexcept;

/// @return La description de @p screen.
[[nodiscard]] const RpgScreenDescriptor& rpgScreenDescriptor(RpgScreenId screen) noexcept;

/// @return L'écran suivant dans le cycle ; du dernier on revient au premier. C'est ce qui permet
///         d'atteindre n'importe quel écran depuis n'importe quel autre sans repasser par le menu
///         (`EX-IHM-090`).
[[nodiscard]] RpgScreenId nextRpgScreen(RpgScreenId screen) noexcept;

/// @return L'écran précédent dans le cycle ; du premier on revient au dernier.
[[nodiscard]] RpgScreenId previousRpgScreen(RpgScreenId screen) noexcept;

/// @return `true` si @p screen suspend la simulation (`EX-IHM-091`).
[[nodiscard]] bool pausesGame(RpgScreenId screen) noexcept;

}  // namespace hmi
