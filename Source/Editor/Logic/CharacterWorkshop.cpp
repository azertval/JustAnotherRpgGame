// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/CharacterWorkshop.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <map>
#include <set>
#include <span>
#include <sstream>
#include <system_error>
#include <tuple>
#include <utility>

#include <nlohmann/json.hpp>

#include "Core/Resources/MeshFile.h"
#include "Core/Resources/SkeletonFile.h"
#include "Core/Resources/SkeletonPose.h"
#include "Editor/Logic/Sha256.h"

namespace hmi {

namespace {

using Json = nlohmann::ordered_json;

constexpr std::string_view MANIFEST_FILE = "manifest.json";
constexpr std::string_view COMMON_LEVEL = "Common/Characters";
constexpr std::string_view LEVEL_FOLDER = "Characters";
constexpr std::string_view MANNEQUIN_PREFIX = "Mannequins/";

[[nodiscard]] std::optional<std::string> readBytes(const std::filesystem::path& file) {
    std::ifstream input(file, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::ostringstream text;
    text << input.rdbuf();
    return std::move(text).str();
}

// Le texte d'un fichier JSON du dépôt : deux espaces, LF, une fin de ligne — ce qu'écrivait
// l'installateur, et ce que `check_json_files.py` attend.
[[nodiscard]] std::string jsonText(const Json& json) {
    return json.dump(2) + "\n";
}

// Un chemin tel qu'il s'écrit dans une fiche ou un manifeste : des `/`, sans `./` ni `a/../`.
[[nodiscard]] std::string writtenPath(std::string_view path) {
    return std::filesystem::path(path).lexically_normal().generic_string();
}

[[nodiscard]] bool validSegment(std::string_view segment) {
    return !segment.empty() && segment != "." && segment != ".." &&
           std::ranges::all_of(segment, [](char c) {
               return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                      c == '-' || c == '_';
           });
}

// Un chemin relatif fait de segments simples : `bandit`, `Heroes/brawler`.
[[nodiscard]] bool validRelative(std::string_view path) {
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t end = std::min(path.find('/', start), path.size());
        if (!validSegment(path.substr(start, end - start))) {
            return false;
        }
        start = end + 1;
    }
    return true;
}

[[nodiscard]] std::string lastSegment(std::string_view path) {
    const std::size_t slash = path.rfind('/');
    return std::string{slash == std::string_view::npos ? path : path.substr(slash + 1)};
}

// La largeur et la hauteur d'un PNG, lues dans son en-tête ; rien si ce n'en est pas un.
[[nodiscard]] std::optional<std::pair<int, int>> pngSize(std::string_view bytes) {
    static constexpr std::string_view SIGNATURE = "\x89PNG\r\n\x1a\n";
    if (bytes.size() < 24 || bytes.substr(0, 8) != SIGNATURE || bytes.substr(12, 4) != "IHDR") {
        return std::nullopt;
    }
    const auto number = [bytes](std::size_t offset) {
        int value = 0;
        for (std::size_t index = 0; index < 4; ++index) {
            value = (value << 8) | static_cast<unsigned char>(bytes[offset + index]);
        }
        return value;
    };
    return std::pair{number(16), number(20)};
}

[[nodiscard]] std::string field(const Json& json, std::string_view key) {
    const auto found = json.find(key);
    return found != json.end() && found->is_string() ? found->get<std::string>() : std::string{};
}

// Un objet JSON dont les clés sont triées : l'ordre des tables d'un manifeste.
[[nodiscard]] Json sortedObject(const Json& object) {
    std::map<std::string, Json> ordered;
    if (object.is_object()) {
        for (const auto& [key, value] : object.items()) {
            ordered.emplace(key, value);
        }
    }
    Json sorted = Json::object();
    for (auto& [key, value] : ordered) {
        sorted[key] = std::move(value);
    }
    return sorted;
}

[[nodiscard]] std::set<std::string> names(const Json& manifest, std::string_view key) {
    std::set<std::string> found;
    if (const auto list = manifest.find(key); list != manifest.end() && list->is_array()) {
        for (const Json& item : *list) {
            if (item.is_string()) {
                found.insert(item.get<std::string>());
            }
        }
    }
    return found;
}

[[nodiscard]] Json sourceEntry(std::string_view written, std::string_view bytes) {
    Json entry = Json::object();
    entry["file"] = writtenPath(written);
    entry["sha256"] = sha256Hex(bytes);
    return entry;
}

// Ce qu'un modèle doit à sa silhouette : un squelette, ses os, ses clips. Vide s'il s'y tient.
[[nodiscard]] std::string modelFault(const core::MeshData& mesh,
                                     const core::SkeletonDescription& skeleton) {
    if (!mesh.skinned || mesh.rig.empty()) {
        return "the model is not bound to a skeleton (scripts/assetsGeneration/rig_character.py)";
    }
    std::set<std::string_view> joints;
    for (const core::MeshJoint& joint : mesh.rig.joints) {
        joints.insert(joint.name);
    }
    for (const core::SkeletonBone& bone : skeleton.bones) {
        if (!joints.contains(bone.name)) {
            return "the model has no bone \"" + bone.name + "\" of skeleton \"" +
                   skeleton.silhouette + "\"";
        }
    }
    for (const core::SkeletonClip& clip : skeleton.clips) {
        if (core::findClip(mesh.rig, clip.name) == nullptr) {
            return "the model has no clip \"" + clip.name + "\" of skeleton \"" +
                   skeleton.silhouette + "\"";
        }
    }
    return {};
}

[[nodiscard]] std::string sizeFault(std::string_view bytes, int side, std::string_view what) {
    const std::optional<std::pair<int, int>> size = pngSize(bytes);
    if (!size) {
        return std::string{what} + " is not a PNG file";
    }
    if (size->first != side || size->second != side) {
        return std::string{what} + " is " + std::to_string(size->first) + " x " +
               std::to_string(size->second) + " px, " + std::to_string(side) + " x " +
               std::to_string(side) + " expected";
    }
    return {};
}

[[nodiscard]] std::filesystem::path assetsOf(const std::filesystem::path& dataRoot) {
    return dataRoot / "Assets";
}

[[nodiscard]] std::filesystem::path skeletonFileOf(const std::filesystem::path& dataRoot,
                                                   std::string_view silhouette) {
    return assetsOf(dataRoot) / std::filesystem::path(core::skeletonFilePath(silhouette));
}

// Les binaires des kits ne sont pas dans Git : une base verrouillée sans kit installé n'en a pas.
[[nodiscard]] bool kitsMissing(const std::filesystem::path& dataRoot) {
    std::error_code error;
    const std::filesystem::path assets = assetsOf(dataRoot);
    return std::filesystem::exists(assets / "kits.lock.json", error) &&
           !std::filesystem::is_directory(assets / ".kits", error);
}

// Les manifestes que le plan modifie, chacun lu une fois : celui du niveau et celui du commun
// peuvent être le même fichier.
class Manifests {
public:
    [[nodiscard]] Json* open(const std::filesystem::path& file, std::string& error) {
        const std::string key = file.lexically_normal().generic_string();
        if (const auto known = _documents.find(key); known != _documents.end()) {
            return &known->second.json;
        }
        const std::optional<std::string> text = readBytes(file);
        if (!text) {
            error = "no manifest at " + file.generic_string() +
                    " (the level folder is created with its zone)";
            return nullptr;
        }
        Json json = Json::parse(*text, nullptr, /*allow_exceptions=*/false);
        if (!json.is_object()) {
            error = file.generic_string() + " is not a JSON object";
            return nullptr;
        }
        _order.push_back(key);
        Document& document = _documents[key];
        document.file = file;
        document.before = *text;
        document.json = std::move(json);
        return &document.json;
    }

