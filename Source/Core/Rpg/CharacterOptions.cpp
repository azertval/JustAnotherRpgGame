// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/CharacterOptions.h"

#include <algorithm>
#include <set>
#include <system_error>

#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/RpgEnumNames.h"

namespace core {

namespace {

// Les entrees de catalogue ne portent pas de champ `version` : ce sont des donnees, pas des
// documents de format. `0` desactive la garde de version, comme pour le bestiaire (LOT-33).
constexpr int SANS_GARDE_DE_VERSION = 0;

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
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

void lireTraits(const nlohmann::json& objet, const char* champ, std::vector<NamedTrait>& sortie) {
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_array()) {
        return;
    }
    for (const auto& element : *trouve) {
        if (element.is_object()) {
            sortie.push_back(
                {.name = lireTexte(element, "name"), .text = lireTexte(element, "text")});
        }
    }
}

// Une caracteristique inconnue est SIGNALEE, jamais ignoree : c'est le scenario d'EX-CNT-011, ou
// la donnee nomme ce que le moteur ne connait pas, la valeur tombe dans un cas par defaut, et le
// personnage sort avec un modificateur faux que rien n'annonce.
[[nodiscard]] std::vector<Ability> lireCaracteristiques(const nlohmann::json& objet,
                                                        const char* champ,
                                                        const std::string& fichier,
                                                        std::vector<std::string>& erreurs) {
    std::vector<Ability> valeurs;
    for (const std::string& nom : lireTextes(objet, champ)) {
        const std::optional<Ability> lue = parseAbility(nom);
        if (lue.has_value()) {
            valeurs.push_back(*lue);
        } else {
            std::string message = fichier;
            message += " : ";
            message += champ;
            message += " '";
            message += nom;
            message +=
                "' inconnue du moteur. Une caracteristique ignoree fausse un "
                "modificateur sans qu'aucun message ne le dise.";
            erreurs.push_back(std::move(message));
        }
    }
    return valeurs;
}

void lireStatut(const nlohmann::json& objet, ProvisionalStatus& sortie) {
    const auto trouve = objet.find("status");
    if (trouve == objet.end() || !trouve->is_object()) {
        return;
    }
    const auto provisoire = trouve->find("provisoire");
    sortie.provisional =
        provisoire != trouve->end() && provisoire->is_boolean() && provisoire->get<bool>();
    sortie.reason = lireTexte(*trouve, "raison");
    sortie.removalCriterion = lireTexte(*trouve, "retraitSi");
}

// Balaie un dossier et applique `lecteur` a chaque document. Le dossier est BALAYE, jamais
// enumere dans le code : une liste de noms ecrite en C++ serait une seconde source de verite.
template <typename Lecteur>
void balayer(const std::filesystem::path& dossier, std::vector<std::string>& erreurs,
             Lecteur lecteur) {
    std::error_code code;
    if (!std::filesystem::is_directory(dossier, code)) {
        // Un dossier absent n'est PAS un catalogue vide : les deux se ressemblent a l'execution,
        // et les confondre fait chercher le defaut du mauvais cote pendant longtemps.
        erreurs.push_back(dossier.string() + " : dossier absent ou illisible.");
        return;
    }
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(dossier, code)) {
        if (entree.is_regular_file(code) && entree.path().extension() == ".json") {
            fichiers.push_back(entree.path());
        }
    }
    std::ranges::sort(fichiers);
    for (const std::filesystem::path& chemin : fichiers) {
        const JsonDocument document = readJsonObjectFromFile(chemin, SANS_GARDE_DE_VERSION);
        if (!document.ok()) {
            // Le message porte deja le fichier et la ligne (EX-CNT-010).
            erreurs.push_back(document.message);
            continue;
        }
        lecteur(document.root, chemin.filename().string());
    }
}

}  // namespace

const ClassLevel* PlayableClass::atLevel(int level) const {
    const auto trouve = std::ranges::find(progression, level, &ClassLevel::level);
    return trouve == progression.end() ? nullptr : &*trouve;
}

