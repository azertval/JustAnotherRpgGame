// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file Source/Test/Qml/QmlTestSetup.cpp
 * @brief Point d'entrée des tests Qt Quick de `Jadg.Ui` (refonte de la chaîne d'outillage, phase
 * 4).
 *
 * Les formulaires et les briques de la charte v2 n'avaient aucun test : qmllint vérifie qu'un
 * fichier se lit, pas qu'il se construit sans erreur de liaison, ni qu'un bouton désactivé prend
 * l'apparence désactivée, ni qu'un écran ressemble encore à ce qu'il était. Les `tst_*.qml` de ce
 * dossier le font, avec deux services fournis ici au QML :
 *
 * - `jadgUiFiles` : chaque `.ui.qml` du module, lu dans sa ressource — un écran ajouté est testé
 *   sans qu'on ait à le déclarer ;
 * - `referenceImages.compare(fichier, nom)` : une capture comparée à sa référence versionnée
 *   (`References/<nom>.png`), avec une tolérance.
 *
 * **Rendu logiciel, imposé.** Une capture GPU dépend du pilote : le poste (carte graphique) et le
 * runner (WARP) ne produiraient pas les mêmes pixels. Le moteur de rendu logiciel de Qt Quick
 * rastérise par QPainter, identique partout à version de Qt égale ; seul le lissage des polices
 * peut encore varier d'un pixel, d'où la tolérance.
 */

#include <QDir>
#include <QDirIterator>
#include <QFontDatabase>
#include <QImage>
#include <QObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QStringList>
#include <QVariantMap>
#include <algorithm>
#include <cstdlib>

#include <QtQuickTest/quicktest.h>

namespace {

/// Écart par composante (0-255) en deçà duquel deux pixels sont tenus pour identiques : l'arrondi
/// d'un lissage, pas un changement de dessin.
constexpr int kChannelTolerance = 24;

/// Part des pixels qui peuvent différer au-delà de kChannelTolerance : un glyphe lissé autrement
/// sur une ligne de texte, pas un bouton déplacé.
constexpr double kMaxDifferentRatio = 0.002;

/**
 * @brief Comparaison d'une capture à sa référence, appelée depuis le QML.
 *
 * Références dans `JADG_QML_REFERENCES_DIR` (versionné). Sur écart, la capture et une image des
 * différences sont écrites dans `JADG_QML_CAPTURES_DIR` (dossier de build, conservé en artefact par
 * la CI). Avec la variable d'environnement `JADG_UPDATE_REFERENCES=1`, la capture remplace la
 * référence.
 */
class ReferenceImages : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

