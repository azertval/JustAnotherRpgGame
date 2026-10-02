// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/AssetGallery.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include "Core/Data/JsonDocument.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Core/Resources/ScenePlace.h"
#include "Core/Resources/SkeletonFile.h"
#include "HMI/Graphics/AnimationCatalog.h"
#include "HMI/Graphics/SceneTextureTraits.h"

namespace hmi {

namespace {

// Version la plus élevée des manifestes lus ; absente, elle vaut 1 (`core::readJsonObject`).
constexpr int MANIFEST_VERSION = 1;

using json = nlohmann::json;

[[nodiscard]] std::string stemOf(const std::string& fileName) {
    const std::size_t dot = fileName.find('.');
    return dot == std::string::npos ? fileName : fileName.substr(0, dot);
}

[[nodiscard]] std::vector<std::string> stringList(const json& root, const char* key) {
    std::vector<std::string> values;
    const auto found = root.find(key);
    if (found == root.end() || !found->is_array()) {
        return values;
    }
    for (const json& value : *found) {
        if (value.is_string()) {
            values.push_back(value.get<std::string>());
        }
    }
    return values;
}

// Largeur et hauteur d'un PNG, lues dans son en-tête IHDR ; (0, 0) si ce n'est pas un PNG lisible.
[[nodiscard]] std::pair<int, int> pngSize(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::array<unsigned char, 24> header{};
    // Les octets bruts de l'en-tête : istream::read ne lit que des char.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (!file.read(reinterpret_cast<char*>(header.data()), header.size()) || header[1] != 'P' ||
        header[2] != 'N' || header[3] != 'G') {
        return {0, 0};
    }
    const auto bigEndian = [&](std::size_t offset) {
        return static_cast<int>((static_cast<unsigned>(header[offset]) << 24U) |
                                (static_cast<unsigned>(header[offset + 1]) << 16U) |
                                (static_cast<unsigned>(header[offset + 2]) << 8U) |
                                static_cast<unsigned>(header[offset + 3]));
    };
    return {bigEndian(16), bigEndian(20)};
}

// Une forme animée, d'après son `.anim.json` : le clip du nom du fichier s'il existe, le
//        premier sinon.
// Rend : Faux si le fichier est absent ; une erreur est ajoutée s'il est illisible.
bool readAnimatedEntry(AssetGalleryEntry& entry, const std::filesystem::path& descriptor,
                       std::vector<std::string>& errors) {
    std::error_code ignored;
    if (!std::filesystem::is_regular_file(descriptor, ignored)) {
        return false;
    }
    const AnimationDescriptionResult result = AnimationCatalog::loadFromFile(descriptor);
    if (!result.ok() || result.description->clips.clipCount() == 0) {
        errors.push_back(descriptor.generic_string() + " : " +
                         (result.ok() ? std::string("aucun clip") : result.error));
        return false;
    }
    const AnimationDescription& description = *result.description;
    const int named = description.clips.indexOf(stemOf(descriptor.filename().string()));
    const core::AnimationClip& clip = description.clips.clipAt(named >= 0 ? named : 0);
    entry.frameWidth = description.frameWidth;
    entry.frameHeight = description.frameHeight;
    entry.frames = clip.frames;
    entry.frameDuration = clip.frameDuration;
    entry.loop = clip.endMode == core::ClipEndMode::Loop;
    return true;
}

// Un manifeste lu ; absent : rien, sans erreur. Illisible : une erreur nommée.
[[nodiscard]] core::JsonDocument readManifest(const std::filesystem::path& path,
                                              std::vector<std::string>& errors) {
    core::JsonDocument document = core::readJsonObjectFromFile(path, MANIFEST_VERSION);
    if (!document.ok() && document.error != core::JsonReadError::FileNotFound) {
        errors.push_back(path.generic_string() + " : " + document.message);
    }
    return document;
}

// Les modèles d'un atelier de figurines, triés et sans doublon. Tous les dossiers, pas seulement
// ceux que le manifeste retient pour le jeu : la galerie sert justement à voir les autres.
[[nodiscard]] std::vector<std::string> figureModels(const std::filesystem::path& folder,
                                                    const json& manifest) {
    std::vector<std::string> models;
    std::error_code error;
    for (const auto& item : std::filesystem::directory_iterator(folder, error)) {
        if (item.is_directory()) {
            models.push_back(item.path().filename().string());
        }
    }
    // Les PNJ rangés plus bas, et les portraits d'attente (`LOT-145`) : un héros qui a son visage
    // avant son modèle.
    for (const char* list : {"npcs", "portraits"}) {
        for (const std::string& npc : stringList(manifest, list)) {
            if (npc.find('/') != std::string::npos) {
                models.push_back(npc);
            }
        }
    }
    // Les modèles que le manifeste inscrit (`models`, `LOT-1003`) : un mannequin sans squelette y
    // est, et n'est nulle part ailleurs.
    if (const auto declared = manifest.find("models");
        declared != manifest.end() && declared->is_object()) {
        for (const auto& [name, unused] : declared->items()) {
            models.push_back(name);
        }
    }
    std::ranges::sort(models);
    const auto duplicates = std::ranges::unique(models);
    models.erase(duplicates.begin(), duplicates.end());
    return models;
}

// La place d'un modèle de personnage dans la galerie, en cases : un corps de 1,80 m, et de quoi
// tenir un bras tendu ou un corps couché.
constexpr double MODEL_BLOC_COLUMNS = 1.5;
constexpr double MODEL_BLOC_ROWS = 1.0;
// Le losange d'art d'un atelier qui n'en déclare pas : celui du standard.
constexpr int DEFAULT_ART_TILE = 256;

// Les formes du modèle d'un personnage (`LOT-1006`) : une par clip que son squelette déclare ; une
// seule, dans sa pose de liaison, si le squelette ne se lit pas ou si le modèle n'a pas de fiche.
void addModelEntries(const std::filesystem::path& root, const std::string& folder,
                     const AssetGalleryEntry& base, AssetGalleryFamily& family,
                     std::vector<std::string>& errors) {
    const std::filesystem::path directory = root / folder;
    std::error_code ignored;
    std::string file;
    std::optional<core::SkeletonDescription> skeleton;
    if (std::filesystem::is_regular_file(directory / core::CHARACTER_SHEET_FILE, ignored)) {
        const core::CharacterSheetFileResult sheet =
            core::readCharacterSheetFile(directory / core::CHARACTER_SHEET_FILE);
        if (!sheet.ok()) {
            errors.push_back(sheet.message);
            return;
        }
        file = sheet.sheet.model;
        core::SkeletonFileResult read =
            core::readSkeletonFile(root / core::skeletonFilePath(sheet.sheet.skeleton));
        if (read.ok()) {
            skeleton = std::move(read.skeleton);
        } else {
            errors.push_back(read.message);
        }
    } else {
        // Sans fiche : un modèle inscrit au manifeste, du nom de son dossier (un mannequin sans
        // squelette).
        file = directory.filename().string() + ".glb";
    }
    if (!std::filesystem::is_regular_file(directory / file, ignored)) {
        return;  // le kit n'est pas installé, ou le modèle n'est pas encore produit
    }
    AssetGalleryEntry entry = base;
    entry.path = folder + "/" + file;
    entry.mesh = true;
    const int tile = entry.tileWidthPixels();
    entry.frameWidth = static_cast<int>(MODEL_BLOC_COLUMNS * tile);
    entry.frameHeight = static_cast<int>(MODEL_BLOC_ROWS * tile);
    if (!skeleton || skeleton->clips.empty()) {
        entry.form = "model";
        family.entries.push_back(std::move(entry));
        return;
    }
    for (const core::SkeletonClip& clip : skeleton->clips) {
        entry.form = clip.name;
        entry.clip = clip.name;
        entry.clipDuration = clip.duration;
        entry.loop = clip.loop;
        family.entries.push_back(entry);
    }
}

// Les personnages d'un atelier : un dossier par personnage, son modèle, son portrait et son jeton.
//
// Sert aux PNJ (`Npc/`, LOT-91), aux monstres (`Monsters/`, LOT-93) et aux dossiers `Characters/`
// de l'arborescence par niveaux. Un personnage rangé plus bas que l'atelier
// (`Characters/Heroes/brawler`) est trouvé par les listes du manifeste (`figureModels`). Depuis le
// `LOT-1006` un personnage n'a plus de bande : son modèle se montre clip par clip.
void readFigures(const std::filesystem::path& root, const std::string& directory,
                 const std::string& title, AssetGalleryCatalog& catalog) {
    const core::JsonDocument document =
        readManifest(root / directory / "manifest.json", catalog.errors);
    if (!document.ok()) {
        return;
    }
    AssetGalleryFamily family{.title = title, .directory = directory, .entries = {}};
    const std::vector<std::string> models = figureModels(root / directory, document.root);
    const auto declared = static_cast<int>(manifestArtTile(document.root).x);
    const int tile = declared > 0 ? declared : DEFAULT_ART_TILE;
    for (const std::string& model : models) {
        const std::string folder = std::string{directory}.append("/").append(model);
        addModelEntries(root, folder,
                        AssetGalleryEntry{.family = family.title,
                                          .model = model,
                                          .form = {},
                                          .path = {},
                                          .frames = {},
                                          .tilePixels = tile},
                        family, catalog.errors);
        for (const char* still : {"portrait", "token"}) {
            const std::string file = std::string{still} + ".png";
            const auto [width, height] = pngSize(root / directory / model / file);
            if (width > 0) {
                family.entries.push_back(AssetGalleryEntry{.family = family.title,
                                                           .model = model,
                                                           .form = still,
                                                           .path = folder + "/" + file,
                                                           .frameWidth = width,
                                                           .frameHeight = height,
                                                           .frames = {},
                                                           .tilePixels = tile});
            }
        }
    }
    if (!family.entries.empty()) {
        catalog.families.push_back(std::move(family));
    }
}

[[nodiscard]] int classRank(const std::string& textureClass) {
    if (textureClass == "floor") {
        return 0;
    }
    if (textureClass == "tall") {
        return 1;
    }
    return textureClass == "wide" ? 2 : 3;
}

// Les pièces d'un manifeste de scène, en une famille rangée par classe.
// `directory` : Dossier du manifeste, relatif à la racine des assets, séparateurs `/`.
void readSceneFamily(const std::filesystem::path& root, const std::string& directory,
                     const std::string& title, AssetGalleryCatalog& catalog) {
    // Le manifeste des pièces se lit dans Core depuis le LOT-EDITOR-02 (constat A9) : la galerie,
    // l'éditeur et demain la collision déduite lisent la même emprise.
    const std::filesystem::path path = root / directory / "manifest.json";
    const core::ScenePieceManifestResult read = core::ScenePieceManifest::loadFromFile(path);
    if (!read.ok()) {
        if (read.error != core::ScenePieceManifestError::FileNotFound) {
            catalog.errors.push_back(path.generic_string() + " : " + read.message);
        }
        return;
    }
    AssetGalleryFamily family{.title = title, .directory = directory, .entries = {}};
    for (const core::ScenePiece& piece : read.manifest.pieces()) {
        // La galerie montre des images : un maillage (LOT-1003) n'a pas de vignette à y étaler.
        // Sa vue est celle de l'atelier des assets 3D (LOT-1008).
        if (piece.isMesh()) {
            continue;
        }
        family.entries.push_back(AssetGalleryEntry{
            .family = family.title,
            .model = piece.className.empty() ? std::string("autre") : piece.className,
            .form = piece.name,
            .path = directory + "/" + piece.file,
            .frameWidth = piece.width,
            .frameHeight = piece.height,
            .frames = {},
            .footprintColumns = piece.footprintColumns,
            .footprintRows = piece.footprintRows,
            .anchorX = piece.anchorX,
            .anchorY = piece.anchorY,
            .tilePixels = read.manifest.tileWidth()});
        // Une pièce animée -- un effet de `Common/Fx` (`LOT-136`) -- a son `.anim.json` à côté :
        // elle se joue dans la galerie au lieu de s'y étaler en bande.
        std::filesystem::path animation = root / directory / piece.file;
        animation.replace_extension(".anim.json");
        static_cast<void>(readAnimatedEntry(family.entries.back(), animation, catalog.errors));
    }
    std::ranges::stable_sort(family.entries,
                             [](const AssetGalleryEntry& left, const AssetGalleryEntry& right) {
                                 return classRank(left.model) < classRank(right.model);
                             });
    if (!family.entries.empty()) {
        catalog.families.push_back(std::move(family));
    }
}

void readScenes(const std::filesystem::path& root, AssetGalleryCatalog& catalog) {
    // Les lieux d'essai a plat ; l'arborescence par niveaux est lue par readTree.
    for (const std::string& name : core::scenePlaces(root)) {
        if (core::isFlatScenePlace(name)) {
            readSceneFamily(root, core::ownSceneDirectory(name), "Scène · " + name, catalog);
        }
    }
}

// L'arborescence par niveaux : chaque `manifest.json` sous `Common/` et `Regions/`, dans
//        l'ordre de son chemin.
//
// Un manifeste qui déclare des `textures` est un dossier `Scene/` ; un manifeste qui déclare des
// personnages (`npcs`, `portraits`, `models`) est un dossier `Characters/`, dont les PNJ ont la
// forme de l'atelier (`readFigures`). Les manifestes des niveaux encore vides n'ajoutent aucune famille.
void readTree(const std::filesystem::path& root, AssetGalleryCatalog& catalog) {
    std::vector<std::filesystem::path> manifests;
    for (const char* tree : {"Common", "Regions"}) {
        std::error_code error;
        for (auto it = std::filesystem::recursive_directory_iterator(root / tree, error);
             !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
            if (it->is_regular_file() && it->path().filename() == "manifest.json") {
                manifests.push_back(it->path());
            }
        }
    }
    std::ranges::sort(manifests, [](const auto& left, const auto& right) {
        return left.generic_string() < right.generic_string();
    });
    for (const std::filesystem::path& path : manifests) {
        const core::JsonDocument document = readManifest(path, catalog.errors);
        if (!document.ok()) {
            continue;
        }
        const std::filesystem::path directory = path.parent_path();
        const std::string relative = std::filesystem::relative(directory, root).generic_string();
        if (document.root.contains("textures")) {
            // Le lieu, sans le préfixe `Regions/` ni le dossier `Scene` : ce qui le nomme dans
            // l'atlas (« Scène · central-empire/capital/arenarea/arena-of-fate »). Le commun du
            // monde range ses pièces par sorte (« Scène · Common/Terrain »).
            std::string place = relative;
            if (place.ends_with("/Scene")) {
                place.erase(place.size() - std::string_view("/Scene").size());
            }
            if (place.starts_with("Regions/")) {
                place.erase(0, std::string_view("Regions/").size());
            }
            readSceneFamily(root, relative, "Scène · " + place, catalog);
        } else if (document.root.contains("npcs") || document.root.contains("portraits") ||
                   document.root.contains("models")) {
            readFigures(root, relative, "Figurines · " + relative, catalog);
        }
    }
}

// Le nombre de cases que couvrent `pixels` d'art, une case valant `tilePixels`.
[[nodiscard]] int ceilCells(int pixels, int tilePixels) {
    const int tile = std::max(1, tilePixels);
    return pixels <= 0 ? 0 : (pixels + tile - 1) / tile;
}

}  // namespace

