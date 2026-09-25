// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/Bestiary.h"

#include <algorithm>
#include <set>
#include <system_error>

#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/Ability.h"
#include "Core/Rpg/RpgEnumNames.h"

namespace core {

namespace {

// Les fichiers de creature ne portent pas de champ `version` : ce sont des entrees de catalogue,
// pas des documents de format. `0` desactive la garde de version de readJsonObject, comme le fait
// deja le test des schemas.
constexpr int SANS_GARDE_DE_VERSION = 0;

// Un champ obligatoire absent est une donnee invalide, jamais une valeur par defaut : c'est ce
// que `check_rpg_data.py` refuse en CI, et le moteur doit refuser la meme chose. Ces fonctions
// renvoient false et remplissent `raison` plutot que de lever (EX-NFR-040).
[[nodiscard]] bool lireEntier(const nlohmann::json& objet, const char* champ, int& sortie,
                              std::string& raison) {
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_number_integer()) {
        raison = std::string{"champ '"} + champ + "' absent ou non entier";
        return false;
    }
    sortie = trouve->get<int>();
    return true;
}

[[nodiscard]] bool lireTexte(const nlohmann::json& objet, const char* champ, std::string& sortie,
                             std::string& raison) {
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_string()) {
        raison = std::string{"champ '"} + champ + "' absent ou non textuel";
        return false;
    }
    sortie = trouve->get<std::string>();
    return true;
}

[[nodiscard]] std::optional<float> lireReelFacultatif(const nlohmann::json& objet,
                                                      const char* champ) {
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_number()) {
        return std::nullopt;
    }
    return trouve->get<float>();
}

[[nodiscard]] std::string lireTexteFacultatif(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

[[nodiscard]] std::vector<std::string> lireTextes(const nlohmann::json& objet, const char* champ) {
    std::vector<std::string> valeurs;
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_array()) {
        return valeurs;
    }
    for (const auto& element : *trouve) {
        if (element.is_string()) {
            valeurs.push_back(element.get<std::string>());
        }
    }
    return valeurs;
}

// Les des sont lus par `parseDice`, la seule porte d'entree du projet : une notation illisible
// donne std::nullopt, jamais une expression devinee. Un `ld8` issu d'un OCR -- la faute que le
// LOT-30 a documentee -- ressort donc absent, et le test le voit.
[[nodiscard]] std::optional<Dice> lireDesFacultatifs(const nlohmann::json& objet,
                                                     const char* champ) {
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_string()) {
        return std::nullopt;
    }
    return parseDice(trouve->get<std::string>());
}

// Une valeur d'enumeration inconnue est SIGNALEE, pas ignoree : c'est le scenario exact
// d'EX-CNT-011 -- la donnee nomme un type que le moteur ne connait pas, la valeur tombe dans un
// cas par defaut, et l'attaque cesse de faire des degats sans que rien ne l'annonce.
template <typename Enum, typename Analyser>
void lireEnumerations(const nlohmann::json& objet, const char* champ, Analyser analyser,
                      std::vector<Enum>& sortie, const std::string& fichier,
                      std::vector<std::string>& erreurs) {
    for (const std::string& nom : lireTextes(objet, champ)) {
        const std::optional<Enum> valeur = analyser(nom);
        if (valeur.has_value()) {
            sortie.push_back(*valeur);
        } else {
            std::string message = fichier;
            message += " : ";
            message += champ;
            message += " '";
            message += nom;
            message +=
                "' inconnu du moteur. Une valeur ignoree ferait taire l'effet "
                "sans qu'aucun message ne le dise.";
            erreurs.push_back(std::move(message));
        }
    }
}

[[nodiscard]] bool lireCaracteristiques(const nlohmann::json& objet, std::array<int, 6>& sortie,
                                        std::string& raison) {
    const auto trouve = objet.find("abilities");
    if (trouve == objet.end() || !trouve->is_object()) {
        raison = "champ 'abilities' absent ou non objet";
        return false;
    }
    // Les six sont exigees. Une fiche a cinq caracteristiques n'existe pas, et completer la
    // sixieme a 10 rendrait une extraction incomplete indiscernable d'une creature moyenne.
    for (const Ability caracteristique : allAbilities()) {
        const std::string nom{abilityName(caracteristique)};
        const auto valeur = trouve->find(nom);
        if (valeur == trouve->end() || !valeur->is_number_integer()) {
            raison = "caracteristique '" + nom + "' absente";
            return false;
        }
        sortie[static_cast<std::size_t>(caracteristique)] = valeur->get<int>();
    }
    return true;
}

