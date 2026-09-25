// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_quete_des_pommes.cpp
 * @brief Test d'intégration : la quête de la démo, « Des pommes pour l'arène », jouée sans fenêtre
 *        sur le **contenu livré** — les quatre cartes de principe (`LOT-146`), la quête, ses
 *        quatre dialogues et sa rencontre (`LOT-120`) —, par ses trois issues, à graine fixée.
 *
 * Ce qui s'éprouve ici est la chaîne entière telle que le jeu la joue : Market Gate → Stravian
 * Avenue → le parvis d'Arenarea et son déclencheur → le vestiaire A et sa porte close → le sable et
 * son combat → le retour à l'étal de la mère, par les seuls portails ; et l'équilibrage du combat,
 * par simulation à cent graines.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/Encounter.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Combat/MapEncounter.h"
#include "Core/Gameplay/Quest.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/Skill.h"
#include "Core/World/ExplorationSession.h"
#include "Core/World/WorldTravel.h"
#include "HMI/Game/CombatContestants.h"
#include "HMI/Game/GameQuests.h"
#include "HMI/Game/WorldPlay.h"

namespace {

const std::filesystem::path ELEMENTS{JADG_ELEMENTS_DIR};

constexpr std::string_view QUETE = "pommes";
constexpr std::string_view DRAPEAU = "quete.pommes";
constexpr std::string_view RENCONTRE = "arene-combattant";
constexpr std::string_view MARTPART = "central-empire/capital/martpart";
constexpr std::string_view ARENAREA = "central-empire/capital/arenarea";
constexpr std::string_view SABLE = "central-empire/capital/arenarea/arena-of-fate";
constexpr std::string_view VESTIAIRES = "central-empire/capital/arenarea/arena-of-fate/undercroft";
constexpr std::uint64_t GRAINE = 120;
constexpr float PAS = 1.0F / 60.0F;

// Les cases des cartes de principe : ou se tiennent les PNJ, et d'ou on leur parle.
constexpr core::GridPosition MERE{10, 5};
constexpr core::GridPosition DEVANT_LA_MERE{9, 5};
constexpr core::GridPosition ENFANT_CHEZ_SA_MERE{11, 5};
constexpr core::GridPosition STRAVIAN_AVENUE{22, 4};        // devant le portail vers Arenarea
constexpr core::GridPosition HEROFATE_AVENUE{1, 5};         // devant le portail vers Martpart
constexpr core::GridPosition ENTREE_DU_PARVIS{8, 5};        // une case avant la zone du parvis
constexpr core::GridPosition GARDE{12, 5};
constexpr core::GridPosition ENFANT_AU_PARVIS{12, 6};
constexpr core::GridPosition DEVANT_L_ESCALIER{22, 6};      // devant le portail vers l'arene
constexpr core::GridPosition ARRIVEE_AUX_VESTIAIRES{9, 3};  // le condamne arrive ici
constexpr core::GridPosition PORTE_DE_L_ARENE{11, 3};       // close sous condamne
constexpr core::GridPosition PIED_DE_L_ESCALIER{7, 2};      // sous la porte du triomphe
constexpr core::GridPosition MAITRE{8, 10};
constexpr core::GridPosition DEVANT_LE_MAITRE{9, 10};
constexpr core::GridPosition PORTE_DU_TRIOMPHE{16, 4};      // devant le portail vers le -1

/// Le heros de la demo, charge comme le jeu le charge (`hmi::loadDemonstrationState`), depuis le
/// contenu livre.
struct Heros {
    core::CharacterOptions options;
    core::SkillCatalog skills;
    core::ExperienceTable experience;
    core::CharacterCreationRules rules;
    core::LoadedCharacterSheet loaded;
    core::ItemCatalog items;
    core::EquipmentCatalog equipment;
    core::EncumbranceRules encumbrance;

