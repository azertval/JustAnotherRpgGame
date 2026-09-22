// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_asset_gallery.cpp
 * @brief Tests unitaires de la galerie des assets (outil de débug) : forme des blocs, disposition,
 *        visibilité, image jouée, et lecture des assets livrés.
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "HMI/Graphics/AssetGallery.h"

namespace {

/// Une forme d'un lieu qui déclare un losange de @p tile pixels d'art (68 : l'ancienne planche).
hmi::AssetGalleryEntry entry(const std::string& model, int width, int height, int columns = 1,
                             int rows = 1, int tile = 68) {
    hmi::AssetGalleryEntry value;
    value.family = "essai";
    value.model = model;
    value.form = model;
    value.path = model + ".png";
    value.frameWidth = width;
    value.frameHeight = height;
    value.footprintColumns = columns;
    value.footprintRows = rows;
    value.tilePixels = tile;
    return value;
}

hmi::AssetGalleryEntry clip(int frames, double duration, bool loop) {
    hmi::AssetGalleryEntry value = entry("clip", 48, 64);
    for (int index = 0; index < frames; ++index) {
        value.frames.push_back(index);
    }
    value.frameDuration = duration;
    value.loop = loop;
    return value;
}

const hmi::AssetGalleryFamily* familyNamed(const hmi::AssetGalleryCatalog& catalog,
                                           const std::string& title) {
    const auto found =
        std::find_if(catalog.families.begin(), catalog.families.end(),
                     [&](const hmi::AssetGalleryFamily& family) { return family.title == title; });
    return found == catalog.families.end() ? nullptr : &*found;
}

}  // namespace

/**
 * @brief Un bloc est l'emprise plus une case de marge, et grandit avec le dessin.
 * \castest{<b>Un bloc contient son dessin, marge comprise.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer le bloc d'une figure, d'une attaque large, d'un mur, d'une pièce 2×1 et
 * d'une grande pièce.<br/>
 * \tattendu 3×3, 4×3, 3×4, 4×4 et 9×6 ; l'emprise posée en bas, centrée.
 * }
 */
TEST(AssetGalleryTest, FormeDesBlocs) {
    const hmi::AssetGalleryBloc figure = hmi::assetGalleryBlocShape(entry("figure", 48, 64));
    EXPECT_EQ(figure.columns, 3);
    EXPECT_EQ(figure.rows, 3);
    EXPECT_EQ(figure.footprintColumn, 1);
    EXPECT_EQ(figure.footprintRow, 1);

    const hmi::AssetGalleryBloc attaque = hmi::assetGalleryBlocShape(entry("attaque", 96, 64));
    EXPECT_EQ(attaque.columns, 4);
    EXPECT_EQ(attaque.rows, 3);
    EXPECT_EQ(attaque.footprintColumn, 1);

    const hmi::AssetGalleryBloc mur = hmi::assetGalleryBlocShape(entry("mur", 68, 100));
    EXPECT_EQ(mur.columns, 3);
    EXPECT_EQ(mur.rows, 4);
    EXPECT_EQ(mur.footprintRow, 2);

    const hmi::AssetGalleryBloc large = hmi::assetGalleryBlocShape(entry("large", 102, 135, 2, 1));
    EXPECT_EQ(large.columns, 4);
    EXPECT_EQ(large.rows, 4);
    EXPECT_EQ(large.footprintColumn, 1);
    EXPECT_EQ(large.footprintRow, 2);

    const hmi::AssetGalleryBloc piece = hmi::assetGalleryBlocShape(entry("piece", 435, 255));
    EXPECT_EQ(piece.columns, 9);
    EXPECT_EQ(piece.rows, 6);
}

/**
 * @brief Une case de la galerie vaut le losange **du lieu** de la forme : une figurine HD tient
 *        dans le même bloc qu'une figurine de l'ancienne planche (`LOT-103`).
 * \castest{<b>La galerie mesure chaque forme au losange de son lieu.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer le bloc d'une figurine 192 x 256 et d'une creature 384 x 384 a un losange
 * de 256, puis d'une figurine 192 x 256 sans losange declare.<br/>
 * \tattendu 3 x 3, 4 x 4 ; sans losange, la figurine se suppose d'une case de large.
 * }
 */
TEST(AssetGalleryTest, UneCaseVautLeLosangeDuLieu) {
    const hmi::AssetGalleryBloc figure =
        hmi::assetGalleryBlocShape(entry("figure", 192, 256, 1, 1, 256));
    EXPECT_EQ(figure.columns, 3);
    EXPECT_EQ(figure.rows, 3);

    const hmi::AssetGalleryBloc creature =
        hmi::assetGalleryBlocShape(entry("creature", 384, 384, 1, 1, 256));
    EXPECT_EQ(creature.columns, 4);
    EXPECT_EQ(creature.rows, 4);

    const hmi::AssetGalleryEntry sansLosange = entry("figure", 192, 256, 1, 1, 0);
    EXPECT_EQ(sansLosange.tileWidthPixels(), 192);
    EXPECT_EQ(hmi::assetGalleryBlocShape(sansLosange).columns, 3);
}

