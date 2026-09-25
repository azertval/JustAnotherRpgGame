// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Resources/AssetKey.h"

#include <algorithm>
#include <system_error>
#include <utility>

#include "Core/Data/JsonDocument.h"

namespace core {
namespace {

// La table des familles porte un `version`, contrairement aux entrees de catalogue : c'est un
// document de FORMAT -- il decrit le vocabulaire, pas une entree du monde.
constexpr int VERSION_ATTENDUE = 1;

// Un segment de cle : minuscules, chiffres, tirets SIMPLES, ni au debut ni a la fin.
[[nodiscard]] bool segmentValide(std::string_view segment) {
    if (segment.empty() || segment.front() == '-' || segment.back() == '-') {
        return false;
    }
    bool tiretPrecedent = false;
    for (const char lettre : segment) {
        const bool minuscule = lettre >= 'a' && lettre <= 'z';
        const bool chiffre = lettre >= '0' && lettre <= '9';
        if (lettre == '-') {
            if (tiretPrecedent) {
                return false;  // deux tirets de suite
            }
            tiretPrecedent = true;
            continue;
        }
        if (!minuscule && !chiffre) {
            return false;
        }
        tiretPrecedent = false;
    }
    return true;
}

}  // namespace

const AssetFamilyDefinition* AssetFamilyTable::find(std::string_view name) const {
    const auto trouve = std::ranges::find(families, name, &AssetFamilyDefinition::name);
    return trouve == families.end() ? nullptr : &*trouve;
}

bool isValidAssetKey(std::string_view key) {
    const std::size_t separateur = key.find('/');
    if (separateur == std::string_view::npos) {
        return false;
    }
    // Exactement DEUX segments : `beast/wolf`, jamais `beast/wolf/token`. Un troisieme segment
    // serait un chemin qui ne dit pas son nom, et c'est precisement ce qu'EX-CNT-040 ecarte.
    const std::string_view famille = key.substr(0, separateur);
    const std::string_view reste = key.substr(separateur + 1);
    return reste.find('/') == std::string_view::npos && segmentValide(famille) &&
           segmentValide(reste);
}

std::optional<AssetKey> parseAssetKey(std::string_view key) {
    if (!isValidAssetKey(key)) {
        return std::nullopt;
    }
    const std::size_t separateur = key.find('/');
    return AssetKey{.family = std::string(key.substr(0, separateur)),
                    .id = std::string(key.substr(separateur + 1))};
}

std::string defaultAssetKeyFor(std::string_view family, std::string_view id) {
    if (!segmentValide(family) || !segmentValide(id)) {
        return {};
    }
    return std::string(family) + "/" + std::string(id);
}

namespace {

// Une entree de la table des familles, lue champ par champ ; un champ absent garde son defaut.
[[nodiscard]] AssetFamilyDefinition readFamilyDefinition(const nlohmann::json& entree) {
    AssetFamilyDefinition famille;
    if (const auto nom = entree.find("name"); nom != entree.end() && nom->is_string()) {
        famille.name = nom->get<std::string>();
    }
    if (const auto largeur = entree.find("width");
        largeur != entree.end() && largeur->is_number_integer()) {
        famille.width = largeur->get<int>();
    }
    if (const auto hauteur = entree.find("height");
        hauteur != entree.end() && hauteur->is_number_integer()) {
        famille.height = hauteur->get<int>();
    }
    if (const auto dossiers = entree.find("catalogues");
        dossiers != entree.end() && dossiers->is_array()) {
        for (const nlohmann::json& dossier : *dossiers) {
            if (dossier.is_string()) {
                famille.catalogues.push_back(dossier.get<std::string>());
            }
        }
    }
    return famille;
}

// Le champ texte @p champ de @p racine, ou une chaine vide s'il est absent ou d'un autre type.
[[nodiscard]] std::string champTexte(const nlohmann::json& racine, const char* champ) {
    if (const auto trouve = racine.find(champ); trouve != racine.end() && trouve->is_string()) {
        return trouve->get<std::string>();
    }
    return {};
}

// Les fichiers `.json` d'un dossier de catalogue, tries.
[[nodiscard]] std::vector<std::filesystem::path> fichiersDeCatalogue(
    const std::filesystem::path& chemin, std::error_code& code) {
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(chemin, code)) {
        if (entree.is_regular_file(code) && entree.path().extension() == ".json") {
            fichiers.push_back(entree.path());
        }
    }
    std::ranges::sort(fichiers);
    return fichiers;
}

// La cle qu'attend le fichier de catalogue @p fichier de la famille @p famille, ou rien (erreur
// consignee dans @p errors).
[[nodiscard]] std::optional<ExpectedAssetKey> cleAttenduePour(const std::filesystem::path& fichier,
                                                              const AssetFamilyDefinition& famille,
                                                              const AssetFamilyTable& families,
                                                              std::vector<std::string>& errors) {
    const JsonDocument document = readJsonObjectFromFile(fichier, 0);
    if (!document.ok()) {
        errors.push_back(document.message);
        return std::nullopt;
    }
    const std::string identifiant = champTexte(document.root, "id");
    if (identifiant.empty()) {
        errors.push_back(fichier.filename().string() + " : entree sans identifiant.");
        return std::nullopt;
    }

    // Une DEROGATION explicite l'emporte : deux entrees peuvent partager une
    // illustration. Elle doit en revanche etre bien formee et nommer une famille
    // connue -- c'est la seule chose qui fasse echouer la verification, une cle sans
    // image n'etant qu'un etat d'avancement (EX-CNT-041).
    const std::string ecrite = champTexte(document.root, "asset");
    if (!ecrite.empty()) {
        const std::optional<AssetKey> decomposee = parseAssetKey(ecrite);
        if (!decomposee.has_value()) {
            errors.push_back(fichier.filename().string() + " : cle d'asset malformee ('" + ecrite +
                             "').");
            return std::nullopt;
        }
        if (families.find(decomposee->family) == nullptr) {
            errors.push_back(fichier.filename().string() +
                             " : cle d'asset orpheline, famille inconnue ('" + decomposee->family +
                             "').");
            return std::nullopt;
        }
        return ExpectedAssetKey{.key = ecrite,
                                .family = decomposee->family,
                                .sourceFile = fichier.filename().string(),
                                .explicitKey = true};
    }

    const std::string defaut = defaultAssetKeyFor(famille.name, identifiant);
    if (defaut.empty()) {
        errors.push_back(fichier.filename().string() + " : identifiant '" + identifiant +
                         "' ne peut pas former une cle d'asset.");
        return std::nullopt;
    }
    return ExpectedAssetKey{.key = defaut,
                            .family = famille.name,
                            .sourceFile = fichier.filename().string(),
                            .explicitKey = false};
}

}  // namespace

