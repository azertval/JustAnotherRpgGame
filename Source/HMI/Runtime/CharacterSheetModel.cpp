// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/CharacterSheetModel.h"

#include <QVector>
#include <array>
#include <string>

#include "Core/Rpg/Ability.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/DemonstrationCharacter.h"
#include "HMI/Runtime/RuleLabels.h"

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
    // Le chargement est PARTAGE avec l'inventaire : les deux ecrans decrivent le meme personnage,
    // et deux chargements separes auraient pu ne pas voir le meme equipement -- alors que la
    // classe d'armure de la fiche vient de ce que l'inventaire contient.
    const DemonstrationCharacter loaded = loadDemonstrationValues();
    _values = loaded.sheet;

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
