// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQmlIntegration>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Core/Combat/Arena.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/CharacterSheet.h"

/**
 * @file HMI/Runtime/ArenaModel.h
 * @brief Le Colisée, tel que l'écran de mise en place le voit et le commande (`LOT-50`).
 */

namespace hmi {

/**
 * @brief La vue-modèle de l'arène : composer deux camps, lancer, jouer, rejouer.
 *
 * ## Ce qu'elle tient, et ce qu'elle ne décide pas
 *
 * Elle tient une `core::ArenaSession` — la première session de jeu à tenir un
 * `core::CombatState` — et les catalogues dont l'écran compose son affrontement : le bestiaire
 * (`LOT-33`), le personnage de démonstration (`LOT-38`), les arènes et les Marques Héroïques.
 * Elle ne décide **rien** du combat : chaque geste de l'écran devient un appel à la session, et
 * ce que l'écran affiche est relu de la machine à états après chaque geste.
 *
 * ## Signal et notification de changement
 *
 * `changed` couvre tout ce que l'interface affiche : la composition, la grille, l'ordre, le journal
 * et l'issue changent ensemble, et les distinguer n'épargnerait aucun rafraîchissement à ces
 * propriétés, toutes relues d'un coup.
 *
 * La surface de rendu (`hmi::ArenaViewportItem`, `LOT-86` Phase 5+) n'a besoin de reprendre un
 * instantané que lorsque la grille de combat elle-même a pu changer — pas à chaque geste de
 * composition (enrôler, retirer, marquer, régler la graine ou l'IA), qui ne touche encore à aucune
 * session montée. `combatSceneChanged`, émis en plus de `changed` aux seuls gestes qui mutent la
 * grille (`core::ArenaSession::mount`, un déplacement, une attaque, un retrait, la fin du tour, un
 * rejeu), lui épargne ces recompositions inutiles.
 */
class ArenaModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// Nom de l'arène jouable, ou ce qui manque pour en avoir une.
    Q_PROPERTY(QString arenaName READ arenaName NOTIFY changed)
    /// Ce que l'écran doit dire : erreurs de chargement, refus de montage, issue.
    Q_PROPERTY(QString status READ status NOTIFY changed)
    /// Vrai entre `launch` et `backToSetup` : la grille se joue, la composition est figée.
    Q_PROPERTY(bool inCombat READ inCombat NOTIFY changed)
    /// Vrai quand le combat en cours a une issue.
    Q_PROPERTY(bool ended READ ended NOTIFY changed)
    /// Tout ce qu'on peut enrôler : `{id, name, kind, hitPoints, armorClass}`.
    Q_PROPERTY(QVariantList roster READ roster NOTIFY changed)
    /// Les alliés composés : `{id, name, mark}`.
    Q_PROPERTY(QVariantList allies READ allies NOTIFY changed)
    /// Les ennemis composés : `{id, name, mark}`.
    Q_PROPERTY(QVariantList enemies READ enemies NOTIFY changed)
    /// Les huit Marques Héroïques, par identifiant.
    Q_PROPERTY(QStringList marks READ marks NOTIFY changed)
    /// La graine du prochain lancement. Deux lancements à graine égale donnent le même déroulé.
    Q_PROPERTY(int seed READ seed WRITE setSeed NOTIFY changed)
    /// Vrai si les ennemis sont joués par l'IA (`LOT-23`) : leurs tours se jouent seuls, par les
    /// profils de `behaviors.json`. Faux : on commande les deux camps, comme au `LOT-50`.
    Q_PROPERTY(bool enemyAi READ enemyAi WRITE setEnemyAi NOTIFY changed)
    Q_PROPERTY(int gridColumns READ gridColumns NOTIFY changed)
    Q_PROPERTY(int gridRows READ gridRows NOTIFY changed)
    // --- Le calque d'interface de la grille -----------------------------------------------------
    // Ce que QRhi ne dessine pas : des losanges de jetons et du texte. Une entrée par combattant,
    // une par case atteignable — jamais une par case de la grille, dont les centaines de délégués
    // recréés à chaque geste faisaient monter la mémoire (`LOT-86` Phase 7).

    /// Les combattants sur la grille : `{column, row, footprint, side, active, down, hitPoints,
    /// hitPointsRatio}`. Les points de vie d'un ennemi restent secrets : `ensanglante`, `a terre`
    /// ou rien (Guide du Maître, chapitre 8).
    Q_PROPERTY(QVariantList fighters READ fighters NOTIFY changed)
    /// Les cases où le combattant actif peut finir son déplacement : `{column, row}`.
    Q_PROPERTY(QVariantList reachableCells READ reachableCells NOTIFY changed)
    // --- Le ciblage (`LOT-24`) -----------------------------------------------------------------
    // Tout ce qui suit le curseur a son propre signal, `cursorChanged` : un pas de curseur ne doit
    // pas reconstruire le calque de la grille, qui ne lit que `changed`. Un geste qui change le
    // combat émet `changed`, et `changed` entraîne `cursorChanged`.

