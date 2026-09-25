import QtQuick
import Jadg.Ui

/*!
    Habillage commun des trois vues de l'ecran « Carte » (LOT-94, LOT-95 ; maquette 08).

    Par-dessus la carte : la plaque de titre et son fil d'Ariane (monde > region > ville), l'encart
    du personnage, la mini-carte du niveau superieur et son cadre, les boutons de zoom et de retour,
    la barre d'etat (jour, lieu du groupe, aide des commandes) et la barre de navigation.

    La mini-carte ne parait qu'aux niveaux region et ville (`parentImage` non vide) : elle montre la
    carte d'ou l'on vient, et le cadre dore ce qu'on regarde. Un clic dessus remonte d'un niveau.

    Les proprietes portent des VALEURS D'EXEMPLE ; le formulaire les relaie au jumeau.
*/
Item {
    id: root

    property string title: "Tanares"
    property string trail: "Le monde connu"

    // --- L'encart du personnage ---------------------------------------------------------------------
    property string characterName: "Brenna Pierrefonte"
    property string levelText: "Niv. 3"
    property string hitPointsText: "25 / 30"
    property real hitPointsRatio: 25 / 30
    property string experienceText: "900 / —"
    property real experienceRatio: 0
    property url portrait: ""

    // --- La barre d'etat --------------------------------------------------------------------------
    property string clock: "—"
    property string partyLocation: "—"
    property string hint: "Flèches : choisir  ·  Entrée : ouvrir  ·  + / − : agrandir"

    // --- La mini-carte ------------------------------------------------------------------------------
    /// Image du niveau superieur (nom de fichier), ou vide au niveau du monde.
    property string parentImage: ""
    /// Cadre de la vue courante sur l'image du niveau superieur, en fractions.
    property rect parentFrame: Qt.rect(0.39, 0.19, 0.48, 0.48)

    /// Faux au niveau du monde : il n'y a pas de niveau ou remonter.
    property bool canGoBack: false

    signal zoomInRequested()
    signal zoomOutRequested()
    signal backRequested()
    signal questsRequested()
    signal inventoryRequested()
    signal companyRequested()
    signal optionsRequested()

    // --- Plaque de titre ---------------------------------------------------------------------------
    PanelFrame {
        x: 24 * Tokens.uiScale
        y: 24 * Tokens.uiScale
        width: 600 * Tokens.uiScale
        height: 128 * Tokens.uiScale
        subpanel: true

        FixedArt {
            id: crest

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 88 * Tokens.uiScale
            height: 88 * Tokens.uiScale
            key: "ui/ornament/crest-shield"
        }

        Column {
            anchors.left: crest.right
            anchors.right: parent.right
            anchors.leftMargin: Tokens.gapMedium
            anchors.verticalCenter: parent.verticalCenter

            Text {
                width: parent.width
                text: root.title
                color: Tokens.goldLight
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontScreenTitle
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: root.trail
                color: Tokens.textOnPanel
                font.family: Tokens.loreFamily
                font.italic: true
                font.pixelSize: Tokens.fontBody
                elide: Text.ElideRight
            }
        }
    }

    // --- Encart du personnage ------------------------------------------------------------------------
    PanelFrame {
        x: root.width - 520 * Tokens.uiScale
        y: 24 * Tokens.uiScale
        width: 496 * Tokens.uiScale
        height: 128 * Tokens.uiScale
        subpanel: true
        padding: Tokens.gapSmall

        PortraitFrame {
            id: characterPortrait

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            shape: "round"
            size: 108 * Tokens.uiScale
            source: root.portrait
        }

        Column {
            anchors.left: characterPortrait.right
            anchors.right: parent.right
            anchors.leftMargin: Tokens.gapMedium
            anchors.rightMargin: Tokens.gapSmall
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4 * Tokens.uiScale

            Text {
                width: parent.width
                text: root.characterName + "  ·  " + root.levelText
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
                elide: Text.ElideRight
            }

            Gauge {
                width: parent.width
                kind: "health"
                value: root.hitPointsRatio
                label: root.hitPointsText
            }

            Gauge {
                width: parent.width
                kind: "experience"
                value: root.experienceRatio
                label: root.experienceText
            }
        }
    }

    // --- Mini-carte du niveau superieur ----------------------------------------------------------------
    Rectangle {
        id: minimap

        visible: root.parentImage !== ""
        x: root.width - 344 * Tokens.uiScale
        y: 172 * Tokens.uiScale
        width: 320 * Tokens.uiScale
        height: 180 * Tokens.uiScale
        color: Tokens.panel
        border.color: Tokens.panelEdge
        border.width: Tokens.strokeWidth

        Image {
            anchors.fill: parent
            anchors.margins: Tokens.strokeWidth
            source: root.parentImage === "" ? ""
                    : "../../Elements/Assets/" + (root.parentImage.indexOf("/") >= 0
                                                  ? root.parentImage : "Maps/" + root.parentImage)
            sourceSize.width: 640
            fillMode: Image.Stretch
            smooth: true
        }

        Rectangle {
            x: root.parentFrame.x * minimap.width
            y: root.parentFrame.y * minimap.height
            width: root.parentFrame.width * minimap.width
            height: root.parentFrame.height * minimap.height
            color: Qt.rgba(Tokens.goldLight.r, Tokens.goldLight.g, Tokens.goldLight.b, 0.18)
            border.color: Tokens.goldLight
            border.width: Tokens.strokeWidth
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.backRequested()
        }
    }

    // --- Zoom et retour -----------------------------------------------------------------------------
    Column {
        x: root.width - 88 * Tokens.uiScale
        y: (root.parentImage !== "" ? 372 : 172) * Tokens.uiScale
        spacing: Tokens.gapSmall

        OrnateButton {
            width: 64 * Tokens.uiScale
            leftPadding: 0
            rightPadding: 0
            kind: "secondary"
            text: "+"
            focusPolicy: Qt.NoFocus
            onClicked: root.zoomInRequested()
        }

        OrnateButton {
            width: 64 * Tokens.uiScale
            leftPadding: 0
            rightPadding: 0
            kind: "secondary"
            text: "−"
            focusPolicy: Qt.NoFocus
            onClicked: root.zoomOutRequested()
        }

        OrnateButton {
            width: 64 * Tokens.uiScale
            leftPadding: 0
            rightPadding: 0
            kind: "secondary"
            text: "↩"
            enabled: root.canGoBack
            focusPolicy: Qt.NoFocus
            onClicked: root.backRequested()
        }
    }

    // --- Barre d'etat : jour, lieu du groupe, aide des commandes -----------------------------------------
    Item {
        x: 24 * Tokens.uiScale
        y: root.height - 80 * Tokens.uiScale
        width: 1000 * Tokens.uiScale
        height: 56 * Tokens.uiScale

        Rectangle {
            anchors.fill: parent
            visible: !statusArt.delivered
            color: Tokens.panel
            border.color: Tokens.panelEdge
            border.width: Tokens.strokeWidth
        }

        NinePatchArt {
            id: statusArt

            anchors.fill: parent
            key: "ui/plate/status-bar"
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: Tokens.gapLarge
            anchors.verticalCenter: parent.verticalCenter
            spacing: Tokens.gapLarge

            Text {
                text: root.clock
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
            }

            Text {
                text: root.partyLocation
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
            }

            Text {
                text: root.hint
                color: Tokens.textOnPanelMuted
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontCaption
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // --- Barre de navigation --------------------------------------------------------------------------
    PanelFrame {
        x: root.width - 590 * Tokens.uiScale
        y: root.height - 120 * Tokens.uiScale
        width: 566 * Tokens.uiScale
        height: 100 * Tokens.uiScale
        subpanel: true
        padding: Tokens.gapSmall

        Row {
            anchors.centerIn: parent
            spacing: Tokens.gapSmall

            OrnateButton {
                width: 100 * Tokens.uiScale
                leftPadding: 0
                rightPadding: 0
                kind: "primary"
                text: qsTr("Carte")
                enabled: false
                focusPolicy: Qt.NoFocus
            }
            OrnateButton {
                width: 100 * Tokens.uiScale
                leftPadding: 0
                rightPadding: 0
                kind: "secondary"
                text: qsTr("Quêtes")
                focusPolicy: Qt.NoFocus
                onClicked: root.questsRequested()
            }
            OrnateButton {
                width: 100 * Tokens.uiScale
                leftPadding: 0
                rightPadding: 0
                kind: "secondary"
                text: qsTr("Sac")
                focusPolicy: Qt.NoFocus
                onClicked: root.inventoryRequested()
            }
            OrnateButton {
                width: 100 * Tokens.uiScale
                leftPadding: 0
                rightPadding: 0
                kind: "secondary"
                text: qsTr("Équipe")
                focusPolicy: Qt.NoFocus
                onClicked: root.companyRequested()
            }
            OrnateButton {
                width: 100 * Tokens.uiScale
                leftPadding: 0
                rightPadding: 0
                kind: "secondary"
                text: qsTr("Options")
                focusPolicy: Qt.NoFocus
                onClicked: root.optionsRequested()
            }
        }
    }
}
