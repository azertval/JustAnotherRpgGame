// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/AnimationCatalog.h"

#include <fstream>
#include <set>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "Core/Data/JsonDocument.h"

namespace hmi {

namespace {

// Noms des champs du format : nommes plutot que repetes en
// litteraux.
constexpr const char* FIELD_VERSION = "version";
constexpr const char* FIELD_FRAME_WIDTH = "frameWidth";
constexpr const char* FIELD_FRAME_HEIGHT = "frameHeight";
constexpr const char* FIELD_CLIPS = "clips";
constexpr const char* FIELD_FRAMES = "frames";
constexpr const char* FIELD_FRAME_DURATION = "frameDuration";
constexpr const char* FIELD_LOOP = "loop";
constexpr const char* FIELD_NEXT = "next";

// Construit un resultat d'echec. Ne journalise rien : le fichier absent (cas par defaut,
// silencieux) et le fichier invalide (anomalie) empruntent tous deux ce chemin -- c'est a
// l'appelant de decider, a partir du code d'erreur, s'il doit journaliser (EX-NFR-040).
// Traduction de la categorie partagee (LOT-79) vers celle, documentee, de ce catalogue. Le
// `switch` est exhaustif et sans `default` : ajouter une categorie d'un cote fait echouer la
// compilation plutot que de tomber silencieusement dans un cas par defaut.
[[nodiscard]] AnimationCatalogError mapError(core::JsonReadError code) {
    switch (code) {
        case core::JsonReadError::None:
            return AnimationCatalogError::None;
        case core::JsonReadError::FileNotFound:
            return AnimationCatalogError::FileNotFound;
        case core::JsonReadError::ParseError:
            return AnimationCatalogError::ParseError;
        case core::JsonReadError::UnsupportedVersion:
            return AnimationCatalogError::UnsupportedVersion;
        case core::JsonReadError::MalformedStructure:
            return AnimationCatalogError::MalformedStructure;
    }
    return AnimationCatalogError::ParseError;
}

[[nodiscard]] AnimationDescriptionResult failure(std::string message, AnimationCatalogError code) {
    return AnimationDescriptionResult{
        .description = std::nullopt, .error = std::move(message), .errorCode = code};
}

// Issue de l'analyse d'UN clip (voir parseClip ci-dessous) : soit le clip, soit -- exclusif -- de
// quoi construire l'echec a renvoyer par loadFromString (message + code).
struct ClipParseResult {
    std::optional<core::AnimationClip> clip;
    std::string error;
    AnimationCatalogError errorCode = AnimationCatalogError::None;
};

// Analyse l'entree JSON `clips.<name>` en un core::AnimationClip. Ne journalise rien (comme
// failure() ci-dessus) : l'erreur, precise, remonte jusqu'a l'appelant de loadFromString.
[[nodiscard]] ClipParseResult parseClip(const std::string& name, const nlohmann::json& clipJson) {
    if (!clipJson.is_object()) {
        return {.clip = std::nullopt,
                .error = "Le clip « " + name + " » n'est pas un objet.",
                .errorCode = AnimationCatalogError::MalformedStructure};
    }
    if (!clipJson.contains(FIELD_FRAMES) || !clipJson[FIELD_FRAMES].is_array() ||
        clipJson[FIELD_FRAMES].empty()) {
        return {.clip = std::nullopt,
                .error = "Le clip « " + name + " » n'a pas de champ « frames » exploitable.",
                .errorCode = AnimationCatalogError::MalformedStructure};
    }

    core::AnimationClip clip;
    clip.name = name;
    for (const nlohmann::json& frameJson : clipJson[FIELD_FRAMES]) {
        if (!frameJson.is_number_integer() || frameJson.get<int>() < 0) {
            return {.clip = std::nullopt,
                    .error = "Le clip « " + name +
                             " » contient un indice d'image invalide (entier positif attendu).",
                    .errorCode = AnimationCatalogError::MalformedStructure};
        }
        clip.frames.push_back(frameJson.get<int>());
    }

    clip.frameDuration = AnimationCatalog::DEFAULT_FRAME_DURATION_SECONDS;
    if (clipJson.contains(FIELD_FRAME_DURATION)) {
        if (!clipJson[FIELD_FRAME_DURATION].is_number() ||
            clipJson[FIELD_FRAME_DURATION].get<float>() < 0.0F) {
            return {.clip = std::nullopt,
                    .error = "Le champ « frameDuration » du clip « " + name +
                             " » doit etre un nombre positif ou nul.",
                    .errorCode = AnimationCatalogError::MalformedStructure};
        }
        clip.frameDuration = clipJson[FIELD_FRAME_DURATION].get<float>();
    }

    // Absent : boucle (comportement historique, EX-REN-012). Present mais pas un booleen : donnee
    // invalide, jamais devinee.
    bool loop = true;
    if (clipJson.contains(FIELD_LOOP)) {
        if (!clipJson[FIELD_LOOP].is_boolean()) {
            return {.clip = std::nullopt,
                    .error = "Le champ « loop » du clip « " + name + " » n'est pas un booleen.",
                    .errorCode = AnimationCatalogError::MalformedStructure};
        }
        loop = clipJson[FIELD_LOOP].get<bool>();
    }
    clip.endMode = loop ? core::ClipEndMode::Loop : core::ClipEndMode::OneShot;

    if (clipJson.contains(FIELD_NEXT)) {
        if (!clipJson[FIELD_NEXT].is_string()) {
            return {.clip = std::nullopt,
                    .error = "Le champ « next » du clip « " + name + " » n'est pas une chaine.",
                    .errorCode = AnimationCatalogError::MalformedStructure};
        }
        clip.nextClip = clipJson[FIELD_NEXT].get<std::string>();
    }

    return {.clip = std::move(clip), .error = {}, .errorCode = AnimationCatalogError::None};
}

}  // namespace

AnimationDescriptionResult AnimationCatalog::loadFromString(std::string_view json) {
    return fromDocument(core::readJsonObject(json, FORMAT_VERSION, "*.anim.json", FIELD_VERSION));
}

AnimationDescriptionResult AnimationCatalog::fromDocument(const core::JsonDocument& document) {
    if (!document.ok()) {
        return failure(document.message, mapError(document.error));
    }
    const nlohmann::json& root = document.root;

    if (!root.contains(FIELD_FRAME_WIDTH) || !root[FIELD_FRAME_WIDTH].is_number_integer() ||
        !root.contains(FIELD_FRAME_HEIGHT) || !root[FIELD_FRAME_HEIGHT].is_number_integer()) {
        return failure("Champs « frameWidth »/« frameHeight » absents ou non entiers.",
                       AnimationCatalogError::MalformedStructure);
    }
    const int frameWidth = root[FIELD_FRAME_WIDTH].get<int>();
    const int frameHeight = root[FIELD_FRAME_HEIGHT].get<int>();
    if (frameWidth <= 0 || frameHeight <= 0) {
        return failure("« frameWidth »/« frameHeight » doivent etre strictement positifs.",
                       AnimationCatalogError::MalformedStructure);
    }

    if (!root.contains(FIELD_CLIPS) || !root[FIELD_CLIPS].is_object()) {
        return failure("Le champ « clips » est absent ou n'est pas un objet.",
                       AnimationCatalogError::MalformedStructure);
    }

    core::ClipSet clips;
    std::set<std::string> names;
    // Premiere passe : chaque clip, valide independamment des autres (dont son nom).
    for (const auto& [name, clipJson] : root[FIELD_CLIPS].items()) {
        ClipParseResult parsed = parseClip(name, clipJson);
        if (!parsed.clip) {
            return failure(std::move(parsed.error), parsed.errorCode);
        }
        names.insert(name);
        clips.addClip(std::move(*parsed.clip));
    }

    // Seconde passe : « next » doit designer un clip du MEME fichier -- verifie une fois tous les
    // noms connus, pour ne pas dependre de l'ordre d'iteration de l'objet JSON.
    for (const std::string& name : names) {
        const core::AnimationClip& clip = clips.clipAt(clips.indexOf(name));
        if (clip.endMode == core::ClipEndMode::OneShot && !clip.nextClip.empty() &&
            !names.contains(clip.nextClip)) {
            return failure("Le clip « " + name + " » designe un clip suivant inexistant (« " +
                               clip.nextClip + " »).",
                           AnimationCatalogError::MalformedStructure);
        }
    }

    AnimationDescription description;
    description.frameWidth = frameWidth;
    description.frameHeight = frameHeight;
    description.clips = std::move(clips);
    return AnimationDescriptionResult{.description = std::move(description),
                                      .error = {},
                                      .errorCode = AnimationCatalogError::None};
}

AnimationDescriptionResult AnimationCatalog::loadFromFile(const std::filesystem::path& path) {
    return fromDocument(core::readJsonObjectFromFile(path, FORMAT_VERSION, FIELD_VERSION));
}

std::string AnimationCatalog::descriptorFileName(std::string_view assetFileName) {
    return std::filesystem::path(assetFileName).replace_extension(".anim.json").string();
}

AssetValidation AnimationCatalog::validateAgainstTexture(const AnimationDescription& description,
                                                         const std::string& fileName,
                                                         int textureWidth, int textureHeight) {
    // Disposition horizontale, un seul rang (frameRegion) : la hauteur du PNG doit egaler
    // frameHeight, et sa largeur etre un multiple positif de frameWidth.
    if (textureHeight != description.frameHeight || description.frameWidth <= 0 ||
        textureWidth % description.frameWidth != 0 || textureWidth < description.frameWidth) {
        return AssetValidation{
            .valid = false,
            .message = "Animation " + fileName + " refusee : image " +
                       std::to_string(textureWidth) + "x" + std::to_string(textureHeight) +
                       " px incoherente avec une image de " +
                       std::to_string(description.frameWidth) + "x" +
                       std::to_string(description.frameHeight) +
                       " px sur un seul rang (largeur multiple de " +
                       std::to_string(description.frameWidth) + " px attendue)."};
    }

    const int frameCount = textureWidth / description.frameWidth;
    for (int index = 0; index < description.clips.clipCount(); ++index) {
        const core::AnimationClip& clip = description.clips.clipAt(index);
        for (const int frame : clip.frames) {
            if (frame < 0 || frame >= frameCount) {
                return AssetValidation{.valid = false,
                                       .message = "Animation " + fileName +
                                                  " refusee : le clip « " + clip.name +
                                                  " » reference l'image " + std::to_string(frame) +
                                                  ", hors bornes (la spritesheet en contient " +
                                                  std::to_string(frameCount) + ")."};
            }
        }
    }

    return AssetValidation{.valid = true, .message = std::string{}};
}

core::AtlasRegion AnimationCatalog::frameRegion(const AnimationDescription& description,
                                                int frameSheetIndex) {
    return core::AtlasRegion{.x = frameSheetIndex * description.frameWidth,
                             .y = 0,
                             .width = description.frameWidth,
                             .height = description.frameHeight};
}

}  // namespace hmi
