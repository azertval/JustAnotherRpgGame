// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file fuzz_mesh.cpp
 * @brief Cible libFuzzer de `core::readMeshFromGlb` (`LOT-1003`).
 *
 * Le chargeur de maillages : le seul lecteur **binaire** du dépôt. Il suit des décalages, des
 * longueurs et des indices que le fichier lui dicte — vues de tampon, accesseurs, indices de
 * sommet, graphe de nœuds — et alloue d'après des comptes que le fichier annonce.
 *
 * Le lecteur promet de rendre une erreur décrite, jamais de lever : toute exception qui s'échappe,
 * lecture hors bornes ou allocation démesurée est un défaut que le job `fuzz` de nuit rapporte avec
 * l'entrée qui le reproduit. Rejouer une entrée : `fuzz_mesh.exe crash-<empreinte>`.
 */

#include <cstddef>
#include <cstdint>
#include <span>

#include "Core/Resources/MeshFile.h"

/// @brief Point d'entrée appelé par libFuzzer pour chaque entrée générée.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const std::span<const std::byte> bytes(
        static_cast<const std::byte*>(static_cast<const void*>(data)), size);
    (void)core::readMeshFromGlb(bytes);
    return 0;
}
