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
    readonly property var languages: ["fr", "en"]
    readonly property var languageNames: ["Français", "English"]
    readonly property bool logsAvailable: true
    readonly property var defaults: ({ fullscreen: false, vsync: true, diagnostics: false, volume: 100, hudScalePercent: 100, language: "fr" })

    function saveLogs() {
        return "";
    }
}
