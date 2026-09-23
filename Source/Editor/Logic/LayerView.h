// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Core/Levels/TileLayer.h"

/**
 * @file Editor/Logic/LayerView.h
 * @brief Couches d'une carte telles que l'éditeur les montre : lignes du panneau « Couches »,
 *        couche active, visibilité et opacité (`LOT-11`).
 */

namespace hmi {

/**
 * @brief Désignation d'une couche dans l'éditeur : un rang de `core::LevelDraft::layers()`, ou
 *        `std::nullopt` pour la grille racine — la collision, ou la grille unique d'une carte sans
 *        couche visuelle.
 */
using LayerSlot = std::optional<std::size_t>;

/// @brief Une ligne du panneau « Couches ».
struct LayerRow {
    LayerSlot slot;
    core::LayerKind kind = core::LayerKind::Legacy;
    std::string name;
    /// L'étage de la couche (`LOT-129`) : 0 au rez.
    int floor = 0;

    [[nodiscard]] bool operator==(const LayerRow&) const = default;
};

/**
 * @brief Les lignes du panneau, dans l'**ordre de dessin** : la grille racine d'abord, puis les
 *        couches visuelles dans l'ordre de `layers`.
 *
 * La grille racine n'a qu'une ligne, quel que soit le reflet qu'en porte `layers` : son rôle est
 * `Collision` dès qu'une couche visuelle existe, `Legacy` sinon — c'est ce qui dit à l'auteur si
 * ce qu'il peint là se voit en jeu.
 */
[[nodiscard]] std::vector<LayerRow> layerRows(const std::vector<core::TileLayer>& layers);

/// @return @p active s'il désigne encore une couche visuelle de @p layers, la grille racine sinon —
///         à réappliquer après toute mutation (annulation, retrait, ouverture d'une autre carte).
[[nodiscard]] LayerSlot validActiveLayer(const std::vector<core::TileLayer>& layers,
                                         LayerSlot active);

/// @brief Opacité par défaut de la collision **par-dessus** des couches visuelles : assez pour voir
///        où l'on bute, assez peu pour voir le sol dessous. Décision d'ergonomie, sans effet en
///        jeu.
inline constexpr float DEFAULT_COLLISION_OVERLAY_OPACITY = 0.55F;

/// @brief Opacité d'une couche **grisée** : assez pour s'y repérer, assez peu pour ne pas la
///        confondre avec celle qu'on peint (`LOT-EDITOR-02`, phase 3). Aide d'édition.
inline constexpr float DIMMED_LAYER_OPACITY = 0.3F;

/// @brief Comment une couche est montrée dans l'éditeur.
struct LayerDisplay {
    bool visible = true;
    float opacity = 1.0F;
    /// Montrée en retrait (`DIMMED_LAYER_OPACITY`), pour lire une autre couche par-dessus.
    bool dimmed = false;
    /// Verrouillée : visible, mais aucun geste ne la peint (`LOT-EDITOR-02`, phase 3).
    bool locked = false;

    /// @return L'opacité effective : 0 si masquée, réduite si grisée.
    [[nodiscard]] float effectiveOpacity() const noexcept {
        if (!visible) {
            return 0.0F;
        }
        return dimmed ? opacity * DIMMED_LAYER_OPACITY : opacity;
    }

    [[nodiscard]] bool operator==(const LayerDisplay&) const = default;
};

/**
 * @brief Visibilité et opacité des couches **dans l'éditeur** — une aide d'édition, jamais une
 *        propriété de la carte : rien n'est annulable, rien n'est enregistré (même statut que
 *        `hmi::PlaneVisibility`).
 *
 * Les réglages suivent le **rang** des couches, pas leur identité : une couche n'en a pas d'autre
 * dans le format. `swap` accompagne un déplacement fait depuis le panneau ; une annulation qui
 * réordonne les couches peut, elle, laisser un réglage sur la voisine — une gêne visuelle
 * passagère, qu'un clic répare, plutôt qu'un identifiant inventé pour l'éviter.
 */
class LayerViewState {
public:
    /// Oublie tous les réglages (ouverture d'une carte).
    void reset() noexcept;

    /// Ajuste le nombre de réglages à @p layerCount, en gardant ceux qui existent.
    void sync(std::size_t layerCount);

    /**
     * @brief Le réglage de @p slot.
     * @param slot             La couche.
     * @param hasVisualLayers  Vrai si la carte a une couche visuelle : la grille racine n'est alors
     *                         plus l'image mais la collision, montrée par-dessus à
     *                         `DEFAULT_COLLISION_OVERLAY_OPACITY` tant que l'auteur n'a pas choisi.
     */
    [[nodiscard]] LayerDisplay display(LayerSlot slot, bool hasVisualLayers) const;

    void setVisible(LayerSlot slot, bool visible);

    /// L'opacité est bornée à `[0, 1]` ; une valeur non finie est ignorée.
    void setOpacity(LayerSlot slot, float opacity);

    void setDimmed(LayerSlot slot, bool dimmed);
    void setLocked(LayerSlot slot, bool locked);
    /// Échange les réglages des rangs @p a et @p b (déplacement d'une couche).
    void swap(std::size_t a, std::size_t b);

    [[nodiscard]] bool operator==(const LayerViewState&) const = default;

private:
    [[nodiscard]] LayerDisplay& at(std::size_t index);

    std::vector<LayerDisplay> _layers;
    bool _rootVisible = true;
    bool _rootDimmed = false;
    bool _rootLocked = false;
    /// Absente tant que l'auteur n'a pas réglé l'opacité de la grille racine.
    std::optional<float> _rootOpacity;
};

}  // namespace hmi
