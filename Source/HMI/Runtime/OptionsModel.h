// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtQmlIntegration>

namespace core {
class MemoryLogSink;
}

/**
 * @file HMI/Runtime/OptionsModel.h
 * @brief Les réglages du jeu, et ce qu'ils atteignent (`LOT-86`).
 */

namespace hmi {

/**
 * @brief Vue-modèle des options : lit et persiste les réglages, et **publie** leur changement.
 *
 * ## La règle qui gouverne cette classe
 *
 * `EX-IHM-083` : **tout réglage exposé doit atteindre le moteur**. Une case à cocher qui ne fait
 * rien est pire qu'une case absente — elle fait croire à un réglage, et l'utilisateur cherche
 * ensuite pourquoi il n'a pas d'effet.
 *
 * Cette classe ne fait donc que deux choses : persister, et prévenir. C'est l'application qui
 * **branche** chaque signal sur ce qu'il doit atteindre — le moteur audio, la fenêtre, les
 * traducteurs. Le faire ici obligerait la présentation à connaître le moteur, et c'est précisément
 * la frontière que le lot établit.
 *
 * ## Ce qui s'applique quand
 *
 * | Réglage | Atteint |
 * |---|---|
 * | plein écran | la fenêtre, **immédiatement** |
 * | volume | le moteur audio, **immédiatement** |
 * | langue | les traducteurs et le QML, **immédiatement** |
 * | synchronisation verticale | le format de surface, **au prochain lancement** |
 * | anticrénelage | le tampon multi-échantillonné des vues du lieu, **immédiatement** |
 * | définition du rendu | la taille du tampon des vues du lieu, **immédiatement** |
 * | ombres | la carte d'ombres des vues du lieu, **immédiatement** |
 * | compteur de diagnostic | l'affichage tête haute, quand il existera |
 *
 * Les deux dernières lignes sont dites à l'écran, et non tues : un réglage qui s'applique plus tard
 * atteint bien le moteur, mais l'utilisateur doit savoir quand.
 *
 * « Immédiatement » s'entend **à l'écriture** d'une propriété. L'écran des options de la charte v2
 * (LOT-87, T3.2) n'écrit qu'au bouton « Appliquer » : il tient ses valeurs en attente, et
 * « Annuler » les abandonne sans que cette classe en ait rien su.
 */
class OptionsModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /// Le jeu occupe tout l'écran. Écrit, le réglage est enregistré et la fenêtre suit aussitôt.
    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(bool vsync READ vsync WRITE setVsync NOTIFY vsyncChanged)
    Q_PROPERTY(bool diagnostics READ diagnostics WRITE setDiagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    /// Taille des HUD d'exploration et de combat, en pourcentage (75 à 130).
    Q_PROPERTY(
        int hudScalePercent READ hudScalePercent WRITE setHudScalePercent NOTIFY hudScaleChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    /// Échantillons par pixel des vues du lieu : 1 (aucun anticrénelage), 2, 4 ou 8. C'est le
    /// `sampleCount` de `hmi::WorldViewportItem`, que le QML lie à ce réglage.
    Q_PROPERTY(int antialiasing READ antialiasing WRITE setAntialiasing NOTIFY antialiasingChanged)
    /// Définition du rendu du lieu, en pourcentage de celle de la fenêtre (100 à 200) : au-delà
    /// de 100, la scène est dessinée plus grande puis réduite à l'écran (suréchantillonnage).
    Q_PROPERTY(int renderScalePercent READ renderScalePercent WRITE setRenderScalePercent NOTIFY
                   renderScaleChanged)
    /// Les valeurs que l'écran propose pour `antialiasing` et `renderScalePercent`, dans l'ordre
    /// de leurs listes : une seule table, lue par le modèle qui valide et par l'écran qui affiche.
    Q_PROPERTY(QList<int> antialiasingLevels READ antialiasingLevels CONSTANT)
    Q_PROPERTY(QList<int> renderScales READ renderScales CONSTANT)
    /// Le côté de la carte d'ombres des vues du lieu, en texels (`LOT-1007`) : 0 (aucune ombre
    /// portée), 1024, 2048 ou 4096. C'est le `shadowSize` de `hmi::WorldViewportItem`.
    Q_PROPERTY(int shadows READ shadows WRITE setShadows NOTIFY shadowsChanged)
    Q_PROPERTY(QList<int> shadowSizes READ shadowSizes CONSTANT)

    /// Les langues que le jeu propose, par leur **code** (`fr`, `en`). Constantes : une langue
    /// s'ajoute avec son catalogue, donc par une construction, jamais à l'exécution.
    Q_PROPERTY(QStringList languages READ languages CONSTANT)

    /// Les mêmes, par leur **nom**, dans le même ordre — et chacun écrit dans sa propre langue.
    /// Un code est une clé technique : l'afficher aurait demandé au joueur de savoir que « en »
    /// désigne l'anglais, et ce sont précisément les clés à l'écran que ce lot supprime ailleurs.
    /// Ils ne passent pas par `qsTr` : « English » ne se traduit pas en français, il s'écrit ainsi
    /// dans la liste quelle que soit la langue courante — c'est ce qui permet d'en sortir.
    Q_PROPERTY(QStringList languageNames READ languageNames CONSTANT)

    /// `false` en Release, où aucun puits mémoire ne collecte les journaux de session. Le bouton
    /// se désactive alors plutôt que d'échouer une fois cliqué.
    Q_PROPERTY(bool logsAvailable READ logsAvailable CONSTANT)

    /// Les valeurs d'usine, par nom de réglage (ullscreen, sync, diagnostics, olume,
    /// language) : ce que le bouton « Par défaut » des options pose, et ce que le constructeur
    /// prend quand aucun réglage n'est enregistré. Une seule table, lue par les deux : recopiées
    /// en QML, elles auraient divergé au premier défaut changé ici.
    Q_PROPERTY(QVariantMap defaults READ defaults CONSTANT)

public:
    explicit OptionsModel(QObject* parent = nullptr);

    /// Désigne le puits mémoire des journaux de session. Appelé par l'application au démarrage :
    /// la présentation ne va pas le chercher elle-même, elle ne sait pas qu'il existe.
    void setSessionLog(core::MemoryLogSink* sessionLog) noexcept;

    [[nodiscard]] bool fullscreen() const noexcept {
        return _fullscreen;
    }
    [[nodiscard]] bool vsync() const noexcept {
        return _vsync;
    }
    [[nodiscard]] bool diagnostics() const noexcept {
        return _diagnostics;
    }
    [[nodiscard]] int volume() const noexcept {
        return _volume;
    }
    [[nodiscard]] int hudScalePercent() const noexcept {
        return _hudScalePercent;
    }
    [[nodiscard]] QString language() const {
        return _language;
    }
    [[nodiscard]] int antialiasing() const noexcept {
        return _antialiasing;
    }
    [[nodiscard]] int renderScalePercent() const noexcept {
        return _renderScalePercent;
    }
    [[nodiscard]] QList<int> antialiasingLevels() const;
    [[nodiscard]] QList<int> renderScales() const;
    [[nodiscard]] int shadows() const noexcept {
        return _shadows;
    }
    [[nodiscard]] QList<int> shadowSizes() const;
    [[nodiscard]] QStringList languages() const;
    [[nodiscard]] QStringList languageNames() const;
    [[nodiscard]] bool logsAvailable() const noexcept {
        return _sessionLog != nullptr;
    }
    [[nodiscard]] QVariantMap defaults() const;

    void setFullscreen(bool enabled);
    void setVsync(bool enabled);
    void setDiagnostics(bool enabled);
    void setVolume(int percent);
    void setHudScalePercent(int percent);
    void setLanguage(const QString& code);
    /// Une valeur hors de `antialiasingLevels` est ignorée.
    void setAntialiasing(int samples);
    /// Une valeur hors de `renderScales` est ignorée.
    void setRenderScalePercent(int percent);
    /// Une valeur hors de `shadowSizes` est ignorée.
    void setShadows(int texels);

    /**
     * @brief Écrit les journaux de la session dans un fichier horodaté, à côté de l'exécutable.
     * @return Un message **destiné à l'écran** : le chemin écrit, ou la raison de l'échec.
     *         Jamais une chaîne vide — un bouton qui ne répond rien laisse croire qu'il n'a rien
     *         fait, alors qu'il a peut-être échoué.
     */
    [[nodiscard]] Q_INVOKABLE QString saveLogs();

signals:
    void fullscreenChanged();
    void vsyncChanged();
    void diagnosticsChanged();
    void volumeChanged();
    void hudScaleChanged();
    void languageChanged();
    void antialiasingChanged();
    void renderScaleChanged();
    void shadowsChanged();

private:
    core::MemoryLogSink* _sessionLog = nullptr;
    bool _fullscreen = false;
    bool _vsync = true;
    bool _diagnostics = false;
    int _volume = 100;
    int _hudScalePercent = 100;
    QString _language;
    int _antialiasing = 4;
    int _renderScalePercent = 100;
    int _shadows = 2048;
};

}  // namespace hmi