    /// La case visée par le curseur de ciblage, au clavier et à la manette.
    Q_PROPERTY(int cursorColumn READ cursorColumn NOTIFY cursorChanged)
    Q_PROPERTY(int cursorRow READ cursorRow NOTIFY cursorChanged)
    /// Le chemin que le déplacement suivrait jusqu'au curseur : `{column, row}`, départ exclu.
    Q_PROPERTY(QVariantList pathCells READ pathCells NOTIFY cursorChanged)
    /// Ce que le combattant actif peut faire de son tour : `{label, kind, enabled, selected}`, où
    /// `kind` vaut `attack`, `dodge`, `disengage`, `dash` ou `reaction`. Les attaques d'abord,
    /// dans l'ordre du profil.
    Q_PROPERTY(QVariantList turnActions READ turnActions NOTIFY cursorChanged)
    /// Ce que le geste de confirmation ferait sur la case visée, ligne par ligne : chemin et
    /// attaques d'opportunité, ou attaque, jet requis, chance, CA, abri, sources.
    Q_PROPERTY(QStringList preview READ preview NOTIFY cursorChanged)
    /// L'ordre d'initiative : `{name, total, side, active, down}`.
    Q_PROPERTY(QVariantList turnOrder READ turnOrder NOTIFY changed)
    Q_PROPERTY(QString activeName READ activeName NOTIFY changed)
    /// Les ressources restantes du combattant actif, lisibles.
    Q_PROPERTY(QString activeResources READ activeResources NOTIFY changed)
    Q_PROPERTY(QStringList journal READ journal NOTIFY changed)

public:
    /**
     * @brief Construit la vue-modèle et lit ses catalogues.
     * @param parent      Parent Qt.
     * @param contentRoot Racine du **contenu** — les arènes (`World/arena`) et leurs cartes
     *                    (`Levels/`). Vide : à côté de l'exécutable, comme le jeu. Les tests y
     *                    mettent la racine d'essai, faute de contenu livré depuis le `LOT-102`.
     */
    explicit ArenaModel(QObject* parent = nullptr, std::filesystem::path contentRoot = {});
    ~ArenaModel() override;

    [[nodiscard]] QString arenaName() const;
    [[nodiscard]] QString status() const;
    [[nodiscard]] bool inCombat() const noexcept;
    [[nodiscard]] bool ended() const;
    [[nodiscard]] QVariantList roster() const;
    [[nodiscard]] QVariantList allies() const;
    [[nodiscard]] QVariantList enemies() const;
    [[nodiscard]] QStringList marks() const;
    [[nodiscard]] int seed() const noexcept;
    void setSeed(int seed);
    [[nodiscard]] bool enemyAi() const noexcept;
    void setEnemyAi(bool enabled);
    [[nodiscard]] int gridColumns() const;
    [[nodiscard]] int gridRows() const;
    [[nodiscard]] QVariantList fighters() const;
    [[nodiscard]] QVariantList reachableCells() const;
    [[nodiscard]] int cursorColumn() const noexcept {
        return _cursor.column;
    }
    [[nodiscard]] int cursorRow() const noexcept {
        return _cursor.row;
    }
    [[nodiscard]] QVariantList pathCells() const;
    [[nodiscard]] QVariantList turnActions() const;
    [[nodiscard]] QStringList preview() const;
    [[nodiscard]] QVariantList turnOrder() const;
    [[nodiscard]] QString activeName() const;
    [[nodiscard]] QString activeResources() const;
    [[nodiscard]] QStringList journal() const;

    /**
     * @brief La session, en lecture seule, pour la surface de rendu (`hmi::ArenaViewportItem`).
     *
     * Pas une propriété QML : le QML n'a rien à lire d'une session. Elle vit sur le fil graphique ;
     * le rendu ne la lit que dans `synchronize()`, fil graphique bloqué, et n'en garde qu'un
     * instantané en valeurs (`hmi::snapshotArenaScene`) — jamais ce pointeur.
     * @return La session, `nullptr` si les catalogues n'ont donné aucune arène jouable.
     */
    [[nodiscard]] const core::ArenaSession* session() const noexcept {
        return _session.get();
    }