    void appendWrites(CharacterInstallPlan& plan) const {
        for (const std::string& key : _order) {
            const Document& document = _documents.at(key);
            std::string text = jsonText(document.json);
            if (text != document.before) {
                plan.log += "  manifest " + document.file.generic_string() + "\n";
                plan.writes.push_back(CharacterFileWrite{document.file, std::move(text)});
            }
        }
    }

private:
    struct Document {
        std::filesystem::path file;
        std::string before;
        Json json;
    };
    std::map<std::string, Document> _documents;
    std::vector<std::string> _order;
};

void planFile(CharacterInstallPlan& plan, const std::filesystem::path& file, std::string bytes,
              std::string_view label) {
    if (readBytes(file) == std::optional<std::string>{bytes}) {
        return;
    }
    plan.log += "  " + std::string{label} + " " + file.generic_string() + "\n";
    plan.writes.push_back(CharacterFileWrite{file, std::move(bytes)});
}

[[nodiscard]] std::string sheetText(std::string_view model, std::string_view skeleton) {
    Json sheet = Json::object();
    sheet["version"] = core::SKELETON_FORMAT_VERSION;
    sheet["model"] = model;
    sheet["skeleton"] = skeleton;
    return jsonText(sheet);
}

[[nodiscard]] MapCheckFinding finding(MapCheckSeverity severity, std::string level,
                                      std::string message) {
    return MapCheckFinding{.severity = severity,
                           .mapId = std::move(level),
                           .cell = std::nullopt,
                           .message = std::move(message),
                           .entityId = {}};
}

}  // namespace

bool isCharacterScript(std::string_view json) {
    const Json document = Json::parse(json, nullptr, /*allow_exceptions=*/false);
    return document.is_object() && field(document, "format") == CHARACTER_SCRIPT_FORMAT;
}

CharacterDraftResult readCharacterDraft(std::string_view json) {
    CharacterDraftResult result;
    const Json document = Json::parse(json, nullptr, /*allow_exceptions=*/false);
    if (!document.is_object()) {
        result.error = "not a JSON object";
        return result;
    }
    if (field(document, "format") != CHARACTER_SCRIPT_FORMAT) {
        result.error = "not a character sheet (\"format\": \"" +
                       std::string{CHARACTER_SCRIPT_FORMAT} + "\")";
        return result;
    }
    if (const auto version = document.find("version");
        version == document.end() || !version->is_number_integer() ||
        version->get<int>() != CHARACTER_SCRIPT_VERSION) {
        result.error = "unsupported character sheet version";
        return result;
    }
    static constexpr std::array<std::string_view, 11> KNOWN = {
        "format",   "version", "root",     "level", "name",    "skeleton",
        "skeletonSource", "model",   "portrait", "token", "workshop"};
    for (const auto& [key, value] : document.items()) {
        if (std::ranges::find(KNOWN, std::string_view{key}) == KNOWN.end()) {
            result.error = "unknown field \"" + key + "\"";
            return result;
        }
        if (key != "version" && key != "workshop" && !value.is_string()) {
            result.error = "field \"" + key + "\" must be a string";
            return result;
        }
    }
    CharacterDraft& draft = result.draft;
    if (document.contains("root")) {
        draft.root = field(document, "root");
    }
    draft.level = field(document, "level");
    draft.name = field(document, "name");
    draft.skeleton = field(document, "skeleton");
    draft.skeletonSource = field(document, "skeletonSource");
    draft.model = field(document, "model");
    draft.portrait = field(document, "portrait");
    draft.token = field(document, "token");
    if (const auto workshop = document.find("workshop"); workshop != document.end()) {
        if (!workshop->is_object()) {
            result.error = "field \"workshop\" must be an object";
            return result;
        }
        static constexpr std::array<std::string_view, 4> FILES = {"received", "sheet", "retouch",
                                                                  "blend"};
        for (const auto& [key, value] : workshop->items()) {
            if (std::ranges::find(FILES, std::string_view{key}) == FILES.end() ||
                !value.is_string()) {
                result.error = "unknown or malformed field \"workshop." + key + "\"";
                return result;
            }
        }
        draft.workshop.received = field(*workshop, "received");
        draft.workshop.sheet = field(*workshop, "sheet");
        draft.workshop.retouch = field(*workshop, "retouch");
        draft.workshop.blend = field(*workshop, "blend");
    }
    return result;
}

std::string characterDraftText(const CharacterDraft& draft) {
    Json document = Json::object();
    document["format"] = CHARACTER_SCRIPT_FORMAT;
    document["version"] = CHARACTER_SCRIPT_VERSION;
    const auto put = [](Json& target, std::string_view key, const std::string& value) {
        if (!value.empty()) {
            target[std::string{key}] = value;
        }
    };
    if (draft.root != ".") {
        put(document, "root", draft.root);
    }
    put(document, "level", draft.level);
    put(document, "name", draft.name);
    put(document, "skeleton", draft.skeleton);
    put(document, "skeletonSource", draft.skeletonSource);
    put(document, "model", draft.model);
    put(document, "portrait", draft.portrait);
    put(document, "token", draft.token);
    Json workshop = Json::object();
    put(workshop, "received", draft.workshop.received);
    put(workshop, "sheet", draft.workshop.sheet);
    put(workshop, "retouch", draft.workshop.retouch);
    put(workshop, "blend", draft.workshop.blend);
    if (!workshop.empty()) {
        document["workshop"] = std::move(workshop);
    }
    return jsonText(document);
}

CharacterInstallPlan planCharacter(const std::filesystem::path& dataRoot,
                                   const std::filesystem::path& baseDirectory,
                                   const CharacterDraft& draft) {
    CharacterInstallPlan plan;
    const auto refuse = [&plan](std::string message) {
        plan.writes.clear();
        plan.removals.clear();
        plan.log.clear();
        plan.error = std::move(message);
        return plan;
    };
    if (!validRelative(draft.level) || lastSegment(draft.level) != LEVEL_FOLDER) {
        return refuse("\"level\" must name a Characters folder under Assets/ (\"" + draft.level +
                      "\")");
    }
    if (!validRelative(draft.name)) {
        return refuse("\"name\" must be made of letters, digits, '-' and '_' (\"" + draft.name +
                      "\")");
    }
    if (!validSegment(draft.skeleton)) {
        return refuse("\"skeleton\" must name a silhouette (\"" + draft.skeleton + "\")");
    }
    const std::filesystem::path assets = assetsOf(dataRoot);
    const std::filesystem::path levelFolder = assets / std::filesystem::path(draft.level);
    const std::filesystem::path folder = levelFolder / std::filesystem::path(draft.name);
    const std::filesystem::path root = (baseDirectory / draft.root).lexically_normal();
    plan.log = draft.level + "/" + draft.name + "\n";

    Manifests manifests;
    std::string error;
    Json* const manifest = manifests.open(levelFolder / MANIFEST_FILE, error);
    if (manifest == nullptr) {
        return refuse(std::move(error));
    }

    // Le squelette : celui que la fiche apporte, à défaut celui du monde.
    core::SkeletonDescription skeleton;
    if (!draft.skeletonSource.empty()) {
        std::optional<std::string> text = readBytes(root / draft.skeletonSource);
        if (!text) {
            return refuse("skeleton source not found: " + draft.skeletonSource);
        }
        // La source vient d'un script : ses fins de ligne sont ramenées à celles du dépôt, et
        // c'est ce texte que son empreinte signe — la même sur tout poste, quel que soit son
        // réglage des fins de ligne.
        std::string normalised;
        normalised.reserve(text->size());
        for (std::size_t index = 0; index < text->size(); ++index) {
            if ((*text)[index] == '\r' && index + 1 < text->size() && (*text)[index + 1] == '\n') {
                continue;
            }
            normalised.push_back((*text)[index]);
        }
        core::SkeletonFileResult read = core::readSkeletonDescription(normalised);
        if (!read.ok()) {
            return refuse(draft.skeletonSource + ": " + read.message);
        }
        if (read.skeleton.silhouette != draft.skeleton) {
            return refuse(draft.skeletonSource + " describes \"" + read.skeleton.silhouette +
                          "\", not \"" + draft.skeleton + "\"");
        }
        skeleton = std::move(read.skeleton);
        Json* const common =
            manifests.open(assets / std::filesystem::path(COMMON_LEVEL) / MANIFEST_FILE, error);
        if (common == nullptr) {
            return refuse(std::move(error));
        }
        const std::string file = "Skeletons/" + draft.skeleton + "/skeleton.json";
        Json entry = Json::object();
        entry["file"] = file;
        entry["sha256"] = sha256Hex(normalised);
        entry["source"] = sourceEntry(draft.skeletonSource, normalised);
        Json declared = common->value("skeletons", Json::object());
        declared[draft.skeleton] = std::move(entry);
        (*common)["skeletons"] = sortedObject(declared);
        planFile(plan, skeletonFileOf(dataRoot, draft.skeleton), std::move(normalised), "skeleton");
    } else {
        core::SkeletonFileResult read =
            core::readSkeletonFile(skeletonFileOf(dataRoot, draft.skeleton));
        if (!read.ok()) {
            return refuse("unknown skeleton \"" + draft.skeleton +
                          "\": neither installed nor given (\"skeletonSource\")");
        }
        skeleton = std::move(read.skeleton);
    }

    // Le modèle : celui que la fiche nomme, à défaut celui qui est installé.
    const core::CharacterSheetFileResult installed =
        core::readCharacterSheetFile(folder / core::CHARACTER_SHEET_FILE);
    std::string modelFile = lastSegment(draft.name) + ".glb";
    Json models = manifest->value("models", Json::object());
    if (!draft.model.empty()) {
        if (!draft.model.ends_with(".glb")) {
            return refuse("\"model\" must be a .glb file (\"" + draft.model + "\")");
        }
        std::optional<std::string> bytes = readBytes(root / draft.model);
        if (!bytes) {
            return refuse("model not found: " + draft.model);
        }
        const core::MeshFileResult read = core::readMeshFromGlb(std::as_bytes(
            std::span<const char>(bytes->data(), bytes->size())));
        if (!read.ok()) {
            return refuse(draft.model + ": " + read.message);
        }
        if (std::string fault = modelFault(read.mesh, skeleton); !fault.empty()) {
            return refuse(draft.model + ": " + fault);
        }
        Json entry = Json::object();
        entry["model"] = draft.name + "/" + modelFile;
        entry["skeleton"] = draft.skeleton;
        entry["triangles"] = read.mesh.triangleCount();
        Json size = Json::array();
        for (std::size_t axis = 0; axis < 3; ++axis) {
            const double extent = static_cast<double>(read.mesh.maximum[axis]) -
                                  static_cast<double>(read.mesh.minimum[axis]);
            size.push_back(std::round(extent * 1000.0) / 1000.0);
        }
        entry["size"] = std::move(size);
        entry["sha256"] = sha256Hex(*bytes);
        entry["source"] = sourceEntry(draft.model, *bytes);
        models[draft.name] = std::move(entry);
        planFile(plan, folder / modelFile, std::move(*bytes), "model");
    } else {
        if (!installed.ok()) {
            return refuse("no model named, and none installed for " + draft.name);
        }
        modelFile = installed.sheet.model;
        if (const auto entry = models.find(draft.name); entry != models.end()) {
            (*entry)["skeleton"] = draft.skeleton;
        }
    }
    planFile(plan, folder / core::CHARACTER_SHEET_FILE, sheetText(modelFile, draft.skeleton),
             "sheet");

    // Le portrait et le jeton : rangés tels quels, s'ils sont au standard.
    Json sources = manifest->value("sources", Json::object());
    const auto image = [&](const std::string& written, std::string_view file, int side,
                           std::string_view what) -> std::string {
        if (written.empty()) {
            return {};
        }
        std::optional<std::string> bytes = readBytes(root / written);
        if (!bytes) {
            return std::string{what} + " not found: " + written;
        }
        if (std::string fault = sizeFault(*bytes, side, what); !fault.empty()) {
            return written + ": " + fault + " (the workshop never resizes art)";
        }
        sources[draft.name + "/" + std::string{file}] = sourceEntry(written, *bytes);
        planFile(plan, folder / file, std::move(*bytes), what);
        return {};
    };
    if (std::string fault =
            image(draft.portrait, CHARACTER_PORTRAIT_FILE, CHARACTER_PORTRAIT_SIDE, "portrait");
        !fault.empty()) {
        return refuse(std::move(fault));
    }
    if (std::string fault =
            image(draft.token, CHARACTER_TOKEN_FILE, CHARACTER_TOKEN_SIDE, "token");
        !fault.empty()) {
        return refuse(std::move(fault));
    }

    // Ce que le dossier porte en trop : un ancien modèle, une image d'avant.
    const std::set<std::string> kept = {modelFile, std::string{core::CHARACTER_SHEET_FILE},
                                        std::string{CHARACTER_PORTRAIT_FILE},
                                        std::string{CHARACTER_TOKEN_FILE}};
    std::error_code ignored;
    if (std::filesystem::is_directory(folder, ignored)) {
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::directory_iterator(folder, ignored)) {
            const std::string file = entry.path().filename().generic_string();
            if (entry.is_regular_file(ignored) && !kept.contains(file)) {
                plan.removals.push_back(entry.path());
                sources.erase(draft.name + "/" + file);
            }
        }
        std::ranges::sort(plan.removals);
        for (const std::filesystem::path& removed : plan.removals) {
            plan.log += "  removed " + removed.generic_string() + "\n";
        }
    }

