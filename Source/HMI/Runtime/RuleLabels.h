// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <string>
#include <string_view>

/**
 * @file HMI/Runtime/RuleLabels.h
 * @brief Le vocabulaire des **règles**, traduit (`LOT-86`).
 */

namespace hmi {

/**
 * @brief Nom lisible d'un terme de règle — caractéristique, emplacement d'équipement, condition.
 *
 * ## Pourquoi ceux-là ne passent pas par `qsTr`
 *
 * La chrome des écrans (« Points de vie », « Nouvelle partie ») s'écrit en français **dans le
 * fichier QML**, et Qt la traduit par sa source. Ces libellés-ci ne le peuvent pas : leur clé est
 * **calculée** (`rpg.ability.` + l'identifiant que le modèle rend), et `qsTr` exige une chaîne
 * littérale — c'est ce qui permet à `lupdate` de l'extraire sans exécuter le programme.
 *
 * Ce n'est pas qu'une contrainte technique. Ces termes sont un **lexique** : `rpg.glossary.csv`
 * garantit une seule traduction par terme de règle dans tout le jeu, et `scripts/checks/check_glossary.py`
 * le vérifie. Les disperser en littéraux dans les écrans casserait précisément cette garantie.
 *
 * @param key      Clé du catalogue (« rpg.ability.strength »).
 * @param language Langue demandée (« fr », « en »).
 * @return Le libellé traduit ; la **clé elle-même** si le catalogue ne la porte pas — jamais une
 *         chaîne vide, qui donnerait un écran troué sans dire pourquoi.
 */
[[nodiscard]] std::string ruleLabel(std::string_view key, const std::string& language);

/// @return La langue de l'IHM telle que les réglages la persistent (« fr » par défaut).
[[nodiscard]] std::string activeLanguage();

}  // namespace hmi