[[nodiscard]] bool lireVitesse(const nlohmann::json& objet, CreatureSpeed& sortie,
                               std::string& raison) {
    const auto trouve = objet.find("speed");
    if (trouve == objet.end() || !trouve->is_object()) {
        raison = "champ 'speed' absent ou non objet";
        return false;
    }
    const std::optional<float> marche = lireReelFacultatif(*trouve, "walk");
    if (!marche.has_value()) {
        // Une creature sans vitesse de marche porte `walk: 0` -- l'epaulard, le requin geant. Son
        // ABSENCE est donc une extraction incomplete, pas une creature immobile.
        raison =
            "vitesse 'walk' absente ; une creature immobile porte 0, elle n'omet pas le "
            "champ";
        return false;
    }
    sortie.walk = *marche;
    sortie.fly = lireReelFacultatif(*trouve, "fly");
    sortie.swim = lireReelFacultatif(*trouve, "swim");
    sortie.climb = lireReelFacultatif(*trouve, "climb");
    sortie.burrow = lireReelFacultatif(*trouve, "burrow");
    return true;
}

void lireNommes(const nlohmann::json& objet, const char* champ,
                std::vector<CreatureTrait>& sortie) {
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_array()) {
        return;
    }
    for (const auto& element : *trouve) {
        if (element.is_object()) {
            sortie.push_back({.name = lireTexteFacultatif(element, "name"),
                              .text = lireTexteFacultatif(element, "text")});
        }
    }
}

void lireActions(const nlohmann::json& objet, Creature& creature, const std::string& fichier,
                 std::vector<std::string>& erreurs) {
    const auto trouve = objet.find("actions");
    if (trouve == objet.end() || !trouve->is_array()) {
        return;
    }
    for (const auto& element : *trouve) {
        if (!element.is_object()) {
            continue;
        }
        CreatureAction action;
        action.name = lireTexteFacultatif(element, "name");
        action.text = lireTexteFacultatif(element, "text");
        if (const auto bonus = element.find("attackBonus");
            bonus != element.end() && bonus->is_number_integer()) {
            action.attackBonus = bonus->get<int>();
        }
        action.reach = lireReelFacultatif(element, "reach");
        action.rangeNormal = lireReelFacultatif(element, "rangeNormal");
        action.rangeLong = lireReelFacultatif(element, "rangeLong");
        action.damage = lireDesFacultatifs(element, "damage");
        if (element.contains("damage") && !action.damage.has_value()) {
            erreurs.push_back(fichier + " : action '" + action.name +
                              "' porte une notation de des illisible. Une expression devinee "
                              "ferait lancer autre chose que ce que le livre ecrit.");
        }
        const std::string type = lireTexteFacultatif(element, "damageType");
        if (!type.empty()) {
            action.damageType = parseDamageType(type);
            if (!action.damageType.has_value()) {
                std::string message = fichier;
                message += " : type de degats '";
                message += type;
                message += "' inconnu du moteur.";
                erreurs.push_back(std::move(message));
            }
        }
        creature.actions.push_back(std::move(action));
    }
}

