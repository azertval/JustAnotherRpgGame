// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Core/Ecs/AnimationClip.h
 * @brief Clip d'animation en tant que donnée, et jeu de clips nommés.
 */

namespace core {

/// Manière dont un clip se termine à la dernière image.
enum class ClipEndMode {
    /// Revient à la première image et continue (comportement historique, `EX-REN-005`).
    Loop,
    /// S'arrête à la dernière image puis bascule sur `AnimationClip::nextClip`.
    OneShot,
};

/**
 * @brief Clip d'animation : une suite d'images, une durée par image, une manière de finir.
 *
 * Donnée **pure** (`EX-ARCH-011`) : ni méthode, ni type GPU, ni dépendance fichier — `Core` ne
 * connaît ni la spritesheet, ni la taille des images, ni le fichier d'origine (traduction en
 * région de texture laissée à `HMI`). Remplace l'ancien `enum class
 * AnimationClip` figé, qui aurait fait de `Core` un catalogue d'apparences à chaque
 * nouvel objet animé (`EX-ARCH-012`).
 */
struct AnimationClip {
    /// Nom du clip, unique au sein d'un `ClipSet` (ex. « idle », « opening »).
    std::string name;
    /// Suite d'indices d'images (dans la spritesheet de l'asset), dans l'ordre de lecture.
    std::vector<int> frames;
    /// Durée d'une image, en secondes. `0` ou une seule image : jamais animé (pose figée).
    float frameDuration = 0.0f;
    /// Bouclé, ou joué une fois avec bascule sur `nextClip`.
    ClipEndMode endMode = ClipEndMode::Loop;
    /// Nom du clip suivant, consulté seulement si `endMode == OneShot`. Vide (ou un nom qui
    /// n'existe pas dans le jeu) : reste sur la dernière image plutôt que de boucler ou planter.
    std::string nextClip;
};

/**
 * @brief Ensemble nommé de clips, adressable par nom — l'unité que décrit un fichier
 *        `nom-asset.anim.json`.
 *
 * Résout un nom en index à l'ajout plutôt qu'à chaque pas fixe (`AnimationSystem`) : la
 * progression générale d'une animation (avancer l'image courante) n'a donc jamais à comparer de
 * chaînes, même pour des centaines de tuiles animées. Seule la sélection
 * initiale d'un clip par son nom (ex. la projection état → clip du personnage) en compare.
 *
 * Accès **sûr** : interroger un clip par un index hors bornes ou un nom absent ne plante ni ne
 * lève — repli déterministe sur le premier clip du jeu (`EX-NFR-040`).
 */
class ClipSet {
public:
    /// Ajoute un clip au jeu. Un clip du même nom qu'un clip déjà présent le remplace.
    void addClip(AnimationClip clip);

    /// @return L'index du clip nommé @p name dans le jeu, ou `-1` s'il n'existe pas.
    [[nodiscard]] int indexOf(std::string_view name) const noexcept;

    /// @return Le nombre de clips du jeu.
    [[nodiscard]] int clipCount() const noexcept {
        return static_cast<int>(_clips.size());
    }

    /**
     * @brief Clip à un index donné, borné aux clips existants.
     *
     * Un index hors bornes (négatif ou trop grand) retombe sur le premier clip du jeu : repli
     * déterministe (`EX-NFR-040`), jamais un accès hors bornes. Un jeu **vide** renvoie un clip
     * par défaut (nom vide, une seule image d'indice 0, jamais animé) : l'appelant peut toujours
     * lire un `clipAt`, même sans avoir vérifié `clipCount()` au préalable.
     * @param index Index désiré (typiquement `core::Animation::clipIndex`).
     * @return Le clip résolu.
     */
    [[nodiscard]] const AnimationClip& clipAt(int index) const noexcept;

private:
    std::vector<AnimationClip> _clips;
    // Nom -> index dans _clips. std::less<> (comparateur transparent) : indexOf() compare sans
    // construire de std::string temporaire a partir du string_view recu.
    std::map<std::string, int, std::less<>> _byName;
};

}  // namespace core