bool PlayableClass::isProficientWithWeapon(std::string_view weaponId,
                                           std::string_view category) const {
    return std::ranges::any_of(weaponProficiencies, [&](const std::string& maitrise) {
        return maitrise == weaponId || maitrise == category;
    });
}

bool PlayableClass::isProficientWithArmor(std::string_view category) const {
    return std::ranges::find(armorProficiencies, category) != armorProficiencies.end();
}

namespace {

// L'union, dans l'ordre de la table, des listes d'un champ de chaque ligne jusqu'au niveau.
template <typename Champ>
[[nodiscard]] std::vector<std::string> cumulerJusquAu(const std::vector<ClassLevel>& progression,
                                                      int level, Champ champ) {
    std::vector<std::string> cumul;
    for (const ClassLevel& ligne : progression) {
        if (ligne.level > level) {
            continue;
        }
        for (const std::string& identifiant : champ(ligne)) {
            if (std::ranges::find(cumul, identifiant) == cumul.end()) {
                cumul.push_back(identifiant);
            }
        }
    }
    return cumul;
}

}  // namespace

std::vector<std::string> PlayableClass::cantripsAt(int level) const {
    return cumulerJusquAu(
        progression, level,
        [](const ClassLevel& ligne) -> const std::vector<std::string>& { return ligne.cantrips; });
}

std::vector<std::string> PlayableClass::spellsAt(int level) const {
    return cumulerJusquAu(
        progression, level,
        [](const ClassLevel& ligne) -> const std::vector<std::string>& { return ligne.spells; });
}

std::vector<Capacity> resolveCapacities(const PlayableClass& playableClass, int level,
                                        const CapacityCatalog& catalog,
                                        std::vector<std::string>& missing) {
    std::vector<Capacity> actives;
    for (const std::string& identifiant :
         cumulerJusquAu(playableClass.progression, level,
                        [](const ClassLevel& ligne) -> const std::vector<std::string>& {
                            return ligne.features;
                        })) {
        const Capacity* capacite = catalog.find(identifiant);
        if (capacite == nullptr) {
            // La table nomme ce que le catalogue n'a pas : la capacite ne joue pas, et le dire
            // vaut mieux que de la laisser passer pour jouee (EX-CNT-031).
            missing.push_back(identifiant);
            continue;
        }
        // Une capacite qui en REMPLACE une autre la retire : Hit the Mark Improvement ne
        // s'additionne pas a Hit the Mark.
        if (!capacite->replaces.empty()) {
            std::erase_if(actives,
                          [&](const Capacity& active) { return active.id == capacite->replaces; });
        }
        actives.push_back(*capacite);
    }
    return actives;
}

const Species* CharacterOptions::findSpecies(std::string_view id) const {
    const auto trouve = std::ranges::find(species, id, &Species::id);
    return trouve == species.end() ? nullptr : &*trouve;
}

const Background* CharacterOptions::findBackground(std::string_view id) const {
    const auto trouve = std::ranges::find(backgrounds, id, &Background::id);
    return trouve == backgrounds.end() ? nullptr : &*trouve;
}

const PlayableClass* CharacterOptions::findClass(std::string_view id) const {
    const auto trouve = std::ranges::find(classes, id, &PlayableClass::id);
    return trouve == classes.end() ? nullptr : &*trouve;
}

std::vector<std::string> CharacterOptions::requiredMechanisms() const {
    std::set<std::string> uniques;
    for (const Species& espece : species) {
        uniques.insert(espece.requiredMechanisms.begin(), espece.requiredMechanisms.end());
    }
    return {uniques.begin(), uniques.end()};
}

std::vector<std::string> CharacterOptions::provisionalClassIds() const {
    std::vector<std::string> identifiants;
    for (const PlayableClass& classe : classes) {
        if (classe.status.provisional) {
            identifiants.push_back(classe.id);
        }
    }
    return identifiants;
}

int abilityScoreWith(const Species& species, Ability which, int baseScore, int maximumScore) {
    return std::min(baseScore + species.increase(which), maximumScore);
}

