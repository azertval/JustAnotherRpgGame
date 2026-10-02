pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import Jadg.Ui

/*! Inventaire du membre choisi : porté, sac filtré et conséquences de l'équipement. */
Item {
    id: root
    property var equipped: ({})
    property var cells: []
    property int filter: 0
    property string selectedItem: ""
    property string selectedSlot: ""
    property var selection: ({})
    property string carried: "—"
    property string capacity: "—"
    property real loadRatio: 0
    property string gold: "—"
    property string armorClass: "—"
    property string initiative: "—"
    property string speed: "—"
    property string passivePerception: "—"
    property string characterName: "Grom Tranche-Écaille"
    property url portrait: ""
    property bool canManage: true
    property bool dropPending: false
    property alias slotRepeaterLeft: slotsLeft
    property alias slotRepeaterRight: slotsRight
    property alias cellRepeater: cellsRepeater
    property alias allTab: allTabControl
    property alias equipmentTab: equipmentTabControl
    property alias gearTab: gearTabControl
    property alias toolsTab: toolsTabControl
    property alias sortButton: sortControl
    property alias equipButton: equipControl
    property alias dropButton: dropControl
    property alias searchField: searchControl
    signal tabRequested(int index)
    signal closeRequested()
    signal memberStepRequested(int step)
    signal dropConfirmed()
    signal dropCancelled()
    readonly property bool hasSelection: root.selection.name !== undefined
    readonly property var slotLabels: ({
        "head": qsTr("Tête"), "neck": qsTr("Cou"), "cloak": qsTr("Cape"),
        "torso": qsTr("Armure"), "belt": qsTr("Ceinture"), "hands": qsTr("Mains"),
        "bracers": qsTr("Brassards"), "feet": qsTr("Pieds"),
        "main-hand": qsTr("Main principale"), "off-hand": qsTr("Main secondaire"),
        "ranged": qsTr("À distance"), "ammunition": qsTr("Munitions"),
        "ring-left": qsTr("Anneau gauche"), "ring-right": qsTr("Anneau droit"),
        "pouch": qsTr("Sacoche"), "trinket": qsTr("Accessoire")
    })
    width: 1920
    height: 1080
    Rectangle { anchors.fill: parent; color: Tokens.frameEdge }
    PanelFrame { anchors.fill: parent; anchors.margins: 14 * Tokens.uiScale; material: "parchment"; bound: true; padding: 0 }
    TitlePlate { anchors.horizontalCenter: parent.horizontalCenter; y: 16 * Tokens.uiScale; width: 1170 * Tokens.uiScale; text: qsTr("Équipement") }
    CodexTabs { id: codexTabs; anchors.right: parent.right; anchors.rightMargin: 92 * Tokens.uiScale; y: 142 * Tokens.uiScale; currentIndex: 3 }
    Connections {
        target: codexTabs
        function onRequested(index) {
            root.tabRequested(index)
        }
    }
    Row {
        x: 90 * Tokens.uiScale
        y: 146 * Tokens.uiScale
        spacing: Tokens.gapSmall
        OrnateButton { id: previousMemberButton; width: 70 * Tokens.uiScale; height: 44 * Tokens.uiScale; text: "◀"; kind: "secondary" }
        Connections { target: previousMemberButton; function onClicked() { root.memberStepRequested(-1) } }
        OrnateButton { id: nextMemberButton; width: 70 * Tokens.uiScale; height: 44 * Tokens.uiScale; text: "▶"; kind: "secondary" }
        Connections { target: nextMemberButton; function onClicked() { root.memberStepRequested(1) } }
    }

    PanelFrame {
        x: 50 * Tokens.uiScale
        y: 226 * Tokens.uiScale
        width: 570 * Tokens.uiScale
        height: 772 * Tokens.uiScale
        material: "parchment"
        padding: Tokens.gapMedium
        SectionBanner { width: parent.width; text: qsTr("Équipement porté"); material: "parchment" }
        Text { y: 70 * Tokens.uiScale; width: parent.width; text: root.characterName; color: Tokens.text; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
        PortraitFrame { anchors.horizontalCenter: parent.horizontalCenter; y: 220 * Tokens.uiScale; size: 270 * Tokens.uiScale; source: root.portrait }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 516 * Tokens.uiScale
            width: 200 * Tokens.uiScale
            text: qsTr("Armure\n%1\n\nVitesse\n%2").arg(root.armorClass).arg(root.speed)
            horizontalAlignment: Text.AlignHCenter
            color: Tokens.gem
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontReading
        }
        Column {
            y: 110 * Tokens.uiScale
            spacing: 4 * Tokens.uiScale
            Repeater {
                id: slotsLeft
                model: ["head", "cloak", "torso", "hands", "feet", "belt", "bracers", "neck"]
                delegate: ItemSlot {
                    id: leftSlot
                    required property string modelData
                    width: 98 * Tokens.uiScale
                    height: 74 * Tokens.uiScale
                    label: root.equipped[leftSlot.modelData] && root.equipped[leftSlot.modelData].name.length > 0 ? root.equipped[leftSlot.modelData].name : root.slotLabels[leftSlot.modelData]
                    equipped: root.equipped[leftSlot.modelData] !== undefined && root.equipped[leftSlot.modelData].itemId.length > 0
                    selected: root.selectedSlot === leftSlot.modelData
                }
            }
        }
        Column {
            anchors.right: parent.right
            y: 110 * Tokens.uiScale
            spacing: 4 * Tokens.uiScale
            Repeater {
                id: slotsRight
                model: ["main-hand", "off-hand", "ranged", "ammunition", "ring-left", "ring-right", "pouch", "trinket"]
                delegate: ItemSlot {
                    id: rightSlot
                    required property string modelData
                    width: 98 * Tokens.uiScale
                    height: 74 * Tokens.uiScale
                    label: root.equipped[rightSlot.modelData] && root.equipped[rightSlot.modelData].name.length > 0 ? root.equipped[rightSlot.modelData].name : root.slotLabels[rightSlot.modelData]
                    equipped: root.equipped[rightSlot.modelData] !== undefined && root.equipped[rightSlot.modelData].itemId.length > 0
                    selected: root.selectedSlot === rightSlot.modelData
                }
            }
        }
    }
    PanelFrame {
        x: 640 * Tokens.uiScale
        y: 226 * Tokens.uiScale
        width: 580 * Tokens.uiScale
        height: 772 * Tokens.uiScale
        material: "parchment"
        padding: Tokens.gapMedium
        SectionBanner { width: parent.width; text: qsTr("Sac"); material: "parchment" }
        TextField {
            id: searchControl
            objectName: "inventorySearch"
            y: 70 * Tokens.uiScale
            width: parent.width
            height: 48 * Tokens.uiScale
            placeholderText: qsTr("Rechercher un objet…")
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontReading
            color: Tokens.text
            selectByMouse: true
            background: Rectangle { color: Tokens.surfaceAlt; border.color: Tokens.border; border.width: Tokens.strokeWidth }
        }
        Row {
            y: 132 * Tokens.uiScale
            spacing: 4 * Tokens.uiScale
            OrnateTab { id: allTabControl; width: 108 * Tokens.uiScale; leftPadding: Tokens.gapSmall; rightPadding: Tokens.gapSmall; material: "parchment"; text: qsTr("Tous"); checked: root.filter === 0; checkable: false }
            OrnateTab { id: equipmentTabControl; width: 154 * Tokens.uiScale; leftPadding: Tokens.gapSmall; rightPadding: Tokens.gapSmall; material: "parchment"; text: qsTr("Équipement"); checked: root.filter === 1; checkable: false }
            OrnateTab { id: gearTabControl; width: 136 * Tokens.uiScale; leftPadding: Tokens.gapSmall; rightPadding: Tokens.gapSmall; material: "parchment"; text: qsTr("Matériel"); checked: root.filter === 2; checkable: false }
            OrnateTab { id: toolsTabControl; width: 124 * Tokens.uiScale; leftPadding: Tokens.gapSmall; rightPadding: Tokens.gapSmall; material: "parchment"; text: qsTr("Outils"); checked: root.filter === 3; checkable: false }
        }
        Flickable {
            y: 205 * Tokens.uiScale
            width: parent.width
            height: 400 * Tokens.uiScale
            contentHeight: inventoryGrid.implicitHeight
            clip: true
            ScrollBar.vertical: OrnateScrollBar {}
            Grid {
                id: inventoryGrid
                width: parent.width
                columns: 4
                spacing: Tokens.gapSmall
                Repeater {
                    id: cellsRepeater
                    model: root.cells
                    delegate: ItemSlot {
                        id: cell
                        required property var modelData
                        width: 125 * Tokens.uiScale
                        height: 125 * Tokens.uiScale
                        label: cell.modelData.name
                        quantity: cell.modelData.quantity
                        selected: root.selectedItem === cell.modelData.itemId
                    }
                }
            }
            Text { anchors.centerIn: parent; visible: root.cells.length === 0; text: qsTr("Aucun objet."); color: Tokens.textMuted; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading }
        }
        Text {
            y: 615 * Tokens.uiScale
            text: qsTr("Poids : %1 / %2").arg(root.carried).arg(root.capacity)
            color: Tokens.text
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontReading
        }
        Gauge { y: 650 * Tokens.uiScale; width: 330 * Tokens.uiScale; kind: "weight"; value: root.loadRatio }
        Text { y: 691 * Tokens.uiScale; text: root.gold + " " + qsTr("po"); color: Tokens.text; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading }
        OrnateButton { id: sortControl; anchors.right: parent.right; anchors.bottom: parent.bottom; width: 170 * Tokens.uiScale; text: qsTr("Trier"); kind: "secondary"; enabled: root.canManage && root.cells.length > 1 }
    }
    PanelFrame {
        x: 1240 * Tokens.uiScale
        y: 226 * Tokens.uiScale
        width: 630 * Tokens.uiScale
        height: 772 * Tokens.uiScale
        material: "parchment"
        SectionBanner { id: itemTitle; width: parent.width; text: root.selection.name || qsTr("Choisissez un objet"); material: "parchment" }
        Flickable {
            anchors.top: itemTitle.bottom
            anchors.topMargin: Tokens.gapMedium
            anchors.bottom: itemActions.top
            anchors.bottomMargin: Tokens.gapMedium
            width: parent.width
            contentHeight: itemDetails.implicitHeight
            clip: true
            ScrollBar.vertical: OrnateScrollBar {}
            Column {
                id: itemDetails
                width: parent.width
                spacing: Tokens.gapMedium
                Text { width: parent.width; text: root.selection.kind || ""; color: Tokens.text; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading; wrapMode: Text.WordWrap }
                Text { width: parent.width; text: root.selection.damage || root.selection.armor || ""; color: Tokens.gem; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontScreenTitle; wrapMode: Text.WordWrap }
                Text { width: parent.width; text: root.hasSelection ? qsTr("Poids : %1").arg(root.selection.weight || "—") : ""; color: Tokens.text; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading; wrapMode: Text.WordWrap }
                Text { width: parent.width; text: root.selection.text || ""; color: Tokens.text; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading; wrapMode: Text.WordWrap }
                GoldDivider { width: parent.width; visible: root.selection.replaces !== undefined }
                Text { width: parent.width; visible: root.selection.replaces !== undefined; text: qsTr("Remplace : %1").arg(root.selection.replaces || ""); color: Tokens.text; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading; wrapMode: Text.WordWrap }
                Text {
                    width: parent.width
                    visible: root.selection.afterArmor !== undefined
                    text: qsTr("Avant → Après") + "\n" + qsTr("CA : %1 → %2").arg(root.selection.beforeArmor || "").arg(root.selection.afterArmor || "")
                          + (root.selection.afterDamage ? "\n" + qsTr("Dés de l’arme : %1 → %2").arg(root.selection.beforeDamage || "—").arg(root.selection.afterDamage) : "")
                    color: Tokens.text
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontReading
                    wrapMode: Text.WordWrap
                }
                Text { width: parent.width; visible: !root.canManage; text: qsTr("L’équipement se change hors combat."); color: Tokens.gem; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading; wrapMode: Text.WordWrap }
            }
        }
        Column {
            id: itemActions
            width: parent.width
            anchors.bottom: parent.bottom
            spacing: Tokens.gapSmall
            OrnateButton { id: equipControl; width: parent.width; text: root.selectedSlot.length > 0 ? qsTr("Retirer") : qsTr("Équiper"); enabled: root.canManage && (root.selection.canEquip === true || root.selection.canUnequip === true) }
            OrnateButton { id: dropControl; width: parent.width; text: qsTr("Jeter un exemplaire"); kind: "secondary"; enabled: root.canManage && root.selection.canDrop === true }
        }
    }
    OrnateButton { id: closeButton; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 24 * Tokens.uiScale; width: 300 * Tokens.uiScale; kind: "back"; text: qsTr("Retour au jeu") }
    Connections { target: closeButton; function onClicked() { root.closeRequested() } }
    Rectangle {
        anchors.fill: parent
        visible: root.dropPending
        color: Tokens.panel
        opacity: 0.7
        MouseArea { id: dropBackdrop; anchors.fill: parent }
        Connections { target: dropBackdrop; function onClicked() { root.dropCancelled() } }
    }
    PanelFrame {
        anchors.centerIn: parent
        visible: root.dropPending
        width: 720 * Tokens.uiScale
        height: 250 * Tokens.uiScale
        Column {
            anchors.fill: parent
            spacing: Tokens.gapLarge
            Text { width: parent.width; text: qsTr("Jeter un exemplaire de « %1 » ?").arg(root.selection.name || ""); color: Tokens.textOnPanel; font.family: Tokens.bodyFamily; font.pixelSize: Tokens.fontReading; wrapMode: Text.WordWrap }
            Row {
                spacing: Tokens.gapMedium
                OrnateButton { id: dropCancelButton; text: qsTr("Annuler"); kind: "secondary" }
                Connections { target: dropCancelButton; function onClicked() { root.dropCancelled() } }
                OrnateButton { id: dropConfirmButton; text: qsTr("Jeter"); kind: "cancel" }
                Connections { target: dropConfirmButton; function onClicked() { root.dropConfirmed() } }
            }
        }
    }
}
