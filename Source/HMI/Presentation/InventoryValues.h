// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <map>
#include <string>

#include "Core/Rpg/Inventory.h"

/**
 * @file HMI/Presentation/InventoryValues.h
 * @brief Ce que l'écran d'inventaire affiche, calculé hors de tout widget (`LOT-14`).
 */

namespace hmi {

/**
 * @brief Traduit un inventaire en **valeurs affichables**, indexées par l'identifiant que la table
 *        de la planche 2 déclare (`hmi::PlateField::valueId`).
 *
 * Même patron que `hmi::characterSheetValues` (`LOT-38`), et pour la même raison : logique
 * **pure**, sans Qt ni disque (`EX-NFR-010`), de sorte qu'on vérifie par test qu'un sac de trois
 * torches s'affiche « Torche ×3 » et qu'une bourse se répartit en or, argent et cuivre — sans
 * ouvrir de fenêtre.
 */
struct InventoryContext {
    const core::Inventory* inventory = nullptr;
    core::ItemLookup lookup{};
    /// Ce que l'équipement porté produit (`core::derivedStatsFor`) : poids porté, capacité de
    /// charge, encombrement. **Recalculé**, jamais accumulé — c'est le critère du lot.
    core::DerivedStats derived{};
    /// Texte affiché lorsqu'un emplacement est vide. Le même tiret cadratin que le reste des
    /// écrans : « rien à cet emplacement » et « pas encore alimenté » se ressemblent à l'écran, et
    /// rien ne gagne à les distinguer par deux signes.
    std::string emptyMark = "—";
};

/// @return Les valeurs de l'inventaire, prêtes à être publiées par `hmi::InventoryModel`. Un
/// contexte
///         sans inventaire rend une table **vide** : l'écran garde alors ses tirets.
[[nodiscard]] std::map<std::string, std::string> inventoryValues(const InventoryContext& context);

/**
 * @brief Répartit une somme en **pièces de cuivre** entre or, argent et cuivre.
 *
 * Le modèle ne connaît qu'une unité — le cuivre — parce qu'une seule unité interne supprime les
 * conversions dispersées (`LOT-32`). La feuille, elle, a **trois** médaillons de monnaie gravés :
 * la répartition est donc une règle d'affichage, et elle vit ici plutôt que dans le modèle.
 *
 * @param copper Somme totale, en pièces de cuivre.
 * @param gold   Reçoit le nombre de pièces d'or.
 * @param silver Reçoit le nombre de pièces d'argent.
 * @param rest   Reçoit ce qui demeure, en pièces de cuivre.
 */
void splitPurse(int copper, int& gold, int& silver, int& rest);

}  // namespace hmi