namespace {

// Augmentations de caracteristiques d'une espece ; une entree inconnue est signalee et ignoree.
void lireAugmentations(const nlohmann::json& racine, const std::string& fichier, Species& espece,
                       std::vector<std::string>& erreurs) {
    const auto increases = racine.find("abilityScoreIncrease");
    if (increases == racine.end() || !increases->is_object()) {
        return;
    }
    for (const auto& [nom, valeur] : increases->items()) {
        const std::optional<Ability> lue = parseAbility(nom);
        if (!lue.has_value() || !valeur.is_number_integer()) {
            std::string message = fichier;
            message += " : augmentation '";
            message += nom;
            message += "' inconnue du moteur.";
            erreurs.push_back(std::move(message));
            continue;
        }
        espece.abilityScoreIncrease[static_cast<std::size_t>(*lue)] = valeur.get<int>();
    }
}

[[nodiscard]] std::optional<Species> lireEspece(const nlohmann::json& racine,
                                                const std::string& fichier,
                                                std::vector<std::string>& erreurs) {
    Species espece;
    espece.id = lireTexte(racine, "id");
    espece.name = lireTexte(racine, "name");
    espece.source = lireTexte(racine, "source");
    espece.parentSpecies = lireTexte(racine, "parentSpecies");
    const std::optional<CreatureSize> taille = parseCreatureSize(lireTexte(racine, "size"));
    if (!taille.has_value()) {
        erreurs.push_back(fichier + " : taille inconnue du moteur.");
        return std::nullopt;
    }
    espece.size = *taille;
    const auto vitesse = racine.find("speed");
    if (vitesse == racine.end() || !vitesse->is_number()) {
        erreurs.push_back(fichier + " : champ 'speed' absent ou non numerique.");
        return std::nullopt;
    }
    espece.speed = vitesse->get<float>();
    lireAugmentations(racine, fichier, espece, erreurs);
    if (const auto parNiveau = racine.find("hitPointsPerLevel");
        parNiveau != racine.end() && parNiveau->is_number_integer()) {
        espece.hitPointsPerLevel = parNiveau->get<int>();
    }
    espece.languages = lireTextes(racine, "languages");
    espece.weaponProficiencies = lireTextes(racine, "weaponProficiencies");
    espece.requiredMechanisms = lireTextes(racine, "mecanismesRequis");
    lireTraits(racine, "traits", espece.traits);
    return espece;
}

[[nodiscard]] Background lireHistorique(const nlohmann::json& racine) {
    Background historique;
    historique.id = lireTexte(racine, "id");
    historique.name = lireTexte(racine, "name");
    historique.source = lireTexte(racine, "source");
    historique.skillProficiencies = lireTextes(racine, "skillProficiencies");
    historique.text = lireTexte(racine, "text");
    if (const auto langues = racine.find("languageCount");
        langues != racine.end() && langues->is_number_integer()) {
        historique.languageCount = langues->get<int>();
    }
    if (const auto capacite = racine.find("feature");
        capacite != racine.end() && capacite->is_object()) {
        historique.feature =
            NamedTrait{.name = lireTexte(*capacite, "name"), .text = lireTexte(*capacite, "text")};
    }
    return historique;
}

// Progression d'une classe, triee par niveau ; une ligne incomplete est signalee et ignoree.
void lireProgression(const nlohmann::json& racine, const std::string& fichier,
                     PlayableClass& classe, std::vector<std::string>& erreurs) {
    if (const auto progression = racine.find("progression");
        progression != racine.end() && progression->is_array()) {
        for (const auto& element : *progression) {
            if (!element.is_object()) {
                continue;
            }
            ClassLevel niveau;
            const auto valeur = element.find("level");
            const auto bonus = element.find("proficiencyBonus");
            if (valeur == element.end() || !valeur->is_number_integer() || bonus == element.end() ||
                !bonus->is_number_integer()) {
                erreurs.push_back(fichier +
                                  " : ligne de progression sans niveau ni "
                                  "bonus de maitrise.");
                continue;
            }
            niveau.level = valeur->get<int>();
            niveau.proficiencyBonus = bonus->get<int>();
            niveau.features = lireTextes(element, "features");
            niveau.cantrips = lireTextes(element, "cantrips");
            niveau.spells = lireTextes(element, "spells");
            classe.progression.push_back(std::move(niveau));
        }
    }
    std::ranges::sort(classe.progression, {}, &ClassLevel::level);
}

[[nodiscard]] std::optional<PlayableClass> lireClasse(const nlohmann::json& racine,
                                                      const std::string& fichier,
                                                      std::vector<std::string>& erreurs) {
    PlayableClass classe;
    classe.id = lireTexte(racine, "id");
    classe.name = lireTexte(racine, "name");
    classe.source = lireTexte(racine, "source");
    const auto de = racine.find("hitDie");
    if (de == racine.end() || !de->is_number_integer()) {
        erreurs.push_back(fichier +
                          " : champ 'hitDie' absent ou non entier. Le de de "
                          "vie decide des points de vie a chaque niveau.");
        return std::nullopt;
    }
    classe.hitDie = de->get<int>();
    classe.primaryAbility = lireCaracteristiques(racine, "primaryAbility", fichier, erreurs);
    classe.savingThrowProficiencies =
        lireCaracteristiques(racine, "savingThrowProficiencies", fichier, erreurs);
    classe.armorProficiencies = lireTextes(racine, "armorProficiencies");
    classe.weaponProficiencies = lireTextes(racine, "weaponProficiencies");
    if (const auto choix = racine.find("skillChoices");
        choix != racine.end() && choix->is_object()) {
        if (const auto nombre = choix->find("count");
            nombre != choix->end() && nombre->is_number_integer()) {
            classe.skillChoices.count = nombre->get<int>();
        }
        classe.skillChoices.from = lireTextes(*choix, "from");
    }
    if (const auto incantation = racine.find("spellcasting");
        incantation != racine.end() && incantation->is_object()) {
        // La caracteristique d'incantation est EXIGEE : sans elle, ni DD ni jet d'attaque de sort
        // ne se calculent, et une classe qui lancerait des sorts a +0 passerait pour faible.
        const std::optional<Ability> caracteristique =
            parseAbility(lireTexte(*incantation, "ability"));
        const auto lancers = incantation->find("castsPerDay");
        if (!caracteristique.has_value() || lancers == incantation->end() ||
            !lancers->is_number_integer()) {
            erreurs.push_back(fichier +
                              " : 'spellcasting' sans caracteristique connue ou sans "
                              "'castsPerDay'. L'incantation simplifiee exige les deux.");
            return std::nullopt;
        }
        classe.spellcasting =
            Spellcasting{.ability = *caracteristique, .castsPerDay = lancers->get<int>()};
    }
    lireStatut(racine, classe.status);
    lireProgression(racine, fichier, classe, erreurs);
    return classe;
}

}  // namespace

