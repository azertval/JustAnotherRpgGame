pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Jadg.Ui

/*!
    Groupe -- FORMULAIRE, cote conception (LOT-138, LOT-141).

    Pas de maquette propre : les briques et les jetons de la charte v2, sur la structure du journal.
    Panneau sombre : le groupe se compose par-dessus le jeu, comme le journal et la carte.

    EN HAUT, LES QUATRE PROFILS COTE A COTE (LOT-141), dans l'ordre de marche : portrait, nom,
    espece et classe, niveau, jauge de vie, CA et vitesse. Le premier mene -- on le deplace, il parle
    et jette les des du dialogue --, les autres le suivent dans cet ordre. Le meneur porte une
    marque, pas seulement une teinte (EX-IHM-071) ; le personnage designe au clavier aussi. Une
    place vide est une case a prendre.

    EN BAS, LES PERSONNAGES qu'on peut prendre, en lignes compactes, et les gestes : prendre ou
    laisser, mener, avancer, reculer, ouvrir la fiche.

    Les proprietes portent des VALEURS D'EXEMPLE ; le jumeau les lie a `PartyModel`. Cliquer un
    personnage emet `candidateClicked(index)` ; les boutons, `toggleClicked`, `leadClicked`,
    `moveClicked(offset)`, `sheetClicked`.
*/
ScreenPage {
    id: root

    /// Les membres, meneur en tete : `id`, `label` (nom), `value` (points de vie), `ratio`,
    /// `role` (classe), `species`, `level`, `armorClass`, `speed`, `portrait`.
    property var members: exampleMembers
    /// Les personnages qu'on peut prendre : `id`, `name`, `className`, `species`, `level`,
    /// `value`, `armorClass`, `speed`, `portrait`, `rank` (-1 hors du groupe), `leader`.
    property var candidates: exampleCandidates
    /// Le personnage designe au clavier (indice dans `candidates`).
    property int currentIndex: 0
    /// L'identifiant du personnage designe, pour marquer son profil.
    property string currentId: "heros-brawler"
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
    /// Ouvrir la fiche du personnage designe (LOT-141).
    signal sheetClicked()

    readonly property var exampleMembers: [
        { id: "heros-brawler", label: "Grom Tranche-Écaille", value: "15 / 15", ratio: 1, role: "Brawler", species: "Demi-orc", level: "1", armorClass: "14", speed: "9 m", portrait: "" },
        { id: "heros-priest", label: "Helga Pierre-Sûre", value: "6 / 12", ratio: 0.5, role: "Priest", species: "Nain des collines", level: "1", armorClass: "17", speed: "7,5 m", portrait: "" },
        { id: "heros-scoundrel", label: "Nessa Double-Vie", value: "10 / 10", ratio: 1, role: "Scoundrel", species: "Humain", level: "1", armorClass: "14", speed: "12 m", portrait: "" }
    ]
    readonly property var exampleCandidates: [
        { id: "heros-brawler", name: "Grom Tranche-Écaille", className: "Brawler", species: "Demi-orc", level: "1", value: "15 / 15", armorClass: "14", speed: "9 m", portrait: "", rank: 0, leader: true },
        { id: "heros-priest", name: "Helga Pierre-Sûre", className: "Priest", species: "Nain des collines", level: "1", value: "6 / 12", armorClass: "17", speed: "7,5 m", portrait: "", rank: 1, leader: false },
        { id: "heros-scoundrel", name: "Nessa Double-Vie", className: "Scoundrel", species: "Humain", level: "1", value: "10 / 10", armorClass: "14", speed: "12 m", portrait: "", rank: 2, leader: false },
        { id: "heros-mage", name: "Faelar Trace-Carte", className: "Mage", species: "Elfe d'automne", level: "1", value: "8 / 8", armorClass: "12", speed: "9 m", portrait: "", rank: -1, leader: false }
    ]

    title: qsTr("Groupe")
    material: "dark"

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.gapMedium

        // --- Les quatre profils, cote a cote, dans l'ordre de marche (LOT-141) ---------------------
        PanelFrame {
            Layout.fillWidth: true
            Layout.preferredHeight: 392 * Tokens.uiScale
            subpanel: true

            SectionBanner {
                id: orderBanner

                anchors.left: parent.left
                anchors.right: parent.right
                material: "dark"
                text: qsTr("Ordre de marche")
            }

            Row {
                id: profiles

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: orderBanner.bottom
                anchors.topMargin: Tokens.gapMedium
                spacing: Tokens.gapMedium

                Repeater {
                    model: root.maxSize

                    Rectangle {
                        id: profile

                        required property int index

                        readonly property bool filled: profile.index < root.members.length
                        readonly property var member: profile.filled ? root.members[profile.index] : null
                        readonly property bool current: profile.filled && profile.member.id === root.currentId
                        readonly property bool leader: profile.index === 0 && profile.filled

                        width: (profiles.width - (root.maxSize - 1) * profiles.spacing) / root.maxSize
                        height: 300 * Tokens.uiScale
                        color: profile.current ? Tokens.panelRaised : "transparent"
                        border.color: profile.current ? Tokens.goldLight : Tokens.panelEdge
                        border.width: Tokens.strokeWidth

                        // Une place vide : la case a prendre.
                        Text {
                            anchors.centerIn: parent
                            visible: !profile.filled
                            text: qsTr("Place libre")
                            color: Tokens.textOnPanelMuted
                            font.family: Tokens.loreFamily
                            font.italic: true
                            font.pixelSize: Tokens.fontBody
                        }

                        Column {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: Tokens.gapMedium
                            spacing: Tokens.gapSmall / 2
                            visible: profile.filled

                            PortraitFrame {
                                anchors.horizontalCenter: parent.horizontalCenter
                                shape: "square"
                                size: 128 * Tokens.uiScale
                                source: profile.filled ? profile.member.portrait : ""
                                active: profile.leader
                            }

                            Text {
                                width: parent.width
                                text: profile.filled ? profile.member.label : ""
                                color: Tokens.textOnPanel
                                font.family: Tokens.titleFamily
                                font.pixelSize: Tokens.fontBody
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                width: parent.width
                                text: profile.filled ? profile.member.species + " · " + profile.member.role + " " + profile.member.level : ""
                                color: Tokens.textOnPanelMuted
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontCaption
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }

                            Gauge {
                                width: parent.width
                                height: 26 * Tokens.uiScale
                                kind: "health"
                                value: profile.filled && profile.member.ratio !== undefined ? profile.member.ratio : 0
                                label: profile.filled ? profile.member.value : ""
                            }

                            Text {
                                width: parent.width
                                text: profile.filled ? qsTr("CA %1 · Vitesse %2").arg(profile.member.armorClass).arg(profile.member.speed) : ""
                                color: Tokens.textOnPanelMuted
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontCaption
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }

                            // Le meneur se dit en toutes lettres, avec sa marque (EX-IHM-071) ; les
                            // autres, leur rang.
                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: Tokens.gapSmall

                                FocusMark {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: profile.leader
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: profile.leader ? qsTr("Meneur") : qsTr("Rang %1").arg(profile.index + 1)
                                    color: profile.leader ? Tokens.goldLight : Tokens.textOnPanel
                                    font.family: Tokens.titleFamily
                                    font.pixelSize: Tokens.fontCaption
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- Les personnages, en lignes compactes -------------------------------------------------
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
                anchors.topMargin: Tokens.gapSmall
                spacing: 2 * Tokens.uiScale

                Repeater {
                    model: root.candidates

                    Item {
                        id: candidate

                        required property int index
                        required property var modelData

                        readonly property bool current: candidate.index === root.currentIndex
                        readonly property bool member: candidate.modelData.rank >= 0

                        width: parent.width
                        height: 48 * Tokens.uiScale

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

                        Text {
                            anchors.left: candidateMark.right
                            anchors.leftMargin: Tokens.gapMedium
                            anchors.verticalCenter: parent.verticalCenter
                            width: 360 * Tokens.uiScale
                            text: candidate.modelData.name
                            color: candidate.member ? Tokens.textOnPanel : Tokens.textOnPanelMuted
                            font.family: Tokens.titleFamily
                            font.pixelSize: Tokens.fontBody
                            font.bold: candidate.member
                            elide: Text.ElideRight
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 440 * Tokens.uiScale
                            anchors.right: candidateState.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: candidate.modelData.species + " · " + candidate.modelData.className + " " + candidate.modelData.level
                                  + " · " + qsTr("PV %1 · CA %2 · Vitesse %3").arg(candidate.modelData.value).arg(candidate.modelData.armorClass).arg(candidate.modelData.speed)
                            color: Tokens.textOnPanelMuted
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontCaption
                            elide: Text.ElideRight
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

            OrnateButton {
                kind: "default"
                text: qsTr("Fiche")
                onClicked: root.sheetClicked()
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Entrée : prendre ou laisser · M : mener · Page préc. / suiv. : avancer, reculer · F : fiche · Échap : fermer")
            color: Tokens.textOnPanelMuted
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontCaption
            wrapMode: Text.WordWrap
        }
    }
}
