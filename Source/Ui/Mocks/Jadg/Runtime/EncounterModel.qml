pragma Singleton
import QtQuick

/*!
    Doublure de `hmi::EncounterModel` pour Qt Design Studio (LOT-118).

    Memes proprietes et methodes que le type C++ (`Source/HMI/Runtime/EncounterModel.h`), avec un
    combat d'exemple sur une zone de la carte, pour que l'affichage de combat se dessine dans
    l'atelier. Les methodes ne font rien : dans l'atelier, personne ne se bat.
*/
QtObject {
    readonly property bool active: true
    readonly property bool busy: false
    readonly property string outcome: ""
    readonly property string encounterName: "Les rats du donjon"
    readonly property int zoneColumn: 10
    readonly property int zoneRow: 10
    readonly property string heroName: "Grom Tranche-Écaille"
    readonly property string heroHitPoints: "15 / 15"
    readonly property real heroHitPointsRatio: 1
    readonly property var target: ({ name: "Rat", side: "enemies", hitPoints: "", hitPointsRatio: 1,
                                    armorClass: "10", speed: "9 m", conditions: "Aucun" })
    property int seed: 0

    readonly property string status: "Exemple de conception"
    readonly property bool inCombat: true
    readonly property bool ended: false
    readonly property int gridColumns: 20
    readonly property int gridRows: 14
    readonly property var fighters: [
        { column: 9, row: 12, footprint: 1, side: "allies", active: true, down: false, hitPoints: "15/15", hitPointsRatio: 1 },
        { column: 12, row: 10, footprint: 1, side: "enemies", active: false, down: false, hitPoints: "", hitPointsRatio: 1 },
        { column: 13, row: 11, footprint: 1, side: "enemies", active: false, down: true, hitPoints: "a terre", hitPointsRatio: 0 }
    ]
    readonly property var reachableCells: [
        { column: 10, row: 12 }, { column: 11, row: 12 }, { column: 9, row: 11 }, { column: 10, row: 11 }
    ]
    readonly property int cursorColumn: 12
    readonly property int cursorRow: 10
    readonly property var pathCells: []
    readonly property var turnActions: [
        { label: "Grande hache", kind: "attack", enabled: true, selected: true },
        { label: "Esquiver", kind: "dodge", enabled: true, selected: false },
        { label: "Se desengager", kind: "disengage", enabled: true, selected: false },
        { label: "Se precipiter", kind: "dash", enabled: true, selected: false },
        { label: "Reaction : saisir les opportunites", kind: "reaction", enabled: true, selected: false }
    ]
    readonly property var preview: [ "Grande hache -> Rat", "Jet requis 6 : 75 % de chances de toucher" ]
    readonly property var turnOrder: [
        { name: "Grom Tranche-Écaille", total: 14, side: "allies", active: true, down: false },
        { name: "Rat", total: 11, side: "enemies", active: false, down: false },
        { name: "Rat", total: 9, side: "enemies", active: false, down: true }
    ]
    readonly property string activeName: "Grom Tranche-Écaille"
    readonly property string activeResources: "action 1 · movement 6"
    readonly property var journal: [ "initiative Grom #1 = 14", "Tour de Grom" ]

    signal changed()
    signal cursorChanged()
    signal combatSceneChanged()
    signal finished(string outcome)

    function begin(encounterId) { return true }
    function leave() {}
    function skipAnimations() {}
    function tapCell(column, row) {}
    function moveCursor(columns, rows) {}
    function pointCursor(column, row) {}
    function centerCursor() {}
    function cycleTarget(step) {}
    function selectAction(index) {}
    function cycleAction(step) {}
    function confirm() {}
    function dodge() {}
    function disengage() {}
    function dash() {}
    function endTurn() {}
    function withdraw() {}
}
