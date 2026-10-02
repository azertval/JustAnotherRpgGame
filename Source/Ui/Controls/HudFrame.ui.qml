pragma ComponentBehavior: Bound
import QtQuick
import Jadg.Ui

/*! Cadre d'exploration. Les groupes se redimensionnent autour de leur bord d'ancrage ;
    la scène et les coordonnées du monde restent indépendantes de la préférence du HUD. */
Item {
    id: root
    property string mode: "exploration"
    property bool pending: false
    property bool showExperience: false
    property bool showQuests: true
    property string characterName: "Grom"
    property string level: "2"
    property string hitPointsText: "25 / 32"
    property real hitPointsRatio: 25 / 32
    property string experienceText: ""
    property real experienceRatio: 0
    property url portrait: ""
    property var party: [
        { id: "grom", label: "Grom", value: "25 / 32", ratio: 0.78, portrait: "" },
        { id: "helga", label: "Helga", value: "18 / 18", ratio: 1, portrait: "" },
        { id: "faelar", label: "Faelar", value: "12 / 12", ratio: 1, portrait: "" },
        { id: "nessa", label: "Nessa", value: "17 / 20", ratio: 0.85, portrait: "" }
    ]
    property int activeMember: 0
    property var quests: []
    property string trackedObjective: ""
    property string clock: ""
    property string location: "Martpart · Place du marché"
    property url minimap: ""
    property string interactionText: ""
    property real interactionX: width / 2
    property real interactionY: height / 2
    property bool objectiveExpanded: true
    signal memberClicked(int index)
    signal objectiveToggleRequested()
    signal interactRequested()

    property alias viewportHost: viewportArea
    property alias inventoryButton: inventoryControl
    property alias journalButton: journalControl
    property alias mapButton: mapControl
    property alias optionsButton: optionsControl
    property alias characterButton: characterControl
    property alias groupButton: groupControl
    default property alias content: contentArea.data

    width: 1920
    height: 1080

    Rectangle {
        id: viewportArea
        objectName: "worldViewportHost"
        anchors.fill: parent
        color: Tokens.background
    }
    Item { id: contentArea; anchors.fill: parent; z: 1 }

    Item {
        id: locationGroup
        z: 5
        visible: root.mode === "exploration"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Tokens.gapMedium
        width: 480 * Tokens.uiScale
        height: 86 * Tokens.uiScale
        scale: Tokens.hudScale
        transformOrigin: Item.TopLeft
        SectionBanner {
            width: parent.width
            text: root.location
            material: "dark"
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: Tokens.gapLarge
            anchors.bottom: parent.bottom
            text: root.clock
            visible: text.length > 0
            color: Tokens.textOnPanel
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontBody
            style: Text.Outline
            styleColor: Tokens.panel
        }
    }

    Item {
        id: orientationGroup
        z: 5
        visible: root.mode === "exploration"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Tokens.gapMedium
        width: 310 * Tokens.uiScale
        height: (root.minimap.toString().length > 0 ? 310 : 70) * Tokens.uiScale
                + (objective.visible ? objective.height : 0)
        scale: Tokens.hudScale
        transformOrigin: Item.TopRight
        Item {
            id: mapDisc
            anchors.horizontalCenter: parent.horizontalCenter
            width: 230 * Tokens.uiScale
            height: width
            visible: root.minimap.toString().length > 0
            Image {
                anchors.fill: parent
                anchors.margins: 34 * Tokens.uiScale
                source: root.minimap
                fillMode: Image.PreserveAspectFit
            }
            FixedArt { anchors.fill: parent; key: "ui/medallion/minimap-ring" }
        }
        OrnateButton {
            id: mapControl
            objectName: "hudMap"
            anchors.horizontalCenter: parent.horizontalCenter
            y: mapDisc.visible ? mapDisc.height : 0
            width: 240 * Tokens.uiScale
            kind: "secondary"
            text: qsTr("Carte")
            iconKey: "ui/icon/nav/map"
        }
        PanelFrame {
            id: objective
            anchors.top: mapControl.bottom
            anchors.topMargin: Tokens.gapSmall
            width: parent.width
            height: (root.objectiveExpanded ? 172 : 62) * Tokens.uiScale
            visible: root.showQuests && root.trackedObjective.length > 0
            subpanel: true
            Column {
                anchors.fill: parent
                spacing: Tokens.gapSmall
                OrnateButton {
                    id: objectiveToggle
                    width: parent.width
                    height: 40 * Tokens.uiScale
                    kind: "secondary"
                    text: qsTr("Objectif suivi") + (root.objectiveExpanded ? " ▾" : " ▸")
                }
                Connections {
                    target: objectiveToggle
                    function onClicked() {
                        root.objectiveToggleRequested()
                    }
                }
                Text {
                    width: parent.width
                    visible: root.objectiveExpanded
                    text: root.trackedObjective
                    color: Tokens.textOnPanel
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontBody
                    wrapMode: Text.WordWrap
                    maximumLineCount: 4
                    elide: Text.ElideRight
                }
            }
        }
    }

    PanelFrame {
        id: partyDock
        objectName: "explorationPartyDock"
        z: 5
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.gapMedium
        width: 590 * Tokens.uiScale
        height: 194 * Tokens.uiScale
        visible: root.mode === "exploration"
        scale: Tokens.hudScale
        transformOrigin: Item.BottomLeft
        subpanel: true
        padding: Tokens.gapSmall
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            Repeater {
                model: root.party
                delegate: Item {
                    id: member
                    required property var modelData
                    required property int index
                    width: 140 * Tokens.uiScale
                    height: 162 * Tokens.uiScale
                    PortraitFrame {
                        anchors.horizontalCenter: parent.horizontalCenter
                        size: 116 * Tokens.uiScale
                        shape: "round"
                        source: member.modelData.portrait || ""
                        active: member.index === root.activeMember
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 99 * Tokens.uiScale
                        width: parent.width - Tokens.gapSmall
                        horizontalAlignment: Text.AlignHCenter
                        text: (member.index === root.activeMember ? "◆ " : "") + member.modelData.label
                        color: member.index === root.activeMember ? Tokens.goldLight : Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontBody
                        elide: Text.ElideRight
                    }
                    Gauge {
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 128 * Tokens.uiScale
                        width: 126 * Tokens.uiScale
                        kind: "health"
                        value: member.modelData.ratio || 0
                        label: member.modelData.value
                    }
                    MouseArea {
                        id: memberPointer
                        anchors.fill: parent
                    }
                    Connections {
                        target: memberPointer
                        function onClicked() {
                            root.memberClicked(member.index)
                        }
                    }
                }
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            text: qsTr("Tab · Changer de meneur")
            color: Tokens.textOnPanel
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontCaption
        }
    }

    Item {
        id: navigationDock
        objectName: "hudNavigation"
        z: 5
        anchors.right: parent.right
        anchors.top: root.mode === "combat" ? parent.top : undefined
        anchors.bottom: root.mode === "combat" ? undefined : parent.bottom
        anchors.margins: Tokens.gapMedium
        width: (root.mode === "combat" ? 288 : 760) * Tokens.uiScale
        height: 90 * Tokens.uiScale
        scale: Tokens.hudScale
        transformOrigin: root.mode === "combat" ? Item.TopRight : Item.BottomRight
        PanelFrame { anchors.fill: parent; subpanel: true; padding: 0 }
        Row {
            anchors.centerIn: parent
            spacing: 2 * Tokens.uiScale
            OrnateButton {
                id: characterControl
                objectName: "hudCharacter"
                width: 164 * Tokens.uiScale
                text: qsTr("Personnage")
                kind: "secondary"
            }
            OrnateButton {
                id: inventoryControl
                visible: root.mode !== "combat"
                width: 174 * Tokens.uiScale
                text: qsTr("Équipement")
                kind: "secondary"
            }
            OrnateButton {
                id: journalControl
                visible: root.mode !== "combat"
                width: 124 * Tokens.uiScale
                text: qsTr("Journal")
                kind: "secondary"
            }
            OrnateButton {
                id: groupControl
                visible: root.mode !== "combat"
                width: 132 * Tokens.uiScale
                text: qsTr("Groupe")
                kind: "secondary"
            }
            OrnateButton {
                id: optionsControl
                width: 112 * Tokens.uiScale
                text: root.mode === "combat" ? qsTr("Options") : qsTr("Menu")
                kind: "secondary"
            }
        }
    }

    PanelFrame {
        objectName: "interactionPrompt"
        z: 6
        visible: root.mode === "exploration" && root.interactionText.length > 0
        x: Math.max(Tokens.gapMedium, Math.min(root.width - width * Tokens.hudScale - Tokens.gapMedium, root.interactionX - width * Tokens.hudScale / 2))
        y: Math.max(100 * Tokens.uiScale, Math.min(root.height - height * Tokens.hudScale - 240 * Tokens.uiScale, root.interactionY - height * Tokens.hudScale))
        width: 270 * Tokens.uiScale
        height: 68 * Tokens.uiScale
        subpanel: true
        scale: Tokens.hudScale
        transformOrigin: Item.TopLeft
        Text {
            anchors.centerIn: parent
            text: root.interactionText
            color: Tokens.textOnPanel
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontSectionTitle
        }
        MouseArea { id: interactPointer; anchors.fill: parent }
        Connections { target: interactPointer; function onClicked() { root.interactRequested() } }
    }
}
