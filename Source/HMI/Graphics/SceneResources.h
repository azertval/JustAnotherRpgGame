// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <memory>

#include "HMI/Graphics/RhiContext.h"

class QRhi;
class QRhiResourceUpdateBatch;

/**
 * @file HMI/Graphics/SceneResources.h
 * @brief Les ressources QRhi que **toute** surface de rendu du projet possède (`LOT-86`).
 */

namespace hmi {

class SpriteBatch;
class TextureAtlas;

/**
 * @brief Lot de sprites et atlas — créés ensemble, libérés ensemble, dans le bon ordre.
 *
 * **Pourquoi cette classe.** Depuis le `LOT-86`, deux surfaces dessinent la même scène : le
 * canevas de l'éditeur (`hmi::EditorViewport`, un `QRhiWidget`) et les éléments Qt Quick du jeu
 * (`hmi::WorldViewportItem`, `hmi::GameViewportItem`, des `QQuickRhiItem`). Elles n'ont ni le
 * même hôte, ni la même boucle, ni les mêmes entrées — mais elles créent **exactement** les mêmes
 * ressources graphiques.
 * Les écrire deux fois aurait suffi à les faire diverger : c'est ce qui arrive toujours, et ça ne
 * se voit qu'à l'exécution, sur une seule des deux.
 *
 * **L'ordre de libération est la raison d'être du regroupement.** Ce qui tient une texture doit
 * mourir avant la texture, et la texture avant le pipeline qui l'échantillonne. Libérer dans le
 * désordre ne produit pas une erreur nette mais un plantage à la fermeture, intermittent selon le
 * pilote. `release()` fixe cet ordre une fois pour toutes ; aucun appelant n'a plus à s'en
 * souvenir.
 *
 * **Ce qui n'est pas ici** : le brouillon d'édition, le `DraftRenderer`, la caméra, la carte jouée.
 * Ils appartiennent à un seul des deux hôtes — les remonter ici rendrait la classe dépendante de
 * l'éditeur, et le jeu paierait pour ce dont il ne se sert pas.
 */
class SceneResources {
public:
    SceneResources();
    ~SceneResources();

    SceneResources(const SceneResources&) = delete;
    SceneResources& operator=(const SceneResources&) = delete;

    /**
     * @brief Crée les ressources sur @p rhi, en déposant les téléversements dans @p updates.
     *
     * @p updates est le lot de la première image : les textures se chargent paresseusement et
     * leurs pixels y transitent. L'appelant le soumet ensuite, **hors** de toute passe de rendu —
     * c'est la contrainte de QRhi que `hmi::RhiContext` documente.
     */
    void create(QRhi* rhi, QRhiResourceUpdateBatch* updates);

    /// Libère tout, dans l'ordre imposé par les dépendances entre ressources.
    void release() noexcept;

    /// @return `true` si `create` a réussi et que rien n'a été libéré depuis.
    [[nodiscard]] bool created() const noexcept {
        return _spriteBatch != nullptr;
    }

    /// Déclare le lot de téléversements de l'image en cours (`nullptr` pour le retirer en fin
    /// d'image). Les propriétaires de textures y déposent leurs pixels pendant la composition.
    void setFrameUpdates(QRhiResourceUpdateBatch* updates) noexcept {
        _context.updates = updates;
    }

    /// @return Le contexte de rendu : le `QRhi` et le lot de téléversements de l'image en cours.
    [[nodiscard]] RhiContext& context() noexcept {
        return _context;
    }
    /// @return Le pipeline 2D ; valide seulement si `created()`.
    [[nodiscard]] SpriteBatch& sprites() noexcept {
        return *_spriteBatch;
    }
    /// @return L'atlas procédural des tuiles ; valide seulement si `created()`.
    [[nodiscard]] TextureAtlas& atlas() noexcept {
        return *_atlas;
    }

private:
    RhiContext _context;
    std::unique_ptr<SpriteBatch> _spriteBatch;
    std::unique_ptr<TextureAtlas> _atlas;
};

}  // namespace hmi
