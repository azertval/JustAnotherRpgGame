// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QPixmap>
#include <QString>
#include <QWidget>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

/**
 * @file Editor/Ui/LevelBrowserPanel.h
 * @brief Panneau « Niveaux » : liste, recherche et gestion des fichiers de niveaux.
 */

class QModelIndex;
class QSortFilterProxyModel;
class QStandardItemModel;

namespace hmi {

/**
 * @brief Panneau de gestion des niveaux : liste filtrable + créer/renommer/dupliquer/supprimer.
 *
 * Liste les fichiers `.json` d'un dossier, avec **recherche** incrémentale
 * (`QSortFilterProxyModel`) pour rester lisible quel que soit leur nombre. Les opérations
 * (`EX-IHM-021`) délèguent à `hmi::LevelFileOperations` (couche pure, validée/testée) ; les
 * erreurs sont signalées à l'utilisateur, jamais silencieuses. Un double-clic (ou « Ouvrir ») émet
 * `levelOpenRequested` — le garde-fou des modifications non enregistrées est appliqué par
 * l'appelant (`MainWindow`).
 *
 * Un second onglet montre le **graphe du monde** (`LOT-11`, `hmi::WorldGraphView`) : les cartes du
 * même dossier et leurs portails. Il est relu à chaque `refresh()` ; un double-clic sur une carte
 * émet le même `levelOpenRequested` que la liste, et **tirer d'une carte à une autre** demande un
 * lien (`LOT-EDITOR-09`).
 *
 * Un troisième montre la **ville** (`hmi::CityMapView`) : les quartiers posés sur son plan, et la
 * carte que chacun ouvre. La liste, elle, dit **où en est** chaque carte (`hmi::MapState`), se
 * filtre par état et se montre en **vignettes** — à cent cartes, c'est le tableau de bord du monde
 * (`EX-EDIT-092`).
 */
class LevelBrowserPanel : public QWidget {
    Q_OBJECT

public:
    explicit LevelBrowserPanel(std::filesystem::path levelsDir, QWidget* parent = nullptr);
    ~LevelBrowserPanel() override;

    /// Recharge la liste depuis le dossier (après une opération ou un changement externe), puis le
    /// graphe du monde.
    void refresh();

    /// Relit le graphe du monde depuis le dossier, sans toucher à la liste (après un
    /// enregistrement, qui peut changer les portails d'une carte sans changer les fichiers).
    void refreshWorldGraph();

signals:
    /// Émis quand l'utilisateur demande l'ouverture d'un niveau (chemin absolu du fichier).
    void levelOpenRequested(const QString& path);
    /// Émis par « Rename » : le renommage propagé (`LOT-EDITOR-14`) réécrit d'autres fichiers, et
    /// peut-être la carte ouverte ; c'est l'appelant (`MainWindow`) qui le mène.
    void mapRenameRequested(const QString& mapId);
    /// Émis quand un lien est tiré d'une carte à une autre sur le graphe (`LOT-EDITOR-09`) : le
    /// plan et son écriture sont menés par `MainWindow`.
    void mapLinkRequested(const QString& fromMap, const QString& toMap);

private:
    void onNew();
    void onRename();
    void onDuplicate();
    void onDelete();
    void onActivated(const QModelIndex& index);
    /// Remplit la vue de ville : celle que le choix désigne, avec ses quartiers.
    void refreshCity();
    /// Bascule la liste entre lignes et vignettes (`LOT-EDITOR-09`).
    void applyThumbnailMode();
    /**
     * @brief La vignette de la carte @p path, rendue par le peintre du canevas et gardée.
     * @param path Le fichier de la carte.
     * @return L'image, nulle si la carte ne se lit pas.
     */
    [[nodiscard]] QPixmap thumbnailFor(const std::filesystem::path& path);

    /// Chemin du niveau sélectionné, ou chemin vide si aucune sélection.
    [[nodiscard]] std::filesystem::path selectedPath() const;

    /// Les widgets du panneau, construits en code (`LevelBrowserPanel.cpp`).
    struct Widgets;
    std::unique_ptr<Widgets> _ui;
    std::filesystem::path _dir;
    QStandardItemModel* _model;     ///< Modèle source (données), rempli par refresh().
    QSortFilterProxyModel* _proxy;  ///< Filtre de recherche au-dessus du modèle.
    /// Les vignettes déjà rendues, par carte et horodatage de son fichier : une carte récrite
    /// perd la sienne, les autres la gardent.
    std::map<std::string, QPixmap> _thumbnails;
};

}  // namespace hmi
