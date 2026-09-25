// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_interaction.cpp
 * @brief Tests des entites de carte et de l'interaction (LOT-10).
 *
 * Les trois criteres d'acceptation du lot ont chacun leur cas : ouvrir un coffre deux fois ne
 * donne le butin qu'une fois Y COMPRIS APRES ALLER-RETOUR DE CARTE ; l'interaction ne traverse
 * pas un mur ; et a deux cibles a portee, celle designee est deterministe.
 */

#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Ecs/Components/Interactable.h"
#include "Core/Ecs/Components/Transform.h"
#include "Core/Ecs/World.h"
#include "Core/Gameplay/Interaction.h"
#include "Core/Gameplay/MapEntitySpawner.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"

namespace {

constexpr int LARGEUR = 6;
constexpr int HAUTEUR = 4;

// Une carte vide, sauf ce que le test y pose.
[[nodiscard]] core::TileMap carteVide() {
    return core::TileMap(LARGEUR, HAUTEUR);
}

[[nodiscard]] core::Interactable coffre(int colonne, int ligne, std::string cle) {
    core::Interactable objet;
    objet.type = "chest";
    objet.position = {colonne, ligne};
    objet.consumedFlag = std::move(cle);
    objet.promptKey = "interaction.chest";
    return objet;
}

[[nodiscard]] core::Interactable panneau(int colonne, int ligne) {
    core::Interactable objet;
    objet.type = "sign";
    objet.position = {colonne, ligne};
    objet.promptKey = "interaction.sign";
    return objet;
}

[[nodiscard]] std::vector<core::InteractionCandidate> candidats(
    const std::vector<core::Interactable>& objets) {
    std::vector<core::InteractionCandidate> liste;
    for (std::size_t i = 0; i < objets.size(); ++i) {
        liste.push_back({&objets[i], i});
    }
    return liste;
}

// Le personnage se tient au CENTRE de la case (colonne, ligne), en cases.
[[nodiscard]] core::Vector2 auCentre(int colonne, int ligne) {
    return {static_cast<float>(colonne) + 0.5F, static_cast<float>(ligne) + 0.5F};
}

}  // namespace

/**
 * @brief La case visee est la voisine dans la direction dominante, jamais une diagonale.
 * \castest{<b>La case visee suit la direction dominante de l'orientation.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Viser avec quatre orientations cardinales, puis une orientation a 30 degres.<br/>
 * 2. Viser avec une orientation nulle.<br/>
 * \tattendu Les quatre cardinales donnent la voisine attendue ; l'orientation oblique donne la
 * case de la composante dominante et non la diagonale ; l'orientation nulle ne vise rien.
 * }
 */
TEST(InteractionTest, LaCaseViseeSuitLaDirectionDominante) {
    const core::GridPosition depart{2, 2};
    EXPECT_EQ(core::aimedCell(depart, {1.0F, 0.0F}), (core::GridPosition{3, 2}));
    EXPECT_EQ(core::aimedCell(depart, {-1.0F, 0.0F}), (core::GridPosition{1, 2}));
    EXPECT_EQ(core::aimedCell(depart, {0.0F, 1.0F}), (core::GridPosition{2, 3}));
    EXPECT_EQ(core::aimedCell(depart, {0.0F, -1.0F}), (core::GridPosition{2, 1}));

    // 30 degres : la composante horizontale domine. Viser en diagonale rendrait la cible
    // imprevisible a la manette analogique, alors que le joueur doit savoir ce qu'il designe.
    EXPECT_EQ(core::aimedCell(depart, {0.866F, 0.5F}), (core::GridPosition{3, 2}));
    EXPECT_EQ(core::aimedCell(depart, {0.5F, 0.866F}), (core::GridPosition{2, 3}));

    // Orientation nulle : rien n'est vise. Rendre une voisine arbitraire ferait ouvrir un coffre
    // que le joueur ne regarde pas.
    EXPECT_EQ(core::aimedCell(depart, {0.0F, 0.0F}), depart);
}

