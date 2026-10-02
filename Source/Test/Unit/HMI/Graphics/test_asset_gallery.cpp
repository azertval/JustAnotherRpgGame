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
 * \tattendu Aucune erreur ; le PNJ est un modèle, dont l'attaque dure 0,8 s et se joue une fois ;
 * monstres et lieu présents ; tous les fichiers existent.
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
    EXPECT_TRUE(attack->mesh);
    EXPECT_EQ(attack->path, "Npc/figurant/figurant.glb");
    EXPECT_EQ(attack->clip, "attack");
    EXPECT_DOUBLE_EQ(attack->clipDuration, 0.8F);
    EXPECT_FALSE(attack->loop);
    EXPECT_EQ(attack->frameCount(), 1) << "un modèle n'a pas d'images";

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
 * \tattendu Chaque PNG, JPEG ou modèle `.glb` est une forme de la galerie, ou une planche source,
 * une image d'interface, une carte plein écran ou une police ; les portraits de PNJ y sont.
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
    EXPECT_TRUE(
        hmi::assetGalleryExcludes("Regions/central-empire/capital/martpart/Map/martpart.jpg"));
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
 * @brief Un personnage paraît par son **modèle**, une forme par clip que son squelette déclare ;
 *        un modèle inscrit sans fiche paraît dans sa pose ; ce qui n'a pas de modèle ne paraît
 *        pas (`LOT-1006`).
 * \castest{<b>Les personnages paraissent en modeles, clip par clip.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Écrire un dossier Monsters/ : un lion avec sa fiche, son modèle et un squelette à
 * deux clips ; un tigre inscrit au manifeste (`models`) sans fiche ; un fantôme dont la fiche nomme
 * un fichier absent ; un vestige qui n'a qu'une bande d'images. 2. Lire le catalogue.<br/>
 * \tattendu Une famille « Monstres » de trois formes : le lion en `idle` (boucle, 1 s) et `attack`
 * (une fois, 0,8 s), le tigre en `model`, sans clip ; rien pour le fantôme ni pour le vestige ;
 * aucune erreur.
 * }
 */
TEST(AssetGalleryTest, LesPersonnagesParaissentEnModeles) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "jadg_asset_gallery_monsters";
    std::filesystem::remove_all(root);
    const std::filesystem::path monsters = root / "Monsters";
    for (const char* const name : {"lion", "tigre", "fantome", "vestige"}) {
        std::filesystem::create_directories(monsters / name);
    }
    std::filesystem::create_directories(root / "Common" / "Characters" / "Skeletons" / "fauve");
    std::ofstream(root / "Common" / "Characters" / "Skeletons" / "fauve" / "skeleton.json")
        << R"({"version": 1, "silhouette": "fauve", "bones": [{"name": "Root", "parent": ""}],)"
           R"( "clips": [{"name": "idle", "duration": 1.0, "loop": true},)"
           R"( {"name": "attack", "duration": 0.8, "loop": false, "key": 0.4}]})";
    std::ofstream(monsters / "manifest.json")
        << R"({"version": 1, "tile": [256, 159], "npcs": [], "models": {"tigre": {}}})";
    std::ofstream(monsters / "lion" / "character.json")
        << R"({"version": 1, "model": "lion.glb", "skeleton": "fauve"})";
    std::ofstream(monsters / "lion" / "lion.glb") << "glb";
    std::ofstream(monsters / "tigre" / "tigre.glb") << "glb";
    std::ofstream(monsters / "fantome" / "character.json")
        << R"({"version": 1, "model": "fantome.glb", "skeleton": "fauve"})";
    std::ofstream(monsters / "vestige" / "idle.anim.json")
        << R"({"version": 1, "frameWidth": 96, "frameHeight": 96, "clips": {"idle": )"
           R"({"frames": [0, 1], "frameDuration": 0.15, "loop": true}}})";

    const hmi::AssetGalleryCatalog catalog = hmi::AssetGalleryCatalog::load(root);
    std::filesystem::remove_all(root);

    for (const std::string& error : catalog.errors) {
        ADD_FAILURE() << error;
    }
    const hmi::AssetGalleryFamily* const family = familyNamed(catalog, "Monstres");
    ASSERT_NE(family, nullptr);
    EXPECT_EQ(family->directory, "Monsters");
    ASSERT_EQ(family->entries.size(), 3U);
    const hmi::AssetGalleryEntry& idle = family->entries[0];
    EXPECT_EQ(idle.model, "lion");
    EXPECT_EQ(idle.form, "idle");
    EXPECT_EQ(idle.path, "Monsters/lion/lion.glb");
    EXPECT_TRUE(idle.mesh);
    EXPECT_EQ(idle.clip, "idle");
    EXPECT_TRUE(idle.loop);
    EXPECT_DOUBLE_EQ(idle.clipDuration, 1.0);
    EXPECT_EQ(idle.frameWidth, 384) << "une case et demie, pour un bras tendu ou un corps couché";
    EXPECT_EQ(idle.frameHeight, 256);
    const hmi::AssetGalleryEntry& attack = family->entries[1];
    EXPECT_EQ(attack.form, "attack");
    EXPECT_FALSE(attack.loop);
    EXPECT_DOUBLE_EQ(attack.clipDuration, 0.8F);
    const hmi::AssetGalleryEntry& still = family->entries[2];
    EXPECT_EQ(still.model, "tigre");
    EXPECT_EQ(still.form, "model");
    EXPECT_EQ(still.path, "Monsters/tigre/tigre.glb");
    EXPECT_TRUE(still.mesh);
    EXPECT_TRUE(still.clip.empty());

    // Le clip d'un modèle dans le temps de la galerie : une boucle revient, un clip joué une fois
    // se tient sur sa fin avant de reprendre.
    EXPECT_NEAR(hmi::assetGalleryClipSeconds(idle, 2.25), 0.25, 1e-9);
    EXPECT_NEAR(hmi::assetGalleryClipSeconds(attack, 0.5), 0.5, 1e-6);
    EXPECT_NEAR(hmi::assetGalleryClipSeconds(attack, 1.2), 0.8, 1e-6) << "tenu sur sa fin";
    EXPECT_NEAR(hmi::assetGalleryClipSeconds(attack, 1.5), 0.1, 1e-6) << "puis il reprend";
    EXPECT_DOUBLE_EQ(hmi::assetGalleryClipSeconds(still, 3.0), 0.0);
}

