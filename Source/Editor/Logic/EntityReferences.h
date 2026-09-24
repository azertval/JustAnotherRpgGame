// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/Encounter.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/WorldGraph.h"

/**
 * @file Editor/Logic/EntityReferences.h
 * @brief Ce que le panneau « Entités » propose et vérifie : dialogues, rencontres, cartes et points
 *        d'arrivée, lus des catalogues (`LOT-11`).
 */

namespace hmi {

/**
 * @brief Les catalogues que les entités d'une carte référencent, lus une fois à l'ouverture de
 *        l'éditeur puis à la demande (enregistrement d'une carte, rechargement des assets).
 */
struct EditorReferences {
    /// Identifiants des dialogues **acceptés** au chargement, triés. Un dialogue refusé ne se
    /// propose pas : le poser produirait un PNJ muet en jeu.
    std::vector<std::string> dialogues;
    core::EncounterCatalog encounters;
    /// Pour l'emprise des créatures d'une rencontre (`core::analyzeEncounterTerrain`).
    core::Bestiary bestiary;
    core::WorldGraph world;
    /// Les figurines que toute carte peut poser, triées : les slugs de `Assets/Npc/manifest.json`,
    /// `Monsters/<slug>` pour chaque monstre de `Assets/Monsters/manifest.json`, et les slugs du
    /// monde (`Assets/Common/Characters`, `LOT-124`). Celles d'un lieu s'y ajoutent carte par carte
    /// (`referenceContext`).
    std::vector<std::string> figures;
    /// Le dossier `Assets/` où se cherchent les figurines d'un lieu ; vide sans données.
    std::filesystem::path assets;
    /// Les drapeaux qu'un dialogue accepté pose (`SetFlag`, et le drapeau d'une quête démarrée),
    /// triés.
    std::vector<std::string> flags;
    /// Les fiches de lieu de l'atlas (`World/locations/<id>.json`), triées.
    std::vector<std::string> locations;
    /// Les objets du catalogue (`Rpg/items`), triés.
    std::vector<std::string> items;
};

/**
 * @brief Lit les catalogues sous @p root, la racine des éléments déployés à côté de l'exécutable :
 *        `World/dialogues`, `World/locations`, `Rpg/encounters`, `Rpg/creatures`, `Rpg/items`,
 *        les manifestes des figurines sous `Assets`, et `Levels`.
 *
 * Un dossier absent donne un catalogue vide, jamais une exception (`EX-NFR-040`) : l'éditeur reste
 * utilisable, et les références qu'il ne peut vérifier sont signalées comme inconnues.
 */
[[nodiscard]] EditorReferences loadEditorReferences(const std::filesystem::path& root);

/**
 * @brief Le contexte de validation des entités de la carte éditée.
 *
 * Les points d'arrivée de la carte éditée sont pris **dans le brouillon** et non dans son fichier :
 * un portail vers un point qu'on vient de poser, sans avoir encore enregistré, ne doit pas être
 * signalé comme cassé.
 *
 * @param references      Les catalogues.
 * @param editedMapId     L'identifiant de la carte éditée (le nom de son fichier, sans extension).
 * @param editedEntities  Les entités du brouillon.
 * @param place           Le lieu de la carte : ses figurines, et celles de ses niveaux communs,
 *                        s'ajoutent à celles de toute carte (`core::resolveFigures`, `LOT-124`).
 */
[[nodiscard]] core::EntityReferenceContext referenceContext(
    const EditorReferences& references, std::string_view editedMapId,
    const std::vector<core::MapEntity>& editedEntities, std::string_view place = {});

/// @return `carte#id` : l'entité @p entityId de la carte @p mapId (décision D8).
[[nodiscard]] std::string entityRef(std::string_view mapId, std::string_view entityId);

/**
 * @brief Les valeurs que le panneau propose pour la propriété @p spec de @p entity, triées.
 *
 * Vide pour une propriété qui n'est pas un choix. Pour un point d'arrivée, ce sont les points de la
 * carte que nomme `targetMap` **de la même entité** — vide tant qu'elle n'en nomme aucune.
 */
[[nodiscard]] std::vector<std::string> entityChoices(const core::EntityPropertySpec& spec,
                                                     const core::MapEntity& entity,
                                                     const core::EntityReferenceContext& context);

}  // namespace hmi