    // Le manifeste du niveau : le personnage y est en modèle, plus en portrait d'attente.
    std::set<std::string> npcs = names(*manifest, "npcs");
    npcs.insert(draft.name);
    (*manifest)["npcs"] = npcs;
    std::set<std::string> portraits = names(*manifest, "portraits");
    portraits.erase(draft.name);
    if (portraits.empty()) {
        manifest->erase("portraits");
    } else {
        (*manifest)["portraits"] = portraits;
    }
    if (!models.empty()) {
        (*manifest)["models"] = sortedObject(models);
    }
    (*manifest)["sources"] = sortedObject(sources);
    // Le commentaire d'un dossier créé vide (« Vide : … ») ne dit plus vrai.
    if (field(*manifest, "comment").starts_with("Vide")) {
        manifest->erase("comment");
    }
    manifests.appendWrites(plan);
    if (plan.writes.empty() && plan.removals.empty()) {
        plan.log += "  nothing to write: already installed\n";
    }
    return plan;
}

std::string writeCharacter(const CharacterInstallPlan& plan) {
    if (!plan.ok()) {
        return plan.error;
    }
    std::error_code error;
    for (const CharacterFileWrite& write : plan.writes) {
        std::filesystem::create_directories(write.file.parent_path(), error);
        if (error) {
            return "cannot create " + write.file.parent_path().generic_string() + ": " +
                   error.message();
        }
        std::ofstream output(write.file, std::ios::binary | std::ios::trunc);
        output << write.bytes;
        if (!output) {
            return "cannot write " + write.file.generic_string();
        }
    }
    for (const std::filesystem::path& removed : plan.removals) {
        std::filesystem::remove(removed, error);
        if (error) {
            return "cannot remove " + removed.generic_string() + ": " + error.message();
        }
    }
    return {};
}

