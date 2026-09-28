// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/CharacterSheetModel.h"

#include <QStringList>
#include <QVector>
#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <system_error>

#include "Core/Combat/Damage.h"
#include "Core/Rpg/Ability.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/ClassCapacities.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/Party.h"
#include "Core/Rpg/Spell.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/DemonstrationCharacter.h"
#include "HMI/Runtime/RuleLabels.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {
namespace {

// Tiret cadratin : à l'écran, « inconnu » et « pas encore alimenté » se ressemblent, et rien ne
// gagne à les distinguer par deux signes différents.
constexpr const char* EMPTY_MARK = "—";

// Les six caractéristiques, dans l'ordre où une feuille de personnage les présente.
constexpr std::array<core::Ability, 6> ABILITIES{
    core::Ability::Strength,     core::Ability::Dexterity, core::Ability::Constitution,
    core::Ability::Intelligence, core::Ability::Wisdom,    core::Ability::Charisma,
};

[[nodiscard]] QString toQt(const std::string& text) {
    return QString::fromStdString(text);
}

// Rend : La valeur de `key` dans `values`, ou une chaîne vide.
[[nodiscard]] QString lookup(const std::map<std::string, std::string>& values,
                             const std::string& key) {
    const auto found = values.find(key);
    return found == values.end() ? QString() : toQt(found->second);
}

// Les niveaux a venir que l'onglet Classe montre : la page de la classe, d'un coup d'oeil, sans
// derouler vingt niveaux.
constexpr int UPCOMING_LEVELS = 4;

// Le niveau ou la table de la classe donne la capacite `id`, ou 0 si elle ne la nomme pas.
[[nodiscard]] int levelOf(const core::PlayableClass& playableClass, const std::string& id) {
    for (const core::ClassLevel& ligne : playableClass.progression) {
        if (std::ranges::find(ligne.features, id) != ligne.features.end()) {
            return ligne.level;
        }
    }
    return 0;
}

[[nodiscard]] QVariantMap capacityRow(const core::Capacity& capacity, int level) {
    return QVariantMap{
        {"id", toQt(capacity.id)},
        {"name", toQt(capacity.name)},
        {"iconKey", QStringLiteral("ui/icon/capacity/") +
                        toQt(capacity.iconId.empty() ? capacity.id : capacity.iconId)},
        {"text", toQt(capacity.text)},
        {"level", level},
        {"narrative", capacity.narrative}};
}

// Le fichier de la fiche `characterId` : un candidat de la partie, sinon une fiche pre-tiree du
// binaire ; vide si rien ne la porte.
[[nodiscard]] std::filesystem::path characterFileOf(const std::string& characterId) {
    if (const WorldModel* const partie = WorldModel::current()) {
        const auto candidat =
            std::ranges::find(partie->candidates(), characterId, &core::PartyCandidate::id);
        if (candidat != partie->candidates().end()) {
            return candidat->file;
        }
    }
    const std::filesystem::path fichier =
        executableDirectory() / "Rpg" / "characters" / (characterId + ".json");
    std::error_code erreur;
    return std::filesystem::is_regular_file(fichier, erreur) ? fichier : std::filesystem::path{};
}

}  // namespace

CharacterSheetModel::CharacterSheetModel(QObject* parent)
    : QObject(parent), _abilities(this), _skills(this) {}

QString CharacterSheetModel::value(const char* key) const {
    const auto found = _values.find(key);
    if (found == _values.end() || found->second.empty()) {
        return QString::fromUtf8(EMPTY_MARK);
    }
    return toQt(found->second);
}

QVariantMap CharacterSheetModel::values() const {
    QVariantMap table;
    for (const auto& [key, text] : _values) {
        table.insert(toQt(key), text.empty() ? QString::fromUtf8(EMPTY_MARK) : toQt(text));
    }
    return table;
}

void CharacterSheetModel::loadDemonstrationCharacter() {
    loadCharacter(QString());
}

void CharacterSheetModel::loadShownCharacter() {
    const WorldModel* const partie = WorldModel::current();
    loadCharacter(partie != nullptr ? partie->shownCharacterId() : QString());
}

