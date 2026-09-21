// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_stamps.cpp
 * @brief Tests des tampons et des préfabriqués (`LOT-EDITOR-08`) : ce que le découpage prend, ce
 *        que la pose écrit, le miroir, la bibliothèque, les modèles de carte — et l'acceptation :
 *        un étal de la carte d'essai reposé ailleurs avec son marchand.
 */

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileType.h"
#include "Editor/Logic/LayerView.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/PieceCatalog.h"
#include "Editor/Logic/Stamps.h"

namespace {

using core::GridPosition;
using core::LevelDraft;
using core::MapEntity;
using core::TileType;
using hmi::Stamp;

// La racine d'essai de l'éditeur (`LOT-123`) : ces tampons se prenaient sur les cartes
// LIVRÉES, que la table rase du `LOT-102` emporte. Les deux cartes d'essai partagent une
// planche, comme les deux quartiers d'alors : un préfabriqué de l'une se pose sur l'autre.
[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_EDITOR_DATA_DIR);
}

/// Une carte du disque, avec le manifeste de son lieu : le brouillon déduit alors ses emprises.
[[nodiscard]] LevelDraft carte(const std::string& identifiant, const std::string& lieu) {
    const std::filesystem::path path = dataRoot() / "Levels" / (identifiant + ".json");
    core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(path);
    EXPECT_TRUE(loaded.ok()) << loaded.error;
    LevelDraft draft = LevelDraft::fromLevel(*loaded.level);
    const hmi::PlaceAssets assets = hmi::loadPlaceAssets(dataRoot(), lieu);
    EXPECT_TRUE(assets.manifest.has_value());
    draft.setPieceManifest(std::make_shared<const core::ScenePieceManifest>(*assets.manifest));
    return draft;
}

[[nodiscard]] LevelDraft laPlace() {
    return carte("bourg/place", "bourg");
}

/// La couche du tampon nommée @p nom.
[[nodiscard]] const hmi::StampLayer& couche(const Stamp& stamp, const std::string& nom) {
    const auto found = std::ranges::find(stamp.layers, nom, &hmi::StampLayer::name);
    EXPECT_NE(found, stamp.layers.end()) << "aucune couche " << nom;
    return *found;
}

/// La pièce du tampon ancrée en @p cell, vide s'il n'y en a pas.
[[nodiscard]] std::string pieceEn(const hmi::StampLayer& layer, GridPosition cell) {
    const auto found = std::ranges::find(layer.pieces, cell, &hmi::StampPiece::anchor);
    return found == layer.pieces.end() ? std::string{} : found->piece;
}

/// Le rang de la couche de décor d'une carte : `layers()` porte aussi le reflet de la grille
/// racine, son rang n'est donc pas celui qu'on croit.
[[nodiscard]] std::size_t decor(const LevelDraft& draft) {
    const std::optional<std::size_t> index = hmi::pieceTargetLayer(draft.layers(), false);
    EXPECT_TRUE(index.has_value());
    return index.value_or(0);
}

/// Le texte d'une carte sans son compteur d'identifiants : ce qu'une annulation doit rendre, le
/// compteur ne reculant jamais (décision D8).
[[nodiscard]] std::string sansCompteur(std::string texte) {
    const std::size_t debut = texte.find("\"nextEntityId\"");
    if (debut != std::string::npos) {
        const std::size_t fin = texte.find('\n', debut);
        texte.erase(debut, fin == std::string::npos ? std::string::npos : fin - debut + 1);
    }
    return texte;
}

/// Le numéro d'un identifiant `e<n>`.
[[nodiscard]] int entityIdNumberOf(const std::string& id) {
    return core::entityIdNumber(id).value_or(0);
}

/// L'étal du marché de la Place : `feature-1`, 2 × 1, ancré ici.
constexpr GridPosition ETAL{.column = 21, .row = 24};

const hmi::LayerViewState LIBRE{};

}  // namespace