/**
 * @brief Ouvrir un coffre deux fois ne donne le butin qu'une fois.
 * \castest{<b>Un coffre ne donne son butin qu'a la premiere ouverture.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Interagir deux fois avec le meme coffre.<br/>
 * \tattendu La premiere interaction consomme, la seconde ne trouve plus de cible.
 * }
 */
TEST(InteractionTest, UnCoffreNeDonneSonButinQuUneFois) {
    const core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;
    const std::vector<core::Interactable> objets = {coffre(3, 2, "village/chest@3,2")};

    const core::InteractionTarget premiere = core::findInteractionTarget(
        auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets), drapeaux);
    ASSERT_TRUE(premiere.found());
    const core::InteractionOutcome butin = core::interact(premiere, drapeaux);
    EXPECT_TRUE(butin.happened);
    EXPECT_TRUE(butin.consumed) << "la premiere ouverture donne le butin";
    EXPECT_EQ(butin.type, "chest");

    // La seconde ne trouve MEME PLUS de cible : un coffre vide n'est plus une cible, et continuer
    // a l'afficher comme telle promettrait au joueur quelque chose qui n'arrivera pas.
    const core::InteractionTarget seconde = core::findInteractionTarget(
        auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets), drapeaux);
    EXPECT_FALSE(seconde.found());
    const core::InteractionOutcome rien = core::interact(seconde, drapeaux);
    EXPECT_FALSE(rien.happened);
}

/**
 * @brief L'etat d'un coffre survit a un aller-retour de carte.
 * \castest{<b>Un coffre ouvert le reste apres avoir quitte la carte et y etre revenu.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir un coffre, puis DETRUIRE les entites de la carte et les recreer depuis la
 * couche objects, comme le fait un changement de carte.<br/>
 * \tattendu Le coffre recree n'est plus une cible : l'etat vit dans les drapeaux de monde, pas
 * dans l'entite.
 * }
 */
TEST(InteractionTest, LEtatDUnCoffreSurvitAUnAllerRetourDeCarte) {
    const core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;

    // `tileMap` n'a volontairement pas de defaut : l'omettre est une erreur de compilation, pas
    // une grille vide silencieuse (LOT-03).
    core::LevelData donnees{.name = "village", .tileMap = carteVide()};
    donnees.entities.push_back({"chest", {3, 2}, {}});

    // Premier passage : le monde est peuple depuis la couche objects, le coffre est ouvert.
    {
        core::World monde;
        const core::Level niveau{donnees};
        ASSERT_EQ(core::spawnMapEntities(monde, niveau, niveau.name()), 1U);
        std::vector<core::Interactable> objets;
        for (auto [entite, interactif] : monde.view<core::Interactable>()) {
            objets.push_back(interactif);
        }
        ASSERT_EQ(objets.size(), 1U);
        const core::InteractionTarget cible = core::findInteractionTarget(
            auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets), drapeaux);
        ASSERT_TRUE(cible.found());
        EXPECT_TRUE(core::interact(cible, drapeaux).consumed);
    }

    // Second passage : un monde NEUF, peuple depuis le meme fichier de carte. L'entite est
    // reconstruite a l'identique et ne sait rien de ce qui s'est passe -- c'est precisement
    // pourquoi l'etat ne peut pas vivre en elle.
    {
        core::World monde;
        const core::Level niveau{donnees};
        ASSERT_EQ(core::spawnMapEntities(monde, niveau, niveau.name()), 1U);
        std::vector<core::Interactable> objets;
        for (auto [entite, interactif] : monde.view<core::Interactable>()) {
            objets.push_back(interactif);
        }
        ASSERT_EQ(objets.size(), 1U);
        const core::InteractionTarget cible = core::findInteractionTarget(
            auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets), drapeaux);
        EXPECT_FALSE(cible.found())
            << "un drapeau porte par l'entite disparaitrait avec elle, et le coffre redonnerait "
               "son butin a chaque passage";
    }
}

