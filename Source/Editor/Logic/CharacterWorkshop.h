// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Editor/Logic/MapFormat.h"

/**
 * @file Editor/Logic/CharacterWorkshop.h
 * @brief L'**atelier des assets**, vue *Character* (`LOT-1008`, `EX-EDIT-102` à `EX-EDIT-104`) :
 *        écrire la fiche d'un personnage sans ouvrir un fichier.
 *
 * Un personnage est une fiche qui lie un modèle, son portrait et son jeton
 * (`Planning/standards/personnages-3d.md`). Ce que l'auteur choisit dans la fenêtre *Asset
 * workshop* tient dans une **fiche d'atelier** (`hmi::CharacterDraft`), un fichier JSON rangé avec
 * les sources du personnage ; l'installer écrit, sous `Assets/<niveau>/<nom>/`, le modèle, la
 * fiche `character.json`, le portrait et le jeton, et inscrit le personnage au manifeste de son
 * niveau. Logique **pure** : la fenêtre l'appelle, `LevelEditor --apply` la rejoue sans fenêtre,
 * et les deux écrivent les mêmes octets.
 *
 * ## La fiche d'atelier
 *
 * @code{.json}
 * {
 *   "format": "jadg-editor-character",
 *   "version": 1,
 *   "root": "..",
 *   "level": "Regions/central-empire/capital/arenarea/arena-of-fate/Characters",
 *   "name": "bandit",
 *   "skeleton": "humanoid",
 *   "model": "Lies/bandit/bandit.glb",
 *   "portrait": "Personnages/bandit/portraits/portrait.png",
 *   "token": "Personnages/bandit/portraits/token.png",
 *   "workshop": {
 *     "received": "Standard/Personnages/bandit/bandit.glb",
 *     "sheet": "Personnages/bandit/liaison.json",
 *     "retouch": "Personnages/bandit/retouche.json",
 *     "blend": "Lies/bandit/bandit.blend"
 *   }
 * }
 * @endcode
 *
 * - `root` : le dossier, relatif à la fiche d'atelier, d'où partent tous ses chemins (défaut : le
 *   dossier de la fiche). Les chemins s'inscrivent au manifeste **tels qu'ils sont écrits** : c'est
 *   la provenance de l'asset, lisible sur un autre poste.
 * - `level` : le dossier `Characters` du niveau, relatif à `Assets/` ; `name` : le personnage dans
 *   ce dossier (`bandit`, `Heroes/brawler`).
 * - `skeleton` : la silhouette à laquelle le modèle est lié ; sa description est celle du monde
 *   (`Common/Characters/Skeletons/<silhouette>/skeleton.json`). `skeletonSource` l'installe ou la
 *   rafraîchit d'abord, depuis le fichier que la chaîne de liaison a écrit.
 * - `model`, `portrait`, `token` : ce qu'on installe. Un champ **absent** garde ce qui est
 *   installé ; un personnage sans modèle installé doit en nommer un.
 * - `workshop` : ce dont l'aller-retour par Blender a besoin (`hmi::retouchFiles`,
 *   `Editor/Logic/BlenderRetouch.h`) ; l'installation ne le lit pas.
 *
 * ## Ce que l'installation vérifie
 *
 * Le modèle se lit, porte un squelette, et **chaque clip que sa silhouette déclare** ; le portrait
 * fait 512 × 512, le jeton 128 × 128 (`style-3d.md`, §7) — l'atelier ne retaille rien, il range ce
 * qui est au standard. Tout est vérifié avant la première écriture : une fiche refusée n'écrit
 * rien.
 *
 * ## Le contrôle
 *
 * `hmi::checkCharacters` relit ce qui est installé : c'est ce que `LevelEditor --check` ajoute au
 * contrôle des cartes, et ce que montre le panneau *Problems*.
 */

