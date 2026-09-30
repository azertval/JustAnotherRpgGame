// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QWidget>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "Editor/Logic/QuestEditing.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTabWidget;
class QTableWidget;
class QTreeWidget;

/**
 * @file Editor/Ui/QuestsPanel.h
 * @brief Le panneau « Quests » : le mode Quêtes de l'éditeur (`LOT-144`, `EX-EDIT-100`).
 */

namespace hmi {

struct EditorReferences;

/// @brief Ce qu'un renommage ou un retrait récrira, calculé quand la fenêtre est prête (cartes
///        enregistrées) : le panneau ne le calcule pas trop tôt.
using PlanFactory = std::function<RefactorPlan()>;

/**
 * @brief Écrire une quête sans JSON, à côté des cartes qu'elle traverse.
 *
 * La liste des quêtes de `World/quests` ; pour la quête ouverte, trois onglets :
 *
 * - **Quest** : son nom, sa source, le titre du journal dans chaque langue, les drapeaux qu'elle
 *   déclare (valeurs `a|b|c`, initiale) ; renommer un drapeau ou une valeur passe par un plan ;
 * - **Steps** : les étapes dans l'ordre du récit — conditions choisies parmi les drapeaux et
 *   valeurs connus, effets, issue, lieu (`carte#id`), texte du journal — et **Play this step**,
 *   qui règle l'état de partie du canevas sur des valeurs qui l'atteignent ;
 * - **Uses** : pour un drapeau et une valeur, qui le lit ou le pose ; un double-clic mène à
 *   l'entité.
 *
 * Le panneau tient un brouillon ; « Save » l'écrit par `hmi::planSaveQuest`, qui refuse ce que le
 * jeu refuserait, l'erreur nommant l'étape. Textes en anglais, hors charte v2.
 */
class QuestsPanel : public QWidget {
    Q_OBJECT

public:
    explicit QuestsPanel(std::filesystem::path dataRoot, QWidget* parent = nullptr);

    /// @brief Les catalogues que les listes proposent : drapeaux posés, déclarés, entités.
    void setReferences(const EditorReferences* references);

    /// @brief Relit la liste des quêtes et la quête ouverte ; le brouillon est jeté.
    void reload();

    /// @return Vrai si le brouillon a des modifications non enregistrées.
    [[nodiscard]] bool isDirty() const noexcept {
        return _dirty;
    }

    /// @brief Demande quoi faire des modifications. @return `false` si l'auteur renonce.
    bool askAboutChanges();

signals:
    /// Un renommage ou un retrait, à montrer puis écrire par la fenêtre (`carryOutPlan`).
    void planRequested(const hmi::PlanFactory& makePlan, const QString& title);
    /// La quête vient d'être enregistrée : références et contrôle sont à relire.
    void questSaved();
    /// « Play this step » : l'état de partie qui atteint l'étape.
    void playStepRequested(const std::vector<std::string>& entries);
    /// Aller à une citation : une entité d'une carte, un fichier.
    void citationActivated(const hmi::Citation& citation);

private:
    [[nodiscard]] QWidget* buildQuestTab();
    [[nodiscard]] QWidget* buildStepsTab();
    [[nodiscard]] QWidget* buildUsesTab();

    void openQuest(const std::string& questId);
    void fillAll();
    void fillFlags();
    void fillSteps();
    void fillStep();
    void fillUseChoices();
    void showUses();
    void markDirty();
    void showStatus(const QString& text, bool error = false);
    void setEditable(bool editable);

    void readFlagRow(int row);
    void readConditionRow(int row);
    void readEffectRow(int row);
    void renameStep(const std::string& newId);
    void moveStep(int delta);

    void newQuest();
    void saveQuest();
    void renameQuest();
    void deleteQuest();
    void renameFlag();
    void renameFlagValue();
    void playStep();
    void goToStepPlace();

    /// Les drapeaux connus : posés par quelqu'un, et déclarés par le brouillon.
    [[nodiscard]] QStringList knownFlags() const;
    /// Les valeurs déclarées de @p flag : le brouillon d'abord, les autres quêtes ensuite.
    [[nodiscard]] QStringList declaredValues(const std::string& flag) const;
    /// Les déclarations de toutes les quêtes, celles du brouillon à la place des siennes.
    [[nodiscard]] std::vector<core::QuestFlag> allDeclarations() const;
    [[nodiscard]] core::QuestStep* currentStep();

    std::filesystem::path _root;
    const EditorReferences* _references = nullptr;
    std::vector<std::string> _languages;

    QuestDraft _draft;
    /// Les drapeaux tels qu'enregistrés : leur identifiant se renomme par un plan, pas à la main.
    std::vector<std::string> _savedFlags;
    bool _hasQuest = false;
    bool _isNew = false;
    bool _dirty = false;
    /// Vrai pendant qu'on remplit les widgets : leurs signaux ne sont pas des gestes.
    bool _filling = false;
    int _step = -1;
    /// La quête à rouvrir au prochain `reload` : celle qu'un renommage vient de nommer.
    std::string _selectAfterReload;
    /// Les usages des drapeaux, lus à la première recherche, jetés quand un fichier change.
    std::optional<std::vector<FlagUse>> _uses;

    QComboBox* _questList = nullptr;
    QLabel* _status = nullptr;
    QTabWidget* _pages = nullptr;
    QLineEdit* _name = nullptr;
    QLineEdit* _source = nullptr;
    std::map<std::string, QLineEdit*> _titles;
    QTableWidget* _flags = nullptr;
    QListWidget* _steps = nullptr;
    QWidget* _stepEditor = nullptr;
    QLineEdit* _stepId = nullptr;
    QComboBox* _stepAt = nullptr;
    QTableWidget* _conditions = nullptr;
    QTableWidget* _effects = nullptr;
    QComboBox* _outcome = nullptr;
    std::map<std::string, QLineEdit*> _stepTexts;
    QComboBox* _useFlag = nullptr;
    QComboBox* _useValue = nullptr;
    QTreeWidget* _useTree = nullptr;
    std::vector<QWidget*> _editors;
};

}  // namespace hmi