[[nodiscard]] bool lireCreature(const nlohmann::json& racine, const std::string& fichier,
                                Creature& creature, std::vector<std::string>& erreurs) {
    std::string raison;
    std::string taille;
    const bool obligatoires = lireTexte(racine, "id", creature.id, raison) &&
                              lireTexte(racine, "name", creature.name, raison) &&
                              lireTexte(racine, "source", creature.source, raison) &&
                              lireTexte(racine, "size", taille, raison) &&
                              lireTexte(racine, "creatureType", creature.creatureType, raison) &&
                              lireEntier(racine, "armorClass", creature.armorClass, raison) &&
                              lireEntier(racine, "hitPoints", creature.hitPoints, raison) &&
                              lireVitesse(racine, creature.speed, raison) &&
                              lireCaracteristiques(racine, creature.abilities, raison);
    if (!obligatoires) {
        erreurs.push_back(fichier + " : " + raison + ".");
        return false;
    }

    const std::optional<CreatureSize> lue = parseCreatureSize(taille);
    if (!lue.has_value()) {
        erreurs.push_back(fichier + " : taille '" + taille + "' inconnue du moteur.");
        return false;
    }
    creature.size = *lue;

    const std::optional<float> puissance = lireReelFacultatif(racine, "challengeRating");
    if (!puissance.has_value()) {
        erreurs.push_back(fichier + " : champ 'challengeRating' absent ou non numerique.");
        return false;
    }
    creature.challengeRating = *puissance;

    creature.alignment = lireTexteFacultatif(racine, "alignment");
    creature.description = lireTexteFacultatif(racine, "description");
    creature.hitDice = lireDesFacultatifs(racine, "hitDice");
    creature.senses = lireTextes(racine, "senses");
    creature.languages = lireTextes(racine, "languages");
    creature.requiredMechanisms = lireTextes(racine, "mecanismesRequis");
    creature.silhouette = lireTexteFacultatif(racine, "silhouette");

    if (const auto competences = racine.find("skills");
        competences != racine.end() && competences->is_object()) {
        for (const auto& [nom, bonus] : competences->items()) {
            if (bonus.is_number_integer()) {
                creature.skills.emplace(nom, bonus.get<int>());
            }
        }
    }

    lireEnumerations<DamageType>(racine, "damageResistances", parseDamageType,
                                 creature.damageResistances, fichier, erreurs);
    lireEnumerations<DamageType>(racine, "damageImmunities", parseDamageType,
                                 creature.damageImmunities, fichier, erreurs);
    lireEnumerations<DamageType>(racine, "damageVulnerabilities", parseDamageType,
                                 creature.damageVulnerabilities, fichier, erreurs);
    lireEnumerations<Condition>(racine, "conditionImmunities", parseCondition,
                                creature.conditionImmunities, fichier, erreurs);
    lireNommes(racine, "traits", creature.traits);
    lireActions(racine, creature, fichier, erreurs);
    return true;
}

}  // namespace

const CreatureAction* Creature::action(std::string_view actionName) const {
    const auto trouve = std::ranges::find(actions, actionName, &CreatureAction::name);
    return trouve == actions.end() ? nullptr : &*trouve;
}

const Creature* Bestiary::find(std::string_view id) const {
    const auto trouve = std::ranges::find(creatures, id, &Creature::id);
    return trouve == creatures.end() ? nullptr : &*trouve;
}

std::vector<std::string> Bestiary::requiredMechanisms() const {
    std::set<std::string> uniques;
    for (const Creature& creature : creatures) {
        uniques.insert(creature.requiredMechanisms.begin(), creature.requiredMechanisms.end());
    }
    return {uniques.begin(), uniques.end()};
}

Bestiary loadBestiary(const std::filesystem::path& directory) {
    Bestiary bestiaire;

    std::error_code code;
    if (!std::filesystem::is_directory(directory, code)) {
        // Un dossier absent n'est PAS un bestiaire vide : les deux se ressemblent a l'execution,
        // et confondre << pas installe >> avec << aucune creature >> fait chercher le defaut du
        // mauvais cote pendant longtemps.
        bestiaire.errors.push_back(directory.string() +
                                   " : dossier de creatures absent ou illisible.");
        return bestiaire;
    }

    // Le dossier est BALAYE. Une liste de quatre-vingt-quatorze noms ecrite ici serait une seconde
    // source de verite, et la premiere creature du LOT-46 en sortirait invisible.
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(directory, code)) {
        if (entree.is_regular_file(code) && entree.path().extension() == ".json") {
            fichiers.push_back(entree.path());
        }
    }
    std::ranges::sort(fichiers);

    for (const std::filesystem::path& chemin : fichiers) {
        const std::string nom = chemin.filename().string();
        const JsonDocument document = readJsonObjectFromFile(chemin, SANS_GARDE_DE_VERSION);
        if (!document.ok()) {
            // Le message porte deja le fichier et la ligne (EX-CNT-010).
            bestiaire.errors.push_back(document.message);
            continue;
        }
        Creature creature;
        if (lireCreature(document.root, nom, creature, bestiaire.errors)) {
            bestiaire.creatures.push_back(std::move(creature));
        }
    }
    std::ranges::sort(bestiaire.creatures, {}, &Creature::id);
    return bestiaire;
}

}  // namespace core
