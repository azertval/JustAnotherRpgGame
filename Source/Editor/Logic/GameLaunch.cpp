// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/GameLaunch.h"

#include <fstream>
#include <system_error>

namespace hmi {

namespace {

/// Le nom du jeu construit, avec l'extension de la plateforme.
#ifdef _WIN32
constexpr const char* GAME_EXECUTABLE = "JustAnotherRpgGame.exe";
#else
constexpr const char* GAME_EXECUTABLE = "JustAnotherRpgGame";
#endif

/// Le dossier de l'essai, sous le dossier temporaire du systeme.
constexpr const char* PLAYTEST_FOLDER = "JustAnotherRpgGame-playtest";

}  // namespace

std::filesystem::path playtestDirectory() {
    std::error_code erreur;
    const std::filesystem::path temporaire = std::filesystem::temp_directory_path(erreur);
    // Un systeme sans dossier temporaire lisible : on retombe sur le dossier courant plutot que de
    // lever -- l'essai echouera a l'ecriture, avec un message, ce qui est deja le bon chemin.
    return (erreur ? std::filesystem::path{"."} : temporaire) / PLAYTEST_FOLDER;
}

std::string writeDraftMaps(const std::filesystem::path& directory,
                           const std::vector<DraftMap>& drafts) {
    std::error_code erreur;
    // Vide, puis recree : ce qui reste d'un essai precedent passerait DEVANT les cartes du jeu
    // sans que rien ne le dise -- on jouerait une carte fermee depuis.
    std::filesystem::remove_all(directory, erreur);
    if (erreur) {
        return "Le dossier d'essai ne se vide pas : " + erreur.message();
    }
    for (const DraftMap& carte : drafts) {
        const std::filesystem::path fichier = directory / (carte.mapId + ".json");
        std::filesystem::create_directories(fichier.parent_path(), erreur);
        if (erreur) {
            return "Le dossier d'essai ne se cree pas : " + erreur.message();
        }
        std::ofstream sortie(fichier, std::ios::binary);
        sortie << carte.json;
        if (!sortie) {
            return "La carte " + carte.mapId + " ne s'ecrit pas dans le dossier d'essai.";
        }
    }
    return {};
}

std::filesystem::path gameExecutable(const std::filesystem::path& editorDirectory) {
    const std::filesystem::path jeu = editorDirectory / GAME_EXECUTABLE;
    std::error_code erreur;
    return std::filesystem::exists(jeu, erreur) ? jeu : std::filesystem::path{};
}

}  // namespace hmi
