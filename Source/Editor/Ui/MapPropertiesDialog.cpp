// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/MapPropertiesDialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <utility>

#include "Core/World/WorldClock.h"

namespace hmi {

namespace {

/// Le texte du choix « rien de dit » d'une liste.
constexpr const char* NOTHING = "(not stated)";

}  // namespace

std::optional<MapPropertiesChoice> askMapProperties(QWidget* parent, const QString& mapId,
                                                    const QString& place,
                                                    const std::vector<std::string>& regions,
                                                    const MapPropertiesChoice& current) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Map properties"));
    auto* const layout = new QVBoxLayout(&dialog);
    auto* const form = new QFormLayout;
    layout->addLayout(form);

    form->addRow(QStringLiteral("Map"),
                 new QLabel(mapId.isEmpty() ? QStringLiteral("(unsaved)") : mapId, &dialog));
    // Le lieu se voit, il ne s'édite pas ici : en changer repeint la carte (LOT-EDITOR-14).
    auto* const placeLabel =
        new QLabel(place.isEmpty() ? QStringLiteral("(none)") : place, &dialog);
    placeLabel->setToolTip(QStringLiteral("Change it with Map > Change sheet…"));
    form->addRow(QStringLiteral("Sheet (place)"), placeLabel);

    // La région : celles du monde, et toute autre à la main -- une carte peut précéder sa région.
    auto* const region = new QComboBox(&dialog);
    region->setEditable(true);
    region->addItem(QString{});
    for (const std::string& known : regions) {
        region->addItem(QString::fromStdString(known));
    }
    region->setCurrentText(QString::fromStdString(current.region));
    form->addRow(QStringLiteral("Region"), region);

    auto* const ambience = new QLineEdit(QString::fromStdString(current.ambience), &dialog);
    ambience->setPlaceholderText(QStringLiteral("what plays here (LOT-28)"));
    form->addRow(QStringLiteral("Ambience"), ambience);

    // L'heure fixe (LOT-1007) : un sous-sol reste dans sa nuit, quelle que soit l'heure du monde.
    auto* const hour = new QLineEdit(QString::fromStdString(current.hour), &dialog);
    hour->setPlaceholderText(QStringLiteral("HH:MM — empty: the hour of the world"));
    hour->setToolTip(QStringLiteral(
        "A map with a fixed hour is always lit as at that hour: a cellar, a shrine."));
    form->addRow(QStringLiteral("Fixed hour"), hour);

    auto* const state = new QComboBox(&dialog);
    state->addItem(QString::fromLatin1(NOTHING));
    for (const MapState known : knownMapStates()) {
        state->addItem(QString::fromUtf8(mapStateLabel(known).data(),
                                         static_cast<int>(mapStateLabel(known).size())));
    }
    state->setCurrentIndex(0);
    for (std::size_t index = 0; index < knownMapStates().size(); ++index) {
        if (knownMapStates()[index] == current.state) {
            state->setCurrentIndex(static_cast<int>(index) + 1);
        }
    }
    form->addRow(QStringLiteral("State"), state);

    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    // Une heure mal écrite ne ferme pas le dialogue : elle se corrige, ou s'efface.
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&dialog, hour] {
        const std::string text = hour->text().trimmed().toStdString();
        if (text.empty() || core::parseClockTime(text).has_value()) {
            dialog.accept();
            return;
        }
        hour->setFocus();
        hour->selectAll();
        hour->setToolTip(QStringLiteral("Write the hour as HH:MM (00:00 to 23:59)."));
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }
    const int chosen = state->currentIndex() - 1;
    return MapPropertiesChoice{
        .region = region->currentText().trimmed().toStdString(),
        .ambience = ambience->text().trimmed().toStdString(),
        .hour = hour->text().trimmed().toStdString(),
        .state = chosen >= 0 && std::cmp_less(chosen, knownMapStates().size())
                     ? knownMapStates()[static_cast<std::size_t>(chosen)]
                     : MapState::Unset};
}

}  // namespace hmi