/**
 * @brief Une bande par famille, une ligne par modèle, retour à la ligne au-delà de la largeur.
 * \castest{<b>La galerie se dispose en bandes, lignes et colonnes.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Disposer deux modèles de deux formes (dont un mur) sur six cases, puis sur cinq.<br/>
 * \tattendu En-tête en ligne 0 ; les formes d'un modèle côte à côte, posées sur le même bas ; à
 * cinq cases, la seconde forme passe à la ligne.
 * }
 */
TEST(AssetGalleryTest, DispositionEnBandes) {
    hmi::AssetGalleryCatalog catalog;
    hmi::AssetGalleryFamily family{.title = "essai", .directory = "essai", .entries = {}};
    family.entries = {entry("a", 48, 64), entry("a", 48, 64), entry("a", 48, 64),
                      entry("b", 68, 100)};
    family.entries[1].model = "b";
    family.entries[2].model = "a";
    family.entries[3].model = "b";
    catalog.families.push_back(family);

    const hmi::AssetGalleryLayout layout = hmi::layoutAssetGallery(catalog, 6);
    ASSERT_EQ(layout.bands.size(), 1U);
    EXPECT_EQ(layout.bands[0].row, 0);
    ASSERT_EQ(layout.blocs.size(), 4U);

    // Modèle « a » : les entrées 0 et 2, côte à côte en ligne 1.
    EXPECT_EQ(layout.blocs[0].entry, 0);
    EXPECT_EQ(layout.blocs[0].row, 1);
    EXPECT_EQ(layout.blocs[1].entry, 2);
    EXPECT_EQ(layout.blocs[1].column, 3);
    EXPECT_EQ(layout.blocs[1].row, 1);

    // Modèle « b » : une figure et un mur sur la même ligne, posés sur le même bas.
    EXPECT_EQ(layout.blocs[2].entry, 1);
    EXPECT_EQ(layout.blocs[3].entry, 3);
    EXPECT_EQ(layout.blocs[3].row, 4);
    EXPECT_EQ(layout.blocs[2].row, 5);
    EXPECT_EQ(layout.rows, 8);
    EXPECT_EQ(layout.columns, 6);

    const hmi::AssetGalleryLayout narrow = hmi::layoutAssetGallery(catalog, 5);
    ASSERT_EQ(narrow.blocs.size(), 4U);
    EXPECT_EQ(narrow.blocs[1].column, 0);
    EXPECT_EQ(narrow.blocs[1].row, 4);
}

/**
 * @brief Dessiné dans la vue, préchargé dans l'anneau, déchargé au-delà.
 * \castest{<b>Seuls les blocs à l'écran sont dessinés.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Classer un bloc 3×3 pour une vue qui le couvre, qui s'en écarte de deux cases, puis
 * de sept.<br/>
 * \tattendu Dessiné, préchargé, déchargé.
 * }
 */
TEST(AssetGalleryTest, VisibiliteParLaVue) {
    const hmi::AssetGalleryBloc bloc;
    EXPECT_EQ(hmi::assetGalleryVisibility(bloc, {0.0, 0.0, 10.0, 10.0}),
              hmi::AssetGalleryVisibility::Drawn);
    EXPECT_EQ(hmi::assetGalleryVisibility(bloc, {5.0, 0.0, 10.0, 10.0}),
              hmi::AssetGalleryVisibility::Preloaded);
    EXPECT_EQ(hmi::assetGalleryVisibility(bloc, {10.0, 0.0, 10.0, 10.0}),
              hmi::AssetGalleryVisibility::Unloaded);
}

/**
 * @brief Un clip bouclé tourne ; un clip joué une fois se tient avant de reprendre.
 * \castest{<b>L'image jouée suit le temps.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander l'image d'un clip bouclé et d'un clip joué une fois à plusieurs
 * instants.<br/>
 * \tattendu Boucle : 1 puis 0 ; une fois : 3, tenue sur 3, puis 0.
 * }
 */