namespace {

// Une sous-espece HERITE de son espece parente (LOT-130) : << les nains des collines >> sont des
// nains, et la page 203 donne au pretre nain les +2 de Constitution, le nain commun et la vision
// dans le noir de tous les nains, plus le +1 de Sagesse et la Tenacite qui sont les siens. Le
// fichier de la sous-espece ne porte que ce qu'elle AJOUTE, comme le livre l'ecrit ; la fusion se
// fait ici, une fois, et sur un seul niveau -- le corpus n'a pas de petite-fille d'espece. La
// taille et la vitesse restent celles que la sous-espece declare (le schema les exige).
void heriterDesEspecesParentes(CharacterOptions& options) {
    // Les parents sont copies d'abord : fusionner en place pendant qu'on lit la liste ferait
    // heriter d'un parent lui-meme deja modifie.
    std::vector<Species> parents;
    for (const Species& espece : options.species) {
        if (espece.parentSpecies.empty()) {
            parents.push_back(espece);
        }
    }
    for (Species& espece : options.species) {
        if (espece.parentSpecies.empty()) {
            continue;
        }
        const auto parent = std::ranges::find(parents, espece.parentSpecies, &Species::id);
        if (parent == parents.end()) {
            options.errors.push_back("species/" + espece.id + ".json : espece parente '" +
                                     espece.parentSpecies + "' inconnue du catalogue.");
            continue;
        }
        for (const Ability caracteristique : allAbilities()) {
            const auto indice = static_cast<std::size_t>(caracteristique);
            espece.abilityScoreIncrease[indice] += parent->abilityScoreIncrease[indice];
        }
        espece.hitPointsPerLevel += parent->hitPointsPerLevel;
        for (const std::string& langue : parent->languages) {
            if (std::ranges::find(espece.languages, langue) == espece.languages.end()) {
                espece.languages.push_back(langue);
            }
        }
        for (const std::string& mecanisme : parent->requiredMechanisms) {
            if (std::ranges::find(espece.requiredMechanisms, mecanisme) ==
                espece.requiredMechanisms.end()) {
                espece.requiredMechanisms.push_back(mecanisme);
            }
        }
        // Les armes du parent aussi (LOT-131) : le nain des collines a l'Entrainement aux armes
        // naines, et la page 203 donne au pretre nain son marteau de guerre a +3.
        for (const std::string& arme : parent->weaponProficiencies) {
            if (std::ranges::find(espece.weaponProficiencies, arme) ==
                espece.weaponProficiencies.end()) {
                espece.weaponProficiencies.push_back(arme);
            }
        }
        // Les traits du parent d'abord, dans l'ordre du livre : l'espece, puis la sous-espece.
        espece.traits.insert(espece.traits.begin(), parent->traits.begin(), parent->traits.end());
    }
}

}  // namespace