/**
 * @brief Une pièce est prise entière : le rectangle s'agrandit jusqu'à son emprise.
 * \castest{<b>Le tampon prend l'étal entier.</b><br/>
 * \tcat Unitaire · Tampons<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Découper la seule case d'ancrage de l'étal 2 × 1 de la carte d'essai.<br/>
 * \tattendu Le tampon fait 2 × 1, nomme l'étal une fois, et dit son lieu.
 * }
 */
TEST(StampsTest, LeTamponPrendLaPieceEntiere) {
    const LevelDraft draft = laPlace();
    const Stamp stamp = hmi::cutStamp(draft, ETAL, ETAL);
    EXPECT_EQ(stamp.width, 2);
    EXPECT_EQ(stamp.height, 1);
    EXPECT_EQ(stamp.place, "bourg");
    const hmi::StampLayer& relief = couche(stamp, "relief");
    EXPECT_EQ(relief.pieces.size(), 1U);
    EXPECT_EQ(pieceEn(relief, {.column = 0, .row = 0}), "feature-1");
    // Le sol est pris lui aussi, sur toute la largeur du tampon.
    EXPECT_EQ(couche(stamp, "sol").types.size(), 2U);
}

/**
 * @brief Une pièce ancrée hors du rectangle n'est pas prise : elle est à ce qu'on laisse.
 * \castest{<b>Une pièce ancrée dehors n'est pas prise.</b><br/>
 * \tcat Unitaire · Tampons<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Découper la seconde case de l'étal, celle que son emprise couvre sans l'ancrer.<br/>
 * \tattendu Le tampon fait une case et ne nomme aucune pièce de décor.
 * }
 */
TEST(StampsTest, UnePieceAncreeDehorsNestPasPrise) {
    const LevelDraft draft = laPlace();
    const GridPosition seconde{.column = ETAL.column + 1, .row = ETAL.row};
    const Stamp stamp = hmi::cutStamp(draft, seconde, seconde);
    EXPECT_EQ(stamp.width, 1);
    EXPECT_EQ(stamp.height, 1);
    EXPECT_TRUE(couche(stamp, "relief").pieces.empty());
}

/**
 * @brief Toute la pose est un geste : un `Ctrl+Z` la défait, entités comprises.
 * \castest{<b>Une pose se défait d'un seul pas.</b><br/>
 * \tcat Unitaire · Tampons<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Découper un morceau de la carte d'essai, le reposer ailleurs, puis annuler.<br/>
 * \tattendu Un pas d'annulation de plus, et la carte revient exactement à son état.
 * }
 */
TEST(StampsTest, UnePoseSeDefaitDUnSeulPas) {
    LevelDraft draft = laPlace();
    const Stamp stamp = hmi::cutStamp(draft, ETAL, ETAL);
    const std::string avant = draft.toJson();
    const std::size_t pas = draft.undoDepth();

    const GridPosition ailleurs{.column = 4, .row = 30};
    const hmi::StampPasteResult result = hmi::pasteStamp(draft, stamp, ailleurs, LIBRE);
    EXPECT_TRUE(result.refusal.empty()) << result.refusal;
    EXPECT_TRUE(result.changed);
    EXPECT_EQ(draft.undoDepth(), pas + 1);
    EXPECT_EQ(draft.layers()[decor(draft)].pieceAt(ailleurs.column, ailleurs.row), "feature-1");

    EXPECT_TRUE(draft.undo());
    EXPECT_EQ(draft.toJson(), avant);
}

/**
 * @brief Le miroir transpose le tampon et pose les jumelles.
 * \castest{<b>Le miroir transpose le tampon.</b><br/>
 * \tcat Unitaire · Tampons<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Découper la façade `front-left` (1 × 2), la refléter.<br/>
 * \tattendu Le tampon passe de 1 × 2 à 2 × 1 et nomme `front-right`.
 * }
 */