AssetFamilyTable loadAssetFamilies(const std::filesystem::path& familiesFile) {
    AssetFamilyTable table;
    const JsonDocument document = readJsonObjectFromFile(familiesFile, VERSION_ATTENDUE);
    if (!document.ok()) {
        table.errors.push_back(document.message);
        return table;
    }
    const auto familles = document.root.find("families");
    if (familles == document.root.end() || !familles->is_array()) {
        table.errors.push_back(familiesFile.string() +
                               " : champ 'families' absent ou non tableau.");
        return table;
    }
    for (const nlohmann::json& entree : *familles) {
        if (!entree.is_object()) {
            continue;
        }
        AssetFamilyDefinition famille = readFamilyDefinition(entree);
        if (famille.name.empty() || !segmentValide(famille.name)) {
            table.errors.push_back(familiesFile.string() + " : famille au nom invalide ('" +
                                   famille.name + "').");
            continue;
        }
        if (famille.width <= 0 || famille.height <= 0) {
            // Une famille sans dimensions ne peut pas valider un fichier : la laisser passer
            // rendrait le contrat muet pour toute une famille, sans que rien ne le signale.
            table.errors.push_back(familiesFile.string() + " : famille '" + famille.name +
                                   "' sans dimensions attendues.");
            continue;
        }
        table.families.push_back(std::move(famille));
    }
    if (table.families.empty() && table.errors.empty()) {
        table.errors.push_back(familiesFile.string() + " : aucune famille declaree.");
    }
    return table;
}

std::vector<ExpectedAssetKey> expectedAssetKeys(const std::filesystem::path& rpgDir,
                                                const AssetFamilyTable& families,
                                                std::vector<std::string>& errors) {
    std::vector<ExpectedAssetKey> attendues;
    std::error_code code;

    for (const AssetFamilyDefinition& famille : families.families) {
        for (const std::string& dossier : famille.catalogues) {
            const std::filesystem::path chemin = rpgDir / dossier;
            if (!std::filesystem::is_directory(chemin, code)) {
                errors.push_back(chemin.string() + " : dossier de catalogue absent (famille '" +
                                 famille.name + "').");
                continue;
            }
            for (const std::filesystem::path& fichier : fichiersDeCatalogue(chemin, code)) {
                if (std::optional<ExpectedAssetKey> attendue =
                        cleAttenduePour(fichier, famille, families, errors)) {
                    attendues.push_back(std::move(*attendue));
                }
            }
        }
    }
    return attendues;
}

}  // namespace core