namespace hmi {

/// Le format d'une fiche d'atelier, écrit dans le fichier.
inline constexpr std::string_view CHARACTER_SCRIPT_FORMAT = "jadg-editor-character";
/// Sa version, et la plus élevée qui soit lue.
inline constexpr int CHARACTER_SCRIPT_VERSION = 1;
/// Le côté d'un portrait et d'un jeton installés, en pixels (`style-3d.md`, §7).
inline constexpr int CHARACTER_PORTRAIT_SIDE = 512;
inline constexpr int CHARACTER_TOKEN_SIDE = 128;
/// Les fichiers d'image d'un personnage, dans son dossier.
inline constexpr std::string_view CHARACTER_PORTRAIT_FILE = "portrait.png";
inline constexpr std::string_view CHARACTER_TOKEN_FILE = "token.png";

/// @brief Les fichiers de l'atelier local dont l'aller-retour par Blender a besoin, relatifs à la
///        racine de la fiche ; vides tant que l'auteur ne les a pas nommés.
struct CharacterWorkshopFiles {
    /// Le maillage reçu, déjà réduit au standard : ce que `rig_character.py` lie.
    std::string received;
    /// La fiche de liaison (`liaison.json`).
    std::string sheet;
    /// La fiche de retouche (`retouche.json`) ; à défaut, à côté de la fiche de liaison.
    std::string retouch;
    /// Le fichier Blender ; à défaut, à côté du modèle lié.
    std::string blend;

    [[nodiscard]] bool operator==(const CharacterWorkshopFiles&) const = default;
};

/// @brief La fiche d'atelier d'un personnage : ce que la vue *Character* édite.
struct CharacterDraft {
    /// Le dossier d'où partent les chemins, relatif à la fiche d'atelier (`.` par défaut).
    std::string root = ".";
    /// Le dossier `Characters` du niveau, relatif à `Assets/`.
    std::string level;
    /// Le personnage dans ce dossier (`bandit`, `Heroes/brawler`).
    std::string name;
    /// La silhouette du squelette (`humanoid`).
    std::string skeleton;
    /// La description du squelette à installer avec lui ; vide : celle du monde vaut.
    std::string skeletonSource;
    /// Le modèle lié (`.glb`) ; vide : celui qui est installé reste.
    std::string model;
    /// Le portrait, 512 × 512 ; vide : celui qui est installé reste.
    std::string portrait;
    /// Le jeton, 128 × 128 ; vide : celui qui est installé reste.
    std::string token;
    CharacterWorkshopFiles workshop;

    [[nodiscard]] bool operator==(const CharacterDraft&) const = default;
};

/// @brief Une fiche d'atelier lue, ou ce qui l'a refusée.
struct CharacterDraftResult {
    CharacterDraft draft;
    /// Message d'échec (anglais, comme toute sortie de l'éditeur), vide en cas de succès.
    std::string error;

    [[nodiscard]] bool ok() const noexcept {
        return error.empty();
    }
};

/// @return Vrai si @p json est une fiche d'atelier (`"format": "jadg-editor-character"`) : ce qui
///         distingue, pour `--apply`, un personnage d'un scénario de gestes.
[[nodiscard]] bool isCharacterScript(std::string_view json);

/// @brief Lit une fiche d'atelier depuis le texte de son fichier.
[[nodiscard]] CharacterDraftResult readCharacterDraft(std::string_view json);

/// @return Le texte canonique de @p draft : ce que la fenêtre enregistre, et que `readCharacterDraft`
///         relit à l'identique. Les champs vides ne s'écrivent pas.
[[nodiscard]] std::string characterDraftText(const CharacterDraft& draft);

/// @brief Un fichier que l'installation écrit.
struct CharacterFileWrite {
    /// Son chemin, absolu.
    std::filesystem::path file;
    std::string bytes;

    [[nodiscard]] bool operator==(const CharacterFileWrite&) const = default;
};

/// @brief Ce que l'installation d'un personnage fera, calculé sans rien écrire.
struct CharacterInstallPlan {
    /// Ce qui s'écrit, dans l'ordre : le squelette, les fichiers du personnage, les manifestes.
    std::vector<CharacterFileWrite> writes;
    /// Ce qui part du dossier du personnage : un ancien modèle, une image que rien ne cite plus.
    std::vector<std::filesystem::path> removals;
    /// Le compte rendu, une ligne par fichier (anglais).
    std::string log;
    /// Message d'échec, vide si le plan est exécutable.
    std::string error;

