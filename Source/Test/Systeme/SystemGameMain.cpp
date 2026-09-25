// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file SystemGameMain.cpp
 * @brief Point d'entrée des tests système du **jeu** : les modèles du jeu (`WorldModel`,
 *        `DialogueModel`, `EncounterModel`, `ScreenRouter`) vivent d'une boucle d'événements — la
 *        marche du héros est un `QTimer` — ; il en faut une, sans fenêtre.
 */

#include <QCoreApplication>

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
