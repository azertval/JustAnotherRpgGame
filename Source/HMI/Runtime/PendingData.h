// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQmlIntegration>

#include "HMI/Runtime/SheetRowModel.h"

class QAbstractItemModel;

/**
 * @file HMI/Runtime/PendingData.h
 * @brief L'ancre des écrans dessinés mais pas encore alimentés (`LOT-86`).
 */

namespace hmi {

/**
 * @brief Le point unique vers lequel pointe **tout ce qui n'a pas encore de source**.
 *
 * ## Le problème que cette classe résout
 *
 * Plusieurs écrans du RPG — journal, dialogue, marchand, ATH de combat, et l'essentiel de l'équipe
 * de mercenaires — n'ont aujourd'hui **aucune donnée** : les lots qui les produiront n'existent pas
 * encore. Leur **mise en page**, elle, est décidée, et la jeter en attendant reviendrait à la
 * redessiner plus tard, différemment, sans que personne ne se souvienne de ce qui avait été
 * tranché.
 *
 * Ils sont donc dessinés maintenant, et chacun de leurs champs porte une **clé d'attribution**
 * nommée qui aboutit ici. Le jour où le lot fonctionnel arrive, il remplace `PendingData` par sa
 * vraie vue-modèle dans le fichier de câblage : **le formulaire ne bouge pas**. C'est précisément
 * ce que la séparation achète.
 *
 * ## Pourquoi un tiret cadratin et non de fausses données
 *
 * Un écran rempli de valeurs plausibles se prend pour un écran fini. Il passe les relectures, on
 * l'oublie, et un jour quelqu'un s'étonne que le marchand vende toujours les mêmes trois objets.
 * Le tiret cadratin est le même signe que le châssis pose sur un champ sans source : à l'écran,
 * « inconnu » et « pas encore alimenté » se ressemblent, et rien ne gagne à les distinguer.
 *
 * Les **valeurs d'exemple**, elles, vivent dans les formulaires `.ui.qml` : Qt Design Studio les
 * affiche, la conception juge sa mise en page dessus, et le jeu ne les voit jamais.
 *
 * ## Comment on retrouve ce qu'il reste à brancher
 *
 * `python scripts/i18n/list_pending_bindings.py` relève toutes les clés depuis le QML lui-même. Aucune
 * liste à tenir à jour en parallèle : l'inventaire est **dérivé du code**, donc toujours exact —
 * et c'est le motif que ce lot applique partout ailleurs.
 */
class PendingData : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit PendingData(QObject* parent = nullptr);

    /// @return Le tiret cadratin, quelle que soit @p key. @p key n'est pas ignorée pour autant :
    /// c'est elle qui nomme la donnée future, et que le relevé retrouve dans le QML.
    [[nodiscard]] Q_INVOKABLE QString value(const QString& key) const;

    /**
     * @brief Une source d'image **vide**, pour un portrait, une carte ou un blason sans source.
     *
     * Distincte de `value` et ce n'est pas un détail : le tiret cadratin qu'elle rend est un
     * TEXTE, et l'affecter à la source d'une image fait chercher un fichier nommé « — ». L'erreur
     * est apparue à la première exécution des trois écrans qui portent une illustration.
     *
     * Une URL vide laisse le bloc afficher son cadre et rien d'autre — il occupe donc déjà sa
     * place, ce qui permet de juger la mise en page avant que l'image existe.
     */
    [[nodiscard]] Q_INVOKABLE QUrl image(const QString& key) const;

    /**
     * @brief Un modèle de @p count lignes vides, pour un bloc de liste pas encore alimenté.
     *
     * Le modèle est **mis en cache par clé**. Sans cela, chaque réévaluation d'une liaison QML en
     * construirait un neuf : la vue se réinitialiserait en boucle, et la position de défilement
     * sauterait à chaque image — un défaut visuel dont la cause ne se lit nulle part.
     */
    [[nodiscard]] Q_INVOKABLE QAbstractItemModel* rows(const QString& key, int count);

private:
    QHash<QString, SheetRowModel*> _models;
};

}  // namespace hmi