std::vector<std::string> characterLevels(const std::filesystem::path& dataRoot) {
    std::vector<std::string> levels;
    const std::filesystem::path assets = assetsOf(dataRoot);
    std::error_code error;
    for (const char* const top : {"Common", "Regions"}) {
        const std::filesystem::path start = assets / top;
        if (!std::filesystem::is_directory(start, error)) {
            continue;
        }
        for (std::filesystem::recursive_directory_iterator walk(start, error), end;
             !error && walk != end; walk.increment(error)) {
            if (!walk->is_directory(error) || walk->path().filename() != LEVEL_FOLDER) {
                continue;
            }
            // Un dossier `Characters` ne contient pas d'autre niveau.
            walk.disable_recursion_pending();
            if (std::filesystem::exists(walk->path() / MANIFEST_FILE, error)) {
                levels.push_back(walk->path().lexically_relative(assets).generic_string());
            }
        }
    }
    std::ranges::sort(levels);
    return levels;
}

std::vector<InstalledCharacter> installedCharacters(const std::filesystem::path& dataRoot) {
    std::vector<InstalledCharacter> characters;
    for (const std::string& level : characterLevels(dataRoot)) {
        const std::optional<std::string> text =
            readBytes(assetsOf(dataRoot) / std::filesystem::path(level) / MANIFEST_FILE);
        if (!text) {
            continue;
        }
        const Json manifest = Json::parse(*text, nullptr, /*allow_exceptions=*/false);
        if (!manifest.is_object()) {
            continue;
        }
        for (const std::string& name : names(manifest, "npcs")) {
            characters.push_back(InstalledCharacter{level, name});
        }
    }
    return characters;
}