/**
 * @brief L'interaction ne traverse pas un mur.
 * \castest{<b>Un objet place sur une case pleine n'est pas atteignable.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser un coffre sur une case, puis rendre cette case pleine.<br/>
 * \tattendu Le coffre est atteignable avant, et ne l'est plus apres.
 * }
 */
TEST(InteractionTest, LInteractionNeTraversePasUnMur) {
    core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;
    const std::vector<core::Interactable> objets = {coffre(3, 2, "village/chest@3,2")};

    EXPECT_TRUE(core::findInteractionTarget(auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets),
                                            drapeaux)
                    .found());

    carte.setTile(3, 2, core::TileType::Wall);
    const core::InteractionTarget derriereLeMur = core::findInteractionTarget(
        auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets), drapeaux);
    EXPECT_FALSE(derriereLeMur.found())
        << "ouvrir a travers une cloison se joue et ne se diagnostique pas";
    EXPECT_EQ(derriereLeMur.aimedCell, (core::GridPosition{3, 2}))
        << "la case visee est rendue meme sans cible : l'invite visuelle en a besoin";
}

/**
 * @brief A deux cibles sur la case visee, la designee est deterministe.
 * \castest{<b>Deux objets sur la meme case designent toujours le meme.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser deux objets sur la case visee et designer la cible dix fois.<br/>
 * \tattendu Le meme est designe a chaque fois -- le premier de la liste, a distance egale.
 * }
 */
TEST(InteractionTest, LaCibleEstDeterministeAEgalite) {
    const core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;
    const std::vector<core::Interactable> objets = {coffre(3, 2, "village/chest@3,2"),
                                                    panneau(3, 2)};

    for (int essai = 0; essai < 10; ++essai) {
        const core::InteractionTarget cible = core::findInteractionTarget(
            auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets), drapeaux);
        ASSERT_TRUE(cible.found());
        EXPECT_EQ(cible.index, 0U)
            << "sans depart, l'ordre de parcours de l'ECS -- qui n'est pas stable -- deciderait";
        EXPECT_EQ(cible.interactable->type, "chest");
    }
}

/**
 * @brief Un panneau se relit indefiniment.
 * \castest{<b>Une entite non consommable reste une cible apres interaction.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Interagir trois fois avec un panneau.<br/>
 * \tattendu Les trois interactions ont lieu, aucune ne consomme, et aucun drapeau n'est leve.
 * }
 */
TEST(InteractionTest, UnPanneauSeRelitIndefiniment) {
    const core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;
    const std::vector<core::Interactable> objets = {panneau(3, 2)};

    for (int essai = 0; essai < 3; ++essai) {
        const core::InteractionTarget cible = core::findInteractionTarget(
            auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets), drapeaux);
        ASSERT_TRUE(cible.found()) << "essai " << essai;
        const core::InteractionOutcome lecture = core::interact(cible, drapeaux);
        EXPECT_TRUE(lecture.happened);
        EXPECT_FALSE(lecture.consumed);
        EXPECT_EQ(lecture.promptKey, "interaction.sign");
    }
    EXPECT_EQ(drapeaux.size(), 0U) << "un panneau ne leve aucun drapeau";
}

/**
 * @brief Deux cartes portent un coffre a la meme case sans se marcher dessus.
 * \castest{<b>Deux coffres de cartes differentes a la meme case ont des drapeaux
 * distincts.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Fabriquer la cle de deux coffres a la case (3, 2), sur deux cartes.<br/>
 * 2. Ouvrir le premier.<br/>
 * \tattendu Les deux cles different, et le second coffre reste fermable.
 * }
 */
