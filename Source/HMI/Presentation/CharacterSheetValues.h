// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <map>
#include <string>

#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/Skill.h"

/**
 * @file HMI/Presentation/CharacterSheetValues.h
 * @brief Ce que l'écran de fiche affiche, calculé hors de tout élément d'interface (`LOT-38`).
 */

namespace hmi {

/**
 * @brief Traduit une fiche de personnage en **valeurs affichables**, indexées par un identifiant
 *        stable (`sheet.hit_points`) que `hmi::CharacterSheetModel` publie au QML.
 *
 * Logique **pure** : aucune dépendance Qt, aucun accès disque (`EX-NFR-010`). C'est ce qui permet
 * de vérifier par test qu'un modificateur s'affiche `+3` et non `3`, qu'une compétence maîtrisée
 * compte son bonus de maîtrise, et que la Perception passive vaut bien 10 + le modificateur — sans
 * ouvrir de fenêtre.
 *
 * ## Pourquoi une table de chaînes, et non une structure de champs
 *
 * L'écran est un formulaire QML (`CharacterSheetForm.ui.qml`) alimenté par
 * `hmi::CharacterSheetModel`, qui nomme ces identifiants une fois, en propriétés. Une structure à
 * quarante membres obligerait à toucher trois fichiers pour chaque champ ajouté — la structure, le
 * modèle, et le code qui les relie. Ici, l'identifiant est le seul contrat entre cette fonction et
 * le modèle qui la publie ; les tests de la fonction vérifient chaque valeur produite.
 *
 * ## Le formatage est ici, la traduction reste dehors
 *
 * Les nombres sont mis en forme ici (signe d'un modificateur, `12 / 18` de points de vie) parce
 * que c'est une règle d'affichage, pas une question de langue. Les **noms** — espèce, classe,
 * historique — viennent des catalogues, qui les portent déjà dans la langue du corpus ; les
 * traduire est le sujet du lexique du `LOT-30`, pas de ce fichier.
 */
struct CharacterSheetContext {
    const core::CharacterSheet* sheet = nullptr;
    const core::CharacterOptions* options = nullptr;
    const core::ExperienceTable* experience = nullptr;
    const core::SkillCatalog* skills = nullptr;
    /// Ce que l'équipement porté produit (`core::derivedStatsFor`, `LOT-14`). **Présent**, il
    /// remplace la classe d'armure et la vitesse de la fiche : celles-ci sont calculées à la
    /// construction, sans rien savoir de l'armure endossée depuis. Absent, la fiche donne les
    /// siennes — un personnage sans inventaire n'est pas un personnage sans armure, c'est un
    /// personnage dont on ne sait pas ce qu'il porte.
    const core::DerivedStats* derived = nullptr;
    /// Texte affiché lorsqu'une valeur existe mais est vide (nom d'espèce inconnu du catalogue,
    /// par exemple). Le même tiret cadratin que le châssis pose sur un champ sans source : à
    /// l'écran, « inconnu » et « pas encore alimenté » se ressemblent, et rien ne gagne à les
    /// distinguer par deux signes différents.
    std::string emptyMark = "—";
};

/// @return Les valeurs de la fiche, prêtes à être publiées par `hmi::CharacterSheetModel::values`.
///         Un contexte incomplet (fiche absente) rend une table **vide**, jamais des zéros :
///         l'écran garde alors ses tirets, ce qui est la vérité.
[[nodiscard]] std::map<std::string, std::string> characterSheetValues(
    const CharacterSheetContext& context);

}  // namespace hmi
