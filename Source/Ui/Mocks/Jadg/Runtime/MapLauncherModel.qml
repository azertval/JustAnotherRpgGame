import QtQuick

/*!
    Doublure de `hmi::MapLauncherModel` pour Qt Design Studio.

    Le vrai modele liste les cartes d'un dossier Levels/ et en ouvre une dans la partie. Dans
    l'atelier, deux cartes d'exemple et des methodes qui ne font rien : personne n'y lance rien.
*/
QtObject {
    readonly property var maps: [
        { id: "capital/arenarea", name: "Arenarea", columns: 128, rows: 88,
          directory: "Levels", file: "Levels/capital/arenarea.json", error: "" },
        { id: "donjon", name: "Donjon d'essai", columns: 40, rows: 30,
          directory: "Levels", file: "Levels/donjon.json", error: "" }
    ]
    readonly property var directories: []
    readonly property string levelsRoot: "Levels"
    readonly property string status: ""

    signal changed()
    signal launched(string mapId)

    function refresh() {}
    function addDirectory(path) { return false }
    function removeDirectory(index) {}
    function launch(mapId, arrival, at, flags, heroFigure) { return false }
}
