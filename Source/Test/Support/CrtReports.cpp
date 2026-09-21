// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file CrtReports.cpp
 * @brief Les assertions de la CRT (build Debug) vont sur la sortie d'erreur des exécutables de
 *        test, pas dans une boîte de dialogue.
 *
 * Un exécutable de test n'a pas de bootstrap d'application : sans ce réglage, une assertion de la
 * bibliothèque standard ouvrait une boîte modale et le test **attendait un clic** — sur le poste
 * comme en CI, où il finissait en délai dépassé sans un mot. Routée vers `stderr`, la même
 * assertion nomme son fichier et sa ligne, puis la CRT conclut par le chemin habituel.
 *
 * Le réglage est posé **avant `main`** : une assertion levée par un constructeur statique d'un
 * autre fichier serait sinon déjà passée.
 */

#include "HMI/Platform/CrashDump.h"

namespace {

/// Posé à l'initialisation statique : `hmi::routeCrtReportsToStderr` n'a besoin de rien d'autre.
const bool CRT_REPORTS_ROUTED = [] {
    hmi::routeCrtReportsToStderr();
    return true;
}();

}  // namespace
