pragma Singleton
import QtQuick

/*!
    Doublure de `hmi::OptionsModel` pour Qt Design Studio (LOT-87).

    Mêmes propriétés que le type C++ de `Source/HMI/Runtime/OptionsModel.h`, valeurs d'exemple :
    ce que la fenêtre et le jumeau des options lisent. Rien n'est persisté, rien n'atteint un moteur.
*/
QtObject {
    property bool fullscreen: false
    property bool vsync: true
    property bool diagnostics: false
    property int volume: 80
    property int hudScalePercent: 100
    property string language: "fr"
    property int antialiasing: 4
    property int renderScalePercent: 100
    readonly property var antialiasingLevels: [1, 2, 4, 8]
    readonly property var renderScales: [100, 125, 150, 200]
    property int shadows: 2048
    readonly property var shadowSizes: [0, 1024, 2048, 4096]
    readonly property var languages: ["fr", "en"]
    readonly property var languageNames: ["Français", "English"]
    readonly property bool logsAvailable: true
    readonly property var defaults: ({ fullscreen: false, vsync: true, diagnostics: false, volume: 100, hudScalePercent: 100, language: "fr", antialiasing: 4, renderScalePercent: 100, shadows: 2048 })

    function saveLogs() {
        return "";
    }
}