    [[nodiscard]] hmi::HeroContestantSource source() const {
        const core::ItemLookup lookup{.items = &items, .equipment = &equipment};
        hmi::HeroContestantSource hero{
            .sheet = loaded.sheet,
            .proficiency = core::proficiencyBonus(loaded.sheet, experience),
            .armorClass = core::derivedStatsFor(loaded.sheet, loaded.inventory, lookup, rules,
                                                encumbrance)
                              .armorClass,
            .weapon = std::nullopt};
        if (const core::Weapon* weapon =
                equipment.findWeapon(loaded.inventory.at(core::EquipmentSlot::MainHand))) {
            hero.weapon = *weapon;
        }
        return hero;
    }
};

[[nodiscard]] Heros chargerLeHeros() {
    const std::filesystem::path rpg = ELEMENTS / "Rpg";
    Heros heros;
    heros.options =
        core::loadCharacterOptions(rpg / "species", rpg / "backgrounds", rpg / "classes");
    heros.skills = core::loadSkills(rpg / "skills");
    heros.experience = core::loadExperienceTable(rpg / "rules" / "experience.json");
    heros.rules = core::loadCharacterCreationRules(rpg / "rules" / "character-creation.json");
    heros.loaded = core::loadCharacterSheet(rpg / "characters" / "heros-brawler.json",
                                            heros.options, heros.rules, heros.experience);
    heros.items = core::loadItems(rpg / "items");
    heros.equipment = core::loadEquipment(rpg / "weapons", rpg / "armors");
    heros.encumbrance = core::loadEncumbranceRules(rpg / "rules" / "encumbrance.json");
    return heros;
}

/// Ce que le combat de l'arene demande au contenu livre.
struct Arene {
    core::EncounterCatalog encounters = core::loadEncounters(ELEMENTS / "Rpg" / "encounters");
    core::Bestiary bestiary = core::loadBestiary(ELEMENTS / "Rpg" / "creatures");
    core::BehaviorCatalog behaviors =
        core::loadBehaviors(ELEMENTS / "Rpg" / "rules" / "behaviors.json");
};

/**
 * @brief Joue la rencontre de l'arene sur le sable, les deux camps par l'IA, a la graine donnee.
 *
 * Le montage est celui de `hmi::EncounterModel::begin` : la zone de combat qui contient le PNJ,
 * le heros sur sa case, les combattants a leurs places, un combat letal dont on ne s'echappe pas.
 * @return L'issue, ou rien si le combat n'a pas pu se monter ou ne s'est pas termine.
 */
[[nodiscard]] std::optional<core::CombatOutcome> jouerLeCombat(
    const core::Level& sable, const Arene& arene, const hmi::HeroContestantSource& heros,
    std::uint64_t graine, core::EncounterRun* engagee = nullptr) {
    const core::Encounter* const rencontre = arene.encounters.find(RENCONTRE);
    if (rencontre == nullptr) {
        return std::nullopt;
    }
    const core::MapEncounterResult prepared = core::prepareMapEncounter(
        sable, SABLE, *rencontre, MAITRE, DEVANT_LE_MAITRE, core::ExplorationSnapshot{}, "");
    if (!prepared.ok()) {
        ADD_FAILURE() << prepared.issue;
        return std::nullopt;
    }
    const core::MapEncounterSetup& setup = *prepared.setup;
    if (engagee != nullptr) {
        *engagee = setup.run;
    }
    core::ArenaSession session(setup.battlefield);
    session.setOpportunityPolicy(core::aiOpportunityPolicy(arene.behaviors));
    core::ArenaBout bout{.contestants = {},
                         .seed = graine,
                         .lethal = true,
                         .heroicMark = false,
                         .flanking = false,
                         .escapable = setup.run.escapable};
    core::ArenaContestant hero = hmi::heroContestant(heros, core::CombatSide::Allies);
    hero.position = setup.heroCell;
    // Le joueur ne joue pas : l'IA tient sa place, avec le profil qui va au-devant de l'ennemi.
    hero.behavior = "aggressive";
    bout.contestants.push_back(std::move(hero));
    for (const core::CombatantPlacement& placement : setup.run.placements) {
        const core::Creature* const creature = arene.bestiary.find(placement.creatureId);
        if (creature == nullptr) {
            ADD_FAILURE() << "creature inconnue : " << placement.creatureId;
            return std::nullopt;
        }
        core::ArenaContestant enemy =
            hmi::creatureContestant(*creature, core::CombatSide::Enemies, &arene.behaviors);
        enemy.position = placement.position;
        bout.contestants.push_back(std::move(enemy));
    }
    const core::ArenaMount mount = session.mount(bout);
    if (mount.allies.empty() || mount.enemies.empty() || !session.start()) {
        ADD_FAILURE() << "montage refuse a la graine " << graine;
        return std::nullopt;
    }
    for (int tours = 0; tours < 600; ++tours) {
        if (session.combat().phase() == core::CombatPhase::Ended) {
            return session.outcome();
        }
        if (!core::playTurn(session, arene.behaviors)) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

/// Un interlocuteur qui parle le commun, dont on decide le jet, et qui note ce que le PNJ demande.
class Auditeur final : public core::DialogueListener {
public:
    explicit Auditeur(int persuasion) : _persuasion(persuasion) {}

    [[nodiscard]] bool speaks(std::string_view langue) const override {
        return langue == "common";
    }
    [[nodiscard]] std::vector<core::Modifier> skillModifiers(
        std::string_view competence) const override {
        if (competence == "persuasion") {
            return {core::Modifier{.source = "test", .value = _persuasion}};
        }
        return {};
    }
    void receiveItem(std::string_view, int) override {}
    void startEncounter(std::string_view rencontre) override {
        rencontres.emplace_back(rencontre);
    }
    void endDemo(std::string_view voie) override {
        fins.emplace_back(voie);
    }

    std::vector<std::string> rencontres;
    std::vector<std::string> fins;

private:
    int _persuasion;
};

/// Ce qu'un dialogue a demande a l'interlocuteur.
struct Demandes {
    std::vector<std::string> rencontres;
    std::vector<std::string> fins;
};

/// La partie telle que le jeu la monte : les cartes livrees, les quetes du demarrage, les
/// dialogues livres, l'echelle de difficulte des regles.
class Partie {
public:
    Partie()
        : _play(core::WorldTravel::directoriesLoader({ELEMENTS / "Levels"}), ELEMENTS / "Assets"),
          _dialogues(core::loadDialogues(ELEMENTS / "World" / "dialogues")),
          _echelle(core::loadDifficultyScale(ELEMENTS / "Rpg" / "rules" / "difficulty.json")) {
        hmi::GameQuests quetes = hmi::loadGameQuests(ELEMENTS);
        erreurs = quetes.errors;
        _play.session().setQuests(std::move(quetes.catalog));
    }

    [[nodiscard]] hmi::WorldPlay& play() {
        return _play;
    }
    [[nodiscard]] core::ExplorationSession& session() {
        return _play.session();
    }
    [[nodiscard]] const core::WorldFlags& drapeaux() {
        return _play.session().flags();
    }
    [[nodiscard]] std::string valeur() {
        return _play.session().flags().value(DRAPEAU).value_or("(non declare)");
    }

    /// Le heros en @p ou, tourne vers @p regard, interagit : le dialogue que la carte demande.
    [[nodiscard]] std::optional<std::string> parlerDepuis(core::GridPosition ou,
                                                          core::Vector2 regard = {1.0F, 0.0F}) {
        _play.session().placeHero(core::cellCenter(ou));
        static_cast<void>(_play.step(core::ExplorationIntent{.move = regard}, 0.0001F));
        const hmi::WorldPlayStep pas =
            _play.step(core::ExplorationIntent{.interact = true}, PAS);
        for (const core::ExplorationEvent& evenement : pas.events) {
            if (evenement.kind == core::ExplorationEventKind::Dialogue) {
                return evenement.value;
            }
        }
        return std::nullopt;
    }

    /// Marche depuis @p ou dans la direction @p sens, au plus deux secondes, jusqu'a l'evenement
    /// @p attendu. @return La valeur de l'evenement, ou rien s'il n'est pas venu.
    [[nodiscard]] std::optional<std::string> marcherJusquA(core::GridPosition ou,
                                                           core::Vector2 sens,
                                                           core::ExplorationEventKind attendu) {
        _play.session().placeHero(core::cellCenter(ou));
        for (int i = 0; i < 120; ++i) {
            const hmi::WorldPlayStep pas = _play.step(core::ExplorationIntent{.move = sens}, PAS);
            for (const core::ExplorationEvent& evenement : pas.events) {
                if (evenement.kind == attendu) {
                    return evenement.value;
                }
            }
        }
        return std::nullopt;
    }

    /// Joue le dialogue @p id avec les reponses @p reponses ; une replique sans reponse se passe
    /// d'elle-meme. @return Ce que le PNJ a demande a l'interlocuteur.
    Demandes converser(const std::string& id, const std::vector<std::string>& reponses,
                       int persuasion = 0) {
        Auditeur auditeur(persuasion);
        const core::DialogueGraph* graphe = _dialogues.find(id);
        if (graphe == nullptr) {
            ADD_FAILURE() << "dialogue inconnu : " << id;
            return {};
        }
        core::DeterministicRandom hasard{GRAINE};
        core::DialogueRunner runner(*graphe, _play.session().flags(), auditeur, _echelle, hasard);
        EXPECT_EQ(runner.start(), core::DialogueState::AwaitingChoice) << id;
        std::size_t rang = 0;
        for (int i = 0; i < 20 && runner.state() == core::DialogueState::AwaitingChoice; ++i) {
            const std::vector<core::AvailableChoice> proposees = runner.choices();
            if (proposees.size() == 1 && proposees.front().id == "continue") {
                static_cast<void>(runner.choose("continue"));
                continue;
            }
            if (rang >= reponses.size()) {
                ADD_FAILURE() << id << " : reponse manquante a la replique " << runner.lineKey();
                break;
            }
            const core::ChoiceResult resultat = runner.choose(reponses[rang++]);
            EXPECT_EQ(resultat, core::ChoiceResult::Advanced)
                << id << " : " << reponses[rang - 1] << " refusee a " << runner.lineKey();
        }
        EXPECT_EQ(runner.state(), core::DialogueState::Ended) << id;
        EXPECT_EQ(rang, reponses.size()) << id << " : des reponses n'ont pas servi";
        return Demandes{.rencontres = auditeur.rencontres, .fins = auditeur.fins};
    }

    /// Un pas sans geste : les consequences des drapeaux, et les etapes atteintes.
    [[nodiscard]] std::vector<std::string> etapesAtteintes() {
        const hmi::WorldPlayStep pas = _play.step(core::ExplorationIntent{}, PAS);
        std::vector<std::string> etapes;
        for (const core::ExplorationEvent& evenement : pas.events) {
            if (evenement.kind == core::ExplorationEventKind::QuestAdvanced) {
                etapes.push_back(evenement.value);
            }
        }
        return etapes;
    }

    std::vector<std::string> erreurs;

private:
    hmi::WorldPlay _play;
    core::DialogueCatalog _dialogues;
    core::DifficultyScale _echelle;
};

// Le debut commun aux trois issues : Market Gate, la mere, Stravian Avenue, le parvis et le garde.
void jusquAuGarde(Partie& partie) {
    ASSERT_TRUE(partie.erreurs.empty()) << partie.erreurs.front();
    ASSERT_NE(partie.session().quests().find(QUETE), nullptr);
    ASSERT_TRUE(partie.play().enter(MARTPART, {}));
    EXPECT_EQ(partie.valeur(), "inconnue");

    // La mere interpelle ; accepter ouvre la quete.
    ASSERT_EQ(partie.parlerDepuis(DEVANT_LA_MERE), "mere");
    static_cast<void>(partie.converser("mere", {"accepter"}));
    EXPECT_EQ(partie.etapesAtteintes(), (std::vector<std::string>{"pommes/acceptee"}));
    EXPECT_EQ(partie.parlerDepuis(DEVANT_LA_MERE), "mere") << "elle attend, l'etal ne bouge pas";

    // Stravian Avenue, par le portail : on arrive sur Herofate Avenue.
    ASSERT_EQ(partie.marcherJusquA(STRAVIAN_AVENUE, {1.0F, 0.0F},
                                   core::ExplorationEventKind::MapEntered),
              ARENAREA);
    EXPECT_EQ(partie.session().mapId(), ARENAREA);

    // Le parvis : sa zone declenche le garde a l'entree.
    ASSERT_EQ(partie.marcherJusquA(ENTREE_DU_PARVIS, {1.0F, 0.0F},
                                   core::ExplorationEventKind::Dialogue),
              "garde");
}

// Le retour a l'etal, depuis Herofate Avenue, et la fin de la demo par la voie attendue.
void retourChezLaMere(Partie& partie, std::string_view voie) {
    ASSERT_EQ(partie.marcherJusquA(HEROFATE_AVENUE, {-1.0F, 0.0F},
                                   core::ExplorationEventKind::MapEntered),
              MARTPART);
    EXPECT_EQ(partie.parlerDepuis(DEVANT_LA_MERE), "mere") << "la mere n'a pas quitte l'etal";
    EXPECT_EQ(partie.parlerDepuis({ENFANT_CHEZ_SA_MERE.column + 1, ENFANT_CHEZ_SA_MERE.row},
                                  {-1.0F, 0.0F}),
              "enfant")
        << "l'enfant est rentre aupres de sa mere";
    const Demandes fin = partie.converser("mere", {"sourire"});
    ASSERT_EQ(fin.fins.size(), 1U);
    EXPECT_EQ(fin.fins.front(), voie);
    EXPECT_EQ(partie.etapesAtteintes(), (std::vector<std::string>{"pommes/rendue"}));
    const core::Quest& quete = *partie.session().quests().find(QUETE);
    EXPECT_EQ(core::questProgress(quete, partie.drapeaux()).status,
              core::QuestStatus::Succeeded);
}

}  // namespace

/**
 * @brief La voie de la parole : la Persuasion réussit, l'enfant est libéré sur le parvis.
 * \castest{<b>La demo se finit par la parole quand la Persuasion reussit.</b><br/>
 * \tcat Integration · Quete de la demo<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Market Gate, la mere, accepter.<br/>2. Stravian Avenue par le portail ; le parvis
 * declenche le garde.<br/>3. Convaincre, avec un jet qui reussit.<br/>4. Revenir a l'etal par les
 * portails, parler a la mere.<br/>
 * \tattendu `enfant-libere` ; le garde et l'enfant ont quitte le parvis ; l'enfant est aupres de
 * sa mere ; la demo se clot par la voie `parole` ; la quete est reussie.
 * }
 */
TEST(QueteDesPommes, LaVoieDeLaParole) {
    Partie partie;
    ASSERT_NO_FATAL_FAILURE(jusquAuGarde(partie));

    static_cast<void>(partie.converser("garde", {"convaincre"}, /*persuasion=*/+20));
    EXPECT_EQ(partie.valeur(), "enfant-libere");
    EXPECT_EQ(partie.etapesAtteintes(), (std::vector<std::string>{"pommes/enfant-libere"}));
    EXPECT_EQ(partie.parlerDepuis({GARDE.column - 1, GARDE.row}), std::nullopt)
        << "le garde a quitte le parvis";
    EXPECT_EQ(partie.parlerDepuis({ENFANT_AU_PARVIS.column - 1, ENFANT_AU_PARVIS.row}),
              std::nullopt)
        << "l'enfant est parti retrouver sa mere";

    ASSERT_NO_FATAL_FAILURE(retourChezLaMere(partie, "parole"));
}

/**
 * @brief La voie de l'arène : la Persuasion échoue, le joueur endosse le crime, descend au
 *        vestiaire, monte sur le sable, gagne, et revient.
 * \castest{<b>La demo se finit par l'arene quand le joueur endosse le crime et gagne.</b><br/>
 * \tcat Integration · Quete de la demo<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Jusqu'au garde ; convaincre avec un jet qui echoue, puis endosser.<br/>2. L'escalier
 * de l'arene : on arrive au vestiaire A, et la porte du couloir arrete le pas.<br/>3. La porte du
 * triomphe : le sable ; le maitre d'arene engage la rencontre ; la jouer a la premiere graine qui
 * la gagne.<br/>4. Redescendre, passer la porte ouverte, revenir a l'etal.<br/>
 * \tattendu `persuasion-echouee` puis `condamne` ; le portail du -1 arrive au vestiaire ; la porte
 * bloque sous `condamne` et s'ouvre apres ; la victoire pose `enfant-libere` ; la demo se clot par
 * la voie `arene`.
 * }
 */
TEST(QueteDesPommes, LaVoieDeLArene) {
    Partie partie;
    ASSERT_NO_FATAL_FAILURE(jusquAuGarde(partie));

    // Le jet echoue : l'option disparait, il reste a endosser.
    static_cast<void>(partie.converser("garde", {"convaincre", "endosser"}, /*persuasion=*/-20));
    EXPECT_EQ(partie.valeur(), "condamne");
    EXPECT_EQ(partie.etapesAtteintes(),
              (std::vector<std::string>{"pommes/persuasion-echouee", "pommes/condamne"}));
    EXPECT_EQ(partie.parlerDepuis({GARDE.column - 1, GARDE.row}), std::nullopt)
        << "le garde a emmene le condamne";

    // L'escalier de l'arene mene au vestiaire A, derriere la porte close.
    ASSERT_EQ(partie.marcherJusquA(DEVANT_L_ESCALIER, {1.0F, 0.0F},
                                   core::ExplorationEventKind::MapEntered),
              VESTIAIRES);
    EXPECT_EQ(partie.session().heroCell(), ARRIVEE_AUX_VESTIAIRES);
    EXPECT_EQ(partie.marcherJusquA(ARRIVEE_AUX_VESTIAIRES, {1.0F, 0.0F},
                                   core::ExplorationEventKind::MapEntered),
              std::nullopt)
        << "la porte de l'arene est close : on ne revient pas au parvis";
    EXPECT_LT(partie.session().heroCell().column, PORTE_DE_L_ARENE.column);

    // La porte du triomphe : le sable, et le maitre d'arene.
    ASSERT_EQ(partie.marcherJusquA(PIED_DE_L_ESCALIER, {0.0F, -1.0F},
                                   core::ExplorationEventKind::MapEntered),
              SABLE);
    ASSERT_EQ(partie.parlerDepuis(DEVANT_LE_MAITRE), "maitre-arene");
    const Demandes defi = partie.converser("maitre-arene", {"combattre"});
    ASSERT_EQ(defi.rencontres, (std::vector<std::string>{std::string{RENCONTRE}}));

    // Le combat, sur le sable, a la premiere graine qui le gagne : la victoire pose son fait.
    const Heros heros = chargerLeHeros();
    ASSERT_TRUE(heros.loaded.errors.empty()) << heros.loaded.errors.front();
    const Arene arene;
    ASSERT_TRUE(arene.bestiary.find("combattant-de-l-arene") != nullptr);
    const core::Level* const sable = partie.session().map();
    ASSERT_NE(sable, nullptr);
    std::optional<std::uint64_t> gagnante;
    core::EncounterRun engagee;
    for (std::uint64_t graine = 1; graine <= 40 && !gagnante; ++graine) {
        if (jouerLeCombat(*sable, arene, heros.source(), graine, &engagee) ==
            core::CombatOutcome::Victory) {
            gagnante = graine;
        }
    }
    ASSERT_TRUE(gagnante.has_value()) << "aucune graine gagnante en quarante";
    static_cast<void>(
        core::endEncounter(engagee, core::CombatOutcome::Victory, partie.session().flags()));
    EXPECT_TRUE(partie.drapeaux().isSet(core::encounterWonFlag(RENCONTRE)));
    EXPECT_EQ(partie.etapesAtteintes(),
              (std::vector<std::string>{"pommes/victoire", "pommes/enfant-libere"}));
    EXPECT_EQ(partie.valeur(), "enfant-libere");
    EXPECT_EQ(partie.parlerDepuis(DEVANT_LE_MAITRE), std::nullopt) << "le maitre s'en est alle";

    // Le retour : l'escalier, la porte ouverte, le parvis, l'avenue, l'etal.
    ASSERT_EQ(partie.marcherJusquA(PORTE_DU_TRIOMPHE, {0.0F, -1.0F},
                                   core::ExplorationEventKind::MapEntered),
              VESTIAIRES);
    ASSERT_EQ(partie.marcherJusquA(ARRIVEE_AUX_VESTIAIRES, {1.0F, 0.0F},
                                   core::ExplorationEventKind::MapEntered),
              ARENAREA)
        << "la porte s'ouvre sous enfant-libere";
    EXPECT_EQ(partie.session().heroCell(), DEVANT_L_ESCALIER);
    ASSERT_NO_FATAL_FAILURE(retourChezLaMere(partie, "arene"));
}

/**
 * @brief La défaite : le combat de l'arène est létal, la démo s'y termine, la quête ne bouge pas.
 * \castest{<b>Une defaite sur le sable ne pose rien : la demo s'y termine.</b><br/>
 * \tcat Integration · Quete de la demo<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Condamne, sur le sable, la rencontre engagee.<br/>2. La jouer a la premiere graine
 * qui la perd.<br/>
 * \tattendu Ni le fait de la victoire ni `enfant-libere` ; la quete reste a `condamne`.
 * }
 */
TEST(QueteDesPommes, LaDefaiteSurLeSable) {
    Partie partie;
    ASSERT_TRUE(partie.erreurs.empty()) << partie.erreurs.front();
    ASSERT_TRUE(partie.session().flags().setValue(DRAPEAU, "condamne"));
    ASSERT_TRUE(partie.play().enter(SABLE, "from-undercroft"));
    ASSERT_EQ(partie.parlerDepuis(DEVANT_LE_MAITRE), "maitre-arene");

    const Heros heros = chargerLeHeros();
    const Arene arene;
    const core::Level* const sable = partie.session().map();
    ASSERT_NE(sable, nullptr);
    std::optional<std::uint64_t> perdante;
    core::EncounterRun engagee;
    for (std::uint64_t graine = 1; graine <= 40 && !perdante; ++graine) {
        if (jouerLeCombat(*sable, arene, heros.source(), graine, &engagee) ==
            core::CombatOutcome::Defeat) {
            perdante = graine;
        }
    }
    ASSERT_TRUE(perdante.has_value()) << "aucune graine perdante en quarante";
    static_cast<void>(
        core::endEncounter(engagee, core::CombatOutcome::Defeat, partie.session().flags()));
    EXPECT_FALSE(partie.drapeaux().isSet(core::encounterWonFlag(RENCONTRE)));
    EXPECT_TRUE(partie.etapesAtteintes().empty());
    EXPECT_EQ(partie.valeur(), "condamne");
}

/**
 * @brief L'équilibrage : sur cent combats simulés avec le héros de la démo, il l'emporte entre 60
 *        et 70 fois (critère du `LOT-120`).
 * \castest{<b>Le heros gagne le combat de l'arene entre 60 et 70 fois sur cent.</b><br/>
 * \tcat Integration · Quete de la demo · Equilibrage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Le sable, la rencontre de l'arene, le heros de la demo joue par l'IA.<br/>2. Cent
 * combats, aux graines 1 a 100.<br/>
 * \tattendu Chaque combat se termine ; entre 60 et 70 victoires.
 * }
 */
TEST(QueteDesPommes, LeCombatSeGagneDeuxFoisSurTrois) {
    const core::LevelLoadResult sable =
        core::WorldTravel::directoriesLoader({ELEMENTS / "Levels"})(SABLE);
    ASSERT_TRUE(sable.ok()) << sable.error;
    const Heros heros = chargerLeHeros();
    ASSERT_TRUE(heros.loaded.errors.empty()) << heros.loaded.errors.front();
    const hmi::HeroContestantSource source = heros.source();
    const Arene arene;

    int victoires = 0;
    for (std::uint64_t graine = 1; graine <= 100; ++graine) {
        const std::optional<core::CombatOutcome> issue =
            jouerLeCombat(*sable.level, arene, source, graine);
        ASSERT_TRUE(issue.has_value()) << "graine " << graine << " : pas d'issue";
        if (*issue == core::CombatOutcome::Victory) {
            ++victoires;
        }
    }
    EXPECT_GE(victoires, 60) << victoires << " victoires sur cent";
    EXPECT_LE(victoires, 70) << victoires << " victoires sur cent";
}

/**
 * @brief La probabilité de victoire tient sur mille graines tirées d'une graine maîtresse — un
 *        échantillon qu'on renouvelle en changeant la graine maîtresse, comme un fuzzer renouvelle
 *        ses entrées : deux tiers, à cinq points près.
 * \castest{<b>Sur mille combats a graines tirees, le heros gagne deux fois sur trois.</b><br/>
 * \tcat Integration · Quete de la demo · Equilibrage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mille graines tirees de la graine maitresse 120.<br/>2. Un combat par graine, les
 * deux camps par l'IA.<br/>
 * \tattendu Chaque combat se termine ; entre 600 et 700 victoires.
 * }
 */
TEST(QueteDesPommes, LaProbabiliteDeVictoireTientSurMilleGraines) {
    const core::LevelLoadResult sable =
        core::WorldTravel::directoriesLoader({ELEMENTS / "Levels"})(SABLE);
    ASSERT_TRUE(sable.ok()) << sable.error;
    const Heros heros = chargerLeHeros();
    ASSERT_TRUE(heros.loaded.errors.empty()) << heros.loaded.errors.front();
    const hmi::HeroContestantSource source = heros.source();
    const Arene arene;

    core::DeterministicRandom maitresse{GRAINE};
    int victoires = 0;
    for (int i = 0; i < 1000; ++i) {
        const auto graine = static_cast<std::uint64_t>(maitresse.nextInt(1, 1'000'000'000));
        const std::optional<core::CombatOutcome> issue =
            jouerLeCombat(*sable.level, arene, source, graine);
        ASSERT_TRUE(issue.has_value()) << "graine " << graine << " : pas d'issue";
        if (*issue == core::CombatOutcome::Victory) {
            ++victoires;
        }
    }
    ::testing::Test::RecordProperty("victoires_sur_mille", victoires);
    EXPECT_GE(victoires, 600) << victoires << " victoires sur mille";
    EXPECT_LE(victoires, 700) << victoires << " victoires sur mille";
}

/**
 * @brief Le jet de Persuasion du garde, au degré « moyenne », réussit une fois sur quatre au
 *        héros de la démo (Charisme 8, sans maîtrise) : la voie pacifique est une chance, la voie
 *        de l'arène est le chemin attendu.
 * \castest{<b>Sur deux mille jets a graines tirees, la Persuasion reussit une fois sur
 * quatre.</b><br/>
 * \tcat Integration · Quete de la demo · Equilibrage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Le modificateur de Persuasion du heros de la demo, par sa fiche.<br/>2. Deux mille
 * jets contre le DD du degre « moyenne », a graines tirees d'une graine maitresse.<br/>
 * \tattendu Entre 20 % et 30 % de reussites.
 * }
 */
TEST(QueteDesPommes, LaPersuasionReussitUneFoisSurQuatre) {
    Heros heros = chargerLeHeros();
    ASSERT_TRUE(heros.loaded.errors.empty()) << heros.loaded.errors.front();
    core::CharacterListener auditeur(heros.loaded.sheet, heros.loaded.inventory, heros.experience,
                                     heros.skills);
    const std::vector<core::Modifier> modificateurs = auditeur.skillModifiers("persuasion");
    const core::DifficultyScale echelle =
        core::loadDifficultyScale(ELEMENTS / "Rpg" / "rules" / "difficulty.json");
    const core::DifficultyTier* moyenne = nullptr;
    for (const core::DifficultyTier& degre : echelle.tiers) {
        if (degre.id == "moyenne") {
            moyenne = &degre;
        }
    }
    ASSERT_NE(moyenne, nullptr);

    core::DeterministicRandom maitresse{GRAINE};
    int reussites = 0;
    for (int i = 0; i < 2000; ++i) {
        core::DeterministicRandom hasard{
            static_cast<std::uint64_t>(maitresse.nextInt(1, 1'000'000'000))};
        if (core::rollCheck(moyenne->dc, modificateurs, core::RollStance::Normal, hasard)
                .succeeded()) {
            ++reussites;
        }
    }
    ::testing::Test::RecordProperty("reussites_sur_deux_mille", reussites);
    EXPECT_GE(reussites, 400) << reussites << " reussites sur deux mille";
    EXPECT_LE(reussites, 600) << reussites << " reussites sur deux mille";
}
