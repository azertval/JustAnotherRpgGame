pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Jadg.Ui

/*!
    Groupe -- FORMULAIRE, cote conception (LOT-138).

    Pas de maquette propre : les briques et les jetons de la charte v2, sur la structure du journal
    -- l'ordre de marche a gauche, les personnages qu'on peut prendre a droite. Panneau sombre : le
    groupe se compose par-dessus le jeu, comme le journal et la carte.

    L'ORDRE DE MARCHE est le groupe : le premier mene -- on le deplace, il parle et jette les des
    du dialogue --, les autres le suivent dans cet ordre. Le meneur porte une marque, pas seulement
    une teinte (EX-IHM-071) ; le personnage designe au clavier aussi.

    Les proprietes portent des VALEURS D'EXEMPLE ; le jumeau les lie a `PartyModel`. Cliquer un
    personnage emet `candidateClicked(index)` ; les trois boutons, `toggleClicked`,
    `leadClicked`, `moveClicked(offset)`.
*/
ScreenPage {
    id: root

    /// Les membres, meneur en tete : `label` (nom), `value` (points de vie), `role` (classe),
    /// `portrait`.
    property var members: exampleMembers
    /// Les personnages qu'on peut prendre : `name`, `className`, `species`, `level`, `value`,
    /// `armorClass`, `speed`, `portrait`, `rank` (-1 hors du groupe), `leader`.
    property var candidates: exampleCandidates
    /// Le personnage designe au clavier (indice dans `candidates`).
    property int currentIndex: 0
    /// Le plafond du groupe.
    property int maxSize: 4
    /// Ce que le designe permet : le prendre (ou le laisser), le faire mener, le deplacer.
    property bool canToggle: true
    property bool canLead: true
    property bool canMove: true
    /// Le libelle du bouton de composition : « Prendre » ou « Laisser ».
    property string toggleText: qsTr("Laisser")

    signal candidateClicked(int index)
    signal toggleClicked()
    signal leadClicked()
    signal moveClicked(int offset)

    readonly property var exampleMembers: [
        { label: "Grom Tranche-Écaille", value: "15 / 15", role: "Brawler", portrait: "" },
        { label: "Faelar Trace-Carte", value: "8 / 8", role: "Mage", portrait: "" },
        { label: "Helga Pierre-Sûre", value: "12 / 12", role: "Priest", portrait: "" }
    ]
    readonly property var exampleCandidates: [
        { name: "Grom Tranche-Écaille", className: "Brawler", species: "Demi-orc", level: "1", value: "15 / 15", armorClass: "14", speed: "9 m", portrait: "", rank: 0, leader: true },
        { name: "Faelar Trace-Carte", className: "Mage", species: "Elfe d'automne", level: "1", value: "8 / 8", armorClass: "12", speed: "9 m", portrait: "", rank: 1, leader: false },
        { name: "Helga Pierre-Sûre", className: "Priest", species: "Nain des collines", level: "1", value: "12 / 12", armorClass: "17", speed: "7,5 m", portrait: "", rank: 2, leader: false },
        { name: "Nessa Double-Vie", className: "Scoundrel", species: "Humain", level: "1", value: "10 / 10", armorClass: "14", speed: "12 m", portrait: "", rank: -1, leader: false }
    ]

    title: qsTr("Groupe")
    material: "dark"

    RowLayout {
        anchors.fill: parent
        spacing: Tokens.gapLarge

        // --- L'ordre de marche ---------------------------------------------------------------
        PanelFrame {
            Layout.fillHeight: true
            Layout.preferredWidth: 520 * Tokens.uiScale
            subpanel: true

            SectionBanner {
                id: orderBanner

                anchors.left: parent.left
                anchors.right: parent.right
                material: "dark"
                text: qsTr("Ordre de marche")
            }

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: orderBanner.bottom
                anchors.topMargin: Tokens.gapMedium
                spacing: Tokens.gapMedium

                Repeater {
                    model: root.members

                    Row {
                        id: memberRow

                        required property int index
                        required property var modelData

                        spacing: Tokens.gapMedium

                        PortraitFrame {
                            shape: "square"
                            size: 120 * Tokens.uiScale
                            source: memberRow.modelData.portrait
                            active: memberRow.index === 0
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Tokens.gapSmall / 2

                            Text {
                                text: memberRow.modelData.label
                                color: Tokens.textOnPanel
                                font.family: Tokens.titleFamily
                                font.pixelSize: Tokens.fontBody
                                font.bold: true
                            }

                            Text {
                                text: memberRow.modelData.role + " · " + qsTr("PV") + " " + memberRow.modelData.value
                                color: Tokens.textOnPanelMuted
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontCaption
                            }

                            // Le meneur se dit en toutes lettres, avec sa marque (EX-IHM-071).
                            Row {
                                spacing: Tokens.gapSmall
                                visible: memberRow.index === 0

                                FocusMark {
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: qsTr("Meneur")
                                    color: Tokens.goldLight
                                    font.family: Tokens.titleFamily
                                    font.pixelSize: Tokens.fontCaption
                                }
                            }
                        }
                    }
                }

                Text {
                    width: parent.width
                    text: qsTr("%1 sur %2 — le meneur parle et jette les dés pour le groupe.").arg(root.members.length).arg(root.maxSize)
                    color: Tokens.textOnPanelMuted
                    font.family: Tokens.loreFamily
                    font.italic: true
                    font.pixelSize: Tokens.fontCaption
                    wrapMode: Text.WordWrap
                }
            }
        }

        // --- Les personnages -------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Tokens.gapMedium

            PanelFrame {
                Layout.fillWidth: true
                Layout.fillHeight: true
                subpanel: true

                SectionBanner {
                    id: candidatesBanner

                    anchors.left: parent.left
                    anchors.right: parent.right
                    material: "dark"
                    text: qsTr("Personnages")
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: candidatesBanner.bottom
                    anchors.topMargin: Tokens.gapMedium
                    spacing: Tokens.gapSmall

                    Repeater {
                        model: root.candidates

                        Item {
                            id: candidate

                            required property int index
                            required property var modelData

                            readonly property bool current: candidate.index === root.currentIndex
                            readonly property bool member: candidate.modelData.rank >= 0

                            width: parent.width
                            height: 104 * Tokens.uiScale

                            Rectangle {
                                anchors.fill: parent
                                color: candidate.current ? Tokens.panelRaised : "transparent"
                                border.color: candidate.current ? Tokens.panelEdge : "transparent"
                                border.width: Tokens.strokeWidth
                            }

                            // Le personnage designe se signale par une marque (EX-IHM-071).
                            FocusMark {
                                id: candidateMark

                                anchors.left: parent.left
                                anchors.leftMargin: Tokens.gapSmall
                                anchors.verticalCenter: parent.verticalCenter
                                opacity: candidate.current ? 1 : 0
                            }

                            PortraitFrame {
                                id: candidatePortrait

                                anchors.left: candidateMark.right
                                anchors.leftMargin: Tokens.gapSmall
                                anchors.verticalCenter: parent.verticalCenter
                                shape: "square"
                                size: 96 * Tokens.uiScale
                                source: candidate.modelData.portrait
                                active: candidate.member
                            }

                            Column {
                                anchors.left: candidatePortrait.right
                                anchors.leftMargin: Tokens.gapMedium
                                anchors.right: candidateState.left
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: Tokens.gapSmall / 2

                                Text {
                                    text: candidate.modelData.name
                                    color: Tokens.textOnPanel
                                    font.family: Tokens.titleFamily
                                    font.pixelSize: Tokens.fontBody
                                    font.bold: true
                                }

                                Text {
                                    text: candidate.modelData.species + " · " + candidate.modelData.className + " " + candidate.modelData.level
                                    color: Tokens.textOnPanelMuted
                                    font.family: Tokens.bodyFamily
                                    font.pixelSize: Tokens.fontCaption
                                }

                                Text {
                                    text: qsTr("PV %1 · CA %2 · Vitesse %3").arg(candidate.modelData.value).arg(candidate.modelData.armorClass).arg(candidate.modelData.speed)
                                    color: Tokens.textOnPanelMuted
                                    font.family: Tokens.bodyFamily
                                    font.pixelSize: Tokens.fontCaption
                                }
                            }

                            Text {
                                id: candidateState

                                anchors.right: parent.right
                                anchors.rightMargin: Tokens.gapMedium
                                anchors.verticalCenter: parent.verticalCenter
                                text: candidate.modelData.leader ? qsTr("Meneur")
                                      : (candidate.member ? qsTr("Rang %1").arg(candidate.modelData.rank + 1)
                                                          : qsTr("Disponible"))
                                color: candidate.modelData.leader ? Tokens.goldLight
                                       : (candidate.member ? Tokens.textOnPanel : Tokens.textOnPanelMuted)
                                font.family: Tokens.titleFamily
                                font.pixelSize: Tokens.fontCaption
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.candidateClicked(candidate.index)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Tokens.gapMedium

                OrnateButton {
                    kind: "primary"
                    text: root.toggleText
                    enabled: root.canToggle
                    onClicked: root.toggleClicked()
                }

                OrnateButton {
                    kind: "secondary"
                    text: qsTr("Mener")
                    enabled: root.canLead
                    onClicked: root.leadClicked()
                }

                OrnateButton {
                    kind: "default"
                    text: qsTr("Avancer")
                    enabled: root.canMove
                    onClicked: root.moveClicked(-1)
                }

                OrnateButton {
                    kind: "default"
                    text: qsTr("Reculer")
                    enabled: root.canMove
                    onClicked: root.moveClicked(1)
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Entrée : prendre ou laisser · M : mener · Page préc. / suiv. : avancer, reculer · Échap : fermer")
                color: Tokens.textOnPanelMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
                wrapMode: Text.WordWrap
            }
        }
    }
}
