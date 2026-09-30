// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/Spell.h"

#include <algorithm>
#include <array>
#include <system_error>
#include <utility>

#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/RpgEnumNames.h"

namespace core {

namespace {

constexpr int SANS_GARDE_DE_VERSION = 0;

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

[[nodiscard]] bool lireBooleen(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return trouve != objet.end() && trouve->is_boolean() && trouve->get<bool>();
}

struct NomDEffet {
    SpellEffectKind genre;
    std::string_view nom;
};

// Les noms sont ceux de `spell.schema.json` : le schema et le moteur disent la meme liste.
constexpr std::array<NomDEffet, 4> EFFETS{{
    {.genre = SpellEffectKind::Fly, .nom = "fly"},
    {.genre = SpellEffectKind::Invisible, .nom = "invisible"},
    {.genre = SpellEffectKind::Bless, .nom = "bless"},
    {.genre = SpellEffectKind::SpiritualWeapon, .nom = "spiritual-weapon"},
}};

[[nodiscard]] std::optional<SpellEffectKind> lireGenreDEffet(std::string_view nom) {
    for (const NomDEffet& entree : EFFETS) {
        if (entree.nom == nom) {
            return entree.genre;
        }
    }
    return std::nullopt;
}

// Le genre d'attaque : au contact, ou a distance par defaut.
[[nodiscard]] bool lireGenreDAttaque(const nlohmann::json& racine, const std::string& fichier,
                                     Spell& sort, std::vector<std::string>& erreurs) {
    const std::string genreDAttaque = lireTexte(racine, "attackKind");
    if (genreDAttaque == "melee") {
        sort.meleeAttack = true;
    } else if (!genreDAttaque.empty() && genreDAttaque != "ranged") {
        erreurs.push_back(fichier + " : 'attackKind' '" + genreDAttaque + "' inconnu du moteur.");
        return false;
    }
    return true;
}

// Les des de soin, s'il y en a : presents, ils doivent se lire.
[[nodiscard]] bool lireSoin(const nlohmann::json& racine, const std::string& fichier, Spell& sort,
                            std::vector<std::string>& erreurs) {
    if (const auto soin = racine.find("healing"); soin != racine.end()) {
        sort.healing = soin->is_string() ? parseDice(soin->get<std::string>()) : std::nullopt;
        if (!sort.healing.has_value()) {
            erreurs.push_back(fichier + " : des de soin illisibles.");
            return false;
        }
    }
    return true;
}

// Le retour a la vie : des points de vie et un delai, tous deux positifs.
[[nodiscard]] bool lireRevenant(const nlohmann::json& racine, const std::string& fichier,
                                Spell& sort, std::vector<std::string>& erreurs) {
    const auto retour = racine.find("revives");
    if (retour == racine.end()) {
        return true;
    }
    SpellRevival revenant;
    const auto pv = retour->is_object() ? retour->find("hitPoints") : retour->end();
    const auto rounds = retour->is_object() ? retour->find("withinRounds") : retour->end();
    if (pv == retour->end() || !pv->is_number_integer() || pv->get<int>() < 1 ||
        rounds == retour->end() || !rounds->is_number_integer() || rounds->get<int>() < 1) {
        erreurs.push_back(fichier + " : 'revives' demande 'hitPoints' et 'withinRounds' positifs.");
        return false;
    }
    revenant.hitPoints = pv->get<int>();
    revenant.withinRounds = rounds->get<int>();
    sort.revives = revenant;
    return true;
}

// Un champ entier facultatif qui, present, doit etre positif.
[[nodiscard]] bool lireEntierPositif(const nlohmann::json& racine, const char* champ,
                                     const std::string& fichier, int& valeur,
                                     std::vector<std::string>& erreurs) {
    if (const auto trouve = racine.find(champ); trouve != racine.end()) {
        if (!trouve->is_number_integer() || trouve->get<int>() < 1) {
            erreurs.push_back(fichier + " : '" + champ + "' doit etre un entier positif.");
            return false;
        }
        valeur = trouve->get<int>();
    }
    return true;
}

// L'effet d'une sauvegarde reussie : moitie des degats, ou aucun par defaut.
[[nodiscard]] bool lireEffetDeSauvegarde(const nlohmann::json& racine, const std::string& fichier,
                                         Spell& sort, std::vector<std::string>& erreurs) {
    const std::string sauvegarde = lireTexte(racine, "saveEffect");
    if (sauvegarde == "half") {
        sort.saveEffect = SaveEffect::Half;
    } else if (!sauvegarde.empty() && sauvegarde != "negates") {
        erreurs.push_back(fichier + " : 'saveEffect' '" + sauvegarde + "' inconnu du moteur.");
        return false;
    }
    return true;
}

// La cible : un allie, soi-meme, ou un ennemi par defaut.
[[nodiscard]] bool lireCible(const nlohmann::json& racine, const std::string& fichier, Spell& sort,
                             std::vector<std::string>& erreurs) {
    const std::string cible = lireTexte(racine, "target");
    if (cible == "ally") {
        sort.target = SpellTarget::Ally;
    } else if (cible == "self") {
        sort.target = SpellTarget::Self;
    } else if (!cible.empty() && cible != "enemy") {
        erreurs.push_back(fichier + " : cible '" + cible + "' inconnue du moteur.");
        return false;
    }
    return true;
}

// La zone : un rayon positif, et une forme que le moteur pose ou non.
[[nodiscard]] bool lireZone(const nlohmann::json& racine, const std::string& fichier, Spell& sort,
                            std::vector<std::string>& erreurs) {
    const auto zone = racine.find("area");
    if (zone == racine.end() || !zone->is_object()) {
        return true;
    }
    const std::string forme = lireTexte(*zone, "shape");
    const auto metres = zone->find("meters");
    if (metres == zone->end() || !metres->is_number() || metres->get<float>() <= 0.0F) {
        erreurs.push_back(fichier + " : zone sans 'meters' positifs.");
        return false;
    }
    if (forme == "sphere") {
        sort.areaRadiusMeters = metres->get<float>();
    } else {
        // Une forme connue du Manuel mais que le moteur ne pose pas encore : le sort se
        // charge, et `spellMechanism` dit qu'il ne se joue pas.
        sort.unsupportedArea = forme;
    }
    return true;
}

// L'effet qui dure : son genre, et les champs que ce genre exige.
[[nodiscard]] bool lireEffetPose(const nlohmann::json& racine, const std::string& fichier,
                                 Spell& sort, std::vector<std::string>& erreurs) {
    const auto effet = racine.find("effect");
    if (effet == racine.end() || !effet->is_object()) {
        return true;
    }
    const std::string genre = lireTexte(*effet, "kind");
    const std::optional<SpellEffectKind> lu = lireGenreDEffet(genre);
    if (!lu.has_value()) {
        erreurs.push_back(fichier + " : effet de sort '" + genre + "' inconnu du moteur.");
        return false;
    }
    SpellEffect pose{.kind = *lu, .meters = 0.0F, .dice = std::nullopt, .durationRounds = 0};
    if (const auto metres = effet->find("meters"); metres != effet->end() && metres->is_number()) {
        pose.meters = metres->get<float>();
    }
    if (pose.kind == SpellEffectKind::Fly && pose.meters <= 0.0F) {
        erreurs.push_back(fichier + " : effet 'fly' sans vitesse ('meters').");
        return false;
    }
    if (const auto des = effet->find("dice"); des != effet->end() && des->is_string()) {
        pose.dice = parseDice(des->get<std::string>());
    }
    if (pose.kind == SpellEffectKind::Bless && !pose.dice.has_value()) {
        erreurs.push_back(fichier + " : effet 'bless' sans des lisibles ('dice').");
        return false;
    }
    if (const auto duree = effet->find("durationRounds");
        duree != effet->end() && duree->is_number_integer()) {
        pose.durationRounds = std::max(0, duree->get<int>());
    }
    sort.effect = pose;
    return true;
}

// Les champs du LOT-133 : projectiles, sans jet, sauvegarde, zone, cible, effet qui dure. Une
// valeur que le moteur ne sait pas lire refuse le sort : joue a moitie, il tromperait plus
// qu'absent et nomme dans les erreurs.
[[nodiscard]] bool lireMecanismes(const nlohmann::json& racine, const std::string& fichier,
                                  Spell& sort, std::vector<std::string>& erreurs) {
    sort.autoHit = lireBooleen(racine, "autoHit");
    sort.cantripScaling = lireBooleen(racine, "cantripScaling");
    sort.addsAbilityModifier = lireBooleen(racine, "addsAbilityModifier");
    sort.bonusAction = lireBooleen(racine, "bonusAction");
    if (!lireGenreDAttaque(racine, fichier, sort, erreurs) ||
        !lireSoin(racine, fichier, sort, erreurs)) {
        return false;
    }
    sort.stabilizes = lireBooleen(racine, "stabilizes");
    return lireRevenant(racine, fichier, sort, erreurs) &&
           lireEntierPositif(racine, "maxTargets", fichier, sort.maxTargets, erreurs) &&
           lireEntierPositif(racine, "projectiles", fichier, sort.projectiles, erreurs) &&
           lireEffetDeSauvegarde(racine, fichier, sort, erreurs) &&
           lireCible(racine, fichier, sort, erreurs) && lireZone(racine, fichier, sort, erreurs) &&
           lireEffetPose(racine, fichier, sort, erreurs);
}

// Les composantes, en lettres separees par des virgules : « V, S, M ».
[[nodiscard]] std::string lireComposantes(const nlohmann::json& composantes) {
    std::string lettres;
    for (const auto& [cle, lettre] :
         {std::pair{"verbal", "V"}, std::pair{"somatic", "S"}, std::pair{"material", "M"}}) {
        if (lireBooleen(composantes, cle)) {
            lettres += (lettres.empty() ? "" : ", ") + std::string{lettre};
        }
    }
    return lettres;
}

// Les degats, leur type et le jet de sauvegarde : presents, ils doivent se lire.
[[nodiscard]] bool lireDegats(const nlohmann::json& racine, const std::string& fichier, Spell& sort,
                              std::vector<std::string>& erreurs) {
    if (const auto des = racine.find("damage"); des != racine.end() && des->is_string()) {
        sort.damage = parseDice(des->get<std::string>());
        if (!sort.damage.has_value()) {
            erreurs.push_back(fichier + " : des de degats '" + des->get<std::string>() +
                              "' illisibles.");
            return false;
        }
    }
    if (const auto type = racine.find("damageType"); type != racine.end() && type->is_string()) {
        sort.damageType = parseDamageType(type->get<std::string>());
        if (!sort.damageType.has_value()) {
            // Un type inconnu ne recoit pas un type par defaut (EX-CBT-032).
            erreurs.push_back(fichier + " : type de degats '" + type->get<std::string>() +
                              "' inconnu du moteur.");
            return false;
        }
    }
    if (const auto sauvegarde = racine.find("savingThrow");
        sauvegarde != racine.end() && sauvegarde->is_string()) {
        sort.savingThrow = parseAbility(sauvegarde->get<std::string>());
        if (!sort.savingThrow.has_value()) {
            erreurs.push_back(fichier + " : jet de sauvegarde '" + sauvegarde->get<std::string>() +
                              "' inconnu du moteur.");
            return false;
        }
    }
    return true;
}

// Les mecanismes que le sort attend du moteur, par leur nom.
[[nodiscard]] std::vector<std::string> lireMecanismesRequis(const nlohmann::json& racine) {
    std::vector<std::string> noms;
    if (const auto mecanismes = racine.find("mecanismesRequis");
        mecanismes != racine.end() && mecanismes->is_array()) {
        for (const auto& element : *mecanismes) {
            if (element.is_string()) {
                noms.push_back(element.get<std::string>());
            }
        }
    }
    return noms;
}

[[nodiscard]] std::optional<Spell> lireSort(const nlohmann::json& racine,
                                            const std::string& fichier,
                                            std::vector<std::string>& erreurs) {
    Spell sort;
    sort.id = lireTexte(racine, "id");
    sort.name = lireTexte(racine, "name");
    sort.source = lireTexte(racine, "source");
    sort.school = lireTexte(racine, "school");
    sort.castingTime = lireTexte(racine, "castingTime");
    sort.range = lireTexte(racine, "range");
    sort.duration = lireTexte(racine, "duration");
    sort.text = lireTexte(racine, "text");
    sort.appliesCondition = lireTexte(racine, "appliesCondition");
    if (const auto composantes = racine.find("components");
        composantes != racine.end() && composantes->is_object()) {
        sort.components = lireComposantes(*composantes);
    }
    sort.concentration = lireBooleen(racine, "concentration");
    sort.ritual = lireBooleen(racine, "ritual");
    sort.attackRoll = lireBooleen(racine, "attackRoll");
    sort.narrative = lireBooleen(racine, "narratif");
    if (sort.id.empty() || sort.name.empty()) {
        erreurs.push_back(fichier + " : sort sans 'id' ou sans 'name'.");
        return std::nullopt;
    }
    const auto niveau = racine.find("level");
    if (niveau == racine.end() || !niveau->is_number_integer()) {
        erreurs.push_back(fichier + " : champ 'level' absent ou non entier.");
        return std::nullopt;
    }
    sort.level = niveau->get<int>();
    if (const auto portee = racine.find("rangeMeters");
        portee != racine.end() && portee->is_number()) {
        sort.rangeMeters = portee->get<float>();
    }
    if (!lireDegats(racine, fichier, sort, erreurs) ||
        !lireMecanismes(racine, fichier, sort, erreurs)) {
        return std::nullopt;
    }
    sort.requiredMechanisms = lireMecanismesRequis(racine);
    return sort;
}

}  // namespace

const Spell* SpellCatalog::find(std::string_view id) const {
    const auto trouve = std::ranges::find(spells, id, &Spell::id);
    return trouve == spells.end() ? nullptr : &*trouve;
}

SpellCatalog loadSpells(const std::filesystem::path& spellsDir) {
    SpellCatalog catalogue;
    std::error_code code;
    if (!std::filesystem::is_directory(spellsDir, code)) {
        catalogue.errors.push_back(spellsDir.string() + " : dossier absent ou illisible.");
        return catalogue;
    }
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(spellsDir, code)) {
        if (entree.is_regular_file(code) && entree.path().extension() == ".json") {
            fichiers.push_back(entree.path());
        }
    }
    std::ranges::sort(fichiers);
    for (const std::filesystem::path& chemin : fichiers) {
        const JsonDocument document = readJsonObjectFromFile(chemin, SANS_GARDE_DE_VERSION);
        if (!document.ok()) {
            catalogue.errors.push_back(document.message);
            continue;
        }
        if (std::optional<Spell> sort =
                lireSort(document.root, chemin.filename().string(), catalogue.errors)) {
            catalogue.spells.push_back(std::move(*sort));
        }
    }
    std::ranges::sort(catalogue.spells, {}, &Spell::id);
    return catalogue;
}

