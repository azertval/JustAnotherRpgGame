// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/PalettePanel.h"

#include <QEvent>
#include <QIcon>
#include <QImage>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QModelIndex>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVariant>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "Editor/Logic/ThumbnailGeometry.h"
#include "Editor/Logic/TileTaxonomy.h"
#include "HMI/Graphics/MissingTexture.h"
#include "HMI/Graphics/ProceduralAtlas.h"
#include "HMI/Graphics/TileVisuals.h"

namespace hmi {

namespace {

// Rôle de données portant le `core::TileType` d'une feuille (les en-têtes n'en ont pas).
constexpr int TILE_TYPE_ROLE = Qt::UserRole + 1;
// Rôles d'une feuille de pièce : son nom court, et si elle va sur la couche de sol.
constexpr int PIECE_NAME_ROLE = Qt::UserRole + 2;
constexpr int PIECE_FLOOR_ROLE = Qt::UserRole + 3;
// Role d'une ligne de prefabrique : son nom (LOT-EDITOR-08).
constexpr int PREFAB_NAME_ROLE = Qt::UserRole + 4;

// Cote des vignettes de la palette, en pixels d'ecran : un multiple entier de la taille d'une case
// (16) -- toute autre valeur reechantillonnerait le pixel art de travers, meme en plus proche
// voisin.
constexpr int THUMBNAIL_SIZE = 32;
// Cote des vignettes de pièce : une pièce debout est haute, elle se lit mal plus petite.
constexpr int PIECE_THUMBNAIL_SIZE = 48;
// Cote des vignettes de prefabrique : un morceau de carte se lit plus grand qu'une piece.
constexpr int PREFAB_THUMBNAIL_SIZE = 72;

// Crée une feuille sélectionnable portant son type de tuile.
[[nodiscard]] QStandardItem* makeLeaf(const TileEntry& entry) {
    auto* const item = new QStandardItem(QString::fromStdString(entry.label));
    item->setEditable(false);
    item->setData(static_cast<int>(entry.type), TILE_TYPE_ROLE);
    return item;
}

// Convertit les pixels RGBA de l'atlas en QImage.
[[nodiscard]] QImage toImage(const ProceduralAtlasImage& decoded) {
    QImage image(decoded.width, decoded.height, QImage::Format_RGBA8888);
    for (int y = 0; y < decoded.height; ++y) {
        const std::uint32_t* const row =
            decoded.pixels.data() + (static_cast<std::size_t>(y) * decoded.width);
        std::memcpy(image.scanLine(y), row, static_cast<std::size_t>(decoded.width) * 4);
    }
    return image;
}

// Crée un en-tête (catégorie/sous-groupe) : affiché, mais non sélectionnable comme tuile.
[[nodiscard]] QStandardItem* makeHeader(const QString& label) {
    auto* const item = new QStandardItem(label);
    item->setEditable(false);
    item->setFlags(Qt::ItemIsEnabled);  // ni sélectionnable, ni porteur de type.
    return item;
}

// Libellé d'une pièce : son nom, et son emprise si elle couvre plus d'une case.
[[nodiscard]] QString pieceLabel(const PieceCatalogEntry& entry) {
    QString label = QString::fromStdString(entry.name);
    if (entry.footprint.columns != 1 || entry.footprint.rows != 1) {
        label +=
            QStringLiteral("  (%1 × %2)").arg(entry.footprint.columns).arg(entry.footprint.rows);
    }
    return label;
}

}  // namespace

PalettePanel::PalettePanel(QWidget* parent)
    : QWidget(parent),
      _tabs(new QTabWidget(this)),
      _piecesPage(new QWidget(this)),
      _search(new QLineEdit(this)),
      _pieceTree(new QTreeView(this)),
      _pieceModel(new QStandardItemModel(this)),
      _tree(new QTreeView(this)),
      _model(new QStandardItemModel(this)),
      _prefabTree(new QTreeView(this)),
      _prefabModel(new QStandardItemModel(this)) {
    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* const piecesLayout = new QVBoxLayout(_piecesPage);
    piecesLayout->setContentsMargins(0, 0, 0, 0);
    _search->setPlaceholderText(QStringLiteral("Search pieces…"));
    _search->setClearButtonEnabled(true);
    piecesLayout->addWidget(_search);
    piecesLayout->addWidget(_pieceTree);
    _pieceTree->setHeaderHidden(true);
    _pieceTree->setModel(_pieceModel);
    _pieceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    _pieceTree->setIconSize(QSize(PIECE_THUMBNAIL_SIZE, PIECE_THUMBNAIL_SIZE));
    connect(_search, &QLineEdit::textChanged, this, [this](const QString&) { buildPieceModel(); });
    connect(_pieceTree->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) { onPieceChanged(current); });
    // Recliquer la pièce courante la reprend, après la gomme ou la pipette par exemple.
    connect(_pieceTree, &QTreeView::clicked, this, &PalettePanel::onPieceChanged);

