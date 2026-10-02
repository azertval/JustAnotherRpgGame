pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Jadg.Ui


/*! Fiche d'identité du mercenaire validée : portrait et six caractéristiques, registre, sceau
    et signature. L'équipement et les règles ont leurs pages et ne doublent pas l'identité. */
Item {
    id: root
    property string characterName: "Grom Tranche-Écaille"
    property string className: "Bagarreur"
    property string level: "2"
    property string background: "Chasseur de dragons"
    property string species: "Demi-orc"
    property string registration: "—"
    property url portrait: ""
    property string hitPointsText: "25 / 32"
    property real hitPointsRatio: 25 / 32
    property string experienceText: "—"
    property real experienceRatio: 0
    property string strengthScore: "14"
    property string strengthModifier: "+2"
    property string dexterityScore: "13"
    property string dexterityModifier: "+1"
    property string constitutionScore: "15"
    property string constitutionModifier: "+2"
    property string intelligenceScore: "10"
    property string intelligenceModifier: "+0"
    property string wisdomScore: "12"
    property string wisdomModifier: "+1"
    property string charismaScore: "8"
    property string charismaModifier: "-1"
    property string armorClass: "16"
    property string initiative: "+1"
    property string speed: "9 m"
    property string proficiencyBonus: "+2"
    property string passivePerception: "11"
    property var skills: []
    property var classRows: []
    property var spellRows: []
    property var selectedSpell: ({})
    property var selectedCapacity: ({})
    property int currentTab: 0
    property int abilityTab: 0
    property string query: ""
    property alias classList: classView
    property alias spellList: spellView
    property alias searchField: searchControl
    signal tabRequested(int index)
    signal abilityTabRequested(int index)
    signal spellSelected(int index)
    signal capacitySelected(int index)
    signal closeRequested
    signal memberStepRequested(int step)

    width: 1920
    height: 1080
    Rectangle {
        anchors.fill: parent
        color: Tokens.frameEdge
    }
    PanelFrame {
        anchors.fill: parent
        anchors.margins: 14 * Tokens.uiScale
        material: "parchment"
        bound: true
        padding: 0
    }
    FixedArt {
        x: 36 * Tokens.uiScale
        y: 20 * Tokens.uiScale
        width: 112 * Tokens.uiScale
        height: 180 * Tokens.uiScale
        key: "ui/ornament/crest-pennant"
    }
    TitlePlate {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 16 * Tokens.uiScale
        width: 1170 * Tokens.uiScale
        text: root.currentTab === 0 ? qsTr("Fiche d’identité") : root.currentTab
                                      === 1 ? qsTr(
                                                  "Capacités de classe") : qsTr(
                                                  "Compétences")
    }
    CodexTabs {
        id: codexTabs
        anchors.right: parent.right
        anchors.rightMargin: 92 * Tokens.uiScale
        y: 142 * Tokens.uiScale
        currentIndex: root.currentTab
    }
    Connections {
        target: codexTabs
        function onRequested(index) {
            root.tabRequested(index)
        }
    }
    Row {
        x: 168 * Tokens.uiScale
        y: 148 * Tokens.uiScale
        spacing: Tokens.gapSmall
        OrnateButton {
            id: previousMemberButton
            width: 70 * Tokens.uiScale
            height: 44 * Tokens.uiScale
            text: "◀"
            kind: "secondary"
        }
        Connections {
            target: previousMemberButton
            function onClicked() {
                root.memberStepRequested(-1)
            }
        }
        OrnateButton {
            id: nextMemberButton
            width: 70 * Tokens.uiScale
            height: 44 * Tokens.uiScale
            text: "▶"
            kind: "secondary"
        }
        Connections {
            target: nextMemberButton
            function onClicked() {
                root.memberStepRequested(1)
            }
        }
    }

    Item {
        visible: root.currentTab === 0
        x: 90 * Tokens.uiScale
        y: 232 * Tokens.uiScale
        width: 650 * Tokens.uiScale
        height: 752 * Tokens.uiScale
        FixedArt {
            anchors.fill: parent
            key: "ui/ornament/compass-watermark/parchment"
            opacity: 0.5
        }
        PortraitFrame {
            x: 35
            y: 38
            size: 580 * Tokens.uiScale
            source: root.portrait
        }
        StatMedallion {
            x: 98 * Tokens.uiScale
            y: 0
            width: 180 * Tokens.uiScale
            height: width
            label: qsTr("FOR")
            value: root.strengthScore
            modifier: root.strengthModifier
        }
        StatMedallion {
            x: 365 * Tokens.uiScale
            y: 0
            width: 180 * Tokens.uiScale
            height: width
            label: qsTr("DEX")
            value: root.dexterityScore
            modifier: root.dexterityModifier
        }
        StatMedallion {
            x: 0
            y: 240 * Tokens.uiScale
            width: 180 * Tokens.uiScale
            height: width
            label: qsTr("CON")
            value: root.constitutionScore
            modifier: root.constitutionModifier
        }
        StatMedallion {
            x: 465 * Tokens.uiScale
            y: 240 * Tokens.uiScale
            width: 180 * Tokens.uiScale
            height: width
            label: qsTr("INT")
            value: root.intelligenceScore
            modifier: root.intelligenceModifier
        }
        StatMedallion {
            x: 98 * Tokens.uiScale
            y: 475 * Tokens.uiScale
            width: 180 * Tokens.uiScale
            height: width
            label: qsTr("SAG")
            value: root.wisdomScore
            modifier: root.wisdomModifier
        }
        StatMedallion {
            x: 365 * Tokens.uiScale
            y: 475 * Tokens.uiScale
            width: 180 * Tokens.uiScale
            height: width
            label: qsTr("CHA")
            value: root.charismaScore
            modifier: root.charismaModifier
        }
        PanelFrame {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 90 * Tokens.uiScale
            material: "parchment"
            subpanel: true
            Row {
                anchors.centerIn: parent
                spacing: 42 * Tokens.uiScale
                Text {
                    text: qsTr("Armure  %1").arg(root.armorClass)
                    color: Tokens.text
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontReading
                }
                Text {
                    text: qsTr("Initiative  %1").arg(root.initiative)
                    color: Tokens.text
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontReading
                }
                Text {
                    text: root.speed
                    color: Tokens.text
                    font.family: Tokens.bodyFamily
                    font.pixelSize: Tokens.fontReading
                }
            }
        }
    }
    PanelFrame {
        visible: root.currentTab === 0
        x: 790 * Tokens.uiScale
        y: 234 * Tokens.uiScale
        width: 1030 * Tokens.uiScale
        height: 750 * Tokens.uiScale
        material: "parchment"
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 7 * Tokens.uiScale
            FieldRow {
                width: parent.width
                height: 50
                label: qsTr("Nom")
                value: root.characterName
                tagWidth: 240 * Tokens.uiScale
            }
            FieldRow {
                width: parent.width
                height: 42 * Tokens.uiScale
                label: qsTr("Espèce")
                value: root.species
                tagWidth: 240 * Tokens.uiScale
            }
            FieldRow {
                width: parent.width
                height: 42 * Tokens.uiScale
                label: qsTr("Classe")
                value: root.className
                tagWidth: 240 * Tokens.uiScale
            }
            FieldRow {
                width: parent.width
                height: 42 * Tokens.uiScale
                label: qsTr("Niveau")
                value: root.level
                tagWidth: 240 * Tokens.uiScale
            }
            FieldRow {
                width: parent.width
                height: 42 * Tokens.uiScale
                label: qsTr("Historique")
                value: root.background
                tagWidth: 240 * Tokens.uiScale
            }
            FieldRow {
                width: parent.width
                height: 42 * Tokens.uiScale
                label: qsTr("Origine")
                value: "—"
                tagWidth: 240 * Tokens.uiScale
            }
            FieldRow {
                width: parent.width
                height: 42 * Tokens.uiScale
                label: qsTr("Matricule")
                value: root.registration
                tagWidth: 240 * Tokens.uiScale
            }
            FieldRow {
                width: parent.width
                height: 42 * Tokens.uiScale
                label: qsTr("Points de vie")
                value: root.hitPointsText
                tagWidth: 240 * Tokens.uiScale
            }
            Gauge {
                width: parent.width
                kind: "health"
                value: root.hitPointsRatio
            }
            FieldRow {
                width: parent.width
                height: 42 * Tokens.uiScale
                label: qsTr("Expérience")
                value: root.experienceText
                tagWidth: 240 * Tokens.uiScale
            }
            Gauge {
                width: parent.width
                kind: "experience"
                value: root.experienceRatio
            }
        }
        FixedArt {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: 116 * Tokens.uiScale
            height: width
            key: "ui/ornament/wax-seal"
        }
        FixedArt {
            x: 180 * Tokens.uiScale
            anchors.bottom: parent.bottom
            width: 120 * Tokens.uiScale
            height: width
            key: "ui/ornament/crossed-crest"
        }
        Text {
            x: 355
            y: 565
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 21
            anchors.bottomMargin: 51
            width: 590 * Tokens.uiScale
            horizontalAlignment: Text.AlignHCenter
            rotation: -5
            text: root.characterName
            color: Tokens.text
            font.family: Tokens.signatureFamily
            font.pixelSize: Tokens.fontDisplay
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: Tokens.fontBody
        }
    }

    Item {
        visible: root.currentTab === 1
        x: 100 * Tokens.uiScale
        y: 230 * Tokens.uiScale
        width: 1720 * Tokens.uiScale
        height: 750 * Tokens.uiScale
        Row {
            spacing: Tokens.gapSmall
            OrnateTab {
                id: capacitiesTab
                width: 330 * Tokens.uiScale
                text: qsTr("Capacités")
                checked: root.abilityTab === 0
                checkable: false
            }
            Connections {
                target: capacitiesTab
                function onClicked() {
                    root.abilityTabRequested(0)
                }
            }
            OrnateTab {
                id: spellbookTab
                width: 330 * Tokens.uiScale
                text: qsTr("Grimoire")
                checked: root.abilityTab === 1
                checkable: false
            }
            Connections {
                target: spellbookTab
                function onClicked() {
                    root.abilityTabRequested(1)
                }
            }
        }
        Text {
            x: 740 * Tokens.uiScale
            y: Tokens.gapMedium
            text: root.characterName + " · " + root.className + " · " + qsTr(
                      "Niveau %1").arg(root.level)
            color: Tokens.text
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontReading
        }
        PanelFrame {
            y: 72 * Tokens.uiScale
            width: 700 * Tokens.uiScale
            height: 668 * Tokens.uiScale
            material: "parchment"
            TextField {
                id: searchControl
                objectName: "abilitySearch"
                width: parent.width
                height: 52 * Tokens.uiScale
                placeholderText: root.abilityTab === 1 ? qsTr("Rechercher un sort…") : qsTr(
                                                             "Rechercher une capacité…")
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontReading
                color: Tokens.text
                selectByMouse: true
                background: Rectangle {
                    color: Tokens.surfaceAlt
                    border.color: Tokens.border
                    border.width: Tokens.strokeWidth
                }
            }
            ListView {
                id: classView
                visible: root.abilityTab === 0
                anchors.top: searchControl.bottom
                anchors.topMargin: Tokens.gapMedium
                anchors.bottom: parent.bottom
                width: parent.width
                clip: true
                spacing: Tokens.gapSmall
                model: root.classRows
                ScrollBar.vertical: OrnateScrollBar {}
                delegate: Item {
                    id: capacityRow
                    required property int index
                    required property string label
                    required property string value
                    required property string detail
                    required property string levelText
                    required property bool upcoming
                    width: ListView.view.width
                    height: 108 * Tokens.uiScale
                    PanelFrame {
                        anchors.fill: parent
                        subpanel: true
                        material: "parchment"
                        opacity: capacityRow.index === classView.currentIndex ? 1 : 0.4
                    }
                    FixedArt {
                        x: Tokens.gapSmall
                        anchors.verticalCenter: parent.verticalCenter
                        width: 70 * Tokens.uiScale
                        height: width
                        key: capacityRow.value
                        opacity: capacityRow.upcoming ? 0.45 : 1
                    }
                    Column {
                        x: 90 * Tokens.uiScale
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 112 * Tokens.uiScale
                        Text {
                            width: parent.width
                            text: capacityRow.label
                            color: Tokens.text
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontReading
                            elide: Text.ElideRight
                        }
                        Text {
                            text: capacityRow.levelText
                            color: Tokens.textMuted
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontBody
                        }
                    }
                    MouseArea {
                        id: capacityPointer
                        anchors.fill: parent
                    }
                    Connections {
                        target: capacityPointer
                        function onClicked() {
                            root.capacitySelected(capacityRow.index)
                        }
                    }
                }
            }
            ListView {
                id: spellView
                visible: root.abilityTab === 1
                anchors.top: searchControl.bottom
                anchors.topMargin: Tokens.gapMedium
                anchors.bottom: parent.bottom
                width: parent.width
                clip: true
                spacing: Tokens.gapSmall
                model: root.spellRows
                ScrollBar.vertical: OrnateScrollBar {}
                delegate: Item {
                    id: spellRow
                    required property int index
                    required property string label
                    required property string value
                    required property string uses
                    required property string levelText
                    width: ListView.view.width
                    height: 108 * Tokens.uiScale
                    PanelFrame {
                        anchors.fill: parent
                        subpanel: true
                        material: "parchment"
                        opacity: spellRow.index === spellView.currentIndex ? 1 : 0.4
                    }
                    FixedArt {
                        x: Tokens.gapSmall
                        anchors.verticalCenter: parent.verticalCenter
                        width: 70 * Tokens.uiScale
                        height: width
                        key: spellRow.value
                    }
                    Column {
                        x: 90 * Tokens.uiScale
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 220 * Tokens.uiScale
                        Text {
                            width: parent.width
                            text: spellRow.label
                            color: Tokens.text
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontReading
                            elide: Text.ElideRight
                        }
                        Text {
                            text: spellRow.levelText
                            color: Tokens.textMuted
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontBody
                        }
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: Tokens.gapMedium
                        anchors.verticalCenter: parent.verticalCenter
                        text: spellRow.uses
                        color: Tokens.gem
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontReading
                    }
                    //     MouseArea { anchors.fill: parent; onClicked: root.spellSelected(spellRow.index) }
                }
            }
            Text {
                anchors.centerIn: parent
                width: parent.width
                visible: root.abilityTab === 1 ? spellView.count === 0 : classView.count === 0
                text: root.query.length
                      > 0 ? qsTr("Aucun résultat.") : root.abilityTab
                            === 1 ? qsTr("Ce personnage ne connaît aucun sort.") : qsTr(
                                        "Aucune capacité disponible.")
                color: Tokens.textMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontReading
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
        PanelFrame {
            x: 736 * Tokens.uiScale
            y: 72 * Tokens.uiScale
            width: 984 * Tokens.uiScale
            height: 668 * Tokens.uiScale
            material: "parchment"
            Flickable {
                anchors.fill: parent
                contentHeight: ruleColumn.implicitHeight
                clip: true
                ScrollBar.vertical: OrnateScrollBar {}
                Column {
                    id: ruleColumn
                    width: parent.width
                    spacing: Tokens.gapMedium
                    SectionBanner {
                        width: parent.width
                        material: "parchment"
                        text: root.abilityTab
                              === 1 ? (root.selectedSpell.name || qsTr(
                                           "Choisissez un sort")) : (root.selectedCapacity.label
                                                                     || qsTr(
                                                                         "Choisissez une capacité"))
                    }
                    Text {
                        width: parent.width
                        text: root.abilityTab === 1 ? (root.selectedSpell.summary
                                                       || "") : (root.selectedCapacity.levelText
                                                                 || "")
                        color: Tokens.gem
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontScreenTitle
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        width: parent.width
                        text: root.abilityTab === 1 ? (root.selectedSpell.details
                                                       || "") : (root.selectedCapacity.detail
                                                                 || "")
                        color: Tokens.text
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontReading
                        wrapMode: Text.WordWrap
                    }
                    GoldDivider {
                        width: parent.width
                    }
                    Text {
                        width: parent.width
                        visible: root.abilityTab === 1
                        text: root.selectedSpell.text || ""
                        color: Tokens.text
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontReading
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        width: parent.width
                        visible: root.abilityTab === 1
                                 && root.selectedSpell.usesText !== undefined
                        text: qsTr("Lancers restants : %1").arg(
                                  root.selectedSpell.usesText
                                  || "") + (root.selectedSpell.perDay
                                            > 0 ? "\n" + qsTr(
                                                      "Récupération : repos long") : "")
                        color: Tokens.gem
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontReading
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    PanelFrame {
        visible: root.currentTab === 2
        x: 180 * Tokens.uiScale
        y: 240 * Tokens.uiScale
        width: 1560 * Tokens.uiScale
        height: 724 * Tokens.uiScale
        material: "parchment"
        Text {
            id: skillsHeader
            text: root.characterName + " · " + qsTr(
                      "Maîtrise %1 · Perception passive %2").arg(
                      root.proficiencyBonus).arg(root.passivePerception)
            color: Tokens.text
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontReading
        }
        GridView {
            anchors.top: skillsHeader.bottom
            anchors.topMargin: Tokens.gapLarge
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            cellWidth: width / 2
            cellHeight: 60 * Tokens.uiScale
            clip: true
            model: root.skills
            delegate: Item {
                id: skillEntry
                required property string rowId
                required property string label
                required property string value
                required property bool marked
                width: GridView.view.cellWidth - Tokens.gapLarge
                height: GridView.view.cellHeight
                SkillRow {
                    anchors.fill: parent
                    skillId: skillEntry.rowId
                    label: skillEntry.label
                    value: skillEntry.value
                    proficient: skillEntry.marked
                }
            }
        }
    }
    OrnateButton {
        id: closeButton
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24 * Tokens.uiScale
        width: 300 * Tokens.uiScale
        kind: "back"
        text: qsTr("Retour au jeu")
    }
    Connections {
        target: closeButton
        function onClicked() {
            root.closeRequested()
        }
    }
}