    [[nodiscard]] bool ok() const noexcept {
        return error.empty();
    }
};

/**
 * @brief Calcule l'installation de @p draft : ce qu'elle écrit et ce qu'elle retire.
 *
 * Un fichier dont les octets installés sont déjà ceux du plan n'est pas réécrit : réenregistrer
 * une fiche qui n'a pas changé ne touche à rien.
 *
 * @param dataRoot      La racine des données (`Source/Elements`) : les assets sont sous `Assets/`.
 * @param baseDirectory Le dossier de la fiche d'atelier, d'où part sa racine (`root`).
 * @param draft         La fiche d'atelier.
 */
[[nodiscard]] CharacterInstallPlan planCharacter(const std::filesystem::path& dataRoot,
                                                 const std::filesystem::path& baseDirectory,
                                                 const CharacterDraft& draft);

/// @brief Exécute @p plan. @return Ce qui a empêché l'écriture, vide si tout est écrit.
[[nodiscard]] std::string writeCharacter(const CharacterInstallPlan& plan);

/// @brief Un personnage installé, tel que les manifestes le déclarent.
struct InstalledCharacter {
    /// Le dossier `Characters` de son niveau, relatif à `Assets/`.
    std::string level;
    std::string name;

    [[nodiscard]] bool operator==(const InstalledCharacter&) const = default;
};

/// @return Les dossiers `Characters` qui portent un manifeste sous `<dataRoot>/Assets`, relatifs à
///         `Assets/`, triés : `Common/Characters`, puis ceux de `Regions/`.
[[nodiscard]] std::vector<std::string> characterLevels(const std::filesystem::path& dataRoot);

/// @return Les personnages en modèle (`npcs`) de tous les niveaux, triés par niveau puis par nom.
[[nodiscard]] std::vector<InstalledCharacter> installedCharacters(
    const std::filesystem::path& dataRoot);

/**
 * @brief La fiche d'atelier d'un personnage **installé** : ce que la fenêtre montre en le rouvrant.
 *
 * Le squelette vient de sa fiche `character.json`, les sources du manifeste de son niveau
 * (`models`, `sources`). Une source que @p workshopRoot ne contient plus est laissée vide : ce qui
 * est installé reste.
 *
 * @param dataRoot     La racine des données.
 * @param workshopRoot La racine de l'atelier local, où les sources du manifeste se cherchent.
 * @param character    Le personnage.
 */
[[nodiscard]] CharacterDraftResult draftOfInstalledCharacter(
    const std::filesystem::path& dataRoot, const std::filesystem::path& workshopRoot,
    const InstalledCharacter& character);

/**
 * @brief Contrôle les personnages installés (`EX-EDIT-104`).
 *
 * Pour chaque personnage en modèle : sa fiche se lit, son squelette est connu, son modèle est là,
 * porte un squelette et chaque clip de sa silhouette, son portrait et son jeton sont là, aux
 * tailles du standard — sauf pour un mannequin (`Mannequins/…`), qui n'en a pas. Les constats
 * portent le niveau du personnage en guise de carte.
 *
 * Les binaires des kits ne sont pas dans Git (`scripts/fetch_assets.py`) : sur une base dont les
 * kits sont verrouillés (`Assets/kits.lock.json`) mais pas installés (`Assets/.kits/`), seules les
 * fiches et les squelettes se contrôlent, et un avertissement le dit.
 */
[[nodiscard]] std::vector<MapCheckFinding> checkCharacters(const std::filesystem::path& dataRoot);

/**
 * @brief L'atelier **sans fenêtre** : `--apply <fiche d'atelier>` installe le personnage.
 *
 * Ne prend la main que si le fichier de `--apply` est une fiche d'atelier (`isCharacterScript`) :
 * un scénario de gestes reste à `hmi::runMapCommand`. Une fiche refusée n'écrit rien et sort en 1.
 *
 * @return Le code de sortie (0, 1 en cas de refus, 2 si la ligne de commande est fausse), ou
 *         `std::nullopt` si la commande n'est pas pour l'atelier.
 */
[[nodiscard]] std::optional<int> runCharacterCommand(const std::vector<std::string>& arguments,
                                                     const std::filesystem::path& dataRoot,
                                                     std::string& output);

}  // namespace hmi
