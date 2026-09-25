// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Levels/LevelWriter.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <variant>

#include <nlohmann/json.hpp>

#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelProperties.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Levels/TileTypeName.h"

namespace core {

namespace {

// Objet JSON a cles ORDONNEES par insertion : l'ordre des champs est celui du format, pas l'ordre
// alphabetique -- "version" en tete, les tuiles en dernier.
using Json = nlohmann::ordered_json;

// Tableaux dont les objets s'ecrivent sur UNE ligne chacun : une case = une ligne de diff.
[[nodiscard]] bool isCellArray(const std::string& key) {
    return key == "tiles" || key == "forced" || key == "cells";
}

// Reemet les proprietes libres d'une couche ou d'une entite A PLAT dans son objet JSON, a cote de
// ses champs connus (EX-LVL-018). Le chargeur range dans cette table toute cle qu'il ne reconnait
// pas, et on la lui rend telle quelle ; core::PropertyMap est ordonnee, l'ecriture deterministe.
void writeProperties(const PropertyMap& properties, Json& object) {
    for (const auto& [key, value] : properties) {
        // Alternative par alternative plutot que std::visit : l'analyseur statique ne suit pas la
        // table de saut de visit et croit la valeur non initialisee.
        if (const auto* const flag = std::get_if<bool>(&value)) {
            object[key] = *flag;
        } else if (const auto* const integer = std::get_if<std::int64_t>(&value)) {
            object[key] = *integer;
        } else if (const auto* const real = std::get_if<double>(&value)) {
            object[key] = *real;
        } else if (const auto* const text = std::get_if<std::string>(&value)) {
            object[key] = *text;
        }
    }
}

[[nodiscard]] Json cellJson(GridPosition cell) {
    Json object;
    object["x"] = cell.column;
    object["y"] = cell.row;
    return object;
}

// Cases d'une couche visuelle, au format {x, y, type, piece?, elevation?}. Une case est omise
// quand elle ne porte rien : ni type, ni piece, ni hauteur (EX-LVL-003).
[[nodiscard]] Json layerTilesJson(const TileLayer& layer) {
    Json array = Json::array();
    for (int row = 0; row < layer.tiles.height(); ++row) {
        for (int column = 0; column < layer.tiles.width(); ++column) {
            const TileType type = layer.tiles.tile(column, row);
            const std::string_view piece = layer.pieceAt(column, row);
            const int elevation = layer.elevationAt(column, row);
            if (type == TileType::Empty && piece.empty() && elevation == 0) {
                continue;
            }
            Json tile;
            tile["x"] = column;
            tile["y"] = row;
            tile["type"] = tileTypeName(type);
            if (!piece.empty()) {
                tile["piece"] = std::string{piece};
            }
            if (elevation != 0) {
                tile["elevation"] = elevation;
            }
            array.push_back(std::move(tile));
        }
    }
    return array;
}

// Vrai pour la couche que le chargeur PROMEUT depuis la grille racine (Collision, ou Legacy pour
// une carte sans couche declaree). Elle n'est jamais reecrite dans "layers" : elle est deja le
// tableau racine "tiles".
[[nodiscard]] bool isPromotedRootLayer(const TileLayer& layer) {
    return layer.kind == LayerKind::Collision || layer.kind == LayerKind::Legacy;
}

// Tableau racine "tiles" : la grille de collision, entree comprise.
[[nodiscard]] Json rootTilesJson(const TileMap& tileMap) {
    Json tiles = Json::array();
    for (int row = 0; row < tileMap.height(); ++row) {
        for (int column = 0; column < tileMap.width(); ++column) {
            const TileType type = tileMap.tile(column, row);
            if (type == TileType::Empty) {
                continue;
            }
            Json tile;
            tile["x"] = column;
            tile["y"] = row;
            tile["type"] = tileTypeName(type);
            tiles.push_back(std::move(tile));
        }
    }
    return tiles;
}

// Tableau racine optionnel "layers" : les couches VISIBLES uniquement, dans leur ordre de
// superposition. Champs connus d'abord, proprietes ensuite, cases en dernier : ce qui dit ce qu'est
// la couche se lit en tete, sans descendre sous des centaines de cases.
[[nodiscard]] Json visibleLayersJson(const std::vector<TileLayer>& layers) {
    Json layersJson = Json::array();
    for (const TileLayer& layer : layers) {
        if (isPromotedRootLayer(layer)) {
            continue;
        }
        Json layerJson;
        // Le nom est libre et facultatif : omis quand il est vide, comme tout champ a sa valeur
        // par defaut. Le role, lui, est toujours ecrit.
        if (!layer.name.empty()) {
            layerJson["name"] = layer.name;
        }
        layerJson["kind"] = layerKindName(layer.kind);
        if (layer.floor != 0) {
            layerJson["floor"] = layer.floor;
        }
        writeProperties(layer.properties, layerJson);
        layerJson["tiles"] = layerTilesJson(layer);
        layersJson.push_back(std::move(layerJson));
    }
    return layersJson;
}

// Tableau racine "entities" (EX-LVL-017). Le type est ecrit meme vide : une entite sans type est
// une donnee fautive qu'il vaut mieux voir dans le fichier que faire disparaitre a
// l'enregistrement.
[[nodiscard]] Json entitiesJson(const std::vector<MapEntity>& entities) {
    Json array = Json::array();
    for (const MapEntity& entity : entities) {
        Json entityJson;
        if (!entity.id.empty()) {
            entityJson["id"] = entity.id;
        }
        entityJson["type"] = entity.type;
        entityJson["x"] = entity.position.column;
        entityJson["y"] = entity.position.row;
        if (entity.elevation != 0) {
            entityJson["elevation"] = entity.elevation;
        }
        if (!entity.cells.empty()) {
            Json cells = Json::array();
            for (const GridPosition cell : entity.cells) {
                cells.push_back(cellJson(cell));
            }
            entityJson["cells"] = std::move(cells);
        }
        writeProperties(entity.properties, entityJson);
        array.push_back(std::move(entityJson));
    }
    return array;
}

// --- Ecriture canonique ---------------------------------------------------------------------
//
// Deux espaces d'indentation, comme tout ecrivain JSON du projet ; mais les objets d'une liste de
// cases ("tiles", "forced", "cells") tiennent sur UNE ligne. Une carte est un fichier versionne :
// poser une piece doit changer une ligne du diff, pas cinq : Martpart passe de 190 a 126 Ko, ses
// 1 172 pieces nommees comprises.

void appendScalar(std::string& out, const Json& value) {
    out += value.dump(-1, ' ', false, Json::error_handler_t::replace);
}

void appendInline(std::string& out, const Json& object) {
    out += '{';
    bool first = true;
    for (const auto& [key, value] : object.items()) {
        if (!first) {
            out += ", ";
        }
        first = false;
        appendScalar(out, Json(key));
        out += ": ";
        appendScalar(out, value);
    }
    out += '}';
}

void appendIndent(std::string& out, int depth) {
    out.append(static_cast<std::size_t>(depth) * 2U, ' ');
}

void appendValue(std::string& out, const Json& value, int depth, bool cellsInline) {
    if (value.is_object()) {
        if (value.empty()) {
            out += "{}";
            return;
        }
        out += "{\n";
        bool first = true;
        for (const auto& [key, member] : value.items()) {
            if (!first) {
                out += ",\n";
            }
            first = false;
            appendIndent(out, depth + 1);
            appendScalar(out, Json(key));
            out += ": ";
            appendValue(out, member, depth + 1, isCellArray(key));
        }
        out += '\n';
        appendIndent(out, depth);
        out += '}';
        return;
    }
    if (value.is_array()) {
        if (value.empty()) {
            out += "[]";
            return;
        }
        out += "[\n";
        bool first = true;
        for (const Json& element : value) {
            if (!first) {
                out += ",\n";
            }
            first = false;
            appendIndent(out, depth + 1);
            if (cellsInline && element.is_object()) {
                appendInline(out, element);
            } else {
                appendValue(out, element, depth + 1, false);
            }
        }
        out += '\n';
        appendIndent(out, depth);
        out += ']';
        return;
    }
    appendScalar(out, value);
}

// Les proprietes de la CARTE (LOT-EDITOR-09), a plat a la racine comme celles d'une couche. Une
// propriete qui porte le nom d'un champ deja ecrit est ignoree : le format fait foi, et le
// chargeur n'en range jamais de telle.
void writeRootProperties(const PropertyMap& properties, Json& root) {
    PropertyMap free;
    for (const auto& [key, value] : properties) {
        if (!root.contains(key)) {
            free.emplace(key, value);
        }
    }
    writeProperties(free, root);
}

[[nodiscard]] std::string canonicalText(const Json& root) {
    std::string out;
    appendValue(out, root, 0, false);
    out += '\n';
    return out;
}

}  // namespace

std::string LevelWriter::toJsonString(const Level& level) {
    return buildJson(level.data());
}

bool LevelWriter::saveToFile(const Level& level, const std::filesystem::path& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    const std::string json = toJsonString(level);
    file.write(json.data(), static_cast<std::streamsize>(json.size()));
    return file.good();
}

std::string LevelWriter::buildJson(const LevelData& data) {
    Json root;
    root["version"] = LEVEL_FORMAT_VERSION;
    root["name"] = data.name;

    // Une variante (decision D12) ne s'ecrit que par ce qu'elle declare : ses cases sont celles de
    // sa base, et les recopier en ferait une seconde carte a tenir d'accord.
    if (!data.base.empty()) {
        root["base"] = data.base;
        if (!data.scene.empty()) {
            root["scene"] = data.scene;
        }
        if (data.nextEntityId != 1) {
            root["nextEntityId"] = data.nextEntityId;
        }
        if (!data.entities.empty()) {
            root["entities"] = entitiesJson(data.entities);
        }
        writeRootProperties(data.properties, root);
        return canonicalText(root);
    }

    const TileMap& tileMap = data.tileMap;
    root["width"] = tileMap.width();
    root["height"] = tileMap.height();
    if (data.nextEntityId != 1) {
        root["nextEntityId"] = data.nextEntityId;
    }
    root["tiles"] = rootTilesJson(tileMap);
    if (!data.forcedCollision.empty()) {
        Json forced = Json::array();
        for (const GridPosition cell : data.forcedCollision) {
            forced.push_back(cellJson(cell));
        }
        root["forced"] = std::move(forced);
    }

    Json layersJson = visibleLayersJson(data.layers);
    if (!layersJson.empty()) {
        root["layers"] = std::move(layersJson);
    }
    if (!data.entities.empty()) {
        root["entities"] = entitiesJson(data.entities);
    }
    writeRootProperties(data.properties, root);
    return canonicalText(root);
}

}  // namespace core
