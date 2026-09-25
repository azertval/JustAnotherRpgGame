// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QtQmlIntegration>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "Core/Math/Vector2.h"
#include "Core/World/CityPlan.h"
#include "Core/World/ExplorationSession.h"
#include "HMI/Game/WorldPlay.h"
#include "HMI/Graphics/WorldSceneComposer.h"

/**
 * @file HMI/Runtime/WorldModel.h
 * @brief Le lieu qu'on parcourt, tel que l'écran de jeu le voit et le commande (`LOT-09`).
 */

namespace hmi {

/**
 * @brief La vue-modèle de l'exploration — **la partie en cours** : entrer sur une carte, marcher,
 *        parler, franchir.
 *
 * ## Ce qu'elle tient, et ce qu'elle ne décide pas
 *
 * Elle tient une `core::ExplorationSession` et fait tourner son **pas fixe** ; chaque geste de
 * l'écran devient une intention passée à la session, et ce que l'écran montre est relu d'elle.
 * Elle ne décide rien du monde : ni la collision, ni la traversée, ni ce qu'un PNJ répond.
 *
 * Même partage que le combat : la surface de rendu (`hmi::WorldViewportItem`)
 * prend un **instantané en valeurs** (`hmi::WorldSceneSnapshot`) dans `synchronize()`, pendant que
 * le fil graphique est bloqué. Le modèle ne lui passe jamais la session.
 */
class WorldModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    // **Un singleton, et c'est le fond du sujet** : la session d'exploration est LA PARTIE, pas un
    // objet de l'écran de jeu. La pile d'écrans ne garde qu'un écran vivant à la fois (`Loader`) ;
    // une session possédée par l'écran mourrait à l'ouverture du dialogue ou du Colisée, et l'on
    // reviendrait sur une carte neuve, héros à la porte. Ce qui est exactement ce que le lot
    // interdit : on revient au même endroit (`LOT-09`).
    QML_SINGLETON

    /// Identifiant de la carte courante, vide avant la première entrée.
    Q_PROPERTY(QString mapId READ mapId NOTIFY changed)
    /// Nom de la carte, tel qu'elle le porte.
    Q_PROPERTY(QString mapName READ mapName NOTIFY changed)
    /// Ce que l'écran doit dire quand rien ne s'affiche : carte absente, portail cassé.
    Q_PROPERTY(QString status READ status NOTIFY changed)
    /// Vrai si une carte est chargée et se joue.
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)
    Q_PROPERTY(int columns READ columns NOTIFY changed)
    Q_PROPERTY(int rows READ rows NOTIFY changed)
    /// La case du héros, en coordonnées **continues** : c'est ce que la caméra suit.
    Q_PROPERTY(qreal heroColumn READ heroColumn NOTIFY heroMoved)
    Q_PROPERTY(qreal heroRow READ heroRow NOTIFY heroMoved)
    /// La figurine du héros (`Assets/Npc/<slug>`). *Provisoire* : le héros créé à « Nouvelle
    /// partie » donnera la sienne (`LOT-43`), et cette valeur par défaut partira avec le reste du
    /// contenu provisoire.
    Q_PROPERTY(QString heroFigure READ heroFigure WRITE setHeroFigure NOTIFY changed)
    /// Vrai quand la carte est gelée : un dialogue ou un combat est à l'écran.
    Q_PROPERTY(bool frozen READ frozen WRITE setFrozen NOTIFY changed)
    /// La fiche d'atlas de la ville qu'on parcourt, qui est aussi la clé de son plan
    /// (`world-maps.json`) ; vide si la ville n'a pas pu être lue (`LOT-96`).
    Q_PROPERTY(QString cityLocation READ cityLocation CONSTANT)
    /// Le quartier où se tient le héros (sa fiche d'atlas), vide hors d'un quartier.
    Q_PROPERTY(QString districtId READ districtId NOTIFY changed)
    /// Les quartiers déjà parcourus depuis « Nouvelle partie », dans l'ordre de la première visite.
    /// Persistés au `LOT-17`.
    Q_PROPERTY(QStringList visitedDistricts READ visitedDistricts NOTIFY changed)