    /// @param captureFile Capture écrite par `grabImage(item).save(…)` côté QML.
    /// @param name        Nom de la référence, sans extension.
    /// @return `{ ok, message, differentRatio }`.
    Q_INVOKABLE QVariantMap compare(const QString& captureFile, const QString& name) const {
        QVariantMap result;
        const QImage actual = QImage(captureFile).convertToFormat(QImage::Format_ARGB32);
        if (actual.isNull()) {
            result.insert(QStringLiteral("ok"), false);
            result.insert(QStringLiteral("message"), QStringLiteral("capture vide"));
            return result;
        }

        const QString referencePath =
            QDir(QStringLiteral(JADG_QML_REFERENCES_DIR)).filePath(name + QStringLiteral(".png"));
        const QDir captures(QStringLiteral(JADG_QML_CAPTURES_DIR));
        captures.mkpath(QStringLiteral("."));

        if (qEnvironmentVariable("JADG_UPDATE_REFERENCES") == QLatin1String("1")) {
            QDir().mkpath(QStringLiteral(JADG_QML_REFERENCES_DIR));
            const bool saved = actual.save(referencePath);
            result.insert(QStringLiteral("ok"), saved);
            result.insert(QStringLiteral("message"),
                          (saved ? QStringLiteral("référence écrite : ")
                                 : QStringLiteral("écriture impossible : ")) +
                              referencePath);
            return result;
        }

        const QImage expected = QImage(referencePath).convertToFormat(QImage::Format_ARGB32);
        if (expected.isNull()) {
            actual.save(captures.filePath(name + QStringLiteral(".actual.png")));
            result.insert(QStringLiteral("ok"), false);
            result.insert(QStringLiteral("message"),
                          QStringLiteral("référence absente (%1) ; capture dans %2. La créer : "
                                         "JADG_UPDATE_REFERENCES=1 ctest -R QmlTests")
                              .arg(referencePath, captures.absolutePath()));
            return result;
        }
        if (expected.size() != actual.size()) {
            actual.save(captures.filePath(name + QStringLiteral(".actual.png")));
            result.insert(QStringLiteral("ok"), false);
            result.insert(QStringLiteral("message"), QStringLiteral("taille %1x%2, référence %3x%4")
                                                         .arg(actual.width())
                                                         .arg(actual.height())
                                                         .arg(expected.width())
                                                         .arg(expected.height()));
            return result;
        }

        QImage diff(actual.size(), QImage::Format_ARGB32);
        diff.fill(Qt::black);
        qsizetype different = 0;
        for (int y = 0; y < actual.height(); ++y) {
            const auto* a = reinterpret_cast<const QRgb*>(actual.constScanLine(y));
            const auto* e = reinterpret_cast<const QRgb*>(expected.constScanLine(y));
            auto* d = reinterpret_cast<QRgb*>(diff.scanLine(y));
            for (int x = 0; x < actual.width(); ++x) {
                const int delta = std::max(
                    {std::abs(qRed(a[x]) - qRed(e[x])), std::abs(qGreen(a[x]) - qGreen(e[x])),
                     std::abs(qBlue(a[x]) - qBlue(e[x])), std::abs(qAlpha(a[x]) - qAlpha(e[x]))});
                if (delta > kChannelTolerance) {
                    ++different;
                    d[x] = qRgb(255, 0, 255);
                } else {
                    // La référence estompée, pour situer les écarts dans l'écran.
                    d[x] = qRgb(qGray(e[x]) / 3, qGray(e[x]) / 3, qGray(e[x]) / 3);
                }
            }
        }
        const double ratio =
            static_cast<double>(different) / static_cast<double>(actual.width() * actual.height());
        const bool ok = ratio <= kMaxDifferentRatio;
        if (!ok) {
            actual.save(captures.filePath(name + QStringLiteral(".actual.png")));
            diff.save(captures.filePath(name + QStringLiteral(".diff.png")));
        }
        result.insert(QStringLiteral("ok"), ok);
        result.insert(QStringLiteral("differentRatio"), ratio);
        result.insert(QStringLiteral("message"),
                      QStringLiteral("%1 pixel(s) différent(s) (%2 %, plafond %3 %)%4")
                          .arg(different)
                          .arg(ratio * 100.0, 0, 'f', 3)
                          .arg(kMaxDifferentRatio * 100.0, 0, 'f', 3)
                          .arg(ok ? QString()
                                  : QStringLiteral(" ; capture et différences dans ") +
                                        captures.absolutePath()));
        return result;
    }
};

class Setup : public QObject {
    Q_OBJECT

public:
    /// Construit avant l'application : ce qui suit doit précéder la base de polices et le style.
    Setup() {
        // La plateforme `offscreen` lit ses polices dans ce seul dossier, et aucune police du
        // système : les captures ne dépendent pas de la machine. Ce sont celles de l'identité, que
        // le jeu enregistre depuis son dossier (App/Game/Main.cpp).
        qputenv("QT_QPA_FONTDIR", QByteArrayLiteral(JADG_FONTS_DIR));
        // Le style du jeu (App/Game/Main.cpp) : les briques restylent Button, CheckBox… et le style
        // natif de Windows refuse cette personnalisation, avertissement à l'appui.
        QQuickStyle::setStyle(QStringLiteral("Basic"));
    }

public slots:
    /// Avant toute fenêtre : le choix du moteur de rendu ne s'applique qu'aux fenêtres à venir.
    void applicationAvailable() {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
        QDirIterator fonts(QStringLiteral(JADG_FONTS_DIR), {QStringLiteral("*.ttf")}, QDir::Files);
        while (fonts.hasNext()) {
            QFontDatabase::addApplicationFont(fonts.next());
        }
    }

    void qmlEngineAvailable(QQmlEngine* engine) {
        // Chemins relatifs au module, sans extension : `Controls/OrnateButton`,
        // `Screens/DialogueForm`.
        QStringList files;
        const QString root = QStringLiteral(":/qt/qml/Jadg/Ui/");
        QDirIterator iterator(root, {QStringLiteral("*.ui.qml")}, QDir::Files,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            QString path = iterator.next().mid(root.size());
            path.chop(QStringLiteral(".ui.qml").size());
            files.append(path);
        }
        files.sort();
        engine->rootContext()->setContextProperty(QStringLiteral("jadgUiFiles"), files);
        QDir().mkpath(QStringLiteral(JADG_QML_CAPTURES_DIR));
        engine->rootContext()->setContextProperty(QStringLiteral("captureDirectory"),
                                                  QStringLiteral(JADG_QML_CAPTURES_DIR));
        engine->rootContext()->setContextProperty(QStringLiteral("referenceImages"),
                                                  new ReferenceImages(engine));
    }
};

}  // namespace

QUICK_TEST_MAIN_WITH_SETUP(QmlTests, Setup)

#include "QmlTestSetup.moc"
