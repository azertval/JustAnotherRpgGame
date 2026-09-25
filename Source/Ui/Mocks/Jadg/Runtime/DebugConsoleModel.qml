pragma Singleton
import QtQuick

/*!
    Doublure de `hmi::DebugConsoleModel` pour Qt Design Studio.

    Le vrai modele rejoue les options du binaire dans la partie (console F9). Dans l'atelier, un
    compte rendu d'exemple, le catalogue reduit a deux options, et des methodes qui ne font rien.
*/
QtObject {
    readonly property var transcript: [ "Console de debug : tapez une ligne d'options du jeu, ou « aide ».",
                                        "> --map=capital/arenarea", "Carte ouverte : capital/arenarea" ]
    readonly property var history: [ "--map=capital/arenarea" ]
    readonly property var options: [
        { name: "--map=", syntax: "--map=<carte>[@<arrivee>]", description: "Ouvre la carte.", live: true },
        { name: "--data=", syntax: "--data=<racine>", description: "Racine du contenu.", live: false }
    ]

    signal transcriptChanged()
    signal screenRequested(string name)
    signal windowSizeRequested(int width, int height)
    signal screenshotRequested(string path)
    signal gameRequested()

    function run(line) {}
    function relaunch(line) { return false }
    function say(line) {}
    function clear() {}
}
