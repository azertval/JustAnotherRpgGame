// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQmlIntegration>
#include <memory>

#include "HMI/Runtime/SheetRowModel.h"

/**
 * @file HMI/Runtime/DialogueModel.h
 * @brief Une conversation avec un PNJ, telle que l'écran de dialogue la montre et la fait avancer
 *        (`LOT-15`).
 */

namespace hmi {

/**
 * @brief La vue-modèle de l'écran de dialogue : elle tient un `core::DialogueRunner` et rien
 *        d'autre ne décide.
 *
 * ## Ce qu'elle tient
 *
 * Le catalogue des dialogues (`Source/Elements/World/dialogues`), l'échelle des degrés de
 * difficulté, le personnage de démonstration comme interlocuteur (`core::CharacterListener`) et
 * une conversation en cours. Chaque réponse cliquée devient `core::DialogueRunner::choose`, et ce
 * que l'écran affiche est relu par `hmi::dialogueScreenValues` après chaque geste.
 *
 * ## Ce qui est un échafaudage, et le dit
 *
 * Il n'existe ni partie ni sauvegarde (`LOT-17`) : les **drapeaux de monde** de la conversation
 * vivent donc le temps du processus, partagés par toutes les instances de ce modèle — rouvrir
 * l'écran retrouve le héraut qui vous a déjà vu. L'interlocuteur est le personnage de
 * démonstration, rechargé à chaque écran : un objet qu'un PNJ lui donne ne survit pas à la
 * fermeture. Le jour où une partie fournira ses drapeaux et son groupe, c'est ce fichier qui
 * change, pas le runner ni le formulaire.
 */
class DialogueModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// Le dialogue à jouer, par identifiant. L'écrire ouvre la conversation.
    Q_PROPERTY(QString dialogueId READ dialogueId WRITE setDialogueId NOTIFY changed)
    Q_PROPERTY(QString speakerName READ speakerName NOTIFY changed)
    Q_PROPERTY(QString attitude READ attitude NOTIFY changed)
    /// La réplique affichée, traduite ; le refus s'il n'y a pas de langue commune.
    Q_PROPERTY(QString line READ line NOTIFY changed)
    /// Le jet que la dernière réponse a joué, restitué ; vide sinon.
    Q_PROPERTY(QString checkOutcome READ checkOutcome NOTIFY changed)
    /// Le même jet, par morceaux (`LOT-117`) : « Persuasion · DD 15 », le d20 tiré (vide s'il ne
    /// l'a pas été), le calcul « 12 + 4 = 16 », l'issue, et si elle est une réussite.
    Q_PROPERTY(QString checkTitle READ checkTitle NOTIFY changed)
    Q_PROPERTY(QString checkDie READ checkDie NOTIFY changed)
    Q_PROPERTY(QString checkDetail READ checkDetail NOTIFY changed)
    Q_PROPERTY(QString checkVerdict READ checkVerdict NOTIFY changed)
    Q_PROPERTY(bool checkSucceeded READ checkSucceeded NOTIFY changed)
    /// Les réponses proposées : rôles `rowId`, `label` (le texte), `value` (le jet annoncé).
    Q_PROPERTY(QAbstractItemModel* replies READ replies CONSTANT)
    /// Vrai quand la conversation est terminée, ou quittée : l'écran se referme.
    Q_PROPERTY(bool finished READ finished NOTIFY changed)
    /// Ce qui empêche de jouer : catalogue illisible, dialogue inconnu. Vide si tout va bien.
    Q_PROPERTY(QString status READ status NOTIFY changed)
    /// Les dialogues jouables du contenu, par identifiant, dans l'ordre alphabétique : ce que le
    /// menu de développement (F9) propose d'ouvrir.
    Q_PROPERTY(QStringList dialogueIds READ dialogueIds CONSTANT)
    /// La graine de la **prochaine** conversation ouverte : 0, la valeur du jeu, la tire d'un
    /// compteur ; une autre la fixe — un test ou un rejeu force ainsi l'issue d'un jet (`LOT-120`),
    /// comme `EncounterModel.seed` fixe celle d'un combat.
    Q_PROPERTY(int seed READ seed WRITE setSeed NOTIFY changed)

public:
    explicit DialogueModel(QObject* parent = nullptr);
    ~DialogueModel() override;

    [[nodiscard]] QString dialogueId() const;
    void setDialogueId(const QString& id);
    [[nodiscard]] QString speakerName() const;
    [[nodiscard]] QString attitude() const;
    [[nodiscard]] QString line() const;
    [[nodiscard]] QString checkOutcome() const;
    [[nodiscard]] QString checkTitle() const;
    [[nodiscard]] QString checkDie() const;
    [[nodiscard]] QString checkDetail() const;
    [[nodiscard]] QString checkVerdict() const;
    [[nodiscard]] bool checkSucceeded() const noexcept;
    [[nodiscard]] QAbstractItemModel* replies() {
        return &_replies;
    }
    [[nodiscard]] bool finished() const noexcept;
    [[nodiscard]] QString status() const;
    [[nodiscard]] QStringList dialogueIds() const;
    [[nodiscard]] int seed() const noexcept {
        return _seed;
    }
    void setSeed(int seed);

    /// Donne la réponse @p rowId — ou quitte, si c'est la ligne « Quitter ».
    Q_INVOKABLE void choose(const QString& rowId);
    /// Donne la réponse de rang @p index (0 pour la première) : les touches `1` à `9`.
    Q_INVOKABLE void chooseAt(int index);
    /// Rouvre la même conversation depuis son entrée, drapeaux conservés.
    Q_INVOKABLE void restart();

signals:
    void changed();
    /// Le PNJ engage une rencontre **sur la carte** (`LOT-118`) : c'est l'écran qui la monte
    /// (`EncounterModel.begin`) et ouvre l'affichage de combat.
    void encounterRequested(const QString& encounterId);
    /// Le PNJ clôt la démo par la voie @p ending (`endDemo`, `LOT-119`) : c'est l'écran qui ouvre
    /// l'écran de fin.
    void demoEnded(const QString& ending);

private:
    struct Session;

    void open();
    void refresh();

    std::unique_ptr<Session> _session;
    int _seed = 0;
    SheetRowModel _replies;
};

}  // namespace hmi