    /// Enrôle une entrée du `roster` dans un camp. Sans effet pendant un combat.
    Q_INVOKABLE void addAlly(const QString& id);
    Q_INVOKABLE void addEnemy(const QString& id);
    Q_INVOKABLE void removeAlly(int index);
    Q_INVOKABLE void removeEnemy(int index);
    /// Attribue une Marque Héroïque à un allié ou un ennemi déjà composé.
    Q_INVOKABLE void assignMark(bool ally, int index, const QString& markId);
    /// Monte l'affrontement et jette l'initiative. Refuse une composition sans les deux camps.
    Q_INVOKABLE void launch();
    /// Le geste sur une case : déplacer le combattant actif si elle est atteignable, attaquer si
    /// elle porte un ennemi à portée. Le curseur s'y pose.
    Q_INVOKABLE void tapCell(int column, int row);
    /// Déplace le curseur d'une case, sans sortir de la grille.
    Q_INVOKABLE void moveCursor(int columns, int rows);
    /// Pose le curseur sur une case (le survol de la souris). Hors de la grille, ou déjà là : rien.
    Q_INVOKABLE void pointCursor(int column, int row);
    /// Ramène le curseur sur le combattant actif.
    Q_INVOKABLE void centerCursor();
    /// Pose le curseur sur l'ennemi debout suivant (@p step = 1) ou précédent (-1), du plus proche
    /// au plus lointain.
    Q_INVOKABLE void cycleTarget(int step);
    /// Choisit l'action du tour d'indice @p index (`turnActions`).
    Q_INVOKABLE void selectAction(int index);
    Q_INVOKABLE void cycleAction(int step);
    /// Joue l'action choisie : une attaque ou un déplacement sur la case visée ; esquiver, se
    /// désengager, se précipiter ; ou basculer la réaction.
    Q_INVOKABLE void confirm();
    /// L'action *esquiver* du combattant actif.
    Q_INVOKABLE void dodge();
    /// L'action *se désengager* du combattant actif.
    Q_INVOKABLE void disengage();
    /// L'action *se précipiter* du combattant actif.
    Q_INVOKABLE void dash();
    Q_INVOKABLE void endTurn();
    Q_INVOKABLE void withdraw();
    /// Remonte le même affrontement à la même graine.
    Q_INVOKABLE void replay();
    /// Revient à la composition, sans la perdre.
    Q_INVOKABLE void backToSetup();

signals:
    void changed();
    /// Le curseur, l'action choisie, la prévisualisation ou le chemin ont changé.
    void cursorChanged();
    /// `changed` restreint à ce qui peut avoir mué la grille de combat : la surface de rendu
    /// (`hmi::ArenaViewportItem`) s'y abonne seule, pour ne reprendre un instantané qu'à ces
    /// gestes-là plutôt qu'à chaque changement de composition.
    void combatSceneChanged();

private:
    struct Fighter;
    struct Catalogs;

    void loadCatalogs();
    /// Charge le personnage de démonstration, par le même chemin que la fiche (LOT-87).
    void loadCharacterCatalog();
    /// Charge la carte jouable de l'arène, réduite à sa zone de combat le cas échéant (LOT-09).
    /// @return true si `_catalogs->level` porte une carte jouable.
    [[nodiscard]] bool loadPlayableLevel();
    /// `emit changed()` puis `emit combatSceneChanged()` : aux gestes qui mutent la grille de
    /// combat (`core::ArenaSession::mount`, un déplacement, une attaque, un retrait, la fin du
    /// tour, un rejeu, un retour à une composition neuve).
    void emitSceneChanged();
    [[nodiscard]] std::optional<Fighter> fighterFor(const QString& id, core::CombatSide side) const;
    [[nodiscard]] core::ArenaBout composeBout() const;
    void refreshMessage(const core::ArenaMount& mount);
    /// Une session neuve sur la carte de l'arène, l'IA décidant des opportunités de ses créatures.
    void resetSession();
    /// Joue les tours de l'IA tant que le combattant actif en a un profil : le joueur reprend la
    /// main à son tour, ou à l'issue.
    void playAiTurns();
    /// Attaque @p target avec l'attaque @p index, ou la première qui peut la viser ; écrit l'issue.
    void attackAt(core::CombatantId target, std::optional<std::size_t> index);
    void moveTo(core::GridPosition cell);
    /// Le combattant actif s'il est commandé par le joueur.
    [[nodiscard]] std::optional<core::CombatantId> playerTurn() const;
    /// Recale le curseur et l'action choisie sur le combattant actif, à chaque changement de tour.
    void followActive();

    std::filesystem::path _contentRoot;
    std::unique_ptr<Catalogs> _catalogs;
    core::GridPosition _cursor{};
    int _selectedAction = 0;
    std::optional<core::CombatantId> _followed;
    std::unique_ptr<core::ArenaSession> _session;
    std::vector<Fighter> _allies;
    std::vector<Fighter> _enemies;
    int _seed = 2026;
    bool _enemyAi = true;
    bool _inCombat = false;
    QString _status;
};

}  // namespace hmi
