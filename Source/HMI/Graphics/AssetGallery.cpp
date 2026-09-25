// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/AssetGallery.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <string_view>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include "Core/Data/JsonDocument.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Core/Resources/ScenePlace.h"
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

// Les figurines d'un atelier : un dossier par modèle, ses bandes animées et son portrait.
//
// Sert aux PNJ (`Npc/`, LOT-91) et aux monstres (`Monsters/`, LOT-93), qui partagent la forme :
// un `manifest.json` qui nomme les `animations`, puis `<modèle>/<animation>.png` et son
// `.anim.json`. Une animation absente d'un modèle — le `cast` d'une bête sans sort — ne fait pas
// d'entrée, et ce n'est pas une erreur. La taille de la cellule est celle de chaque `.anim.json` :
// une figurine Grande (96 × 96) s'affiche comme une Moyenne (48 × 64), sans cas particulier.
//
// Une figurine **orientée** (`LOT-112`) a une bande par animation et par diagonale
// (`walk-se.png`…) : chacune fait son entrée. Un modèle rangé plus bas que l'atelier
// (`Characters/Heroes/brawler`) est trouvé par la liste `npcs` du manifeste.
void readFigures(const std::filesystem::path& root, const std::string& directory,
                 const std::string& title, AssetGalleryCatalog& catalog) {
    const core::JsonDocument document =
        readManifest(root / directory / "manifest.json", catalog.errors);
    if (!document.ok()) {
        return;
    }
    AssetGalleryFamily family{.title = title, .directory = directory, .entries = {}};
    // Tous les dossiers, pas seulement ceux que le manifeste retient pour le jeu : la galerie sert
    // justement à voir les autres.
    std::vector<std::string> models;
    std::error_code error;
    for (const auto& item : std::filesystem::directory_iterator(root / directory, error)) {
        if (item.is_directory()) {
            models.push_back(item.path().filename().string());
        }
    }
    for (const std::string& npc : stringList(document.root, "npcs")) {
        if (npc.find('/') != std::string::npos) {
            models.push_back(npc);
        }
    }
    std::ranges::sort(models);
    const auto duplicates = std::ranges::unique(models);
    models.erase(duplicates.begin(), duplicates.end());
    const std::vector<std::string> animations = stringList(document.root, "animations");
    const auto tile = static_cast<int>(manifestArtTile(document.root).x);
    for (const std::string& model : models) {
        const std::string folder = std::string{directory}.append("/").append(model).append("/");
        for (const std::string& animation : animations) {
            // La bande sans orientation, puis une par diagonale : `walk`, `walk-se`...
            for (const std::string_view facing : {"", "-se", "-sw", "-ne", "-nw"}) {
                const std::string strip = animation + std::string{facing};
                AssetGalleryEntry entry{.family = family.title,
                                        .model = model,
                                        .form = strip,
                                        .path = folder + strip + ".png",
                                        .frames = {},
                                        .tilePixels = tile};
                if (readAnimatedEntry(entry, root / directory / model / (strip + ".anim.json"),
                                      catalog.errors)) {
                    family.entries.push_back(std::move(entry));
                }
            }
        }
        for (const char* still : {"portrait", "token"}) {
            const std::string file = std::string{still} + ".png";
            const auto [width, height] = pngSize(root / directory / model / file);
            if (width > 0) {
                family.entries.push_back(AssetGalleryEntry{.family = family.title,
                                                           .model = model,
                                                           .form = still,
                                                           .path = folder + file,
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
// `animations` est un dossier `Characters/`, dont les PNJ ont la forme de l'atelier
// (`readFigures`). Les manifestes des niveaux encore vides n'ajoutent aucune famille.
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
        } else if (document.root.contains("animations")) {
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
        if (!it->is_regular_file() || (extension != ".png" && extension != ".jpg")) {
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