void CharacterSheetModel::loadCharacter(const QString& characterId) {
    // Le chargement est PARTAGE avec l'inventaire : les deux ecrans decrivent le meme personnage,
    // et deux chargements separes auraient pu ne pas voir le meme equipement -- alors que la
    // classe d'armure de la fiche vient de ce que l'inventaire contient.
    std::filesystem::path fichier;
    if (!characterId.isEmpty()) {
        fichier = characterFileOf(characterId.toStdString());
        if (fichier.empty()) {
            HMI_LOG_WARNING("Fiche : personnage inconnu '" + characterId.toStdString() +
                            "', le personnage joue s'ouvre a sa place.");
        }
    }
    if (fichier.empty()) {
        fichier = playedCharacterFile();
    }
    const DemonstrationState state = loadDemonstrationState(fichier);
    const DemonstrationCharacter loaded = demonstrationValues(state);
    _values = loaded.sheet;
    _characterId = toQt(fichier.stem().string());

    // L'onglet Classe (LOT-141) : ce que la fiche porte (acquis), et ce que la table donnera aux
    // prochains niveaux (a venir), chaque capacite au niveau ou la table la nomme.
    _capacities.clear();
    _upcoming.clear();
    _spells.clear();
    const core::PlayableClass* const classe = state.options.findClass(state.sheet.classId);
    for (const core::Capacity& capacity : state.sheet.capacities) {
        _capacities << capacityRow(capacity, classe != nullptr ? levelOf(*classe, capacity.id) : 0);
    }
    if (classe != nullptr) {
        for (const core::ClassLevel& ligne : classe->progression) {
            if (ligne.level <= state.sheet.level ||
                ligne.level > state.sheet.level + UPCOMING_LEVELS ||
                ligne.level > state.experience.maximumLevel()) {
                continue;
            }
            for (const std::string& id : ligne.features) {
                if (const core::Capacity* const capacity = state.options.capacities.find(id)) {
                    _upcoming << capacityRow(*capacity, ligne.level);
                }
            }
        }
    }
    // L'onglet Sorts : les sorts connus, lancers restants compris (le registre de la partie est
    // deja applique), decrits par le catalogue.
    for (const core::KnownSpell& known : state.sheet.knownSpells) {
        const core::Spell* const spell = state.options.spells.find(known.spellId);
        if (spell == nullptr) {
            continue;
        }
        QStringList details;
        if (!spell->castingTime.empty()) {
            details << tr("Incantation : %1").arg(toQt(spell->castingTime));
        }
        if (!spell->range.empty()) {
            details << tr("Portée : %1").arg(toQt(spell->range));
        }
        if (!spell->duration.empty()) {
            details << tr("Durée : %1").arg(toQt(spell->duration)) +
                           (spell->concentration ? tr(" (concentration)") : QString());
        }
        if (spell->damage.has_value()) {
            details << tr("Dégâts : %1%2")
                           .arg(toQt(core::formatDice(*spell->damage)))
                           .arg(spell->damageType.has_value()
                                    ? " " + toQt(std::string(
                                                core::damageTypeLabel(*spell->damageType)))
                                    : QString());
        }
        if (spell->healing.has_value()) {
            details << tr("Soin : %1").arg(toQt(core::formatDice(*spell->healing)));
        }
        _spells << QVariantMap{
            {"id", toQt(spell->id)},
            {"name", toQt(spell->name)},
            {"iconKey", QStringLiteral("ui/icon/spell/") + toQt(spell->id)},
            {"level", known.level},
            {"perDay", known.perDay},
            {"remaining", known.remaining},
            {"usesText", known.perDay == 0
                             ? tr("à volonté")
                             : QStringLiteral("%1 / %2").arg(known.remaining).arg(known.perDay)},
            {"details", details.join(QLatin1Char('\n'))},
            {"school", toQt(spell->school)}};
    }

    // Les noms francais des six caracteristiques sont une DONNEE, pas une constante de code : ils
    // viennent du lexique des regles (`rpg.glossary.csv`), qui garantit une seule traduction par
    // terme dans tout le jeu. Les ecrire ici en dur aurait cree un deuxieme vocabulaire, et c'est
    // exactement ce que le lexique existe pour empecher.
    const std::string language = activeLanguage();

    QVector<SheetRow> abilityRows;
    abilityRows.reserve(static_cast<qsizetype>(ABILITIES.size()));
    for (const core::Ability ability : ABILITIES) {
        const std::string id(core::abilityName(ability));
        const QString label = toQt(ruleLabel("rpg.ability." + id, language));
        abilityRows.append(SheetRow{
            .id = toQt(id), .label = label, .value = lookup(_values, "sheet.ability." + id)});
    }
    _abilities.setRows(std::move(abilityRows));

    // Les libelles des competences viennent du catalogue de REGLES, jamais d'une table ecrite ici.
    QVector<SheetRow> skillRows;
    skillRows.reserve(static_cast<qsizetype>(loaded.skills.size()));
    for (const auto& [id, name] : loaded.skills) {
        skillRows.append(SheetRow{
            .id = toQt(id), .label = toQt(name), .value = lookup(_values, "sheet.skill." + id)});
    }
    _skills.setRows(std::move(skillRows));

    emit changed();
}

}  // namespace hmi
