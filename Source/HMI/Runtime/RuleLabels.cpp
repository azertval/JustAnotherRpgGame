// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/RuleLabels.h"

#include <QSettings>
#include <QString>

#include "HMI/HmiLog.h"
#include "HMI/Localization/Localization.h"
#include "HMI/Platform/ExecutableDirectory.h"

namespace hmi {
namespace {

/**
 * @brief Le catalogue, charge UNE FOIS pour la langue demandee.
 *
 * Statique de fonction, et non membre d'une vue-modele : la fiche et l'inventaire demandent les
 * memes libelles, et deux catalogues liraient deux fois les memes fichiers pour rendre les memes
 * chaines. Rechargee seulement si la langue change -- ce qui n'arrive qu'au changement de langue,
 * pas a chaque lecture.
 */
[[nodiscard]] const Localization& catalog(const std::string& language) {
    static Localization loaded(executableDirectory() / "Localization");
    static std::string current;
    if (current != language) {
        // Le francais reste le repli : une cle absente d'un catalogue traduit doit rendre le terme
        // francais plutot que sa cle technique. C'est la regle de `Localization::text`, et c'est
        // pour cela que le defaut se charge en premier.
        if (!loaded.loadDefaultLanguage("fr")) {
            HMI_LOG_WARNING("Lexique des regles introuvable : les libelles resteront techniques.");
        }
        if (language != "fr" && !loaded.loadLanguage(language)) {
            HMI_LOG_INFO("Lexique des regles absent pour '" + language +
                         "' : repli sur le francais.");
        }
        current = language;
    }
    return loaded;
}

}  // namespace

/// Le catalogue de la racine de contenu imposee (`--data=`, LOT-118), s'il y en a une : ses cles
/// -- noms de cartes, dialogues d'essai -- passent devant celles du jeu. Rien si le contenu est
/// celui de l'executable.
[[nodiscard]] const Localization* contentCatalog(const std::string& language) {
    static const bool impose = dataDirectory() != executableDirectory();
    if (!impose) {
        return nullptr;
    }
    static Localization loaded(dataDirectory() / "Localization");
    static std::string current;
    if (current != language) {
        static_cast<void>(loaded.loadDefaultLanguage("fr"));
        if (language != "fr") {
            static_cast<void>(loaded.loadLanguage(language));
        }
        current = language;
    }
    return &loaded;
}

std::string ruleLabel(std::string_view key, const std::string& language) {
    if (const Localization* const contenu = contentCatalog(language)) {
        if (std::string texte = contenu->text(key); texte != key) {
            return texte;
        }
    }
    return catalog(language).text(key);
}

std::string activeLanguage() {
    return QSettings()
        .value(QStringLiteral("language"), QStringLiteral("fr"))
        .toString()
        .toStdString();
}

}  // namespace hmi
