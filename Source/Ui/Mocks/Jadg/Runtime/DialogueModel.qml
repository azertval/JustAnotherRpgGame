import QtQuick

/*!
    Doublure de `hmi::DialogueModel` pour Qt Design Studio (LOT-15).

    Memes proprietes et methodes que le type C++ (`Source/HMI/Runtime/DialogueModel.h`), avec la
    premiere replique du heraut du Colisee, pour que l'ecran se dessine dans l'atelier. Les reponses
    sont un `ListModel` aux roles de `SheetRowModel` (`rowId`, `label`, `value`). Les methodes ne
    font rien : dans l'atelier, personne ne parle.
*/
QtObject {
    property string dialogueId: "heraut-colisee"
    property int seed: 0
    readonly property string speakerName: "Le héraut du Colisée"
    readonly property string attitude: "Indifférent"
    readonly property string line: "Halte, voyageur. Vous voici aux portes de l'Arène du Futur. Ici, deux camps règlent ce qu'une guerre réglerait autrement — et personne n'en sort les pieds devant. Que cherchez-vous ?"
    readonly property string checkOutcome: ""
    readonly property string checkTitle: ""
    readonly property string checkDie: ""
    readonly property string checkDetail: ""
    readonly property string checkVerdict: ""
    readonly property bool checkSucceeded: false
    readonly property ListModel replies: ListModel {
        ListElement { rowId: "marque"; label: "Personne n'y meurt ? Comment est-ce possible ?"; value: "" }
        ListElement { rowId: "inscription"; label: "Je veux combattre dans l'arène."; value: "" }
        ListElement { rowId: "partir"; label: "Rien. Je ne faisais que passer."; value: "" }
    }
    readonly property bool finished: false
    readonly property string status: ""
    readonly property var dialogueIds: [ "heraut-colisee", "portier-colisee" ]

    signal changed()
    signal combatRequested(string arenaId)
    signal encounterRequested(string encounterId)
    signal demoEnded(string ending)

    function choose(rowId) {}
    function chooseAt(index) {}
    function restart() {}
}