TEST(StampsTest, LeMiroirTransposeLeTampon) {
    const LevelDraft draft = laPlace();
    const GridPosition facade{.column = 12, .row = 21};
    const Stamp stamp = hmi::cutStamp(draft, facade, facade);
    EXPECT_EQ(stamp.width, 1);
    EXPECT_EQ(stamp.height, 2);
    EXPECT_EQ(pieceEn(couche(stamp, "relief"), {.column = 0, .row = 0}), "front-left");

    const Stamp mirrored = hmi::mirrorStamp(stamp, draft.pieceManifest());
    EXPECT_EQ(mirrored.width, 2);
    EXPECT_EQ(mirrored.height, 1);
    EXPECT_EQ(pieceEn(couche(mirrored, "relief"), {.column = 0, .row = 0}), "front-right");
    // Refléter deux fois rend le tampon de départ.
    EXPECT_EQ(hmi::mirrorStamp(mirrored, draft.pieceManifest()), stamp);
}

/**
 * @brief Une couche verrouillée, ou absente, refuse la pose sans rien écrire.
 * \castest{<b>Une pose refusée n'écrit rien.</b><br/>
 * \tcat Unitaire · Tampons<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Verrouiller le décor, poser. 2. Poser un tampon dont la couche n'existe pas.<br/>
 * \tattendu Deux refus nommés, et la carte n'a pas bougé d'un octet.
 * }
 */
TEST(StampsTest, UnePoseRefuseeNecritRien) {
    LevelDraft draft = laPlace();
    const Stamp stamp = hmi::cutStamp(draft, ETAL, ETAL);
    const std::string avant = draft.toJson();

    hmi::LayerViewState verrou;
    verrou.sync(draft.layers().size());
    verrou.setLocked(hmi::LayerSlot{decor(draft)}, true);
    const hmi::StampPasteResult locked =
        hmi::pasteStamp(draft, stamp, {.column = 4, .row = 30}, verrou);
    EXPECT_FALSE(locked.changed);
    EXPECT_NE(locked.refusal.find("locked"), std::string::npos) << locked.refusal;

    // Une carte sans couche de décor n'a nulle part où poser le relief du tampon.
    LevelDraft nue = LevelDraft::empty("nue", 8, 8);
    nue.addLayer(core::LayerKind::Ground, "sol");
    const hmi::StampPasteResult absent =
        hmi::pasteStamp(nue, stamp, {.column = 0, .row = 0}, LIBRE);
    EXPECT_FALSE(absent.changed);
    EXPECT_NE(absent.refusal.find("relief"), std::string::npos) << absent.refusal;

    const hmi::StampPasteResult dehors =
        hmi::pasteStamp(draft, stamp, {.column = 400, .row = 400}, LIBRE);
    EXPECT_FALSE(dehors.changed);
    EXPECT_NE(dehors.refusal.find("outside"), std::string::npos) << dehors.refusal;

    EXPECT_EQ(draft.toJson(), avant);
}

/**
 * @brief Un préfabriqué fait l'aller-retour par le disque sans rien perdre.
 * \castest{<b>Un préfabriqué se relit tel qu'il a été écrit.</b><br/>
 * \tcat Unitaire · Préfabriqués<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Écrire un tampon dans une bibliothèque temporaire, le relire.<br/>
 * \tattendu Le tampon relu égale celui qu'on a écrit ; un nom invalide est refusé.
 * }
 */