CharacterDraftResult draftOfInstalledCharacter(const std::filesystem::path& dataRoot,
                                               const std::filesystem::path& workshopRoot,
                                               const InstalledCharacter& character) {
    CharacterDraftResult result;
    const std::filesystem::path levelFolder =
        assetsOf(dataRoot) / std::filesystem::path(character.level);
    const core::CharacterSheetFileResult sheet = core::readCharacterSheetFile(
        levelFolder / std::filesystem::path(character.name) / core::CHARACTER_SHEET_FILE);
    if (!sheet.ok()) {
        result.error = character.name + ": " + sheet.message;
        return result;
    }
    CharacterDraft& draft = result.draft;
    draft.level = character.level;
    draft.name = character.name;
    draft.skeleton = sheet.sheet.skeleton;
    const std::optional<std::string> text = readBytes(levelFolder / MANIFEST_FILE);
    const Json manifest =
        text ? Json::parse(*text, nullptr, /*allow_exceptions=*/false) : Json::object();
    if (!manifest.is_object()) {
        return result;
    }
    // Une source que l'atelier local a encore : la fiche la nomme, et la réinstaller ne change
    // rien. Sinon le champ reste vide : ce qui est installé reste.
    const auto present = [&workshopRoot](const Json& entry) -> std::string {
        if (!entry.is_object()) {
            return {};
        }
        const std::string file = field(entry, "file");
        std::error_code error;
        return !file.empty() && std::filesystem::is_regular_file(workshopRoot / file, error)
                   ? file
                   : std::string{};
    };
    if (const auto models = manifest.find("models"); models != manifest.end()) {
        if (const auto entry = models->find(character.name); entry != models->end()) {
            draft.model = present(entry->value("source", Json::object()));
        }
    }
    if (const auto sources = manifest.find("sources"); sources != manifest.end()) {
        const auto source = [&](std::string_view file) {
            const auto entry = sources->find(character.name + "/" + std::string{file});
            return entry != sources->end() ? present(*entry) : std::string{};
        };
        draft.portrait = source(CHARACTER_PORTRAIT_FILE);
        draft.token = source(CHARACTER_TOKEN_FILE);
    }
    return result;
}