TEST(AssetGalleryTest, ImageJouee) {
    const hmi::AssetGalleryEntry boucle = clip(6, 0.15, true);
    EXPECT_EQ(hmi::assetGalleryFrameRank(boucle, 0.16), 1);
    EXPECT_EQ(hmi::assetGalleryFrameRank(boucle, 0.91), 0);

    const hmi::AssetGalleryEntry unique = clip(4, 0.1, false);
    EXPECT_EQ(hmi::assetGalleryFrameRank(unique, 0.35), 3);
    EXPECT_EQ(hmi::assetGalleryFrameRank(unique, 0.75), 3);
    EXPECT_EQ(hmi::assetGalleryFrameRank(unique, 1.05), 0);

    EXPECT_EQ(hmi::assetGalleryFrameRank(entry("fixe", 16, 16), 3.0), 0);
}

/**
 * @brief Les assets d'une racine se lisent sans erreur, chaque forme désignant un fichier.
 * \castest{<b>La galerie lit les assets d'une racine.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire le catalogue de la racine d'essai.<br/>
 * \tattendu Aucune erreur ; PNJ avec attaque de 96 px en 8 images jouée une fois ; monstres et
 * lieu présents ; toutes les textures existent.
 * }
 */
TEST(AssetGalleryTest, AssetsDEssai) {
    const std::filesystem::path root = std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets";
    const hmi::AssetGalleryCatalog catalog = hmi::AssetGalleryCatalog::load(root);
    for (const std::string& error : catalog.errors) {
        ADD_FAILURE() << error;
    }

    const hmi::AssetGalleryFamily* const npcs = familyNamed(catalog, "PNJ");
    ASSERT_NE(npcs, nullptr);
    const auto attack = std::find_if(npcs->entries.begin(), npcs->entries.end(),
                                     [](const hmi::AssetGalleryEntry& value) {
                                         return value.model == "figurant" && value.form == "attack";
                                     });
    ASSERT_NE(attack, npcs->entries.end());
    EXPECT_EQ(attack->frameWidth, 96);
    EXPECT_EQ(attack->frameCount(), 8);
    EXPECT_FALSE(attack->loop);

    EXPECT_NE(familyNamed(catalog, "Monstres"), nullptr);
    EXPECT_NE(familyNamed(catalog, "Scène · bourg"), nullptr);

    for (const hmi::AssetGalleryFamily& family : catalog.families) {
        for (const hmi::AssetGalleryEntry& value : family.entries) {
            EXPECT_TRUE(std::filesystem::is_regular_file(root / value.path)) << value.path;
            EXPECT_GT(value.frameWidth, 0) << value.path;
            EXPECT_GT(value.frameHeight, 0) << value.path;
        }
    }
    EXPECT_FALSE(hmi::layoutAssetGallery(catalog).blocs.empty());
}

/**
 * @brief Toute image livrée paraît dans la galerie, ou relève d'une exclusion nommée
 * (`EX-CNT-042`).
 * \castest{<b>Aucun asset livré n'échappe à la galerie.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire le catalogue de Source/Elements/Assets. 2. Parcourir toutes les images
 * livrées.<br/>
 * \tattendu Chaque PNG ou JPEG est une forme de la galerie, ou une planche source, une image
 * d'interface, une carte plein écran ou une police ; les portraits de PNJ y sont.
 * }
 */
TEST(AssetGalleryTest, ToutAssetLivreEstDansLaGalerie) {
    const std::filesystem::path root(JADG_ASSETS_DIR);
    const hmi::AssetGalleryCatalog catalog = hmi::AssetGalleryCatalog::load(root);
    for (const std::string& path : hmi::assetGalleryUnlisted(root, catalog)) {
        ADD_FAILURE() << path
                      << " : image livrée absente de la galerie des assets (EX-CNT-042). "
                         "L'ajouter à AssetGalleryCatalog::load, ou nommer son exclusion dans "
                         "assetGalleryExcludes.";
    }

    EXPECT_TRUE(hmi::assetGalleryExcludes("UI/background/menu-scene.png"));
    EXPECT_TRUE(hmi::assetGalleryExcludes("Maps/world.jpg"));
    EXPECT_TRUE(hmi::assetGalleryExcludes("Fonts/Cinzel.ttf"));
    EXPECT_FALSE(
        hmi::assetGalleryExcludes("Regions/central-empire/capital/martpart/Scene/street.png"));
    EXPECT_FALSE(hmi::assetGalleryExcludes("Npc/figurant/portrait.png"));
}

/**
 * @brief L'arborescence par niveaux paraît dans la galerie : un dossier `Scene/` de zone est une
 *        famille nommée par son lieu, et un manifeste encore vide n'en fait pas (LOT-104).
 * \castest{<b>Les pièces de l'arborescence par niveaux paraissent dans la galerie.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Écrire `Regions/r/ville/zone/Scene/manifest.json` avec un mur 3 × 1, et
 * `Common/Terrain/manifest.json` sans pièce. 2. Lire le catalogue. 3. Chercher les images non
 * listées.<br/>
 * \tattendu Une famille « Scène · r/ville/zone » ; le mur à son chemin, sa taille, son emprise et
 * son ancre ; aucune famille pour le commun vide ; aucune erreur, aucune image non listée.
 * }
 */