AssetGalleryCatalog AssetGalleryCatalog::load(const std::filesystem::path& assetsRoot) {
    AssetGalleryCatalog catalog;
    readFigures(assetsRoot, "Npc", "PNJ", catalog);
    readFigures(assetsRoot, "Monsters", "Monstres", catalog);
    readScenes(assetsRoot, catalog);
    readTree(assetsRoot, catalog);
    return catalog;
}

bool assetGalleryExcludes(std::string_view path) noexcept {
    // Les cartes rendues des zones (LOT-121) sont des cartes de l'ecran « Carte », comme Maps/.
    const bool zoneMap =
        path.starts_with("Regions/") && path.find("/Map/") != std::string_view::npos;
    return path.starts_with("UI/") || path.starts_with("Maps/") || path.starts_with("Fonts/") ||
           zoneMap;
}

std::vector<std::string> assetGalleryUnlisted(const std::filesystem::path& assetsRoot,
                                              const AssetGalleryCatalog& catalog) {
    std::set<std::string> listed;
    for (const AssetGalleryFamily& family : catalog.families) {
        for (const AssetGalleryEntry& entry : family.entries) {
            listed.insert(entry.path);
        }
    }
    std::vector<std::string> unlisted;
    std::error_code error;
    for (auto it = std::filesystem::recursive_directory_iterator(assetsRoot, error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        const std::string extension = it->path().extension().string();
        if (!it->is_regular_file() ||
            (extension != ".png" && extension != ".jpg" && extension != ".glb")) {
            continue;
        }
        const std::string path = std::filesystem::relative(it->path(), assetsRoot).generic_string();
        if (!listed.contains(path) && !assetGalleryExcludes(path)) {
            unlisted.push_back(path);
        }
    }
    std::ranges::sort(unlisted);
    return unlisted;
}

std::size_t AssetGalleryCatalog::entryCount() const noexcept {
    std::size_t count = 0;
    for (const AssetGalleryFamily& family : families) {
        count += family.entries.size();
    }
    return count;
}

AssetGalleryBloc assetGalleryBlocShape(const AssetGalleryEntry& entry) {
    const int footprintColumns = std::max(1, entry.footprintColumns);
    const int footprintRows = std::max(1, entry.footprintRows);
    // Le dessin monte au-dessus du bas de l'emprise ; en largeur, il est centré sur elle.
    const int tile = entry.tileWidthPixels();
    const int inner = std::max(footprintColumns, ceilCells(entry.frameWidth, tile));
    const int above = std::max(footprintRows, ceilCells(entry.frameHeight, tile));
    AssetGalleryBloc bloc;
    bloc.columns = inner + 2;
    bloc.rows = above + 2;
    bloc.footprintColumn = (bloc.columns - footprintColumns) / 2;
    bloc.footprintRow = bloc.rows - 1 - footprintRows;
    return bloc;
}

AssetGalleryLayout layoutAssetGallery(const AssetGalleryCatalog& catalog, int maximumColumns) {
    AssetGalleryLayout layout;
    int row = 0;

    // Une ligne visuelle : ses blocs sont posés sur le même bas, une fois sa hauteur connue.
    std::vector<AssetGalleryBloc> line;
    int lineColumns = 0;
    int lineRows = 0;
    const auto flush = [&] {
        for (AssetGalleryBloc& bloc : line) {
            bloc.row = row + lineRows - bloc.rows;
            layout.blocs.push_back(bloc);
        }
        layout.columns = std::max(layout.columns, lineColumns);
        row += lineRows;
        line.clear();
        lineColumns = 0;
        lineRows = 0;
    };

    for (int familyIndex = 0; std::cmp_less(familyIndex, catalog.families.size()); ++familyIndex) {
        const AssetGalleryFamily& family = catalog.families[static_cast<std::size_t>(familyIndex)];
        layout.bands.push_back(AssetGalleryBand{.family = familyIndex, .row = row});
        ++row;

        // Les modèles dans l'ordre de leur première forme.
        std::vector<std::string> models;
        std::map<std::string, std::vector<int>> byModel;
        for (int index = 0; std::cmp_less(index, family.entries.size()); ++index) {
            const std::string& model = family.entries[static_cast<std::size_t>(index)].model;
            if (!byModel.contains(model)) {
                models.push_back(model);
            }
            byModel[model].push_back(index);
        }
        for (const std::string& model : models) {
            for (const int index : byModel[model]) {
                AssetGalleryBloc bloc =
                    assetGalleryBlocShape(family.entries[static_cast<std::size_t>(index)]);
                if (lineColumns > 0 && lineColumns + bloc.columns > maximumColumns) {
                    flush();
                }
                bloc.family = familyIndex;
                bloc.entry = index;
                bloc.column = lineColumns;
                lineColumns += bloc.columns;
                lineRows = std::max(lineRows, bloc.rows);
                line.push_back(bloc);
            }
            flush();
        }
    }
    layout.rows = row;
    return layout;
}

AssetGalleryVisibility assetGalleryVisibility(const AssetGalleryBloc& bloc,
                                              const AssetGalleryView& view,
                                              double ringCells) noexcept {
    const auto overlaps = [&](double margin) {
        return bloc.column < view.column + view.columns + margin &&
               bloc.column + bloc.columns > view.column - margin &&
               bloc.row < view.row + view.rows + margin && bloc.row + bloc.rows > view.row - margin;
    };
    if (overlaps(0.0)) {
        return AssetGalleryVisibility::Drawn;
    }
    return overlaps(ringCells) ? AssetGalleryVisibility::Preloaded
                               : AssetGalleryVisibility::Unloaded;
}

double assetGalleryClipSeconds(const AssetGalleryEntry& entry, double seconds) noexcept {
    if (!entry.mesh || entry.clipDuration <= 0.0 || seconds <= 0.0) {
        return 0.0;
    }
    const double cycle =
        entry.loop ? entry.clipDuration : entry.clipDuration + ASSET_GALLERY_ONE_SHOT_HOLD_SECONDS;
    return std::min(std::fmod(seconds, cycle), entry.clipDuration);
}

int assetGalleryFrameRank(const AssetGalleryEntry& entry, double seconds) noexcept {
    const int count = entry.frameCount();
    if (count <= 1 || entry.frameDuration <= 0.0 || seconds <= 0.0) {
        return 0;
    }
    const auto step = static_cast<long long>(std::floor(seconds / entry.frameDuration));
    if (entry.loop) {
        return static_cast<int>(step % count);
    }
    const auto hold = static_cast<long long>(
        std::ceil(ASSET_GALLERY_ONE_SHOT_HOLD_SECONDS / entry.frameDuration));
    return static_cast<int>(std::min<long long>(step % (count + hold), count - 1));
}

}  // namespace hmi
