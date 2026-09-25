// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>
#include <map>
#include <memory>
#include <string>

/**
 * @file HMI/Runtime/InventoryModel.h
 * @brief Ce que le personnage porte, tel que le QML le lit et le modifie (`LOT-86`, `LOT-87`).
 */

namespace hmi {

struct DemonstrationState;

/**
 * @brief Vue-modèle de l'inventaire : emplacements, grille filtrée, sélection, charge, bourse.
 *
 * Même frontière que `hmi::CharacterSheetModel` : cette classe décide **quelles données
 * existent**, le QML décide **comment elles se voient**. Ce qu'elle calcule vient de fonctions
 * pures et testées (`hmi::inventoryValues`, `HMI/Presentation/InventoryScreen.h`).
 *
 * **Aucune statistique n'est stockée ici**, et c'est la règle du `LOT-14` qu'il ne faut pas
 * défaire : le poids porté, la capacité et la classe d'armure sont des *fonctions* du contenu.
 * Depuis la charte v2 (`LOT-87`, T3.5), l'écran **agit** — équiper, retirer, jeter, trier — :
 * chaque action modifie l'inventaire gardé ici, puis **tout** est recalculé depuis lui. Rien n'est
 * ajouté ni retranché à une valeur publiée.
 */
class InventoryModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// La bourse, déjà mise en forme (« 37 po ») ; un tiret cadratin tant que rien ne l'alimente.
    Q_PROPERTY(QString purse READ purse NOTIFY changed)
    Q_PROPERTY(QString carried READ carried NOTIFY changed)
    Q_PROPERTY(QString capacity READ capacity NOTIFY changed)
    Q_PROPERTY(QString backpack READ backpack NOTIFY changed)

    /// Ce que chaque emplacement porte, par identifiant (`main-hand`) : `{itemId, name}`.
    Q_PROPERTY(QVariantMap equipped READ equipped NOTIFY changed)

    /// Filtre de la grille : 0 tous, 1 équipement, 2 matériel, 3 outils (`hmi::ItemFamily`).
    Q_PROPERTY(int filter READ filter WRITE setFilter NOTIFY changed)
    /// Les piles du sac que le filtre retient : `{itemId, name, quantity}`.
    Q_PROPERTY(QVariantList cells READ cells NOTIFY changed)

    /// Sélection courante : un objet du sac (`itemId`), ou un emplacement porté (`slot`).
    Q_PROPERTY(QString selectedItem READ selectedItem NOTIFY changed)
    Q_PROPERTY(QString selectedSlot READ selectedSlot NOTIFY changed)
    /// La fiche de la sélection : `name`, `kind`, `damage`, `armor`, `weight`, `text`, et ce que
    /// l'écran peut en faire (`canEquip`, `canUnequip`, `canDrop`). Vide sans sélection.
    Q_PROPERTY(QVariantMap selection READ selection NOTIFY changed)

    /// Charge : `carried / capacity`, et son remplissage de 0 à 1 (0 si la capacité est inconnue).
    Q_PROPERTY(qreal loadRatio READ loadRatio NOTIFY changed)
    /// Pièces d'or de la bourse (le reste en argent et cuivre est dans `purse`).
    Q_PROPERTY(QString gold READ gold NOTIFY changed)
    /// Les quatre statistiques dérivées de la maquette, recalculées depuis ce qui est porté.
    Q_PROPERTY(QString armorClass READ armorClass NOTIFY changed)
    Q_PROPERTY(QString initiative READ initiative NOTIFY changed)
    Q_PROPERTY(QString speed READ speed NOTIFY changed)
    Q_PROPERTY(QString passivePerception READ passivePerception NOTIFY changed)

public:
    explicit InventoryModel(QObject* parent = nullptr);
    ~InventoryModel() override;

    /// Charge le personnage de démonstration. Voir `hmi::loadDemonstrationState` : la fiche et
    /// l'inventaire décrivent le même personnage, depuis les mêmes données.
    Q_INVOKABLE void loadDemonstrationCharacter();

    /// Sélectionne une pile du sac ; une chaîne vide efface la sélection.
    Q_INVOKABLE void selectItem(const QString& itemId);
    /// Sélectionne un emplacement porté (`main-hand`) ; vide ou inconnu efface la sélection.
    Q_INVOKABLE void selectSlot(const QString& slot);

    /// Équipe l'objet sélectionné du sac à son emplacement naturel. Sans effet s'il ne s'équipe
    /// pas.
    Q_INVOKABLE void equipSelected();
    /// Retire ce que porte l'emplacement sélectionné, et le range dans le sac.
    Q_INVOKABLE void unequipSelected();
    /// Jette un exemplaire de l'objet sélectionné du sac.
    Q_INVOKABLE void dropSelected();
    /// Trie le sac par nom.
    Q_INVOKABLE void sortBackpack();

    [[nodiscard]] QString purse() const {
        return value("inventory.purse");
    }
    [[nodiscard]] QString carried() const {
        return value("inventory.carried");
    }
    [[nodiscard]] QString capacity() const {
        return value("inventory.capacity");
    }
    [[nodiscard]] QString backpack() const {
        return value("inventory.backpack");
    }
    [[nodiscard]] QVariantMap equipped() const;
    [[nodiscard]] int filter() const noexcept {
        return _filter;
    }
    void setFilter(int filter);
    [[nodiscard]] QVariantList cells() const;
    [[nodiscard]] QString selectedItem() const {
        return _selectedItem;
    }
    [[nodiscard]] QString selectedSlot() const {
        return _selectedSlot;
    }
    [[nodiscard]] QVariantMap selection() const;
    [[nodiscard]] qreal loadRatio() const noexcept {
        return _loadRatio;
    }
    [[nodiscard]] QString gold() const;
    [[nodiscard]] QString armorClass() const {
        return sheetValue("sheet.armor_class");
    }
    [[nodiscard]] QString initiative() const {
        return sheetValue("sheet.initiative");
    }
    [[nodiscard]] QString speed() const {
        return sheetValue("sheet.speed");
    }
    [[nodiscard]] QString passivePerception() const {
        return sheetValue("sheet.passive_perception");
    }

signals:
    /// Émis quand l'inventaire change en bloc. Un signal unique, pour la même raison que sur la
    /// fiche : tout est recalculé d'un coup.
    void changed();

private:
    /// Recalcule toutes les valeurs publiées depuis l'état, puis prévient.
    void refresh();

    [[nodiscard]] QString value(const char* key) const;
    [[nodiscard]] QString sheetValue(const char* key) const;

    std::unique_ptr<DemonstrationState> _state;
    std::map<std::string, std::string> _values;
    std::map<std::string, std::string> _sheetValues;
    int _filter = 0;
    QString _selectedItem;
    QString _selectedSlot;
    qreal _loadRatio = 0.0;
};

}  // namespace hmi
