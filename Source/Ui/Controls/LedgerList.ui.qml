pragma ComponentBehavior: Bound
import QtQuick
import Jadg.Ui

/*!
    Registre : un sous-panneau coiffe d'un bandeau, et une liste de lignes libelle / valeur
    (LOT-87, T3.7).

    Pour les listes des ecrans de la charte v2 qui n'ont pas de maquette propre -- recrutement,
    contrats, reserve de la compagnie ; quetes du journal, marchandises (T3.9). Le modele a les roles
    de `SheetRowModel` (`rowId`, `label`, `value`), comme ceux que pose `PendingData.rows()`.

    `material` choisit la matiere du sous-panneau, comme sur `PanelFrame`.

    `interactive` rend chaque ligne cliquable et emet `rowActivated(rowId)` : les reponses d'un
    dialogue (LOT-15) sont une liste comme les autres, que le joueur choisit. Faux par defaut --
    un registre qui se lit ne doit pas promettre un geste.
*/
PanelFrame {
    id: root

    property string title: "Registre"
    property bool interactive: false

    signal rowActivated(string rowId)

    /// Le modele de lignes : roles `rowId`, `label`, `value`.
    property var rows: exampleRows

    readonly property ListModel exampleRows: ListModel {
        ListElement { rowId: "a"; label: "Ligne"; value: "Valeur" }
        ListElement { rowId: "b"; label: "Ligne"; value: "Valeur" }
    }

    material: "parchment"
    subpanel: true

    SectionBanner {
        id: banner

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        material: root.material === "parchment" ? "parchment" : "dark"
        text: root.title
    }

    ListView {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: banner.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: Tokens.gapMedium
        clip: true
        spacing: Tokens.gapSmall
        boundsBehavior: Flickable.StopAtBounds
        model: root.rows

        delegate: Item {
            id: line

            required property string rowId
            required property string label
            required property string value

            width: ListView.view.width
            height: 44 * Tokens.uiScale

            Text {
                anchors.left: parent.left
                anchors.right: valueLabel.left
                anchors.rightMargin: Tokens.gapMedium
                anchors.verticalCenter: parent.verticalCenter
                text: line.label
                color: root.material === "parchment" ? Tokens.text : Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                elide: Text.ElideRight
            }

            Text {
                id: valueLabel

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: line.value
                color: root.material === "parchment" ? Tokens.textMuted : Tokens.textOnPanelMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
            }

            Rectangle {
                anchors.fill: parent
                color: Tokens.accent
                opacity: linePointer.containsMouse ? 0.18 : 0
            }

            MouseArea {
                id: linePointer

                anchors.fill: parent
                enabled: root.interactive
                hoverEnabled: root.interactive
                cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
            Connections {
                target: linePointer
                function onClicked() {
                    root.rowActivated(line.rowId)
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: Tokens.strokeWidth
                color: root.material === "parchment" ? Tokens.border : Tokens.panelEdge
                opacity: 0.5
            }
        }
    }
}