std::vector<MapCheckFinding> checkCharacters(const std::filesystem::path& dataRoot) {
    std::vector<MapCheckFinding> findings;
    const bool binaries = !kitsMissing(dataRoot);
    const std::vector<InstalledCharacter> characters = installedCharacters(dataRoot);
    if (!binaries && !characters.empty()) {
        findings.push_back(finding(MapCheckSeverity::Warning, "Assets",
                                   "asset kits are locked but not installed: models, portraits "
                                   "and tokens were not checked (scripts/fetch_assets.py)"));
    }
    std::map<std::string, core::SkeletonFileResult> skeletons;
    for (const InstalledCharacter& character : characters) {
        const auto error = [&findings, &character](std::string message) {
            findings.push_back(finding(MapCheckSeverity::Error, character.level,
                                       character.name + ": " + std::move(message)));
        };
        const std::filesystem::path folder = assetsOf(dataRoot) /
                                             std::filesystem::path(character.level) /
                                             std::filesystem::path(character.name);
        const core::CharacterSheetFileResult sheet =
            core::readCharacterSheetFile(folder / core::CHARACTER_SHEET_FILE);
        if (!sheet.ok()) {
            error("character sheet unreadable (" + sheet.message + ")");
            continue;
        }
        auto known = skeletons.find(sheet.sheet.skeleton);
        if (known == skeletons.end()) {
            known = skeletons
                        .emplace(sheet.sheet.skeleton,
                                 core::readSkeletonFile(
                                     skeletonFileOf(dataRoot, sheet.sheet.skeleton)))
                        .first;
        }
        if (!known->second.ok()) {
            error("unknown skeleton \"" + sheet.sheet.skeleton + "\"");
            continue;
        }
        if (!binaries) {
            continue;
        }
        const std::filesystem::path model = folder / sheet.sheet.model;
        std::error_code ignored;
        if (!std::filesystem::is_regular_file(model, ignored)) {
            error("model missing (" + sheet.sheet.model + ")");
        } else {
            const core::MeshFileResult read = core::readMeshFile(model);
            if (!read.ok()) {
                error("model unreadable (" + read.message + ")");
            } else if (std::string fault = modelFault(read.mesh, known->second.skeleton);
                       !fault.empty()) {
                error(std::move(fault));
            }
        }
        // Un mannequin tient la place d'un personnage sans modèle : il n'a ni portrait ni jeton,
        // l'interface montre ceux du personnage qu'il remplace (`hmi::ResolvedFigure::named`).
        if (character.name.starts_with(MANNEQUIN_PREFIX)) {
            continue;
        }
        for (const auto& [file, side, what] :
             {std::tuple{CHARACTER_PORTRAIT_FILE, CHARACTER_PORTRAIT_SIDE, "portrait"},
              std::tuple{CHARACTER_TOKEN_FILE, CHARACTER_TOKEN_SIDE, "token"}}) {
            const std::optional<std::string> bytes = readBytes(folder / file);
            if (!bytes) {
                error(std::string{what} + " missing");
            } else if (std::string fault = sizeFault(*bytes, side, what); !fault.empty()) {
                error(std::move(fault));
            }
        }
    }
    return findings;
}