    _tree->setHeaderHidden(true);
    _tree->setModel(_model);
    _tree->setSelectionMode(QAbstractItemView::SingleSelection);

    _prefabTree->setHeaderHidden(true);
    _prefabTree->setModel(_prefabModel);
    _prefabTree->setSelectionMode(QAbstractItemView::SingleSelection);
    _prefabTree->setIconSize(QSize(PREFAB_THUMBNAIL_SIZE, PREFAB_THUMBNAIL_SIZE));
    connect(_prefabTree, &QTreeView::clicked, this, &PalettePanel::onPrefabChosen);
    connect(_prefabTree->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) { onPrefabChosen(current); });

    _tabs->addTab(_piecesPage, QStringLiteral("Pieces"));
    _tabs->addTab(_tree, QStringLiteral("Types"));
    // En dernier : les index 0 (pièces) et 1 (types) restent ceux que le repli sans lieu utilise.
    _tabs->addTab(_prefabTree, QStringLiteral("Prefabs"));
    layout->addWidget(_tabs);

    buildModel();
    _tree->expandAll();
    // Sans lieu, rien à poser : l'onglet des pièces s'éteint jusqu'au premier catalogue, et la
    // bibliothèque reste vide tant qu'aucun préfabriqué n'a été enregistré.
    _tabs->setTabEnabled(0, false);
    _tabs->setTabEnabled(2, false);
    _tabs->setCurrentIndex(1);

    connect(_tree->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) { onCurrentChanged(current); });
    connect(_tree, &QTreeView::clicked, this, &PalettePanel::onCurrentChanged);
}

void PalettePanel::buildModel() {
    for (const TileCategory& category : tileTaxonomy()) {
        QStandardItem* const categoryItem = makeHeader(QString::fromStdString(category.label));
        for (const TileEntry& entry : category.tiles) {
            QStandardItem* const leaf = makeLeaf(entry);
            leaf->setIcon(QIcon(thumbnailFor(entry.type)));
            categoryItem->appendRow(leaf);
        }
        for (const TileSubgroup& subgroup : category.subgroups) {
            QStandardItem* const subgroupItem = makeHeader(QString::fromStdString(subgroup.label));
            for (const TileEntry& entry : subgroup.tiles) {
                QStandardItem* const leaf = makeLeaf(entry);
                leaf->setIcon(QIcon(thumbnailFor(entry.type)));
                subgroupItem->appendRow(leaf);
            }
            categoryItem->appendRow(subgroupItem);
        }
        _model->appendRow(categoryItem);
    }
}

void PalettePanel::setPieceCatalog(std::vector<PieceCatalogGroup> catalog,
                                   const std::filesystem::path& placeDirectory) {
    if (catalog == _catalog && placeDirectory == _placeDirectory) {
        return;
    }
    const bool hadPieces = !_catalog.empty();
    _catalog = std::move(catalog);
    _placeDirectory = placeDirectory;
    buildPieceModel();
    const bool hasPieces = !_catalog.empty();
    _tabs->setTabEnabled(0, hasPieces);
    // Un lieu qui paraît ouvre ses pièces ; un lieu qui s'en va rend la main aux types (repli).
    if (hasPieces != hadPieces) {
        _tabs->setCurrentIndex(hasPieces ? 0 : 1);
    }
}

