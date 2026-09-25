// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Presentation/RpgScreens.h
 * @brief Les écrans du RPG, nommés (`LOT-68`, `EX-IHM-090`).
 *
 * Logique **pure** (aucune dépendance Qt). Le `LOT-68` décrivait ici l'ossature de chaque écran en
 * données ; depuis le `LOT-86`, un écran est un formulaire QML (`Source/Ui/Screens/*Form.ui.qml`)
 * que la pile d'écrans pose quand `hmi::ScreenRouter` le désigne, et il ne reste de la table que
 * cette énumération.
 */

namespace hmi {

/// Les neuf écrans du RPG. `hmi::ScreenRouter::RpgScreen` les reprend valeur pour valeur pour le
/// QML ; des `static_assert` de `ScreenRouter.cpp` tiennent les deux énumérations alignées.
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

}  // namespace hmi