std::optional<int> runCharacterCommand(const std::vector<std::string>& arguments,
                                       const std::filesystem::path& dataRoot,
                                       std::string& output) {
    // `--apply <fiche> [fiche…]` : les fichiers qui suivent, jusqu'à l'option suivante.
    std::vector<std::filesystem::path> files;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (arguments[index] != "--apply") {
            continue;
        }
        while (index + 1 < arguments.size() && !arguments[index + 1].starts_with("--")) {
            files.emplace_back(arguments[++index]);
        }
    }
    if (files.empty()) {
        return std::nullopt;
    }
    // Un scénario de gestes est pour `runMapCommand` : seul le format décide.
    std::vector<std::string> texts;
    std::size_t sheets = 0;
    for (const std::filesystem::path& file : files) {
        std::optional<std::string> text = readBytes(file);
        sheets += text && isCharacterScript(*text) ? 1U : 0U;
        texts.push_back(std::move(text).value_or(std::string{}));
    }
    if (sheets == 0) {
        return std::nullopt;
    }
    if (sheets != files.size()) {
        output += "usage: --apply takes character sheets or one gesture script, not both\n";
        return 2;
    }
    // Fiche par fiche, dans l'ordre : chacune est vérifiée entière avant sa première écriture, et
    // une fiche refusée arrête la commande. Celles d'avant sont installées, le compte rendu le dit.
    std::vector<CharacterInstallPlan> plans;
    for (std::size_t index = 0; index < files.size(); ++index) {
        const CharacterDraftResult draft = readCharacterDraft(texts[index]);
        if (!draft.ok()) {
            output += "error: " + files[index].generic_string() + ": " + draft.error +
                      "\nnothing written for this sheet\n";
            return 1;
        }
        CharacterInstallPlan plan = planCharacter(
            dataRoot, std::filesystem::absolute(files[index]).parent_path(), draft.draft);
        if (!plan.ok()) {
            output += "error: " + files[index].generic_string() + ": " + plan.error +
                      "\nnothing written for this sheet\n";
            return 1;
        }
        // Le plan suivant doit voir ce que celui-ci écrit : deux personnages d'un même niveau
        // partagent leur manifeste.
        if (const std::string error = writeCharacter(plan); !error.empty()) {
            output += "error: " + error + "\n";
            return 1;
        }
        output += plan.log;
        plans.push_back(std::move(plan));
    }
    output += "installed " + std::to_string(plans.size()) + " character" +
              (plans.size() == 1 ? "" : "s") + "\n";
    return 0;
}

}  // namespace hmi
