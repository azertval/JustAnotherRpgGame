import QtQuick
import Jadg.Ui

/*!
    Menu principal -- FORMULAIRE, cote conception (LOT-87, T3.1 ; maquette 06).

    Transcrit de la maquette `06_Main_Menu_Mockup.png` (1672 x 941), cotes ramenees a 1920 x 1080 --
    la definition de conception -- puis multipliees par `Tokens.uiScale` : a 720p, tout l'ecran se
    reduit d'un bloc, dans les proportions de la maquette.

    - la scene en fond (`ui/background/menu-scene`) ; tant qu'elle n'est pas livree, l'aplat
      `panel` tient sa place -- aucune image du corpus ne sert de repli (LOT-94) ;
    - le logotype en haut a gauche, et sous lui six entrees a icone ;
    - l'encart de profil en haut a droite, la citation en bas a gauche, la version en bas a droite.

    **Six entrees, dont deux sans destination.** « Continuer » et « Charger une partie » attendent
    la sauvegarde (LOT-17) : elles sont dessinees, a leur place, dans l'etat `disabled` du bouton,
    et le jumeau ne les rend jamais courantes. `continueAvailable` et `loadAvailable` les rallumeront.

    **Le focus est une marque** (EX-IHM-071) : un losange d'or devant l'entree courante, en plus de la
    plaque grenat de l'etat actif.

    Aucune donnee ecrite ici : le profil et la version sont poses par le jumeau. Les valeurs
    ci-dessous ne sont que celles de la conception.
*/
Item {
    id: root

    /// L'entree courante, 0 a 5, dans l'ordre de l'ecran. Le jumeau la tient.
    property int currentIndex: 1

    /// Rallument « Continuer » et « Charger une partie » le jour ou la sauvegarde existe (LOT-17).
    property bool continueAvailable: false
    property bool loadAvailable: false

    /// L'encart de profil : nom du joueur et portrait (vide : cadre libre).
    property string profileName: "—"
    property url profilePortrait: ""

    /// La version du jeu, sans le « v » (`core::Engine::version()`, posee par le jumeau).
    property string version: "0.0.0"

    // Les entrees sont exposees une a une : c'est le jumeau qui leur attache le survol et le clic,
    // un formulaire ne pouvant pas contenir de code.
    property alias continueEntry: continueControl
    property alias newGameEntry: newGameControl
    property alias loadGameEntry: loadGameControl
    property alias optionsEntry: optionsControl
    property alias creditsEntry: creditsControl
    property alias quitEntry: quitControl

    width: 1920
    height: 1080

    // --- Le fond ----------------------------------------------------------------------------------
    Rectangle {
        anchors.fill: parent
        color: Tokens.panel
    }

    // Tant que la scene n'est pas livree, l'aplat ci-dessus en tient lieu : plus aucune image du
    // corpus ne sert de repli (LOT-94, EX-IHM-076).
    CoverArt {
        id: sceneArt

        anchors.fill: parent
        key: "ui/background/menu-scene"
    }

    // Le degrade qui porte la lisibilite du logo et des entrees, et NON un voile plein : la scene
    // doit rester visible. L'alpha est dans les paliers, pas dans l'opacite de l'element -- une
    // opacite uniforme eclaircirait aussi le bord gauche, la ou le texte a le plus besoin de fond.
    // La scene produite est peinte avec un tiers gauche sombre : elle n'en a pas besoin.
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width * 0.5
        visible: !sceneArt.delivered
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(Tokens.panel.r, Tokens.panel.g, Tokens.panel.b, 0.9) }
            GradientStop { position: 0.6; color: Qt.rgba(Tokens.panel.r, Tokens.panel.g, Tokens.panel.b, 0.65) }
            GradientStop { position: 1.0; color: Qt.rgba(Tokens.panel.r, Tokens.panel.g, Tokens.panel.b, 0.0) }
        }
    }

    // --- Logotype (maquette : 110, 25 -> 640, 345) ------------------------------------------------
    PanelFrame {
        x: 150 * Tokens.uiScale
        y: 105 * Tokens.uiScale
        width: 1620 * Tokens.uiScale
        height: 900 * Tokens.uiScale
        material: "parchment"
        bound: true
        padding: 0
    }
    PanelFrame {
        x: 950 * Tokens.uiScale
        y: 130 * Tokens.uiScale
        width: 795 * Tokens.uiScale
        height: 840 * Tokens.uiScale
        material: "parchment"
        subpanel: true
        padding: 0
    }
    PanelFrame {
        x: 202 * Tokens.uiScale
        y: 167 * Tokens.uiScale
        width: 716 * Tokens.uiScale
        height: 744 * Tokens.uiScale
        material: "parchment"
        subpanel: true
        padding: 28 * Tokens.uiScale
        CoverArt { anchors.fill: parent; key: "ui/background/menu-scene" }
        FixedArt {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20 * Tokens.uiScale
            width: 160 * Tokens.uiScale
            height: 180 * Tokens.uiScale
            key: "ui/ornament/crossed-crest"
        }
    }
    LogoPlate {
        id: logo

        x: 1125 * Tokens.uiScale
        y: 140 * Tokens.uiScale
        width: 440 * Tokens.uiScale
    }

    // --- Les six entrees (maquette : 150, 355 -> 540, 805 ; pas de 88 px a 1080p) -----------------
    Column {
        id: entries

        x: 1040 * Tokens.uiScale
        y: 430 * Tokens.uiScale
        spacing: Tokens.gapMedium

        Row {
            spacing: Tokens.gapSmall

            FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 0 ? 1 : 0 }
            OrnateButton {
                id: continueControl
                kind: "menu"
                iconKey: "ui/icon/menu/continue"
                text: qsTr("Continuer")
                enabled: root.continueAvailable
                highlighted: root.currentIndex === 0
                focusPolicy: Qt.NoFocus
            }
        }

        Row {
            spacing: Tokens.gapSmall

            FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 1 ? 1 : 0 }
            OrnateButton {
                id: newGameControl
                kind: "menu"
                iconKey: "ui/icon/menu/new-game"
                text: qsTr("Nouvelle partie")
                highlighted: root.currentIndex === 1
                focusPolicy: Qt.NoFocus
            }
        }

        Row {
            spacing: Tokens.gapSmall

            FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 2 ? 1 : 0 }
            OrnateButton {
                id: loadGameControl
                kind: "menu"
                iconKey: "ui/icon/menu/load-game"
                text: qsTr("Charger une partie")
                enabled: root.loadAvailable
                highlighted: root.currentIndex === 2
                focusPolicy: Qt.NoFocus
            }
        }

        Row {
            spacing: Tokens.gapSmall

            FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 3 ? 1 : 0 }
            OrnateButton {
                id: optionsControl
                kind: "menu"
                iconKey: "ui/icon/menu/options"
                text: qsTr("Options")
                highlighted: root.currentIndex === 3
                focusPolicy: Qt.NoFocus
            }
        }

        Row {
            spacing: Tokens.gapSmall

            FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 4 ? 1 : 0 }
            OrnateButton {
                id: creditsControl
                kind: "menu"
                iconKey: "ui/icon/menu/credits"
                text: qsTr("Crédits")
                highlighted: root.currentIndex === 4
                focusPolicy: Qt.NoFocus
            }
        }

        Row {
            spacing: Tokens.gapSmall

            FocusMark { anchors.verticalCenter: parent.verticalCenter; opacity: root.currentIndex === 5 ? 1 : 0 }
            OrnateButton {
                id: quitControl
                kind: "menu"
                iconKey: "ui/icon/menu/quit"
                text: qsTr("Quitter")
                highlighted: root.currentIndex === 5
                focusPolicy: Qt.NoFocus
            }
        }
    }

    // --- Encart de profil (maquette : 1355, 30 -> 1630, 100) --------------------------------------
    // 96 px de haut et non les 80 de la maquette : un 9-patch plus petit que ses deux marges (48 + 48)
    // superposerait ses coins.
    PanelFrame {
        id: profile

        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 48 * Tokens.uiScale
        anchors.topMargin: Tokens.gapLarge
        width: 316 * Tokens.uiScale
        height: 96 * Tokens.uiScale
        subpanel: true
        padding: 0

        PortraitFrame {
            id: profilePortraitFrame

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            size: 96 * Tokens.uiScale
            source: root.profilePortrait
        }

        Column {
            anchors.left: profilePortraitFrame.right
            anchors.right: parent.right
            anchors.leftMargin: Tokens.gapMedium
            anchors.rightMargin: Tokens.gapMedium
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: qsTr("Profil joueur")
                color: Tokens.textOnPanelMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
            }

            Text {
                width: parent.width
                text: root.profileName
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                elide: Text.ElideRight
            }
        }
    }

    // --- Citation (maquette : 28, 858 -> 392, 910) ------------------------------------------------
    QuotePlate {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: Tokens.gapLarge
        anchors.bottomMargin: Tokens.gapLarge
        text: qsTr("« Chaque choix façonne une nouvelle histoire. »")
    }

    // --- Version (maquette : 1595, 895 -> 1642, 915) ----------------------------------------------
    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: Tokens.gapLarge
        anchors.bottomMargin: Tokens.gapLarge
        text: "v" + root.version
        color: Tokens.textOnPanel
        font.family: Tokens.bodyFamily
        font.pixelSize: Tokens.fontCaption
        // Posee a meme la scene, sans plaque : un contour de `panel` la garde lisible sur un ciel
        // clair comme sur la carte de repli.
        style: Text.Outline
        styleColor: Tokens.panel
    }
}