public:
    /// La ville où « Nouvelle partie » ouvre le jeu, sous `World/cities/` : la Capitale (`LOT-96`).
    static constexpr const char* START_CITY = "capital";
    /// Pas fixe de la simulation, en millisecondes.
    static constexpr int STEP_MILLISECONDS = 16;

    explicit WorldModel(QObject* parent = nullptr);
    ~WorldModel() override;

    /**
     * @brief Ouvre le jeu à la porte de départ de la ville (`World/cities/capital.json`), et
     *        oublie les quartiers visités. @return Vrai si la carte s'est ouverte.
     */
    Q_INVOKABLE bool startNewGame();

    /**
     * @brief La partie est finie (`LOT-119`) : mort, ou retour au menu depuis l'écran de fin.
     *
     * La session repart de zéro — aucune carte ouverte, drapeaux et quêtes oubliés, sauf ceux que
     * le lancement a posés (`--flags=`) — si bien que le prochain « Nouvelle partie », ou le
     * « Recommencer » de l'écran de mort, ouvre une partie **neuve** et non celle où l'on vient de
     * mourir. Les réglages du lancement (`--map=`, `--at=`, `--levels=`) demeurent.
     */
    Q_INVOKABLE void endGame();

    /// @brief Entre sur @p mapId au point d'arrivée @p arrival (vide : l'entrée de la carte).
    Q_INVOKABLE bool enterMap(const QString& mapId, const QString& arrival);

    /**
     * @brief Remplace la carte où « Nouvelle partie » ouvre le jeu — outil de développement
     *        (`--map=<carte>[@<arrivée>]`, `LOT-96`) : ouvrir Arenarea sans marcher depuis
     *        Martpart, pour la voir ou la capturer.
     */
    void setStartOverride(const QString& mapId, const QString& arrival);

    /**
     * @brief Pose les cartes du brouillon **devant** celles du binaire — l'essai complet de
     *        l'éditeur (`--levels=`, `LOT-EDITOR-10`).
     *
     * Les dossiers sont cherchés dans l'ordre, puis vient toujours le `Levels/` de l'exécutable :
     * on joue les cartes qu'on a sous les yeux, et celles qu'on n'édite pas restent celles du jeu.
     * À appeler **avant** la première entrée : la session est refaite.
     */
    void setLevelDirectories(const std::vector<std::filesystem::path>& directories);

    /**
     * @brief La case où « Nouvelle partie » pose le héros, à la place de l'entrée de la carte
     *        (`--at=`, `LOT-EDITOR-10`) : l'endroit de la carte qu'on veut voir, tout de suite.
     *        Vide : le héros part de l'entrée (le lanceur de cartes le remet ainsi).
     */
    void setStartCell(std::optional<core::GridPosition> cell);

    /**
     * @brief Les drapeaux de monde acquis avant le premier pas (`--flags=`, `LOT-EDITOR-10`) :
     *        la carte telle qu'elle est **après** une quête, sans avoir à la jouer.
     */
    void setStartFlags(const QStringList& flags);

    /// @return La carte du quartier @p districtId de la ville, vide s'il n'en a pas (`LOT-96`).
    [[nodiscard]] Q_INVOKABLE QString mapOfDistrict(const QString& districtId) const;

    /// @brief La direction que le joueur demande, de longueur au plus 1. Tenue jusqu'au prochain
    ///        appel : c'est l'état d'une touche enfoncée, pas un pas.
    Q_INVOKABLE void setMove(qreal x, qreal y);

    /// @brief Le joueur presse la touche d'interaction : le prochain pas la résoudra.
    Q_INVOKABLE void interact();

    [[nodiscard]] QString mapId() const;
    [[nodiscard]] QString mapName() const;
    [[nodiscard]] QString status() const {
        return _status;
    }
    [[nodiscard]] bool loaded() const;
    [[nodiscard]] int columns() const;
    [[nodiscard]] int rows() const;
    [[nodiscard]] qreal heroColumn() const;
    [[nodiscard]] qreal heroRow() const;

    [[nodiscard]] QString heroFigure() const {
        return QString::fromStdString(_play->heroFigure());
    }
    void setHeroFigure(const QString& figure);

    [[nodiscard]] bool frozen() const;
    void setFrozen(bool frozen);

    [[nodiscard]] QString cityLocation() const {
        return QString::fromStdString(_city.location);
    }
    [[nodiscard]] QString districtId() const;
    [[nodiscard]] QStringList visitedDistricts() const {
        return _visitedDistricts;
    }

    /// @return La carte que la surface de rendu dessine, partagée (`hmi::WorldPlay::scene`).
    [[nodiscard]] std::shared_ptr<const WorldSceneSnapshot> scene() const;
    /// @return Les figurines de l'image : les PNJ présents, puis le héros — ou, pendant un combat
    ///         sur la carte, les combattants (`setCombatFigures`, `LOT-118`).
    [[nodiscard]] std::vector<WorldFigureSnapshot> figures() const;

    /**
     * @brief Pendant un combat sur la carte (`LOT-118`), ce sont les **combattants** que la
     *        surface dessine, à la place des figurines de l'exploration : la carte reste, gelée,
     *        et la caméra suit @p heroPoint, le héros de la grille.
     */
    void setCombatFigures(std::vector<WorldFigureSnapshot> figures, core::Vector2 heroPoint);
    /// @brief Le combat est fini : les figurines de l'exploration reprennent.
    void clearCombatFigures();
    /// @return Vrai tant que des combattants tiennent lieu de figurines.
    [[nodiscard]] bool showsCombat() const noexcept {
        return _combatFigures.has_value();
    }
    /// @brief Pose le héros où le combat l'a laissé (`LOT-118`) ; la caméra suit.
    void placeHero(core::CellPoint point);
    /// @return La case de l'entité avec laquelle le héros a interagi en dernier — le PNJ dont le
    ///         dialogue engage la rencontre, l'entité `encounter` —, ou rien.
    [[nodiscard]] std::optional<core::GridPosition> lastInteractionCell() const noexcept {
        return _lastInteractionCell;
    }
    /// @return La carte qu'on parcourt et sa mise en scène : ce que le combat sur la carte lit.
    [[nodiscard]] const WorldPlay& play() const noexcept {
        return *_play;
    }
    /// @return La carte et ses figurines en une valeur (`hmi::WorldPlay::snapshot`).
    [[nodiscard]] WorldSceneSnapshot snapshot() const;
    [[nodiscard]] float diamondRatio() const;

    /**
     * @brief La partie en cours, s'il y en a une : ce que le dialogue et le journal lisent et
     *        écrivent (`LOT-116`). Le dernier `WorldModel` construit — un par moteur QML, un seul
     *        dans le jeu.
     */
    [[nodiscard]] static WorldModel* current() noexcept;

    /// @return Les drapeaux de la partie : ceux de la session, qui survivent au changement de
    /// carte.
    [[nodiscard]] core::WorldFlags& flags() noexcept {
        return _play->session().flags();
    }

    /// @return Les quêtes de la partie, lues au démarrage (`World/quests`).
    [[nodiscard]] const core::QuestCatalog& quests() const noexcept {
        return _play->session().quests();
    }

    /// @return Le numéro de la **carte** : il avance quand elle est à recomposer — entrée sur une
    ///         carte, drapeau qui change ce qui s'y dessine, figurine du héros.
    [[nodiscard]] quint64 sceneRevision() const noexcept {
        return _sceneRevision;
    }

    /// @return Le numéro des **figurines** : il avance à chaque pas qui change leur dessin — le
    ///         héros bouge, part, s'arrête, se tourne.
    [[nodiscard]] quint64 figuresRevision() const noexcept {
        return _figuresRevision;
    }