TEST(StampsTest, UnPrefabriqueFaitLAllerRetour) {
    LevelDraft draft = laPlace();
    // Un marchand sur l'étal : le tampon emporte l'entité avec la pièce.
    draft.placeEntity(
        MapEntity{.type = "npc",
                  .position = ETAL,
                  .properties = {{"dialogue", std::string{"sentinelle-ironhand"}},
                                 {"figure", std::string{"Monsters/ironhand-soldier"}}},
                  .id = {},
                  .elevation = 0,
                  .cells = {}});
    const Stamp stamp = hmi::cutStamp(draft, ETAL, ETAL);
    EXPECT_EQ(stamp.entities.size(), 1U);
    EXPECT_TRUE(stamp.entities.front().id.empty());

    const std::filesystem::path racine =
        std::filesystem::temp_directory_path() / "jadg-prefabs-aller-retour";
    std::filesystem::remove_all(racine);
    EXPECT_EQ(hmi::writePrefab(racine, "bourg", "etal", stamp), "");
    EXPECT_EQ(hmi::prefabNames(racine, "bourg"), std::vector<std::string>{"etal"});

    std::string error;
    const std::optional<Stamp> relu = hmi::readPrefab(racine, "bourg", "etal", error);
    EXPECT_TRUE(error.empty()) << error;
    ASSERT_TRUE(relu.has_value());
    EXPECT_EQ(*relu, stamp);

    EXPECT_FALSE(hmi::writePrefab(racine, "bourg", "Étal du marché", stamp).empty());
    EXPECT_FALSE(hmi::writePrefab(racine, "bourg", "etal-vide", Stamp{}).empty());
    std::filesystem::remove_all(racine);
}

/**
 * @brief Acceptation du `LOT-EDITOR-08` — un étal de la carte d'essai se repose ailleurs avec son
 *        marchand, qui reçoit un nouvel `id`, et l'annulation le retire en un pas.
 * \castest{<b>Un étal de la Place se repose sur le Donjon.</b><br/>
 * \tcat Unitaire · Préfabriqués<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser un marchand sur l'étal du marché, l'enregistrer comme préfabriqué.
 * 2. Ouvrir le Donjon, poser le préfabriqué. 3. Annuler.<br/>
 * \tattendu L'étal et le marchand sont là, le marchand a un identifiant neuf et libre, et un seul
 * `Ctrl+Z` retire tout.
 * }
 */
TEST(DonneesPrefabriques, UnEtalSeReposeAvecSonMarchand) {
    LevelDraft source = laPlace();
    source.placeEntity(
        MapEntity{.type = "npc",
                  .position = ETAL,
                  .properties = {{"dialogue", std::string{"garde-du-bourg"}},
                                 {"figure", std::string{"Monsters/sentinelle"}},
                                 {"name", std::string{"marchand-du-bourg"}}},
                  .id = {},
                  .elevation = 0,
                  .cells = {}});
    const Stamp etal = hmi::cutStamp(source, ETAL, ETAL);
    EXPECT_EQ(etal.width, 2);
    EXPECT_EQ(etal.entities.size(), 1U);

    const std::filesystem::path racine =
        std::filesystem::temp_directory_path() / "jadg-prefabs-etal";
    std::filesystem::remove_all(racine);
    EXPECT_EQ(hmi::writePrefab(racine, "bourg", "etal-du-marche", etal), "");
    std::string error;
    const std::optional<Stamp> prefabrique =
        hmi::readPrefab(racine, "bourg", "etal-du-marche", error);
    ASSERT_TRUE(prefabrique.has_value()) << error;

    // Le Donjon prend ses pièces de la même planche : l'étal s'y pose tel quel.
    LevelDraft cible = carte("donjon", "bourg");
    const std::string avant = cible.toJson();
    const std::size_t pas = cible.undoDepth();
    const std::size_t entitesAvant = cible.entities().size();
    const std::string idLibre = core::entityIdFor(cible.nextEntityId());

    const GridPosition place{.column = 20, .row = 20};
    const hmi::StampPasteResult pose = hmi::pasteStamp(cible, *prefabrique, place, LIBRE);
    EXPECT_TRUE(pose.refusal.empty()) << pose.refusal;
    EXPECT_TRUE(pose.changed);
    EXPECT_EQ(cible.undoDepth(), pas + 1);
    EXPECT_EQ(cible.layers()[decor(cible)].pieceAt(place.column, place.row), "feature-1");
    ASSERT_EQ(pose.entities.size(), 1U);
    const MapEntity& marchand = cible.entities()[pose.entities.front()];
    EXPECT_EQ(marchand.type, "npc");
    EXPECT_EQ(marchand.position, place);
    EXPECT_EQ(marchand.id, idLibre);
    EXPECT_NE(marchand.id, etal.entities.front().id);
    EXPECT_EQ(cible.entities().size(), entitesAvant + 1);

    // Un seul pas d'annulation rend la carte — au compteur d'identifiants près, qui ne recule
    // jamais : un identifiant donné n'est pas réemployé (décision D8).
    EXPECT_TRUE(cible.undo());
    EXPECT_EQ(cible.undoDepth(), pas);
    EXPECT_EQ(sansCompteur(cible.toJson()), sansCompteur(avant));
    EXPECT_EQ(cible.entities().size(), entitesAvant);
    EXPECT_GT(cible.nextEntityId(), entityIdNumberOf(idLibre));
    std::filesystem::remove_all(racine);
}

