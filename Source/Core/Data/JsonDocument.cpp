// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Data/JsonDocument.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace core {

namespace {

/// @brief Construit un échec, en un point unique pour que tous se ressemblent.
[[nodiscard]] JsonDocument failure(JsonReadError code, std::string message,
                                   TextPosition position = {}) {
    return JsonDocument{.root = nlohmann::json::object(),
                        .error = code,
                        .message = std::move(message),
                        .position = position,
                        .version = 0};
}

/// @brief Préfixe « fichier:ligne:colonne : » d'un message, réduit à ce qui est connu.
[[nodiscard]] std::string prefix(std::string_view origin, TextPosition position) {
    std::string out;
    if (!origin.empty()) {
        out += std::string(origin);
    }
    if (position.line > 0) {
        if (!out.empty()) {
            out += ':';
        }
        out += std::to_string(position.line) + ':' + std::to_string(position.column);
    }
    if (!out.empty()) {
        out += " : ";
    }
    return out;
}

}  // namespace

TextPosition positionOf(std::string_view text, std::size_t byteOffset) {
    if (byteOffset == 0 || byteOffset > text.size()) {
        return {};
    }
    // `byte` de nlohmann est 1-indexé et désigne l'octet **fautif** : on compte ce qui précède.
    const std::size_t limit = byteOffset - 1;
    int line = 1;
    int column = 1;
    for (std::size_t i = 0; i < limit; ++i) {
        if (text[i] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }
    return {.line = line, .column = column};
}

namespace {

/// Relit un texte JSON bien formé en suivant un chemin, et note où commence la valeur visée.
class Pisteur {
public:
    Pisteur(std::string_view texte, std::vector<std::string> chemin)
        : _texte(texte), _chemin(std::move(chemin)) {}

    [[nodiscard]] TextPosition chercher() {
        valeur(0);
        return _trouve;
    }

private:
    void espaces() {
        while (_i < _texte.size() && (_texte[_i] == ' ' || _texte[_i] == '\t' ||
                                      _texte[_i] == '\n' || _texte[_i] == '\r')) {
            avancer();
        }
    }

    void avancer() {
        if (_texte[_i] == '\n') {
            ++_ligne;
            _colonne = 1;
        } else {
            ++_colonne;
        }
        ++_i;
    }

    /// Une chaîne, guillemets compris ; rend son contenu, échappements laissés bruts sauf `\"`.
    std::string chaine() {
        std::string contenu;
        avancer();  // le guillemet ouvrant
        while (_i < _texte.size() && _texte[_i] != '"') {
            if (_texte[_i] == '\\' && _i + 1 < _texte.size()) {
                avancer();
            }
            contenu += _texte[_i];
            avancer();
        }
        if (_i < _texte.size()) {
            avancer();  // le guillemet fermant
        }
        return contenu;
    }

    /// Une valeur à la profondeur @p profondeur du chemin courant.
    void valeur(std::size_t profondeur) {
        espaces();
        if (_i >= _texte.size() || _trouve.line > 0) {
            return;
        }
        const bool surLeChemin = profondeur == _suivis && _suivis == _chemin.size();
        if (surLeChemin) {
            _trouve = {.line = _ligne, .column = _colonne};
            return;
        }
        const char c = _texte[_i];
        if (c == '{') {
            conteneur(profondeur, true);
        } else if (c == '[') {
            conteneur(profondeur, false);
        } else if (c == '"') {
            chaine();
        } else {
            while (_i < _texte.size() && _texte[_i] != ',' && _texte[_i] != '}' &&
                   _texte[_i] != ']' && _texte[_i] != ' ' && _texte[_i] != '\n' &&
                   _texte[_i] != '\r' && _texte[_i] != '\t') {
                avancer();
            }
        }
    }

    void conteneur(std::size_t profondeur, bool objet) {
        const char fermant = objet ? '}' : ']';
        avancer();
        std::size_t rang = 0;
        while (_i < _texte.size() && _trouve.line == 0) {
            espaces();
            if (_i >= _texte.size() || _texte[_i] == fermant) {
                break;
            }
            std::string nom;
            if (objet) {
                nom = chaine();
                espaces();
                if (_i < _texte.size() && _texte[_i] == ':') {
                    avancer();
                }
            } else {
                nom = std::to_string(rang);
            }
            // Le chemin ne descend dans un enfant que si tous les pas précédents ont été suivis.
            const bool suivi =
                profondeur == _suivis && _suivis < _chemin.size() && _chemin[_suivis] == nom;
            if (suivi) {
                ++_suivis;
            }
            valeur(profondeur + 1);
            if (suivi && _trouve.line == 0) {
                --_suivis;
            }
            espaces();
            if (_i < _texte.size() && _texte[_i] == ',') {
                avancer();
            }
            ++rang;
        }
        if (_i < _texte.size() && _trouve.line == 0) {
            avancer();
        }
    }

    std::string_view _texte;
    std::vector<std::string> _chemin;
    std::size_t _i = 0;
    std::size_t _suivis = 0;
    int _ligne = 1;
    int _colonne = 1;
    TextPosition _trouve;
};

/// Les pas d'un pointeur JSON, `~1` et `~0` rendus à `/` et `~`.
[[nodiscard]] std::vector<std::string> pasDuPointeur(const nlohmann::json::json_pointer& pointeur) {
    const std::string texte = pointeur.to_string();
    std::vector<std::string> pas;
    std::size_t debut = 1;
    while (debut <= texte.size() && !texte.empty()) {
        const std::size_t fin = std::min(texte.find('/', debut), texte.size());
        std::string brut = texte.substr(debut, fin - debut);
        std::string pasLu;
        for (std::size_t i = 0; i < brut.size(); ++i) {
            if (brut[i] == '~' && i + 1 < brut.size()) {
                pasLu += brut[i + 1] == '1' ? '/' : '~';
                ++i;
            } else {
                pasLu += brut[i];
            }
        }
        pas.push_back(std::move(pasLu));
        debut = fin + 1;
    }
    return pas;
}

}  // namespace

TextPosition positionOfPointer(std::string_view text, const nlohmann::json::json_pointer& pointer) {
    return Pisteur(text, pasDuPointeur(pointer)).chercher();
}

JsonDocument readJsonObject(std::string_view json, int supportedVersion, std::string_view origin,
                            std::string_view versionField) {
    // `parse` en mode non-lançant renverrait `discarded` sans dire **où**. On laisse donc
    // l'exception se produire ici, à l'intérieur de la brique, pour en extraire la position — et
    // aucune ne franchit cette frontière (EX-NFR-040).
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(json);
    } catch (const nlohmann::json::parse_error& e) {
        const TextPosition position = positionOf(json, e.byte);
        return failure(JsonReadError::ParseError,
                       prefix(origin, position) + "JSON malforme : " + e.what(), position);
    } catch (const nlohmann::json::exception& e) {
        // Un texte bien formé que nlohmann ne sait pas représenter : un nombre hors de portée
        // (`1e400`) lève `out_of_range`, pas `parse_error`, et sans position. Trouvé par le fuzzing
        // de nuit : sans cette garde, l'exception franchissait la brique.
        return failure(JsonReadError::ParseError,
                       prefix(origin, {}) + "JSON illisible : " + e.what());
    }

    if (!root.is_object()) {
        return failure(JsonReadError::ParseError,
                       prefix(origin, {}) + "La racine du document n'est pas un objet.");
    }

    // Version absente : vaut 1. Un catalogue écrit avant que le format ne se versionne reste
    // lisible, ce qui évite d'avoir à réécrire les fichiers existants au premier versionnage.
    int version = 1;
    const std::string field(versionField);
    if (root.contains(field)) {
        if (!root[field].is_number_integer()) {
            return failure(JsonReadError::MalformedStructure,
                           prefix(origin, {}) + "Le champ « " + field + " » n'est pas un entier.");
        }
        version = root[field].get<int>();
    }
    if (supportedVersion > 0 && version > supportedVersion) {
        return failure(JsonReadError::UnsupportedVersion,
                       prefix(origin, {}) + "Version de format " + std::to_string(version) +
                           " non geree (cette version du jeu lit jusqu'a " +
                           std::to_string(supportedVersion) + ").");
    }

    return JsonDocument{.root = std::move(root),
                        .error = JsonReadError::None,
                        .message = {},
                        .position = {},
                        .version = version};
}

JsonDocument readJsonObjectFromFile(const std::filesystem::path& path, int supportedVersion,
                                    std::string_view versionField) {
    std::ifstream file(path);
    if (!file) {
        return failure(JsonReadError::FileNotFound,
                       "Fichier introuvable ou illisible : " + path.string());
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    const std::string text = contents.str();
    // Le nom du fichier entre dans les messages : « manifest.json:12:5 : … » se corrige, « JSON
    // malforme » ne se corrige pas.
    return readJsonObject(text, supportedVersion, path.filename().string(), versionField);
}

}  // namespace core
