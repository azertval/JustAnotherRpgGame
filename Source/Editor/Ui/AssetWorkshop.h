// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QDialog>
#include <QElapsedTimer>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "Core/Resources/SkeletonFile.h"
#include "Editor/Logic/BlenderRetouch.h"
#include "Editor/Logic/CharacterWorkshop.h"

class QCheckBox;
class QEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QTimer;
class QVBoxLayout;

/**
 * @file Editor/Ui/AssetWorkshop.h
 * @brief La fenêtre **Asset workshop** (`LOT-1008`, `EX-EDIT-102`, `EX-EDIT-103`) : la vue
 *        *Character*, où l'auteur écrit la fiche d'un personnage sans ouvrir un fichier.
 */

namespace hmi {

class SceneSurface;

/**
 * @brief L'atelier des assets : choisir le modèle, le portrait, le jeton et la silhouette d'un
 *        personnage, le voir animé par le rendu du jeu, le régler dans Blender, l'installer.
 *
 * - À gauche, les personnages installés : en choisir un rouvre sa fiche.
 * - Au centre, la **fiche d'atelier** (`hmi::CharacterDraft`) : niveau, nom, squelette, modèle,
 *   portrait, jeton, et les fichiers de l'aller-retour par Blender. Elle s'enregistre avec les
 *   sources du personnage (*Save sheet*) ; *Install* écrit le personnage sous `Assets/`
 *   (`hmi::planCharacter`), exactement comme `LevelEditor --apply <fiche>`.
 * - À droite, l'**aperçu** : le modèle, dessiné par le rendu du jeu, clip par clip, avec le quart
 *   de tour, à l'heure choisie : midi, l'aube, le crépuscule, la nuit, ou sans éclairage
 *   (`LOT-1007`).
 * - *Edit in Blender* ouvre le modèle lié dans Blender ; *Import from Blender* relit ce qui y a
 *   été réglé, relie le modèle et le contrôle (`hmi::importFromBlenderCommand`). Le compte rendu
 *   des deux s'affiche sous la fiche.
 *
 * Fenêtre non modale : la carte ouverte reste sous la main. Textes en anglais, hors charte v2.
 */
class AssetWorkshop : public QDialog {
    Q_OBJECT

public:
    /// @param dataRoot La racine des données (`Source/Elements`).
    /// @param parent   La fenêtre de l'éditeur.
    explicit AssetWorkshop(std::filesystem::path dataRoot, QWidget* parent = nullptr);
    ~AssetWorkshop() override;
    AssetWorkshop(const AssetWorkshop&) = delete;
    AssetWorkshop& operator=(const AssetWorkshop&) = delete;

    /// @brief Ouvre la fiche d'atelier @p file. @return Faux si elle ne se lit pas (le compte
    ///        rendu dit pourquoi).
    bool openSheet(const std::filesystem::path& file);

    /// @return La fiche telle que les champs la disent.
    [[nodiscard]] CharacterDraft draft() const;

signals:
    /// Un personnage vient d'être installé : les références de l'éditeur sont à relire.
    void characterInstalled();

protected:
    /// Le cadrage de l'aperçu suit la taille de sa surface.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Widgets;

    void buildUi();
    [[nodiscard]] QWidget* buildSheetColumn();
    [[nodiscard]] QWidget* buildPreviewColumn();
    /// @brief Une ligne « champ + Browse… » de la fiche.
    /// @param column La colonne où la poser.
    /// @param label  Son libellé.
    /// @param filter Le filtre du sélecteur de fichier.
    /// @return Le champ.
    [[nodiscard]] QLineEdit* addFileRow(QVBoxLayout* column, const QString& label,
                                        const QString& filter);

    void reloadInstalled();
    void showDraft(const CharacterDraft& draft);
    void openInstalled(const InstalledCharacter& character);
    void newSheet();
    void chooseSheet();
    bool saveSheet(bool askName);
    void install();
    void checkInstalled();
    void editInBlender();
    void importFromBlender();
    /// @brief Lance @p command ; @p done reçoit son code de sortie.
    /// @param command La ligne de commande.
    /// @param what    Ce qui est lancé, pour le compte rendu.
    /// @param done    Appelé à la fin du processus.
    void run(const ProcessCommand& command, const QString& what, std::function<void(int)> done);

    void reloadPreview();
    void refreshPreview();
    void log(const QString& text);
    /// @return Le dossier d'où part la racine de la fiche : celui de son fichier, à défaut la
    ///         racine de l'atelier local.
    [[nodiscard]] std::filesystem::path baseDirectory() const;
    [[nodiscard]] std::filesystem::path sheetRoot() const;
    /// @return @p file tel que la fiche l'écrit : relatif à sa racine.
    [[nodiscard]] QString written(const QString& file) const;

    std::filesystem::path _dataRoot;
    std::filesystem::path _workshopRoot;
    /// Le fichier de la fiche ouverte ; vide tant qu'elle n'est pas enregistrée.
    std::filesystem::path _sheetFile;
    /// La racine écrite dans la fiche ouverte.
    std::string _root = ".";
    /// Ce que le fichier de retouche et le fichier Blender disent dans la fiche, s'ils y sont.
    CharacterWorkshopFiles _workshopExtras;
    std::string _skeletonSource;
    std::optional<core::SkeletonDescription> _skeleton;
    Widgets* _w;
    SceneSurface* _surface = nullptr;
    QProcess* _process = nullptr;
    QTimer* _clock = nullptr;
    QElapsedTimer _elapsed;
    /// Le modèle que l'aperçu montre, relatif à la racine du rendu.
    std::string _previewModel;
    int _quarterTurns = 0;
    /// L'instant d'aperçu retenu : celui où la lecture s'est arrêtée.
    float _heldSeconds = 0.0F;
};

}  // namespace hmi
