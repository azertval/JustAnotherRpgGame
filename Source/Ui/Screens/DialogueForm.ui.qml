import QtQuick
import QtQuick.Layouts
import Jadg.Ui

/*!
    Dialogue -- FORMULAIRE, cote conception (LOT-86, restyle LOT-87 T3.9).

    Pas de maquette : les briques et les jetons de la charte v2, sur la structure v1 -- l'interlocuteur
    a gauche (portrait, nom, attitude), sa replique et les reponses possibles a droite. Parchemin :
    un dialogue suspend le jeu, et se lit comme une page.

    Les proprietes portent des VALEURS D'EXEMPLE ; le jumeau les lie a `DialogueModel` (LOT-15).
    Cliquer une reponse emet `replyChosen(rowId)`.

    LE JET SE MONTRE (LOT-117). La reponse qui mene a un jet l'annonce dans sa colonne de valeur
    (« [Persuasion · DD 15] ») ; une fois joue, il se pose au-dessus de la replique qui en decoule :
    le d20 tire dans son losange -- filet d'or si le jet reussit, grenat s'il echoue --, ce qui
    etait jete (`checkTitle`), le calcul (`checkDetail`) et l'issue (`checkVerdict`). Rien ne
    s'affiche quand le dernier geste n'a rien jete (`checkTitle` vide). Un jet deja rate ne relance
    pas le de : le losange porte alors un tiret.

    LE JOUEUR CHOISIT QUI PARLE (LOT-138, D-28). Le menu du bas aligne les membres du groupe, dans
    l'ordre de marche : celui qui parle porte la marque (EX-IHM-071) et jette les des, avec ses
    modificateurs. Le meneur parle a l'ouverture. Cliquer un membre emet `voiceChosen(id)`.
*/
ScreenPage {
    id: root

    property url portraitSource: ""
    /// Les membres du groupe qui peuvent parler : `id`, `name`, `portrait`, `current`.
    property var voices: exampleVoices
    /// Celui qui parle pour le groupe, et jette les des.
    property string voiceName: "Grom Tranche-Écaille"

    signal voiceChosen(string voiceId)

    readonly property var exampleVoices: [
        { id: "heros-brawler", name: "Grom Tranche-Écaille", portrait: "", current: true },
        { id: "heros-priest", name: "Helga Pierre-Sûre", portrait: "", current: false },
        { id: "heros-scoundrel", name: "Nessa Double-Vie", portrait: "", current: false },
        { id: "heros-mage", name: "Faelar Trace-Carte", portrait: "", current: false }
    ]
    property var replies: exampleReplies
    property string line: "Vous arrivez tard, et par la mauvaise route. Ceux qui viennent par là ont d'ordinaire quelque chose à cacher — ou quelqu'un à fuir. Lequel des deux, pour vous ?"
    property string speakerName: "—"
    property string attitude: "—"
    property string checkOutcome: ""
    property string checkTitle: "Persuasion · DD 15"
    property string checkDie: "12"
    property string checkDetail: "12 + 4 = 16"
    property string checkVerdict: "réussite"
    property bool checkSucceeded: true

    signal replyChosen(string rowId)

    readonly property ListModel exampleReplies: ListModel {
        ListElement { rowId: "a"; label: "Ni l'un ni l'autre. Je cherche du travail."; value: "" }
        ListElement { rowId: "b"; label: "Cela ne vous regarde pas."; value: "" }
        ListElement { rowId: "c"; label: "Qui fuit, ici ?"; value: "" }
    }

    title: qsTr("Dialogue")
    material: "parchment"

    ColumnLayout {
        anchors.fill: parent
        spacing: Tokens.gapMedium

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Tokens.gapLarge

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                spacing: Tokens.gapMedium

                PortraitFrame {
                    Layout.alignment: Qt.AlignHCenter
                    shape: "square"
                    size: 360 * Tokens.uiScale
                    source: root.portraitSource
                }

                FieldRow {
                    Layout.fillWidth: true
                    label: qsTr("Nom")
                    value: root.speakerName
                }

                FieldRow {
                    Layout.fillWidth: true
                    label: qsTr("Attitude")
                    value: root.attitude
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                spacing: Tokens.gapLarge

                PanelFrame {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 340 * Tokens.uiScale
                    material: "parchment"
                    subpanel: true

                    SectionBanner {
                        id: lineBanner

                        anchors.left: parent.left
                        anchors.right: parent.right
                        text: qsTr("Réplique")
                    }

                    Row {
                        id: checkRow

                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: lineBanner.bottom
                        anchors.topMargin: Tokens.gapSmall
                        visible: root.checkTitle.length > 0
                        height: visible ? dieBadge.height : 0
                        spacing: Tokens.gapMedium

                        // Le d20 : un losange, le de tire au centre.
                        Item {
                            id: dieBadge

                            width: 64 * Tokens.uiScale
                            height: 64 * Tokens.uiScale

                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width * 0.7
                                height: parent.height * 0.7
                                rotation: 45
                                color: Tokens.panelRaised
                                border.color: root.checkSucceeded ? Tokens.goldLight : Tokens.gemLight
                                border.width: Tokens.strokeWidth * 2
                            }

                            Text {
                                anchors.centerIn: parent
                                text: root.checkDie.length > 0 ? root.checkDie : "–"
                                color: Tokens.textOnPanel
                                font.family: Tokens.titleFamily
                                font.pixelSize: Tokens.fontSectionTitle
                                font.bold: true
                            }
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - dieBadge.width - parent.spacing
                            spacing: Tokens.gapSmall / 2

                            Text {
                                width: parent.width
                                text: root.checkTitle
                                color: Tokens.text
                                font.family: Tokens.titleFamily
                                font.pixelSize: Tokens.fontBody
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Text {
                                width: parent.width
                                text: root.checkDetail + "  —  " + root.checkVerdict
                                color: root.checkSucceeded ? Tokens.success : Tokens.error
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: checkRow.bottom
                        anchors.bottom: parent.bottom
                        anchors.topMargin: Tokens.gapMedium
                        text: root.line
                        color: Tokens.text
                        font.family: Tokens.loreFamily
                        font.italic: true
                        font.pixelSize: Tokens.fontSectionTitle
                        wrapMode: Text.WordWrap
                        elide: Text.ElideRight
                    }
                }

                LedgerList {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: qsTr("Réponses")
                    rows: root.replies
                    interactive: true
                    onRowActivated: (rowId) => root.replyChosen(rowId)
                }
            }
        }

        // --- Qui parle pour le groupe (LOT-138, D-28) ---------------------------------------------
        Row {
            Layout.fillWidth: true
            spacing: Tokens.gapMedium

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Parle pour le groupe :")
                color: Tokens.textMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
            }

            Repeater {
                model: root.voices

                Item {
                    id: voice

                    required property var modelData

                    width: voiceRow.implicitWidth + Tokens.gapMedium
                    height: 88 * Tokens.uiScale

                    Row {
                        id: voiceRow

                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Tokens.gapSmall

                        // Celui qui parle se signale par une marque, pas par sa seule teinte.
                        FocusMark {
                            anchors.verticalCenter: parent.verticalCenter
                            opacity: voice.modelData.current ? 1 : 0
                        }

                        PortraitFrame {
                            anchors.verticalCenter: parent.verticalCenter
                            shape: "square"
                            size: 80 * Tokens.uiScale
                            source: voice.modelData.portrait
                            active: voice.modelData.current
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: voice.modelData.name
                            color: voice.modelData.current ? Tokens.text : Tokens.textMuted
                            font.family: Tokens.titleFamily
                            font.pixelSize: Tokens.fontCaption
                            font.bold: voice.modelData.current
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.voiceChosen(voice.modelData.id)
                    }
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Tab : changer")
                color: Tokens.textMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
            }
        }
    }
}