TEST(AssetGalleryTest, ArborescenceParNiveaux) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "jadg_asset_gallery_tree";
    std::filesystem::remove_all(root);
    const std::filesystem::path scene = root / "Regions" / "r" / "ville" / "zone" / "Scene";
    std::filesystem::create_directories(scene);
    std::filesystem::create_directories(root / "Common" / "Terrain");
    std::ofstream(scene / "manifest.json")
        << R"({"version": 1, "disposition": "zone", "tile": [256, 159], "textures": {)"
           R"("scene/zone/wall-arcade-u": {"file": "wall-arcade-u.png", "class": "wide",)"
           R"( "footprint": [3, 1], "size": [426, 539], "anchor": [6, 295]}}})";
    std::ofstream(scene / "wall-arcade-u.png") << "png";
    std::ofstream(root / "Common" / "Terrain" / "manifest.json")
        << R"({"version": 1, "tile": [256, 159], "textures": {}})";

    const hmi::AssetGalleryCatalog catalog = hmi::AssetGalleryCatalog::load(root);
    const std::vector<std::string> unlisted = hmi::assetGalleryUnlisted(root, catalog);
    std::filesystem::remove_all(root);

    EXPECT_TRUE(catalog.errors.empty());
    EXPECT_TRUE(unlisted.empty());
    ASSERT_EQ(catalog.families.size(), 1U);
    const hmi::AssetGalleryFamily& family = catalog.families.front();
    EXPECT_EQ(family.title, "Scène · r/ville/zone");
    EXPECT_EQ(family.directory, "Regions/r/ville/zone/Scene");
    ASSERT_EQ(family.entries.size(), 1U);
    const hmi::AssetGalleryEntry& wall = family.entries.front();
    EXPECT_EQ(wall.form, "wall-arcade-u");
    EXPECT_EQ(wall.model, "wide");
    EXPECT_EQ(wall.path, "Regions/r/ville/zone/Scene/wall-arcade-u.png");
    EXPECT_EQ(wall.frameWidth, 426);
    EXPECT_EQ(wall.frameHeight, 539);
    EXPECT_EQ(wall.footprintColumns, 3);
    EXPECT_EQ(wall.footprintRows, 1);
    EXPECT_EQ(wall.anchorX, 6);
    EXPECT_EQ(wall.anchorY, 295);
}

/**
 * @brief Les figurines de monstres forment leur famille, au gabarit que dit chaque `.anim.json`,
 *        et une bête sans sort n'a pas d'entrée `cast` (LOT-93).
 * \castest{<b>Une figurine Grande sans sort paraît dans la galerie.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Écrire un dossier Monsters/ : un manifeste qui nomme idle et cast, un lion qui n'a
 * que idle, en cellules de 96 × 96. 2. Lire le catalogue.<br/>
 * \tattendu Une famille « Monstres » ; le idle du lion en 96 × 96 et 6 images ; aucune entrée
 * cast, aucune erreur.
 * }
 */
TEST(AssetGalleryTest, FigurinesDeMonstres) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "jadg_asset_gallery_monsters";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "Monsters" / "lion");
    std::ofstream(root / "Monsters" / "manifest.json")
        << R"({"version": 1, "animations": ["idle", "cast"], "monsters": []})";
    std::ofstream(root / "Monsters" / "lion" / "idle.anim.json")
        << R"({"version": 1, "frameWidth": 96, "frameHeight": 96, "clips": {"idle": )"
           R"({"frames": [0, 1, 2, 3, 4, 5], "frameDuration": 0.15, "loop": true}}})";

    const hmi::AssetGalleryCatalog catalog = hmi::AssetGalleryCatalog::load(root);
    std::filesystem::remove_all(root);

    EXPECT_TRUE(catalog.errors.empty());
    const hmi::AssetGalleryFamily* const monsters = familyNamed(catalog, "Monstres");
    ASSERT_NE(monsters, nullptr);
    EXPECT_EQ(monsters->directory, "Monsters");
    ASSERT_EQ(monsters->entries.size(), 1U);
    const hmi::AssetGalleryEntry& idle = monsters->entries.front();
    EXPECT_EQ(idle.model, "lion");
    EXPECT_EQ(idle.form, "idle");
    EXPECT_EQ(idle.path, "Monsters/lion/idle.png");
    EXPECT_EQ(idle.frameWidth, 96);
    EXPECT_EQ(idle.frameHeight, 96);
    EXPECT_EQ(idle.frameCount(), 6);
    EXPECT_TRUE(idle.loop);
}
