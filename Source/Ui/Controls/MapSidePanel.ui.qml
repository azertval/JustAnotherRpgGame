pragma ComponentBehavior: Bound
import QtQuick
import Jadg.Ui

/*!
    Fiche posee sur une carte : celle d'une region, ou celle d'une ville (LOT-94, LOT-95).

    De haut en bas : le nom, une ligne d'ambiance (le regime de la region), des faits brefs
    (`facts`), les sept statistiques regionales du livre en cinq crans (`grades` -- une liste vide
    retire le bloc : une ville n'en a pas), la liste des lieux, et le texte du lieu choisi.

    - `entries` : `{ name, number, placed, gateway }`. Un lieu non pose sur la carte se lit en
      estompe ; un lieu `gateway` porte la marque d'un plan de ville a ouvrir.
    - `entryChosen(index)` au clic, `entryActivated(index)` au double clic : le jumeau choisit puis
      ouvre.
*/
PanelFrame {
    id: root

    property string title: "Central Empire"
    property string lore: "Absolute monarchy under Emperor Baleroth."
    property var facts: ["Tanarean Empire", "2 300 000 habitants"]

    /// Sept crans de 0 a 4, dans l'ordre de `statisticLabels` ; vide pour retirer le bloc.
    property var grades: [0, 1, 4, 3, 1, 1, 3]
    property var statisticLabels: ["Liberté", "Crime", "Prospérité", "Corruption", "Magie",
        "Monstres", "Stabilité"]

    property string entriesTitle: "Lieux"
    property var entries: [
        { name: "The Capital City", number: 0, placed: true, gateway: true },
        { name: "Hajal City", number: 0, placed: true, gateway: false },
        { name: "Cursed Ground", number: 0, placed: false, gateway: false }
    ]
    property int selectedIndex: 0
    property string description: "The Imperial Capital, situated in the heart of Tanares, serves as Emperor Baleroth's seat of power and is the largest city on the continent."

    signal entryChosen(int index)
    signal entryActivated(int index)

    subpanel: true

    Column {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Tokens.gapSmall

        Text {
            width: parent.width
            text: root.title
            color: Tokens.goldLight
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontSectionTitle
            font.weight: Font.DemiBold
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            visible: root.lore !== ""
            text: root.lore
            color: Tokens.textOnPanel
            font.family: Tokens.loreFamily
            font.italic: true
            font.pixelSize: Tokens.fontCaption
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }

        Repeater {
            model: root.facts

            Text {
                required property string modelData

                width: header.width
                text: modelData
                color: Tokens.textOnPanelMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
                elide: Text.ElideRight
            }
        }

        GoldDivider {
            width: parent.width
            visible: root.grades.length > 0
        }

        Grid {
            width: parent.width
            visible: root.grades.length > 0
            columns: 2
            columnSpacing: Tokens.gapMedium
            rowSpacing: 4 * Tokens.uiScale

            Repeater {
                model: root.grades

                Item {
                    id: statistic

                    required property int modelData
                    required property int index

                    width: (header.width - Tokens.gapMedium) / 2
                    height: Tokens.fontCaption + Tokens.gapSmall

                    Text {
                        anchors.left: parent.left
                        anchors.right: pips.left
                        anchors.rightMargin: Tokens.gapSmall
                        anchors.verticalCenter: parent.verticalCenter
                        text: statistic.index < root.statisticLabels.length
                              ? root.statisticLabels[statistic.index] : ""
                        color: Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                        elide: Text.ElideRight
                    }

                    Row {
                        id: pips

                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3 * Tokens.uiScale

                        Repeater {
                            model: 5

                            Rectangle {
                                required property int index

                                width: 9 * Tokens.uiScale
                                height: 9 * Tokens.uiScale
                                rotation: 45
                                antialiasing: true
                                color: index <= statistic.modelData ? Tokens.goldLight : "transparent"
                                border.color: Tokens.panelEdge
                                border.width: 1
                            }
                        }
                    }
                }
            }
        }

        GoldDivider {
            width: parent.width
        }

        Text {
            width: parent.width
            text: root.entriesTitle
            color: Tokens.goldLight
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontBody
            font.weight: Font.DemiBold
        }
    }

    ListView {
        id: list

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: detail.top
        anchors.topMargin: Tokens.gapSmall
        anchors.bottomMargin: Tokens.gapSmall
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.entries
        currentIndex: root.selectedIndex
        highlightMoveDuration: 0

        delegate: Rectangle {
            id: line

            required property var modelData
            required property int index

            readonly property bool chosen: root.selectedIndex === line.index

            width: list.width
            height: Tokens.fontBody + Tokens.gapMedium
            color: line.chosen ? Qt.rgba(Tokens.gemLight.r, Tokens.gemLight.g, Tokens.gemLight.b, 0.55)
                               : "transparent"
            border.color: line.chosen ? Tokens.panelEdge : "transparent"
            border.width: 1

            Text {
                anchors.left: parent.left
                anchors.right: mark.left
                anchors.leftMargin: Tokens.gapSmall
                anchors.rightMargin: Tokens.gapSmall
                anchors.verticalCenter: parent.verticalCenter
                text: (line.modelData.number > 0 ? line.modelData.number + " · " : "")
                      + line.modelData.name
                color: line.chosen ? Tokens.goldLight
                                   : line.modelData.placed && line.modelData.locked !== true
                                     ? Tokens.textOnPanel : Tokens.textOnPanelMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                elide: Text.ElideRight
            }

            Text {
                id: mark

                anchors.right: parent.right
                anchors.rightMargin: Tokens.gapSmall
                anchors.verticalCenter: parent.verticalCenter
                text: line.modelData.gateway ? "◈" : ""
                color: Tokens.goldLight
                font.pixelSize: Tokens.fontBody
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.entryChosen(line.index)
                onDoubleClicked: root.entryActivated(line.index)
            }
        }
    }

    Column {
        id: detail

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: Tokens.gapSmall

        GoldDivider {
            width: parent.width
        }

        Text {
            width: parent.width
            height: 7 * (Tokens.fontCaption + 4 * Tokens.uiScale)
            text: root.description
            color: Tokens.textOnPanel
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontCaption
            wrapMode: Text.WordWrap
            maximumLineCount: 7
            elide: Text.ElideRight
        }
    }
}
