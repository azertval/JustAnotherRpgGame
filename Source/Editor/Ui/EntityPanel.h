// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QString>
#include <QWidget>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core/Levels/LevelProperties.h"
#include "Core/Levels/MapEntity.h"
#include "Core/World/EntityKinds.h"
#include "Editor/Logic/EditorDiagnostics.h"

/**
 * @file Editor/Ui/EntityPanel.h
 * @brief Panneau « Entités » : famille à poser, liste filtrable des entités, propriétés de l'entité
 *        sélectionnée et avertissements (`LOT-11`, `LOT-EDITOR-05`).
 */

class QFormLayout;

namespace core {
class LevelDraft;
}

namespace hmi {

/**
 * @brief Vue des entités du brouillon courant — remplace le panneau « Propriétés » retiré au
 *        `LOT-01`.
 *
 * Même patron que `hmi::LinkPanel` : le panneau reflète et demande, le viewport applique. Le
 * formulaire est **dérivé** de `core::knownEntityKinds` : une famille ou une propriété ajoutée à la
 * table y apparaît sans toucher au panneau, avec le contrôle que sa nature appelle (liste, texte,
 * entier borné, case à cocher). Une propriété que la table ne déclare pas est montrée telle quelle,
 * sans contrôle : elle est transportée, pas éditée (`EX-EDIT-011`).
 *
 * La liste se filtre (`hmi::filterEntities`) et se sélectionne à plusieurs, comme le canevas : le
 * formulaire montre l'entité **principale**, la dernière prise, et son verdict : celui d'une zone
 * de combat face au groupe, le budget d'une rencontre pour le niveau du groupe choisi ici
 * (`LOT-EDITOR-05`, `LOT-143`, `EX-EDIT-072`, `EX-EDIT-073`).
 */
class EntityPanel : public QWidget {
    Q_OBJECT

public:
    explicit EntityPanel(QWidget* parent = nullptr);
    ~EntityPanel() override;

    /**
     * @brief Reconstruit liste, formulaire et avertissements.
     *
     * Le formulaire n'est refait que si l'entité sélectionnée, ses propriétés ou les choix proposés
     * ont changé : un coup de pinceau ailleurs sur la carte ne doit pas effacer un nom en cours de
     * saisie.
     */
    void refresh(const core::LevelDraft& draft, const std::vector<std::size_t>& selection,
                 std::optional<std::size_t> selected, const core::EntityReferenceContext& context,
                 const std::vector<EditorDiagnostic>& diagnostics, const std::string& verdict);

    /// @brief Montre @p level comme niveau du groupe du budget, sans émettre `partyLevelChanged`.
    void setPartyLevel(int level);

    /// @return Le type que l'outil « Entité » pose, vide en simple sélection.
    [[nodiscard]] std::string kindToPlace() const;

signals:
    void kindToPlaceChanged(const QString& type);
    void entitySelected(std::optional<std::size_t> index);
    /// La liste a changé la sélection : @p indices, dont @p primary est la dernière prise.
    void entitiesSelected(const std::vector<std::size_t>& indices,
                          std::optional<std::size_t> primary);
    void propertyChanged(std::size_t index, const QString& key, const core::PropertyValue& value);
    /// Retirer toutes les entités sélectionnées.
    void removeRequested();
    /// L'auteur a choisi le niveau du groupe dont se calcule le budget des rencontres.
    void partyLevelChanged(int level);

private:
    void rebuildKinds();
    void rebuildTable();
    void rebuildForm();
    /// Retire les lignes du formulaire ; leurs champs sont détruits au retour à la boucle.
    void clearForm();
    /**
     * @brief Ajoute au formulaire le champ d'une propriété déclarée.
     * @param index   Rang de l'entité dans la carte.
     * @param spec    Déclaration de la propriété.
     * @param value   Valeur portée par l'entité, ou la valeur par défaut.
     * @param choices Valeurs proposées, pour une propriété de choix.
     */
    void addPropertyRow(std::size_t index, const core::EntityPropertySpec& spec,
                        const core::PropertyValue& value, const std::vector<std::string>& choices);
    /**
     * @brief Ajoute au formulaire la liste déroulante d'une propriété de choix.
     * @param index   Rang de l'entité dans la carte.
     * @param spec    Déclaration de la propriété.
     * @param value   Valeur portée par l'entité, ou la valeur par défaut.
     * @param choices Valeurs proposées.
     */
    void addChoiceRow(std::size_t index, const core::EntityPropertySpec& spec,
                      const core::PropertyValue& value, const std::vector<std::string>& choices);
    void rebuildWarnings();

    /// Les widgets du panneau, construits en code (`EntityPanel.cpp`).
    struct Widgets;
    std::unique_ptr<Widgets> _ui;
    QFormLayout* _form;
    std::vector<core::MapEntity> _entities;
    std::vector<std::size_t> _selection;
    std::optional<std::size_t> _selected;
    /// Le verdict de l'entité principale (`hmi::entityVerdicts`), vide sinon.
    std::string _verdict;
    core::EntityReferenceContext _context;
    std::vector<EditorDiagnostic> _diagnostics;
    /// Ce que montre le formulaire, pour ne le refaire que s'il a changé.
    std::optional<std::size_t> _formIndex;
    std::optional<core::MapEntity> _formEntity;
    std::vector<std::vector<std::string>> _formChoices;
    bool _rebuilding = false;
};

}  // namespace hmi
