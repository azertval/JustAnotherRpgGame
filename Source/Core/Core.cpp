// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Core.h"

namespace core {

// Retourne la version du moteur.
// Chaîne de version sémantique (par exemple "0.0.5").
std::string Engine::version() {
    // JADG_VERSION est défini par Source/Core/CMakeLists.txt depuis PROJECT_VERSION :
    // le numéro n'existe qu'à un seul endroit (le `project()` racine), d'où il alimente aussi le
    // Doxyfile (contrôlé par scripts/docs/build_docs.py) et le tag de release.
    return JADG_VERSION;
}

}  // namespace core
