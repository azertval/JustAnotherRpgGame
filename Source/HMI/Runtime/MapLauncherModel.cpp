// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/MapLauncherModel.h"

#include <QVariantMap>
#include <string>
#include <utility>

#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "HMI/Game/LaunchOptions.h"
#include "HMI/Game/LevelScan.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/RuleLabels.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {

namespace {

[[nodiscard]] QString versQt(const std::filesystem::path& chemin) {
    return QString::fromStdString(chemin.string());
}

/// @brief Ce que la liste dit d'une carte : son nom et sa taille, lus dans le fichier — ou
///        l'erreur qui empêche de l'ouvrir, pour qu'une carte cassée se voie ici plutôt que sur
///        un écran de jeu vide.
[[nodiscard]] QVariantMap decrire(const LevelEntry& carte) {
    QVariantMap ligne;
    ligne.insert(QStringLiteral("id"), QString::fromStdString(carte.mapId));
    ligne.insert(QStringLiteral("file"), versQt(carte.file));
    ligne.insert(QStringLiteral("directory"), versQt(carte.directory));
    const core::LevelLoadResult lue = core::LevelLoader::loadFromFile(carte.file);
    if (!lue.ok()) {
        ligne.insert(QStringLiteral("name"), QString{});
        ligne.insert(QStringLiteral("columns"), 0);
        ligne.insert(QStringLiteral("rows"), 0);
        ligne.insert(QStringLiteral("error"), QString::fromStdString(lue.error));
        return ligne;
    }
    // Le nom d'une carte est une cle (`map.<identifiant>.name`, LOT-EDITOR-07), comme le HUD la
    // lit ; une carte d'essai sans libelle revient telle quelle.
    ligne.insert(QStringLiteral("name"),
                 QString::fromStdString(ruleLabel(lue.level->name(), activeLanguage())));
    ligne.insert(QStringLiteral("columns"), lue.level->tileMap().width());
    ligne.insert(QStringLiteral("rows"), lue.level->tileMap().height());
    ligne.insert(QStringLiteral("error"), QString{});
    return ligne;
}

}  // namespace

MapLauncherModel::MapLauncherModel(QObject* parent) : QObject(parent) {
    refresh();
}

QStringList MapLauncherModel::directories() const {
    QStringList dossiers;
    for (const std::filesystem::path& dossier : _directories) {
        dossiers.push_back(versQt(dossier));
    }
    return dossiers;
}

QString MapLauncherModel::levelsRoot() const {
    return versQt(dataDirectory() / "Levels");
}

std::vector<std::filesystem::path> MapLauncherModel::allDirectories() const {
    std::vector<std::filesystem::path> dossiers = _directories;
    dossiers.push_back(dataDirectory() / "Levels");
    return dossiers;
}

void MapLauncherModel::refresh() {
    _maps.clear();
    for (const LevelEntry& carte : scanLevelDirectories(allDirectories())) {
        _maps.push_back(decrire(carte));
    }
    emit changed();
}

bool MapLauncherModel::addDirectory(const QString& path) {
    const std::filesystem::path dossier = std::filesystem::absolute(path.trimmed().toStdString());
    if (!std::filesystem::is_directory(dossier)) {
        setStatus(QStringLiteral("Dossier introuvable : %1").arg(versQt(dossier)));
        return false;
    }
    _directories.push_back(dossier);
    setStatus(QStringLiteral("Dossier ajoute : %1").arg(versQt(dossier)));
    refresh();
    return true;
}

void MapLauncherModel::removeDirectory(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= _directories.size()) {
        return;
    }
    _directories.erase(_directories.begin() + index);
    refresh();
}

bool MapLauncherModel::launch(const QString& mapId, const QString& arrival, const QString& at,
                              const QString& flags, const QString& heroFigure) {
    WorldModel* const world = WorldModel::current();
    if (world == nullptr) {
        setStatus(QStringLiteral("Aucune partie : le modele du monde n'existe pas."));
        return false;
    }
    if (mapId.trimmed().isEmpty()) {
        setStatus(QStringLiteral("Choisir une carte."));
        return false;
    }
    QString remarque;
    // Le meme ordre que Main.cpp : les dossiers d'abord (la session est refaite), puis l'etat,
    // puis la carte.
    if (!_directories.empty()) {
        world->setLevelDirectories(_directories);
    }
    if (const QString cellule = at.trimmed(); !cellule.isEmpty()) {
        const std::optional<core::GridPosition> lue = parseStartCell(cellule.toStdString());
        if (!lue) {
            remarque = QStringLiteral(" ; case illisible (attendu <colonne>,<ligne>), le heros "
                                      "part de l'entree");
        }
        world->setStartCell(lue);
    } else {
        world->setStartCell(std::nullopt);
    }
    if (const QString drapeaux = flags.trimmed(); !drapeaux.isEmpty()) {
        QStringList poses;
        for (const std::string& drapeau : parseWorldFlags(drapeaux.toStdString())) {
            poses.push_back(QString::fromStdString(drapeau));
        }
        world->setStartFlags(poses);
    }
    if (const QString figure = heroFigure.trimmed(); !figure.isEmpty()) {
        world->setHeroFigure(figure);
    }
    world->setStartOverride(mapId.trimmed(), arrival.trimmed());
    if (!world->startNewGame()) {
        setStatus(QStringLiteral("La carte « %1 » ne s'ouvre pas : %2")
                      .arg(mapId.trimmed(), world->status()));
        return false;
    }
    HMI_LOG_INFO("Lanceur de cartes : " + mapId.trimmed().toStdString());
    setStatus(QStringLiteral("Carte ouverte : %1%2").arg(mapId.trimmed(), remarque));
    emit launched(mapId.trimmed());
    return true;
}

void MapLauncherModel::setStatus(QString status) {
    _status = std::move(status);
    emit changed();
}

}  // namespace hmi
