import QtQuick
import Jadg.Ui

/*!
    Fin de la demo -- FORMULAIRE, cote conception (LOT-119 ; maquette `ecrans-de-fin.svg`).

    La quete est bouclee : un panneau sombre sur la scene du menu, le titre en or, ce que le joueur
    a fait -- la phrase de la quete, puis la VOIE qu'il a suivie (`path`, que le jumeau lit du
    dialogue qui a clos la demo : « par la voie de l'arene », « par la parole ») --, ce qui vient
    dans la version suivante, et deux boutons : les credits, ou le menu.

    Les boutons sont exposes pour que le jumeau y branche le survol et le clic ; `currentIndex` dit
    lequel le clavier ou la manette designe, et la marque du focus le signale.
*/
Item {
    id: root

    /// Le bouton designe : 0 credits, 1 menu.
    property int currentIndex: 0

    /// La voie suivie, deja traduite. Vide : la ligne ne s'affiche pas.
    property string path: "par la voie de l'arène"

    property alias creditsEntry: creditsControl
    property alias menuEntry: menuControl

    width: 1920
    height: 1080

    Rectangle {
        anchors.fill: parent
        color: Tokens.panel
    }

    CoverArt {
        anchors.fill: parent
        key: "ui/background/menu-scene"
    }

    PanelFrame {
        id: panel

        anchors.centerIn: parent
        width: 1000 * Tokens.uiScale
        height: 600 * Tokens.uiScale

        Column {
            anchors.centerIn: parent
            width: parent.width - 2 * Tokens.gapLarge
            spacing: Tokens.gapLarge

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Fin de la démo")
                color: Tokens.goldLight
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontDisplay
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 4 * Tokens.uiScale
            }

            GoldDivider {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 320 * Tokens.uiScale
            }

            Column {
                width: parent.width
                spacing: Tokens.gapSmall

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Vous avez rendu son fils à sa mère")
                    color: Tokens.textOnPanel
                    font.family: Tokens.loreFamily
                    font.italic: true
                    font.pixelSize: Tokens.fontSectionTitle
                    wrapMode: Text.WordWrap
                }

                Text {
                    width: parent.width
                    visible: root.path.length > 0
                    horizontalAlignment: Text.AlignHCenter
                    text: root.path
                    color: Tokens.goldLight
                    font.family: Tokens.titleFamily
                    font.pixelSize: Tokens.fontSectionTitle
                    wrapMode: Text.WordWrap
                }
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("À venir : quatre classes, le combat de groupe, puis toute la Capitale impériale.")
                color: Tokens.textOnPanelMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                wrapMode: Text.WordWrap
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Tokens.gapLarge

                Row {
                    spacing: Tokens.gapSmall

                    FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 0 ? 1 : 0 }
                    OrnateButton {
                        id: creditsControl
                        kind: "primary"
                        text: qsTr("Crédits")
                        highlighted: root.currentIndex === 0
                        focusPolicy: Qt.NoFocus
                    }
                }

                Row {
                    spacing: Tokens.gapSmall

                    FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 1 ? 1 : 0 }
                    OrnateButton {
                        id: menuControl
                        kind: "default"
                        text: qsTr("Menu")
                        highlighted: root.currentIndex === 1
                        focusPolicy: Qt.NoFocus
                    }
                }
            }
        }
    }
}