void PalettePanel::setPrefabs(std::vector<PrefabItem> prefabs) {
    if (prefabs.size() == _prefabs.size() &&
        std::equal(prefabs.begin(), prefabs.end(), _prefabs.begin(),
                   [](const PrefabItem& left, const PrefabItem& right) {
                       return left.name == right.name && left.detail == right.detail;
                   })) {
        return;  // la bibliothèque n'a pas bougé : garder la sélection.
    }
    _prefabs = std::move(prefabs);
    const QSignalBlocker blocker(_prefabTree->selectionModel());
    _prefabModel->clear();
    for (const PrefabItem& prefab : _prefabs) {
        auto* const item = new QStandardItem(prefab.name + QStringLiteral("\n") + prefab.detail);
        item->setEditable(false);
        item->setData(prefab.name, PREFAB_NAME_ROLE);
        item->setToolTip(prefab.name + QStringLiteral(" — ") + prefab.detail);
        if (!prefab.thumbnail.isNull()) {
            item->setIcon(QIcon(prefab.thumbnail));
        }
        _prefabModel->appendRow(item);
    }
    _tabs->setTabEnabled(2, !_prefabs.empty());
}

void PalettePanel::onPrefabChosen(const QModelIndex& current) {
    if (!current.isValid()) {
        return;
    }
    const QString name = current.data(PREFAB_NAME_ROLE).toString();
    if (!name.isEmpty()) {
        emit prefabSelected(name);
    }
}

void PalettePanel::buildPieceModel() {
    const QSignalBlocker blocker(_pieceTree->selectionModel());
    _pieceModel->clear();
    QModelIndex reselect;
    for (const PieceCatalogGroup& group :
         filterPieceCatalog(_catalog, _search->text().toStdString())) {
        QStandardItem* const header = makeHeader(QString::fromStdString(group.label));
        for (const PieceCatalogEntry& entry : group.pieces) {
            auto* const leaf = new QStandardItem(pieceLabel(entry));
            leaf->setEditable(false);
            leaf->setIcon(QIcon(pieceThumbnail(entry)));
            leaf->setToolTip(QString::fromStdString(pieceDescription(entry)));
            leaf->setData(QString::fromStdString(entry.name), PIECE_NAME_ROLE);
            leaf->setData(entry.floor, PIECE_FLOOR_ROLE);
            header->appendRow(leaf);
        }
        _pieceModel->appendRow(header);
    }
    _pieceTree->expandAll();
    // La pièce choisie reste choisie d'une recherche à l'autre, si elle y paraît encore.
    for (int groupRow = 0; groupRow < _pieceModel->rowCount() && !reselect.isValid(); ++groupRow) {
        const QStandardItem* const header = _pieceModel->item(groupRow);
        for (int row = 0; row < header->rowCount(); ++row) {
            if (header->child(row)->data(PIECE_NAME_ROLE).toString() == _selectedPiece) {
                reselect = header->child(row)->index();
                break;
            }
        }
    }
    if (reselect.isValid()) {
        _pieceTree->selectionModel()->setCurrentIndex(reselect,
                                                      QItemSelectionModel::ClearAndSelect);
    }
}

// Vignette d'un type : sa couleur dans l'atlas procedural, celle que le canevas peint.
QPixmap PalettePanel::thumbnailFor(core::TileType type) {
    const ProceduralAtlasImage atlas = buildProceduralAtlasImage();
    const QImage source = toImage(atlas);
    const core::AtlasRegion region = regionForTile(type);
    const QImage tile = source.copy(region.x, region.y, region.width, region.height);

    // Mise a l'echelle en PLUS PROCHE VOISIN, a la resolution REELLE : sans quoi
    // l'interpolation lisse de Qt (fond d'ecran a 125%/150%) rendrait le pixel art flou, incoherent
    // avec le rendu du canevas (EX-ARCH-022).
    const qreal scale = devicePixelRatioF();
    const int pixelSize = thumbnailPixelSize(THUMBNAIL_SIZE, scale);
    QPixmap pixmap = QPixmap::fromImage(
        tile.scaled(pixelSize, pixelSize, Qt::KeepAspectRatio, Qt::FastTransformation));
    pixmap.setDevicePixelRatio(scale);
    return pixmap;
}