/**
 * @brief Les modèles livrés se lisent, et la bibliothèque livrée ne laisse aucun constat.
 * \castest{<b>La bibliothèque livrée se lit.</b><br/>
 * \tcat Unitaire · Modèles de carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire les modèles de `Editor/Templates`. 2. Contrôler toute la bibliothèque.<br/>
 * \tattendu Les trois modèles — intérieur, rue, arène — sont là, chacun avec ses couches, et le
 * contrôle ne rend aucun constat.
 * }
 */
TEST(DonneesPrefabriques, LesModelesLivresSeLisent) {
    const std::vector<hmi::MapTemplate> modeles = hmi::mapTemplates(dataRoot());
    std::vector<std::string> identifiants;
    for (const hmi::MapTemplate& modele : modeles) {
        identifiants.push_back(modele.id);
        EXPECT_FALSE(modele.layers.empty()) << modele.id;
        EXPECT_GT(modele.width, 0);
        EXPECT_GT(modele.height, 0);
        EXPECT_TRUE(std::ranges::any_of(
            modele.layers, [](const hmi::MapTemplateLayer& couche) { return couche.scene; }))
            << modele.id << " : aucune couche ne nomme le lieu";
    }
    EXPECT_EQ(identifiants, (std::vector<std::string>{"arena", "interior", "street"}));

    const std::vector<hmi::LibraryFinding> constats = hmi::checkEditorLibrary(dataRoot());
    for (const hmi::LibraryFinding& constat : constats) {
        ADD_FAILURE() << constat.file.string() << " : " << constat.message;
    }
}

/**
 * @brief Un modèle ne nomme aucune pièce : il sert tous les lieux.
 * \castest{<b>Un modèle qui nomme une pièce est refusé.</b><br/>
 * \tcat Unitaire · Modèles de carte<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Lire un modèle dont le tampon pose une pièce.<br/>
 * \tattendu La lecture est refusée et le dit.
 * }
 */
TEST(StampsTest, UnModeleNeNommeAucunePiece) {
    const nlohmann::json json = {
        {"format", std::string{hmi::MAP_TEMPLATE_FORMAT}},
        {"version", hmi::MAP_TEMPLATE_VERSION},
        {"id", "essai"},
        {"width", 4},
        {"height", 4},
        {"layers", nlohmann::json::array({{{"name", "sol"}, {"kind", "ground"}, {"scene", true}}})},
        {"stamp",
         {{"width", 1},
          {"height", 1},
          {"layers", nlohmann::json::array(
                         {{{"name", "sol"},
                           {"kind", "ground"},
                           {"types", nlohmann::json::array({"dirt"})},
                           {"pieces", nlohmann::json::array({{{"at", nlohmann::json::array({0, 0})},
                                                              {"piece", "street"},
                                                              {"type", "empty"}}})}}})}}}};
    std::string error;
    EXPECT_FALSE(hmi::mapTemplateFromJson(json, error).has_value());
    EXPECT_NE(error.find("piece"), std::string::npos) << error;
}