namespace {

/// Les 24 premiers octets d'un PNG de @p cote pixels de cote : tout ce que la galerie en lit.
void enTetePng(const std::filesystem::path& chemin, unsigned char cote) {
    std::ofstream(chemin, std::ios::binary)
        << std::string{"\x89PNG\r\n\x1a\n", 8} << std::string{"\0\0\0\rIHDR", 8}
        << std::string{"\0\0\0", 3} << static_cast<char>(cote) << std::string{"\0\0\0", 3}
        << static_cast<char>(cote);
}

}  // namespace

/**
 * @brief Un heros range par classe parait dans la galerie par son modele, clip par clip, avec son
 *        portrait et son jeton ; son `.glb` compte parmi les assets a montrer (`LOT-1006`,
 *        `EX-CNT-042`).
 * \castest{<b>Le modele du heros, son portrait et son jeton paraissent dans la galerie.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ecrire `Common/Characters/manifest.json`, qui nomme `Heroes/brawler` dans `npcs`,
 * sa fiche, son modele, le squelette humanoide a deux clips, un portrait et un jeton ; a cote, un
 * modele que rien n'inscrit.<br/>2. Lire le catalogue.<br/>3. Chercher les assets non listes.<br/>
 * \tattendu Le modele `Heroes/brawler` a ses entrees `idle` et `walk`, son portrait et son jeton ;
 * le seul asset non liste est le modele que rien n'inscrit ; aucune erreur.
 * }
 */