QPixmap PalettePanel::pieceThumbnail(const PieceCatalogEntry& entry) const {
    const qreal scale = devicePixelRatioF();
    const int side = thumbnailPixelSize(PIECE_THUMBNAIL_SIZE, scale);
    QImage source;
    if (!entry.missing && !entry.file.empty()) {
        source.load(QString::fromStdWString((_placeDirectory / entry.file).wstring()));
    }
    if (source.isNull()) {
        // Une pièce absente se montre comme le canevas la montre : en damier (EX-NFR-040).
        source = toImage(buildMissingTextureImage());
    }
    // Une pièce réduite se lisse ; une pièce agrandie garde ses pixels (EX-ARCH-022).
    const bool shrinks = source.width() > side || source.height() > side;
    const QImage fitted =
        source.scaled(side, side, Qt::KeepAspectRatio,
                      shrinks ? Qt::SmoothTransformation : Qt::FastTransformation);
    QImage square(side, side, QImage::Format_ARGB32_Premultiplied);
    square.fill(Qt::transparent);
    QPainter painter(&square);
    painter.drawImage((side - fitted.width()) / 2, (side - fitted.height()) / 2, fitted);
    painter.end();
    QPixmap pixmap = QPixmap::fromImage(square);
    pixmap.setDevicePixelRatio(scale);
    return pixmap;
}

bool PalettePanel::event(QEvent* event) {
    if (event->type() == QEvent::ScreenChangeInternal) {
        // Un deplacement vers un ecran d'echelle differente doit regenerer les vignettes :
        // seule la mise a l'echelle doit etre rejouee.
        _model->clear();
        buildModel();
        _tree->expandAll();
        buildPieceModel();
    }
    return QWidget::event(event);
}

void PalettePanel::onCurrentChanged(const QModelIndex& current) {
    const QVariant tileData = current.data(TILE_TYPE_ROLE);
    if (!tileData.isValid()) {
        return;  // en-tête (catégorie/sous-groupe) : pas un type sélectionnable.
    }
    _selected = static_cast<core::TileType>(tileData.toInt());
    emit tileSelected(_selected);
}

void PalettePanel::onPieceChanged(const QModelIndex& current) {
    const QVariant name = current.data(PIECE_NAME_ROLE);
    if (!name.isValid()) {
        return;  // en-tête de groupe.
    }
    _selectedPiece = name.toString();
    _selectedPieceFloor = current.data(PIECE_FLOOR_ROLE).toBool();
    emit pieceSelected(_selectedPiece, _selectedPieceFloor);
}

void PalettePanel::showPiece(const QString& piece, bool floor) {
    _selectedPiece = piece;
    _selectedPieceFloor = floor;
    if (!_tabs->isTabEnabled(0)) {
        return;
    }
    _tabs->setCurrentIndex(0);
    // Une recherche qui cacherait la pièce prise s'efface ; le modèle refait la resélectionne.
    if (!_search->text().isEmpty()) {
        _search->clear();  // textChanged refait le modèle, signaux de sélection bloqués.
    } else {
        buildPieceModel();
    }
}

void PalettePanel::showTile(core::TileType type) {
    _selected = type;
    _tabs->setCurrentIndex(1);
    const QSignalBlocker blocker(_tree->selectionModel());
    const QModelIndexList found =
        _model->match(_model->index(0, 0), TILE_TYPE_ROLE, static_cast<int>(type), 1,
                      Qt::MatchExactly | Qt::MatchRecursive);
    if (!found.isEmpty()) {
        _tree->selectionModel()->setCurrentIndex(found.front(),
                                                 QItemSelectionModel::ClearAndSelect);
        _tree->scrollTo(found.front());
    }
}

}  // namespace hmi