bool isAttackSpell(const Spell& spell) noexcept {
    return spell.attackRoll && spell.damage.has_value() && spell.damageType.has_value();
}

std::string_view spellEffectKindName(SpellEffectKind kind) noexcept {
    for (const NomDEffet& entree : EFFETS) {
        if (entree.genre == kind) {
            return entree.nom;
        }
    }
    return "?";
}

std::optional<SpellMechanism> spellMechanism(const Spell& spell) noexcept {
    if (spell.narrative || !spell.unsupportedArea.empty()) {
        return std::nullopt;
    }
    const bool blesse = spell.damage.has_value() && spell.damageType.has_value();
    if (blesse && spell.attackRoll) {
        return SpellMechanism::AttackRoll;
    }
    if (blesse && spell.autoHit) {
        return SpellMechanism::AutoHit;
    }
    if (blesse && spell.savingThrow.has_value()) {
        return SpellMechanism::SavingThrow;
    }
    if (spell.healing.has_value()) {
        return SpellMechanism::Healing;
    }
    if (spell.stabilizes) {
        return SpellMechanism::Stabilize;
    }
    if (spell.revives.has_value()) {
        return SpellMechanism::Revive;
    }
    if (spell.effect.has_value()) {
        return SpellMechanism::Effect;
    }
    return std::nullopt;
}

std::optional<Dice> spellDamageAt(const Spell& spell, int casterLevel) noexcept {
    if (!spell.damage.has_value()) {
        return std::nullopt;
    }
    Dice des = *spell.damage;
    if (spell.cantripScaling && spell.level == 0) {
        // Manuel, « Tours de magie » : les degats augmentent d'un de aux niveaux 5, 11 et 17.
        for (const int palier : {5, 11, 17}) {
            if (casterLevel >= palier) {
                des.count += spell.damage->count;
            }
        }
    }
    return des;
}

}  // namespace core