CharacterOptions loadCharacterOptions(const std::filesystem::path& speciesDir,
                                      const std::filesystem::path& backgroundsDir,
                                      const std::filesystem::path& classesDir) {
    CharacterOptions options;

    balayer(speciesDir, options.errors,
            [&options](const nlohmann::json& racine, const std::string& fichier) {
                if (std::optional<Species> espece = lireEspece(racine, fichier, options.errors)) {
                    options.species.push_back(std::move(*espece));
                }
            });

    balayer(backgroundsDir, options.errors,
            [&options](const nlohmann::json& racine, const std::string&) {
                options.backgrounds.push_back(lireHistorique(racine));
            });

    balayer(
        classesDir, options.errors,
        [&options](const nlohmann::json& racine, const std::string& fichier) {
            if (std::optional<PlayableClass> classe = lireClasse(racine, fichier, options.errors)) {
                options.classes.push_back(std::move(*classe));
            }
        });

    std::ranges::sort(options.species, {}, &Species::id);
    std::ranges::sort(options.backgrounds, {}, &Background::id);
    std::ranges::sort(options.classes, {}, &PlayableClass::id);
    heriterDesEspecesParentes(options);
    return options;
}

CharacterOptions loadCharacterOptions(const std::filesystem::path& speciesDir,
                                      const std::filesystem::path& backgroundsDir,
                                      const std::filesystem::path& classesDir,
                                      const std::filesystem::path& capacitiesDir,
                                      const std::filesystem::path& spellsDir) {
    CharacterOptions options = loadCharacterOptions(speciesDir, backgroundsDir, classesDir);
    options.capacities = loadCapacities(capacitiesDir);
    options.spells = loadSpells(spellsDir);
    // Les erreurs voyagent AVEC les donnees : une seule liste a lire pour qui charge (EX-CNT-010).
    options.errors.insert(options.errors.end(), options.capacities.errors.begin(),
                          options.capacities.errors.end());
    options.errors.insert(options.errors.end(), options.spells.errors.begin(),
                          options.spells.errors.end());
    return options;
}

CharacterOptions loadCharacterOptions(const std::filesystem::path& rpgRoot) {
    return loadCharacterOptions(rpgRoot / "species", rpgRoot / "backgrounds", rpgRoot / "classes",
                                rpgRoot / "capacities", rpgRoot / "spells");
}

}  // namespace core