TEST(AssetGalleryTest, UnHerosEnModeleRangeParClasse) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "jadg_asset_gallery_hero";
    std::filesystem::remove_all(root);
    const std::filesystem::path characters = root / "Common" / "Characters";
    const std::filesystem::path heros = characters / "Heroes" / "brawler";
    std::filesystem::create_directories(heros);
    std::filesystem::create_directories(characters / "Skeletons" / "humanoid");
    std::filesystem::create_directories(characters / "Heroes" / "oublie");
    std::ofstream(characters / "manifest.json")
        << R"({"version": 1, "tile": [256, 159], "npcs": ["Heroes/brawler"]})";
    std::ofstream(characters / "Skeletons" / "humanoid" / "skeleton.json")
        << R"({"version": 1, "silhouette": "humanoid", "bones": [{"name": "Root", "parent": ""}],)"
           R"( "clips": [{"name": "idle", "duration": 1.0, "loop": true},)"
           R"( {"name": "walk", "duration": 0.5, "loop": true}]})";
    std::ofstream(heros / "character.json")
        << R"({"version": 1, "model": "brawler.glb", "skeleton": "humanoid"})";
    std::ofstream(heros / "brawler.glb") << "glb";
    std::ofstream(characters / "Heroes" / "oublie" / "oublie.glb") << "glb";
    enTetePng(heros / "portrait.png", 200);
    enTetePng(heros / "token.png", 128);

    const hmi::AssetGalleryCatalog catalog = hmi::AssetGalleryCatalog::load(root);
    const std::vector<std::string> unlisted = hmi::assetGalleryUnlisted(root, catalog);
    std::filesystem::remove_all(root);

    EXPECT_TRUE(catalog.errors.empty());
    const hmi::AssetGalleryFamily* const figurines =
        familyNamed(catalog, "Figurines · Common/Characters");
    ASSERT_NE(figurines, nullptr);
    std::vector<std::string> formes;
    for (const hmi::AssetGalleryEntry& entry : figurines->entries) {
        EXPECT_EQ(entry.model, "Heroes/brawler");
        formes.push_back(entry.form);
        if (entry.form == "walk") {
            EXPECT_EQ(entry.path, "Common/Characters/Heroes/brawler/brawler.glb");
            EXPECT_TRUE(entry.mesh);
            EXPECT_DOUBLE_EQ(entry.clipDuration, 0.5);
        }
    }
    EXPECT_EQ(formes, (std::vector<std::string>{"idle", "walk", "portrait", "token"}));
    EXPECT_EQ(unlisted, (std::vector<std::string>{"Common/Characters/Heroes/oublie/oublie.glb"}));
}

/**
 * @brief Un effet de `Common/Fx` se joue dans la galerie : sa bande se decoupe par son
 *        `.anim.json` (`LOT-136`, `EX-CNT-042`).
 * \castest{<b>Un effet parait anime dans la galerie.</b><br/>
 * \tcat Unitaire · Galerie des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire `Common/Fx/manifest.json` qui cite `fire-bolt.png` (2048 x 256), et son
 * `.anim.json` a huit images de 256 x 256.<br/>2. Lire le catalogue.<br/>3. Chercher les images
 * non listees.<br/>
 * \tattendu Une famille « Scene · Common/Fx » ; l'entree `fire-bolt` a des images de 256 x 256,
 * huit, jouees une fois ; aucune erreur, aucune image non listee.
 * }
 */
TEST(AssetGalleryTest, UnEffetSeJoueDansLaGalerie) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "jadg_asset_gallery_fx";
    std::filesystem::remove_all(root);
    const std::filesystem::path fx = root / "Common" / "Fx";
    std::filesystem::create_directories(fx);
    std::ofstream(fx / "manifest.json")
        << R"({"version": 1, "tile": [256, 159], "ground": 252, "textures": {"fire-bolt": )"
           R"({"file": "fire-bolt.png", "size": [2048, 256], "class": "fx"}}})";
    std::ofstream(fx / "fire-bolt.anim.json")
        << R"({"version": 1, "frameWidth": 256, "frameHeight": 256, "clips": {"fire-bolt": )"
           R"({"frames": [0, 1, 2, 3, 4, 5, 6, 7], "frameDuration": 0.06, "loop": false}}})";
    std::ofstream(fx / "fire-bolt.png") << "png";

    const hmi::AssetGalleryCatalog catalog = hmi::AssetGalleryCatalog::load(root);
    const std::vector<std::string> unlisted = hmi::assetGalleryUnlisted(root, catalog);
    std::filesystem::remove_all(root);

    EXPECT_TRUE(catalog.errors.empty()) << (catalog.errors.empty() ? "" : catalog.errors.front());
    const hmi::AssetGalleryFamily* const effets = familyNamed(catalog, "Scène · Common/Fx");
    ASSERT_NE(effets, nullptr);
    ASSERT_EQ(effets->entries.size(), 1U);
    const hmi::AssetGalleryEntry& trait = effets->entries.front();
    EXPECT_EQ(trait.form, "fire-bolt");
    EXPECT_EQ(trait.frameWidth, 256);
    EXPECT_EQ(trait.frameHeight, 256);
    EXPECT_EQ(trait.frameCount(), 8);
    EXPECT_FALSE(trait.loop);
    EXPECT_TRUE(unlisted.empty()) << unlisted.front();
}
