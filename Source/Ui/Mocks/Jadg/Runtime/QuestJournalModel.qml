import QtQuick

/*!
    Doublure de `hmi::QuestJournalModel` pour Qt Design Studio (LOT-116).

    Memes proprietes et methodes que le type C++ (`Source/HMI/Runtime/QuestJournalModel.h`), avec
    une quete en cours, pour que l'ecran se dessine dans l'atelier. Les listes sont des `ListModel`
    aux roles de `SheetRowModel` (`rowId`, `label`, `value`). Les methodes ne font rien.
*/
QtObject {
    readonly property ListModel quests: ListModel {
        ListElement { rowId: "pommes"; label: "› Des pommes pour l'arène"; value: "En cours" }
    }
    readonly property string detail: "Un garde emmène l'enfant vers l'arène. Le rattraper sur le parvis d'Arenarea."
    readonly property ListModel objectives: ListModel {
        ListElement { rowId: "acceptee"; label: "Retrouver l'enfant de la marchande"; value: "✓" }
        ListElement { rowId: "parvis"; label: "Rattraper le garde sur le parvis"; value: "" }
    }
    readonly property string selected: "pommes"

    signal changed()

    function select(questId) {}
    function selectNeighbour(step) {}
    function refresh() {}
}