TEST(InteractionTest, DeuxCartesNeSeMarchentPasDessus) {
    const core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;
    const std::string cleVillage = core::keyForEntity("village", "chest", 3, 2);
    const std::string cleDonjon = core::keyForEntity("donjon", "chest", 3, 2);
    EXPECT_NE(cleVillage, cleDonjon)
        << "le nom de carte ouvre la cle : sans lui, vider un coffre en viderait un autre ailleurs";

    const std::vector<core::Interactable> auVillage = {coffre(3, 2, cleVillage)};
    const std::vector<core::Interactable> auDonjon = {coffre(3, 2, cleDonjon)};
    const core::InteractionTarget cible = core::findInteractionTarget(
        auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(auVillage), drapeaux);
    ASSERT_TRUE(cible.found());
    EXPECT_TRUE(core::interact(cible, drapeaux).consumed);

    EXPECT_TRUE(core::findInteractionTarget(auCentre(2, 2), {1.0F, 0.0F}, carte,
                                            candidats(auDonjon), drapeaux)
                    .found())
        << "le coffre du donjon n'a pas ete ouvert";
}

/**
 * @brief Un type d'entite inconnu produit tout de meme une entite, sans composant interactif.
 * \castest{<b>Un objet de type inconnu apparait sur la carte sans etre interactif.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Peupler un monde depuis une couche objects portant un type inconnu.<br/>
 * \tattendu L'entite existe et porte un Transform, mais aucun Interactable.
 * }
 */
TEST(InteractionTest, UnTypeInconnuProduitUneEntiteNonInteractive) {
    core::LevelData donnees{.name = "village", .tileMap = carteVide()};
    donnees.entities.push_back({"statue-de-sel", {1, 1}, {}});
    donnees.entities.push_back({"chest", {3, 2}, {}});

    core::World monde;
    const core::Level niveau{donnees};
    EXPECT_EQ(core::spawnMapEntities(monde, niveau, niveau.name()), 2U)
        << "refuser un type inconnu ferait disparaitre un objet sans que son auteur comprenne "
           "pourquoi (EX-NFR-040)";

    std::size_t interactifs = 0;
    for (auto [entite, interactif] : monde.view<core::Interactable>()) {
        ++interactifs;
        EXPECT_EQ(interactif.type, "chest");
    }
    EXPECT_EQ(interactifs, 1U);
}

/**
 * @brief Les drapeaux de monde se relisent tries, forme que la sauvegarde ecrira.
 * \castest{<b>Les drapeaux acquis se relisent dans un ordre stable.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Lever trois drapeaux dans le desordre, puis les relire.<br/>
 * 2. Lever deux fois le meme.<br/>
 * \tattendu La relecture est triee ; le second passage renvoie false et ne cree pas de doublon.
 * }
 */
TEST(InteractionTest, LesDrapeauxSeRelisentTries) {
    core::WorldFlags drapeaux;
    EXPECT_TRUE(drapeaux.set("village/chest@3,2"));
    EXPECT_TRUE(drapeaux.set("donjon/chest@1,1"));
    EXPECT_TRUE(drapeaux.set("village/chest@0,0"));
    EXPECT_FALSE(drapeaux.set("village/chest@3,2")) << "un fait deja acquis n'est pas neuf";
    EXPECT_EQ(drapeaux.size(), 3U);

    const std::vector<std::string> tous = drapeaux.all();
    ASSERT_EQ(tous.size(), 3U);
    EXPECT_TRUE(std::is_sorted(tous.begin(), tous.end()));

    drapeaux.clear("donjon/chest@1,1");
    EXPECT_FALSE(drapeaux.isSet("donjon/chest@1,1"));
    EXPECT_EQ(drapeaux.size(), 2U);
}

/**
 * @brief Un PNJ s'aborde a moins d'une case et demie, diagonale et dos compris.
 * \castest{<b>Toute cible a moins de 1,5 case du heros est atteignable.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser un panneau en diagonale du heros, puis derriere lui, puis a deux cases.<br/>
 * \tattendu La diagonale et le dos sont a portee ; deux cases ne le sont pas.
 * }
 */
