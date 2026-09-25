import QtQuick
import Jadg.Ui

/*!
    Ecran de mort -- FORMULAIRE, cote conception (LOT-119 ; maquette `ecrans-de-fin.svg`).

    Le combat sur la carte est LETAL : le heros y tombe, la demo s'y termine. L'ecran est un
    RECOUVREMENT de la scene du combat, figee et assombrie d'un voile grenat -- on voit encore ou
    l'on est tombe --, puis le titre, deux lignes, et deux boutons : recommencer une partie neuve,
    ou rendre le menu.

    `sceneHost` recoit la scene (le jumeau y pose la surface de rendu de la carte gelee) ; dans
    l'atelier, un aplat sombre en tient lieu. Les deux boutons sont exposes pour que le jumeau y
    branche le survol et le clic ; `currentIndex` dit lequel le clavier ou la manette designe, et
    la marque du focus le signale -- jamais la seule teinte.
*/
Item {
    id: root

    /// Le bouton designe : 0 recommencer, 1 menu.
    property int currentIndex: 0

    property alias sceneHost: sceneHostItem
    property alias restartEntry: restartControl
    property alias menuEntry: menuControl

    width: 1920
    height: 1080

    Rectangle {
        anchors.fill: parent
        color: Tokens.panel
    }

    Item {
        id: sceneHostItem

        anchors.fill: parent
    }

    // Le voile : la scene reste DEVINABLE, teintee du grenat de la maquette.
    Rectangle {
        anchors.fill: parent
        color: Tokens.panel
        opacity: 0.55
    }

    Rectangle {
        anchors.fill: parent
        color: Tokens.gemShadow
        opacity: 0.45
    }

    Column {
        anchors.centerIn: parent
        spacing: Tokens.gapLarge

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Vous êtes mort")
            color: Tokens.textEnemy
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
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Tokens.gapSmall

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Le sable de l'Arena of Fate a bu votre sang.")
                color: Tokens.textOnPanel
                font.family: Tokens.loreFamily
                font.italic: true
                font.pixelSize: Tokens.fontSectionTitle
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("L'enfant attend toujours dans sa cellule.")
                color: Tokens.textOnPanel
                font.family: Tokens.loreFamily
                font.italic: true
                font.pixelSize: Tokens.fontSectionTitle
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Tokens.gapLarge

            Row {
                spacing: Tokens.gapSmall

                FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 0 ? 1 : 0 }
                OrnateButton {
                    id: restartControl
                    kind: "primary"
                    text: qsTr("Recommencer")
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
