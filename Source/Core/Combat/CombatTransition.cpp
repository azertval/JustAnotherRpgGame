// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/CombatTransition.h"

#include <utility>
#include <variant>

#include "Core/Gameplay/WorldFlags.h"

namespace core {

EncounterRun beginEncounter(const Encounter& encounter, const ExplorationSnapshot& exploration,
                            GridPosition trigger, std::string defeatFlagKey) {
    return {.exploration = exploration,
            .encounterId = encounter.id,
            .defeatFlagKey = std::move(defeatFlagKey),
            .placements = placeCombatants(encounter, trigger),
            .escapable = encounter.escapable};
}

ExplorationSnapshot endEncounter(const EncounterRun& run, CombatOutcome outcome,
                                 WorldFlags& flags) {
    // SEULE une victoire acquiert le drapeau. Poser le drapeau a la sortie, quelle qu'elle soit,
    // ferait de la fuite un moyen de nettoyer une carte -- et le defaut ne se verrait pas : la
    // carte se viderait, ce qui ressemble a une progression.
    if (outcome == CombatOutcome::Victory) {
        if (!run.defeatFlagKey.empty()) {
            static_cast<void>(flags.set(run.defeatFlagKey));
        }
        // Le fait de la victoire, que la quete lit : une rencontre engagee par un dialogue n'a
        // pas de cle d'entite, et sans lui la victoire ne laisserait aucune trace (LOT-120).
        if (!run.encounterId.empty()) {
            static_cast<void>(flags.set(encounterWonFlag(run.encounterId)));
        }
    }
    return run.exploration;
}

bool encounterAlreadyCleared(const WorldFlags& flags, std::string_view defeatFlagKey) {
    return !defeatFlagKey.empty() && flags.isSet(defeatFlagKey);
}

namespace {

/// @return La propriete @p nom si elle porte une chaine, sinon une chaine vide.
[[nodiscard]] std::string proprieteTexte(const PropertyMap& proprietes, const char* nom) {
    const auto trouve = proprietes.find(nom);
    if (trouve == proprietes.end()) {
        return {};
    }
    const auto* const texte = std::get_if<std::string>(&trouve->second);
    return texte != nullptr ? *texte : std::string{};
}

/// @return La propriete @p nom si elle porte un booleen, sinon `false`.
[[nodiscard]] bool proprieteBooleen(const PropertyMap& proprietes, const char* nom) {
    const auto trouve = proprietes.find(nom);
    if (trouve == proprietes.end()) {
        return false;
    }
    const auto* const valeur = std::get_if<bool>(&trouve->second);
    return valeur != nullptr && *valeur;
}

}  // namespace

std::optional<EncounterTrigger> encounterTriggerFor(const MapEntity& entity,
                                                    std::string_view mapName) {
    if (entity.type != ENCOUNTER_ENTITY_TYPE) {
        return std::nullopt;
    }
    const std::string rencontre = proprieteTexte(entity.properties, "encounterId");
    if (rencontre.empty()) {
        // Un declencheur qui ne nomme aucune rencontre n'en est pas un. Le refuser ici plutot que
        // d'engager une rencontre vide : celle-ci se terminerait aussitot par une victoire, et
        // l'ennemi disparaitrait sans combat.
        return std::nullopt;
    }

    EncounterTrigger declencheur;
    declencheur.encounterId = rencontre;
    declencheur.position = entity.position;
    // Une ZONE se redeclenche : pas de cle. Un ennemi POSE se combat une fois, et sa cle est
    // fabriquee -- jamais ecrite a la main, sinon deux ennemis finiraient par la partager.
    if (!proprieteBooleen(entity.properties, "respawns")) {
        declencheur.defeatFlagKey =
            keyForEntity(mapName, entity.type, entity.position.column, entity.position.row);
    }
    return declencheur;
}

}  // namespace core