TEST(InteractionTest, LaPorteeEstDUneCaseEtDemie) {
    const core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;

    const std::vector<core::Interactable> enDiagonale = {panneau(3, 3)};
    EXPECT_TRUE(core::findInteractionTarget(auCentre(2, 2), {1.0F, 0.0F}, carte,
                                            candidats(enDiagonale), drapeaux)
                    .found())
        << "la diagonale est a 1,41 case";

    const std::vector<core::Interactable> dansLeDos = {panneau(1, 2)};
    EXPECT_TRUE(core::findInteractionTarget(auCentre(2, 2), {1.0F, 0.0F}, carte,
                                            candidats(dansLeDos), drapeaux)
                    .found())
        << "le heros n'a pas a se tourner exactement vers le PNJ";

    const std::vector<core::Interactable> aDeuxCases = {panneau(4, 2)};
    EXPECT_FALSE(core::findInteractionTarget(auCentre(2, 2), {1.0F, 0.0F}, carte,
                                             candidats(aDeuxCases), drapeaux)
                     .found());

    // La position est CONTINUE : au bord de sa case, le heros atteint la case d'apres, a peine.
    EXPECT_TRUE(core::findInteractionTarget({3.2F, 2.5F}, {1.0F, 0.0F}, carte,
                                            candidats(aDeuxCases), drapeaux)
                    .found())
        << "de (3,2 ; 2,5) au centre de (4, 2) : 1,3 case";
}

/**
 * @brief Deux murs qui se touchent par le coin ferment la diagonale.
 * \castest{<b>L'interaction ne passe pas entre deux murs en diagonale.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser un panneau en diagonale, puis murer une case de cote, puis l'autre.<br/>
 * \tattendu Un seul mur laisse passer ; les deux ferment le passage.
 * }
 */
TEST(InteractionTest, DeuxMursEnCoinFermentLaDiagonale) {
    core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;
    const std::vector<core::Interactable> objets = {panneau(3, 3)};

    carte.setTile(3, 2, core::TileType::Wall);
    EXPECT_TRUE(core::findInteractionTarget(auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets),
                                            drapeaux)
                    .found());
    carte.setTile(2, 3, core::TileType::Wall);
    EXPECT_FALSE(core::findInteractionTarget(auCentre(2, 2), {1.0F, 0.0F}, carte, candidats(objets),
                                             drapeaux)
                     .found())
        << "on ne tend pas la main entre deux murs qui se touchent";
}

/**
 * @brief Ce que le heros regarde passe avant ce qui est plus pres.
 * \castest{<b>A deux cibles a portee, la cible visee l'emporte.</b><br/>
 * \tcat Unitaire · Interaction<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un panneau devant le heros et un coffre dans son dos, plus proche.<br/>
 * \tattendu Le panneau, vise, est designe ; tourne vers le coffre, c'est le coffre.
 * }
 */
TEST(InteractionTest, LaCibleViseeLEmporte) {
    const core::TileMap carte = carteVide();
    core::WorldFlags drapeaux;
    const std::vector<core::Interactable> objets = {coffre(1, 2, "village/chest@1,2"),
                                                    panneau(3, 2)};
    // Le heros est a gauche de sa case : le coffre, derriere lui, est plus proche que le panneau.
    const core::Vector2 ici{2.2F, 2.5F};

    const core::InteractionTarget versLaDroite =
        core::findInteractionTarget(ici, {1.0F, 0.0F}, carte, candidats(objets), drapeaux);
    ASSERT_TRUE(versLaDroite.found());
    EXPECT_EQ(versLaDroite.interactable->type, "sign");

    const core::InteractionTarget versLaGauche =
        core::findInteractionTarget(ici, {-1.0F, 0.0F}, carte, candidats(objets), drapeaux);
    ASSERT_TRUE(versLaGauche.found());
    EXPECT_EQ(versLaGauche.interactable->type, "chest");

    // Ni l'un ni l'autre vise : le plus proche.
    const core::InteractionTarget versLeBas =
        core::findInteractionTarget(ici, {0.0F, 1.0F}, carte, candidats(objets), drapeaux);
    ASSERT_TRUE(versLeBas.found());
    EXPECT_EQ(versLeBas.interactable->type, "chest");
}
