// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/CharacterSheet.h"

#include <algorithm>
#include <utility>

#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/Equipment.h"
#include "Core/Rpg/Scale.h"

namespace core {

namespace {

// La table d'experience ne porte pas de champ `version` : c'est une donnee de regle, pas un
// document de format.
constexpr int SANS_GARDE_DE_VERSION = 0;

// Le gain de points de vie d'un niveau ne descend jamais sous 1. Une Constitution desastreuse fait
// gagner peu de points de vie ; elle n'en fait pas perdre.
constexpr int GAIN_MINIMAL_PAR_NIVEAU = 1;

}  // namespace

CharacterCreationRules loadCharacterCreationRules(const std::filesystem::path& path) {
    CharacterCreationRules regles;
    const JsonDocument document = readJsonObjectFromFile(path, SANS_GARDE_DE_VERSION);
    if (!document.ok()) {
        regles.errors.push_back(document.message);
        return regles;
    }
    for (const auto& [champ, sortie] :
         {std::pair{"unarmoredArmorClass", &regles.unarmoredArmorClass},
          std::pair{"maximumAbilityScore", &regles.maximumAbilityScore}}) {
        const auto trouve = document.root.find(champ);
        if (trouve == document.root.end() || !trouve->is_number_integer()) {
            // Une regle absente ne se devine pas : lui donner une valeur par defaut la ferait
            // passer pour une regle du jeu, alors que ce serait une valeur inventee par le
            // chargeur.
            regles.errors.push_back(path.string() + " : champ '" + champ +
                                    "' absent ou non entier.");
            continue;
        }
        *sortie = trouve->get<int>();
    }
    return regles;
}

// -- Table d'experience -----------------------------------------------------------------------

int ExperienceTable::levelFor(int experiencePoints) const {
    int atteint = 0;
    for (const ExperienceLevel& ligne : levels) {
        if (experiencePoints >= ligne.experience) {
            atteint = std::max(atteint, ligne.level);
        }
    }
    return atteint;
}

int ExperienceTable::proficiencyBonusAt(int level) const {
    const auto trouve = std::ranges::find(levels, level, &ExperienceLevel::level);
    return trouve == levels.end() ? 0 : trouve->proficiencyBonus;
}

int ExperienceTable::thresholdAt(int level) const {
    const auto trouve = std::ranges::find(levels, level, &ExperienceLevel::level);
    return trouve == levels.end() ? 0 : trouve->experience;
}

int ExperienceTable::maximumLevel() const {
    int maximum = 0;
    for (const ExperienceLevel& ligne : levels) {
        maximum = std::max(maximum, ligne.level);
    }
    return maximum;
}

ExperienceTable loadExperienceTable(const std::filesystem::path& path) {
    ExperienceTable table;
    const JsonDocument document = readJsonObjectFromFile(path, SANS_GARDE_DE_VERSION);
    if (!document.ok()) {
        // Le message porte deja le fichier et la ligne (EX-CNT-010).
        table.errors.push_back(document.message);
        return table;
    }
    const auto niveaux = document.root.find("levels");
    if (niveaux == document.root.end() || !niveaux->is_array()) {
        table.errors.push_back(path.string() + " : champ 'levels' absent ou non tableau.");
        return table;
    }
    for (const auto& element : *niveaux) {
        if (!element.is_object()) {
            continue;
        }
        const auto niveau = element.find("level");
        const auto px = element.find("experience");
        const auto bonus = element.find("proficiencyBonus");
        if (niveau == element.end() || !niveau->is_number_integer() || px == element.end() ||
            !px->is_number_integer() || bonus == element.end() || !bonus->is_number_integer()) {
            table.errors.push_back(path.string() + " : ligne de table incomplete.");
            continue;
        }
        table.levels.push_back({.level = niveau->get<int>(),
                                .experience = px->get<int>(),
                                .proficiencyBonus = bonus->get<int>()});
    }
    std::ranges::sort(table.levels, {}, &ExperienceLevel::level);
    return table;
}

namespace {

// Les six valeurs de BASE, avant augmentation d'espece. Les six sont exigees : une fiche a
// cinq caracteristiques n'existe pas, et laisser la sixieme a zero donnerait un modificateur
// de -5 que rien ne signalerait.
[[nodiscard]] std::array<int, 6> lireCaracteristiquesDeBase(const nlohmann::json& racine,
                                                            const std::filesystem::path& path,
                                                            std::vector<std::string>& erreurs) {
    std::array<int, 6> base{};
    const auto caracteristiques = racine.find("baseAbilities");
    if (caracteristiques == racine.end() || !caracteristiques->is_object()) {
        erreurs.push_back(path.string() + " : champ 'baseAbilities' absent ou non objet.");
        return base;
    }
    for (const Ability caracteristique : allAbilities()) {
        const std::string nom{abilityName(caracteristique)};
        const auto valeur = caracteristiques->find(nom);
        if (valeur == caracteristiques->end() || !valeur->is_number_integer()) {
            erreurs.push_back(path.string() + " : caracteristique '" + nom + "' absente.");
            continue;
        }
        base[static_cast<std::size_t>(caracteristique)] = valeur->get<int>();
    }
    return base;
}

// Les augmentations que l'espece laisse AU CHOIX du joueur (LOT-130), telles que la fiche les a
// tranchees : un objet caracteristique -> entier. Absent, rien n'est ajoute ; une caracteristique
// inconnue est signalee et ignoree, jamais devinee.
[[nodiscard]] std::array<int, 6> lireChoixDEspece(const nlohmann::json& racine,
                                                  const std::filesystem::path& path,
                                                  std::vector<std::string>& erreurs) {
    std::array<int, 6> choix{};
    const auto objet = racine.find("speciesAbilityChoice");
    if (objet == racine.end() || !objet->is_object()) {
        return choix;
    }
    for (const auto& [nom, valeur] : objet->items()) {
        const std::optional<Ability> lue = parseAbility(nom);
        if (!lue.has_value() || !valeur.is_number_integer()) {
            erreurs.push_back(path.string() + " : choix d'espece '" + nom + "' inconnu du moteur.");
            continue;
        }
        choix[static_cast<std::size_t>(*lue)] = valeur.get<int>();
    }
    return choix;
}

// Les chaines du tableau @p champ de @p racine, ajoutees a @p sortie ; le reste est ignore.
void lireEnsembleDeTextes(const nlohmann::json& racine, const char* champ,
                          std::set<std::string>& sortie) {
    const auto tableau = racine.find(champ);
    if (tableau == racine.end() || !tableau->is_array()) {
        return;
    }
    for (const auto& element : *tableau) {
        if (element.is_string()) {
            sortie.insert(element.get<std::string>());
        }
    }
}

// Les objets portes, par emplacement ; un emplacement inconnu est signale et ignore.
void lireEquipes(const nlohmann::json& inventaire, const std::filesystem::path& path,
                 LoadedCharacterSheet& resultat) {
    const auto portes = inventaire.find("equipped");
    if (portes == inventaire.end() || !portes->is_object()) {
        return;
    }
    for (const auto& [nom, valeur] : portes->items()) {
        const std::optional<EquipmentSlot> emplacement = parseEquipmentSlot(nom);
        if (!emplacement.has_value()) {
            resultat.errors.push_back(path.string() + " : emplacement d'equipement '" + nom +
                                      "' inconnu du moteur.");
            continue;
        }
        if (valeur.is_string()) {
            static_cast<void>(equip(resultat.inventory, *emplacement, valeur.get<std::string>()));
        }
    }
}

// Le contenu du sac ; une ligne sans identifiant est ignoree, une quantite absente vaut 1.
void lireSac(const nlohmann::json& inventaire, Inventory& sortie) {
    const auto sac = inventaire.find("backpack");
    if (sac == inventaire.end() || !sac->is_array()) {
        return;
    }
    for (const auto& ligne : *sac) {
        if (!ligne.is_object()) {
            continue;
        }
        const auto identifiant = ligne.find("itemId");
        if (identifiant == ligne.end() || !identifiant->is_string()) {
            continue;
        }
        const auto quantite = ligne.find("quantity");
        addToBackpack(
            sortie, identifiant->get<std::string>(),
            (quantite != ligne.end() && quantite->is_number_integer()) ? quantite->get<int>() : 1);
    }
}

// Ce que le personnage PORTE (LOT-14). Rien n'en est derive ici : ni classe d'armure, ni poids
// total, ni encombrement. `core::derivedStatsFor` les recalcule depuis ce contenu a chaque
// lecture, et c'est ce qui les empeche de deriver.
void lireInventaire(const nlohmann::json& racine, const std::filesystem::path& path,
                    LoadedCharacterSheet& resultat) {
    const auto inventaire = racine.find("inventory");
    if (inventaire == racine.end() || !inventaire->is_object()) {
        return;
    }
    lireEquipes(*inventaire, path, resultat);
    lireSac(*inventaire, resultat.inventory);
    if (const auto bourse = inventaire->find("purseCopper");
        bourse != inventaire->end() && bourse->is_number_integer()) {
        resultat.inventory.purseCopper = bourse->get<int>();
    }
}

}  // namespace

LoadedCharacterSheet loadCharacterSheet(const std::filesystem::path& path,
                                        const CharacterOptions& options,
                                        const CharacterCreationRules& rules,
                                        const ExperienceTable& table) {
    LoadedCharacterSheet resultat;
    const JsonDocument document = readJsonObjectFromFile(path, SANS_GARDE_DE_VERSION);
    if (!document.ok()) {
        resultat.errors.push_back(document.message);
        return resultat;
    }

    const auto texte = [&document](const char* champ) {
        const auto trouve = document.root.find(champ);
        return (trouve != document.root.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                                      : std::string{};
    };

    const std::array<int, 6> base =
        lireCaracteristiquesDeBase(document.root, path, resultat.errors);

    // Les trois choix sont resolus DANS LE CATALOGUE, et un identifiant inconnu est signale : une
    // fiche qui reference une espece absente s'afficherait sans vitesse ni augmentation, ce qui
    // ressemble a un personnage faible et non a une donnee fausse.
    const std::string especeId = texte("speciesId");
    const std::string classeId = texte("classId");
    const std::string historiqueId = texte("backgroundId");
    const Species* const espece = options.findSpecies(especeId);
    const PlayableClass* const classe = options.findClass(classeId);
    const Background* const historique = options.findBackground(historiqueId);
    if (espece == nullptr) {
        resultat.errors.push_back(path.string() + " : espece inconnue '" + especeId + "'.");
    }
    if (classe == nullptr) {
        resultat.errors.push_back(path.string() + " : classe inconnue '" + classeId + "'.");
    }
    if (historique == nullptr) {
        resultat.errors.push_back(path.string() + " : historique inconnu '" + historiqueId + "'.");
    }

    // La fiche est CONSTRUITE, jamais recopiee : points de vie, classe d'armure, valeurs finales
    // et seuil d'experience sont derives par la regle (LOT-13). Les ecrire dans le fichier en
    // ferait une seconde source, qui differerait de la premiere au premier ajustement de regle.
    resultat.sheet =
        buildCharacterSheet(texte("name"), base, espece, classe, historique, rules, table,
                            lireChoixDEspece(document.root, path, resultat.errors));

    const auto niveau = document.root.find("level");
    if (niveau != document.root.end() && niveau->is_number_integer() && classe != nullptr) {
        // Monter par l'EXPERIENCE, et non en posant le niveau : c'est le meme chemin que celui
        // qu'une partie empruntera, donc les memes points de vie et le meme bonus de maitrise.
        const int cible = niveau->get<int>();
        const int seuil = table.thresholdAt(cible);
        if (seuil > resultat.sheet.experiencePoints) {
            gainExperience(resultat.sheet, table, classe->hitDie,
                           seuil - resultat.sheet.experiencePoints);
        }
    }

    lireEnsembleDeTextes(document.root, "skillProficiencies", resultat.sheet.skillProficiencies);

    // Les langues CHOISIES (LOT-15) : un historique en accorde un nombre, et la fiche dit
    // lesquelles. Celles de l'espece sont deja la, recopiees par `buildCharacterSheet`.
    lireEnsembleDeTextes(document.root, "languages", resultat.sheet.languages);

    lireInventaire(document.root, path, resultat);

    // Ce que la classe donne AU NIVEAU ATTEINT (LOT-131) : capacites actives, sorts connus. Apres
    // la montee, pour que la table soit lue au bon niveau.
    if (classe != nullptr) {
        std::vector<std::string> manquants;
        applyClassFeatures(resultat.sheet, *classe, options, rules, manquants);
        for (const std::string& manquant : manquants) {
            resultat.warnings.push_back(path.string() + " : la classe '" + classe->id +
                                        "' nomme '" + manquant +
                                        "', que le moteur ne joue pas encore (EX-CNT-031).");
        }
    }

    return resultat;
}

// -- Fiche ---------------------------------------------------------------------------------------

float CharacterSheet::speedInTiles() const {
    return tilesFromMeters(effectiveSpeedMeters());
}

float CharacterSheet::effectiveSpeedMeters() const {
    return speedMeters + speedBonusFrom(capacities);
}

const KnownSpell* CharacterSheet::knownSpell(std::string_view spellId) const {
    const auto trouve = std::ranges::find(knownSpells, spellId, &KnownSpell::spellId);
    return trouve == knownSpells.end() ? nullptr : &*trouve;
}

bool isProficientWith(const CharacterSheet& sheet, const Weapon& weapon) {
    if (sheet.classId.empty()) {
        // Sans classe, rien ne dit ce que la fiche ne maitrise pas.
        return true;
    }
    return sheet.weaponProficiencies.contains(weapon.id) ||
           sheet.weaponProficiencies.contains(weapon.category);
}

void applyClassFeatures(CharacterSheet& sheet, const PlayableClass& playableClass,
                        const CharacterOptions& options, const CharacterCreationRules& rules,
                        std::vector<std::string>& missing) {
    sheet.capacities = resolveCapacities(playableClass, sheet.level, options.capacities, missing);
    // La CA sans armure, RECALCULEE depuis ses sources (EX-CBT-030) : une capacite peut la
    // calculer autrement. L'armure portee la remplace dans `derivedStatsFor`, par le meme chemin.
    sheet.armorClass = armorClassFor(sheet, rules, nullptr, nullptr);

    // Les sorts connus : la table les donne, le catalogue les decrit. Un lancer deja depense d'un
    // sort deja connu est CONSERVE : monter de niveau n'est pas un repos.
    std::vector<KnownSpell> anciens = std::move(sheet.knownSpells);
    sheet.knownSpells.clear();
    const auto connaitre = [&](const std::string& identifiant, bool mineur) {
        const Spell* sort = options.spells.find(identifiant);
        if (sort == nullptr) {
            missing.push_back(identifiant);
            return;
        }
        const int parJour = (!mineur && playableClass.spellcasting.has_value())
                                ? playableClass.spellcasting->castsPerDay
                                : 0;
        KnownSpell connu{
            .spellId = identifiant, .level = sort->level, .perDay = parJour, .remaining = parJour};
        if (const auto ancien = std::ranges::find(anciens, identifiant, &KnownSpell::spellId);
            ancien != anciens.end() && ancien->perDay == parJour) {
            connu.remaining = ancien->remaining;
        }
        sheet.knownSpells.push_back(std::move(connu));
    };
    for (const std::string& identifiant : playableClass.cantripsAt(sheet.level)) {
        connaitre(identifiant, true);
    }
    for (const std::string& identifiant : playableClass.spellsAt(sheet.level)) {
        connaitre(identifiant, false);
    }
}

void longRest(CharacterSheet& sheet) {
    sheet.currentHitPoints = sheet.maximumHitPoints;
    for (KnownSpell& sort : sheet.knownSpells) {
        sort.remaining = sort.perDay;
    }
}

bool spendSpellUse(CharacterSheet& sheet, std::string_view spellId) {
    const auto trouve = std::ranges::find(sheet.knownSpells, spellId, &KnownSpell::spellId);
    if (trouve == sheet.knownSpells.end() || !trouve->available()) {
        return false;
    }
    if (trouve->perDay > 0) {
        --trouve->remaining;
    }
    return true;
}

int proficiencyBonus(const CharacterSheet& sheet, const ExperienceTable& table) {
    return table.proficiencyBonusAt(sheet.level);
}

int savingThrowModifier(const CharacterSheet& sheet, const ExperienceTable& table, Ability which) {
    const int maitrise =
        sheet.savingThrowProficiencies.contains(which) ? proficiencyBonus(sheet, table) : 0;
    return sheet.modifier(which) + maitrise;
}

SkillCheckModifier skillModifier(const CharacterSheet& sheet, const ExperienceTable& table,
                                 const SkillCatalog& catalog, std::string_view skillId) {
    const SkillDefinition* competence = catalog.find(skillId);
    if (competence == nullptr) {
        // Une competence inconnue ne se devine pas : renvoyer un modificateur nu SANS le dire
        // laisserait croire a une maitrise absente plutot qu'a une donnee manquante.
        return {};
    }
    SkillCheckModifier resultat;
    resultat.found = true;
    resultat.proficient = sheet.skillProficiencies.contains(std::string{skillId});
    // Un test maitrise prend la maitrise, et ce qu'une capacite y ajoute (Adventurer's Aptitude,
    // LOT-135).
    resultat.value = sheet.modifier(competence->ability) +
                     (resultat.proficient ? proficiencyBonus(sheet, table) +
                                                proficientCheckBonusFrom(sheet.capacities)
                                          : 0);
    return resultat;
}

int maximumHitPointsFor(int hitDie, int level, int constitutionModifier, int bonusPerLevel) {
    if (hitDie <= 0 || level <= 0) {
        return 0;
    }
    // Niveau 1 : le MAXIMUM du de. La regle du livre, et la raison pour laquelle un magicien de
    // niveau 1 n'a pas 3 points de vie.
    int total = hitDie + constitutionModifier + bonusPerLevel;
    // Niveaux suivants : la valeur fixe de la classe, << la valeur moyenne (arrondie au superieur)
    // du de >> (Basic Rules p. 11) -- soit (de / 2) + 1 pour tout de pair, et tous le sont. Ce que
    // l'espece ajoute par niveau (Tenacite naine) s'ajoute HORS du plancher : le plancher protege
    // d'une Constitution desastreuse, il ne doit pas absorber un bonus.
    const int moyenne = (hitDie / 2) + 1;
    for (int niveau = 2; niveau <= level; ++niveau) {
        total += std::max(moyenne + constitutionModifier, GAIN_MINIMAL_PAR_NIVEAU) + bonusPerLevel;
    }
    return std::max(total, GAIN_MINIMAL_PAR_NIVEAU);
}

LevelUpResult gainExperience(CharacterSheet& sheet, const ExperienceTable& table, int hitDie,
                             int amount) {
    LevelUpResult resultat;
    resultat.previousLevel = sheet.level;
    resultat.newLevel = sheet.level;
    resultat.previousProficiencyBonus = table.proficiencyBonusAt(sheet.level);
    resultat.newProficiencyBonus = resultat.previousProficiencyBonus;
    if (amount <= 0) {
        // Perdre de l'experience n'est pas une regle de ce jeu. L'accepter en silence ferait
        // DESCENDRE un personnage de niveau, et le defaut passerait pour de l'equilibrage.
        return resultat;
    }

    sheet.experiencePoints += amount;
    const int atteint = std::min(table.levelFor(sheet.experiencePoints), table.maximumLevel());
    if (atteint <= sheet.level) {
        return resultat;
    }

    const int avant = sheet.maximumHitPoints;
    sheet.level = atteint;
    sheet.maximumHitPoints = maximumHitPointsFor(
        hitDie, sheet.level, sheet.modifier(Ability::Constitution), sheet.hitPointsPerLevelBonus);
    // Les points de vie COURANTS montent du meme gain, pas jusqu'au maximum : monter de niveau
    // n'est pas un soin, et rendre toute sa vie a un personnage blesse ferait de la montee de
    // niveau une potion gratuite.
    sheet.currentHitPoints += sheet.maximumHitPoints - avant;

    resultat.newLevel = sheet.level;
    resultat.hitPointsGained = sheet.maximumHitPoints - avant;
    resultat.newProficiencyBonus = table.proficiencyBonusAt(sheet.level);
    return resultat;
}

LevelUpResult levelUpTo(CharacterSheet& sheet, int level, const PlayableClass& playableClass,
                        const CharacterOptions& options, const CharacterCreationRules& rules,
                        const ExperienceTable& table, std::vector<std::string>& missing) {
    LevelUpResult resultat;
    resultat.previousLevel = sheet.level;
    resultat.newLevel = sheet.level;
    resultat.previousProficiencyBonus = table.proficiencyBonusAt(sheet.level);
    resultat.newProficiencyBonus = resultat.previousProficiencyBonus;
    const int cible = std::min(level, table.maximumLevel());
    if (cible <= sheet.level) {
        return resultat;
    }
    // Par l'experience du seuil, et non en posant le niveau : les memes points de vie et le meme
    // bonus de maitrise qu'une partie jouee.
    const int seuil = table.thresholdAt(cible);
    if (seuil > sheet.experiencePoints) {
        resultat =
            gainExperience(sheet, table, playableClass.hitDie, seuil - sheet.experiencePoints);
    }
    applyClassFeatures(sheet, playableClass, options, rules, missing);
    return resultat;
}

CharacterSheet buildCharacterSheet(std::string name, const std::array<int, 6>& baseAbilities,
                                   const Species* species, const PlayableClass* playableClass,
                                   const Background* background,
                                   const CharacterCreationRules& rules,
                                   const ExperienceTable& table,
                                   const std::array<int, 6>& chosenIncreases) {
    CharacterSheet fiche;
    fiche.name = std::move(name);
    fiche.abilities = baseAbilities;

    if (species != nullptr) {
        fiche.speciesId = species->id;
        fiche.speedMeters = species->speed;
        fiche.hitPointsPerLevelBonus = species->hitPointsPerLevel;
        fiche.languages.insert(species->languages.begin(), species->languages.end());
        fiche.weaponProficiencies.insert(species->weaponProficiencies.begin(),
                                         species->weaponProficiencies.end());
        for (const Ability caracteristique : allAbilities()) {
            const auto indice = static_cast<std::size_t>(caracteristique);
            // La table de l'espece d'abord, puis ce qu'elle laisse au choix (LOT-130), sous le
            // meme plafond : un choix ne fait pas depasser ce que la table n'aurait pas depasse.
            fiche.abilities[indice] =
                std::min(abilityScoreWith(*species, caracteristique, baseAbilities[indice],
                                          rules.maximumAbilityScore) +
                             chosenIncreases[indice],
                         rules.maximumAbilityScore);
        }
    }

    if (playableClass != nullptr) {
        fiche.classId = playableClass->id;
        for (const Ability caracteristique : playableClass->savingThrowProficiencies) {
            fiche.savingThrowProficiencies.insert(caracteristique);
        }
        // Les maitrises d'armes et d'armures de la classe (LOT-131), reunies a celles de l'espece.
        fiche.weaponProficiencies.insert(playableClass->weaponProficiencies.begin(),
                                         playableClass->weaponProficiencies.end());
        fiche.armorProficiencies.insert(playableClass->armorProficiencies.begin(),
                                        playableClass->armorProficiencies.end());
        fiche.maximumHitPoints = maximumHitPointsFor(playableClass->hitDie, fiche.level,
                                                     fiche.modifier(Ability::Constitution),
                                                     fiche.hitPointsPerLevelBonus);
    }
    fiche.currentHitPoints = fiche.maximumHitPoints;

    if (background != nullptr) {
        fiche.backgroundId = background->id;
        for (const std::string& competence : background->skillProficiencies) {
            fiche.skillProficiencies.insert(competence);
        }
    }

    // La classe d'armure sans armure, LUE DANS LA DONNEE, plus le modificateur de Dexterite.
    // L'armure portee la remplacera au LOT-14, qui livre les armures.
    fiche.armorClass = rules.unarmoredArmorClass + fiche.modifier(Ability::Dexterity);
    fiche.experiencePoints = table.thresholdAt(fiche.level);
    return fiche;
}

}  // namespace core