signals:
    /// La carte, son nom, son état : tout ce que l'écran relit d'un coup.
    void changed();
    /// Le héros a bougé : la caméra suit, la scène se redessine.
    void heroMoved();
    /// Les figurines ou la carte ont changé sans que le héros bouge — il s'arrête, se tourne, un
    /// drapeau fait paraître un PNJ : la scène se redessine.
    void figuresChanged();
    /// Le héros est entré sur une carte (`mapId`).
    void mapEntered(const QString& mapId);
    /// Il faut ouvrir le dialogue nommé — c'est l'écran qui l'ouvre, pas le modèle.
    void dialogueRequested(const QString& dialogueId);
    /// Il faut engager la rencontre nommée.
    void encounterRequested(const QString& encounterId);
    /// Un portail exige un drapeau que la partie n'a pas (`flag`).
    void portalLocked(const QString& flag);
    /// Un portail ne mène nulle part : la carte ou le point d'arrivée manque.
    void portalBroken(const QString& mapId);
    /// Un portail est condamné (`LOT-126`) : il est là, il ne s'ouvre pas.
    void portalSealed(const QString& mapId);
    /// Une quête a atteint une étape (`quest`) : le journal se relit.
    void questAdvanced(const QString& quest, const QString& step);

private:
    /// Un pas fixe : avance la session, joue ses événements, publie ce qui a changé.
    void step();
    /// Pose le héros sur la case de `--at=`, si elle est sur la carte (`LOT-EDITOR-10`).
    void placeHeroAtStartCell();
    /// Lit les quêtes de `World/quests` et les donne à la session (`LOT-116`).
    void installQuests();
    /// Refait la session sur les cartes de `_levelDirectories`, puis le `Levels/` du contenu.
    void rebuildSession();
    /// Pose @p flags sur la session (`--flags=`), sans les retenir.
    void applyFlags(const QStringList& flags);
    /// Retient le quartier de la carte courante parmi les quartiers visités.
    void noteDistrictVisit();

    /// La carte qu'on parcourt, et sa mise en scène — partagée avec l'essai de l'éditeur.
    std::unique_ptr<WorldPlay> _play;
    QTimer _clock;
    QString _status;
    /// Le graphe de la ville qu'on parcourt, lu une fois (`LOT-96`).
    core::CityPlan _city;
    QStringList _visitedDistricts;
    /// La carte et l'arrivée imposées par `--map=`, vides sinon.
    QString _startMapOverride;
    QString _startArrivalOverride;
    /// La case imposée par `--at=`, absente sinon (`LOT-EDITOR-10`).
    std::optional<core::GridPosition> _startCell;
    /// Les dossiers de `--levels=`, lus avant le `Levels/` du contenu (`LOT-EDITOR-10`).
    std::vector<std::filesystem::path> _levelDirectories;
    /// Les drapeaux de `--flags=` : reposés à chaque partie neuve (`endGame`).
    QStringList _startFlags;
    core::Vector2 _move{};
    bool _interact = false;
    quint64 _sceneRevision = 1;
    quint64 _figuresRevision = 1;
    /// Les combattants d'un combat sur la carte, tant qu'il dure (`LOT-118`).
    std::optional<std::vector<WorldFigureSnapshot>> _combatFigures;
    /// Le héros de la grille pendant ce combat : ce que la caméra suit.
    core::Vector2 _combatHero{};
    /// La case de la dernière interaction : d'où une rencontre engagée par un dialogue part.
    std::optional<core::GridPosition> _lastInteractionCell;
};

}  // namespace hmi
