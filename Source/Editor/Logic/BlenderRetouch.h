// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Editor/Logic/CharacterWorkshop.h"

/**
 * @file Editor/Logic/BlenderRetouch.h
 * @brief L'**aller-retour par Blender** d'un personnage (`LOT-1008`, `EX-EDIT-103`, décision
 *        D-44) : ce que l'atelier des assets lance pour que l'auteur règle, à la souris, les
 *        articulations et les clips d'un modèle.
 *
 * L'éditeur ne parle pas à Blender : il lance le script de la chaîne
 * (`scripts/assetsGeneration/retouch_character.py`), qui ouvre le modèle lié dans Blender, puis
 * relit ce que l'auteur y a réglé. Rien ne revient de Blender qu'en **données** — une
 * articulation déplacée dans la fiche de liaison, un clip modifié dans la fiche de retouche — et
 * le modèle est relié par `rig_character.py`, puis contrôlé : la chaîne reste rejouable.
 *
 * Ici, la part **pure** : où sont les fichiers de l'atelier, où sont Python, Blender et le script,
 * et les deux lignes de commande. La fenêtre les exécute (`QProcess`).
 */

namespace hmi {

/// @brief Les fichiers de l'aller-retour d'un personnage, en chemins absolus.
struct RetouchFiles {
    /// Le modèle lié : ce que Blender ouvre, et ce que l'import réécrit.
    std::filesystem::path model;
    /// Le maillage reçu, que l'import relie.
    std::filesystem::path received;
    /// La fiche de liaison.
    std::filesystem::path sheet;
    /// La fiche de retouche : à défaut, `retouche.json` à côté de la fiche de liaison.
    std::filesystem::path retouch;
    /// Le fichier Blender : à défaut, à côté du modèle lié, à son nom.
    std::filesystem::path blend;
    /// La description du squelette installé : les os et les clips que le modèle doit tenir.
    std::filesystem::path skeleton;

    [[nodiscard]] bool operator==(const RetouchFiles&) const = default;
};

/**
 * @brief Les fichiers de l'aller-retour de @p draft.
 * @param dataRoot      La racine des données, où le squelette installé se lit.
 * @param baseDirectory Le dossier de la fiche d'atelier, d'où part sa racine.
 * @param draft         La fiche d'atelier.
 */
[[nodiscard]] RetouchFiles retouchFiles(const std::filesystem::path& dataRoot,
                                        const std::filesystem::path& baseDirectory,
                                        const CharacterDraft& draft);

/// @return Ce qui manque à @p files pour ouvrir Blender (anglais), vide si rien : le modèle lié,
///         le maillage reçu et la fiche de liaison doivent exister — sans eux l'import ne
///         pourrait pas relier.
[[nodiscard]] std::string retouchReadiness(const RetouchFiles& files);

/// @brief Les outils de l'aller-retour : l'interpréteur Python, Blender et le script de la chaîne.
struct RetouchTools {
    /// L'interpréteur, et les arguments qui le précisent (`py`, `-3`).
    std::string python;
    std::vector<std::string> pythonArguments;
    std::filesystem::path blender;
    std::filesystem::path script;

    [[nodiscard]] bool operator==(const RetouchTools&) const = default;
};

/// Lit une variable d'environnement ; rien si elle n'est pas définie.
using EnvironmentLookup = std::function<std::optional<std::string>(const std::string&)>;

/**
 * @brief Cherche les outils de l'aller-retour.
 *
 * - le **script** : `scripts/assetsGeneration/retouch_character.py`, en remontant depuis
 *   @p dataRoot jusqu'à la racine du dépôt ;
 * - **Python** : la variable `JADG_PYTHON`, à défaut le lanceur `py -3` sous Windows, `python3`
 *   ailleurs ;
 * - **Blender** : la variable `BLENDER`, à défaut son emplacement par défaut sur le poste — le
 *   même ordre que `reduce_model.find_blender`.
 *
 * @param dataRoot    La racine des données.
 * @param environment Lit l'environnement ; injecté pour les tests.
 * @param error       Reçoit ce qui manque (anglais), vide si tout est trouvé.
 */
[[nodiscard]] RetouchTools findRetouchTools(const std::filesystem::path& dataRoot,
                                            const EnvironmentLookup& environment,
                                            std::string& error);

/// @brief Un programme et ses arguments, prêts pour `QProcess`.
struct ProcessCommand {
    std::string program;
    std::vector<std::string> arguments;

    [[nodiscard]] bool operator==(const ProcessCommand&) const = default;
};

/// @return La commande qui ouvre @p files dans Blender (`retouch_character.py open`).
[[nodiscard]] ProcessCommand openInBlenderCommand(const RetouchTools& tools,
                                                  const RetouchFiles& files);

/// @return La commande qui relit Blender, relie le modèle et le contrôle
///         (`retouch_character.py import … --source … --output …`).
[[nodiscard]] ProcessCommand importFromBlenderCommand(const RetouchTools& tools,
                                                      const RetouchFiles& files);

}  // namespace hmi
