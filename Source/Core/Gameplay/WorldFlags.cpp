// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Gameplay/WorldFlags.h"

#include <algorithm>

namespace core {

bool WorldFlags::isSet(std::string_view key) const {
    return _values.find(key) != _values.end();
}

bool WorldFlags::set(std::string_view key) {
    // Le retour dit si le fait etait NEUF. C'est cette valeur qui repond a << le coffre a-t-il deja
    // ete ouvert ? >>, et la rendre evite au gameplay de faire un `isSet` puis un `set` -- deux
    // appels entre lesquels un autre pourrait se glisser.
    if (_declared.find(key) != _declared.end()) {
        return false;
    }
    const bool neuf = _values.emplace(std::string(key), std::string()).second;
    if (neuf) {
        ++_revision;
    }
    return neuf;
}

void WorldFlags::clear(std::string_view key) {
    const auto trouve = _values.find(key);
    if (trouve != _values.end()) {
        _values.erase(trouve);
        ++_revision;
    }
}

bool WorldFlags::declare(std::string_view key, std::vector<std::string> values,
                         std::string_view initial) {
    if (values.empty() || std::find(values.begin(), values.end(), initial) == values.end()) {
        return false;
    }
    if (const auto deja = _declared.find(key); deja != _declared.end()) {
        return deja->second.values == values && deja->second.initial == initial;
    }
    _declared.emplace(std::string(key), Declaration{std::move(values), std::string(initial)});
    ++_revision;
    return true;
}

const std::vector<std::string>* WorldFlags::declaredValues(std::string_view key) const {
    const auto trouve = _declared.find(key);
    return trouve == _declared.end() ? nullptr : &trouve->second.values;
}

bool WorldFlags::setValue(std::string_view key, std::string_view value) {
    const auto declaration = _declared.find(key);
    if (declaration == _declared.end()) {
        return false;
    }
    const std::vector<std::string>& permises = declaration->second.values;
    if (std::find(permises.begin(), permises.end(), value) == permises.end()) {
        return false;
    }
    const auto trouve = _values.find(key);
    if (trouve == _values.end()) {
        _values.emplace(std::string(key), std::string(value));
    } else if (trouve->second != value) {
        trouve->second = value;
    } else {
        return true;
    }
    ++_revision;
    return true;
}

std::optional<std::string> WorldFlags::value(std::string_view key) const {
    if (const auto trouve = _values.find(key); trouve != _values.end()) {
        return trouve->second;
    }
    if (const auto declaration = _declared.find(key); declaration != _declared.end()) {
        return declaration->second.initial;
    }
    return std::nullopt;
}

std::vector<std::string> WorldFlags::all() const {
    std::vector<std::string> cles;
    cles.reserve(_values.size());
    for (const auto& [cle, valeur] : _values) {
        cles.push_back(cle);
    }
    return cles;
}

std::vector<std::pair<std::string, std::string>> WorldFlags::entries() const {
    return {_values.begin(), _values.end()};
}

std::string keyForEntity(std::string_view mapName, std::string_view entityType, int column,
                         int row) {
    std::string cle;
    cle.reserve(mapName.size() + entityType.size() + 16);
    cle.append(mapName).append("/").append(entityType).append("@");
    cle.append(std::to_string(column)).append(",").append(std::to_string(row));
    return cle;
}

}  // namespace core
