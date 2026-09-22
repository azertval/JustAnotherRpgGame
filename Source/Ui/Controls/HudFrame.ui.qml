pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import Jadg.Ui

/*!
    Chassis du HUD de jeu : le cadre de la maquette 01, autour de la scene (LOT-87, T4.1).

    Ce que la vue d'exploration (`GameViewForm`) et le HUD de combat (`CombatHudForm`) partagent :
    l'hote de la surface de rendu, le personnage actif (portrait, niveau, nom, vie, experience), la
    colonne des membres, la barre de boussole, la mini-carte, les quetes, le jour et le lieu, les
    raccourcis de navigation et l'indicateur de mode. Ce qui est propre au combat -- journal, roue
    et barre d'actions, fiche de la cible, ordre d'initiative -- se pose dans le formulaire, comme
    le contenu d'un `ScreenPage`.

    **La scene n'est pas la maquette.** La maquette peint une scene isometrique ; le moteur rend le
    viewport de la scene 2D HD (LOT-103). Le cadre se pose PAR-DESSUS l'hote, sans fond : tout ce
    qui n'est pas un bloc laisse voir la scene.

    Cotes relevees sur `01_InGame_HUD_Mockup.png` (1672 x 940), ramenees a 1920 x 1080 (x 1,148),
    puis multipliees par `Tokens.uiScale`. Les proprietes portent des VALEURS D'EXEMPLE ; les
    jumeaux les remplacent.

    Les listes ont les roles de `SheetRowModel` (`rowId`, `label`, `value`), ceux que pose
    `PendingData.rows()` :
    - `party` : `label` le nom, `value` les points de vie (`42 / 52`), et le role facultatif
      `ratio` (0 a 1) qui remplit la jauge -- une chaine ne se decoupe pas dans un `.ui.qml`, ou
      Qt Design Studio refuse tout appel de fonction ;
    - `quests` : `label` le titre, `value` l'objectif en cours ; la PREMIERE ligne est la quete
      suivie.
*/
Item {
    id: root

    /// Vrai tant qu'aucune donnee reelle n'alimente le HUD.
    property bool pending: false

    /// Le mode de jeu courant : `exploration` ou `combat` (`IGameMode::name()`).
    property string mode: "exploration"

    // --- Le personnage actif -----------------------------------------------------------------------
    property string characterName: "Brenna Pierrefonte"
    property string level: "3"
    property string hitPointsText: "25 / 30"
    property real hitPointsRatio: 25 / 30
    property string experienceText: "900 / 2700"
    property real experienceRatio: 900 / 2700
    property url portrait: ""

    // --- Les membres et les quetes ----------------------------------------------------------------
    property var party: exampleParty
    /// Le membre actif (indice dans `party`), ou -1.
    property int activeMember: 0
    property var quests: exampleQuests

    // --- La barre d'etat et la mini-carte ------------------------------------------------------------
    property string clock: "Jour 3, 10 h 24"
    property string location: "Forêt de la Brume"
    property url minimap: ""

    property alias viewportHost: viewportHost
    property alias inventoryButton: inventoryControl
    property alias journalButton: journalControl
    property alias mapButton: mapControl
    property alias optionsButton: optionsControl

    /// Ce que le formulaire pose par-dessus la scene : la plein-ecran de conception, sous le cadre.
    default property alias content: contentArea.data

    readonly property ListModel exampleParty: ListModel {
        ListElement { rowId: "brenna"; label: "Brenna"; value: "25 / 30"; ratio: 0.83 }
        ListElement { rowId: "sarre"; label: "Sarre"; value: "38 / 45"; ratio: 0.84 }
        ListElement { rowId: "ourse"; label: "Ourse"; value: "28 / 32"; ratio: 0.88 }
        ListElement { rowId: "ilse"; label: "Ilse"; value: "22 / 28"; ratio: 0.79 }
    }

    readonly property ListModel exampleQuests: ListModel {
        ListElement { rowId: "convoi"; label: "Le convoi disparu"; value: "Trouver le marchand disparu (1/3)" }
        ListElement { rowId: "pillards"; label: "Les pillards"; value: "Éliminer les bandits (2/5)" }
    }

    width: 1920
    height: 1080

    // L'hote de la surface de rendu. La surface elle-meme (`GameViewport`) est un type C++ que
    // l'atelier ne connait pas : c'est le jumeau qui la pose ici, a l'execution. Dans Qt Design
    // Studio, l'hote se dessine comme un aplat au parchemin -- ce que la surface efface de toute
    // facon tant qu'aucune scene n'est jouee.
    Rectangle {
        id: viewportHost

        anchors.fill: parent
        color: Tokens.background
    }

    Item {
        id: contentArea

        anchors.fill: parent
    }

    // --- Personnage actif (maquette : 18, 12 -> 520, 212) ---------------------------------------------
    Item {
        id: namePlate

        x: 200 * Tokens.uiScale
        y: 36 * Tokens.uiScale
        width: 340 * Tokens.uiScale
        height: 52 * Tokens.uiScale

        Rectangle {
            anchors.fill: parent
            visible: !namePlateArt.delivered
            color: Tokens.panel
            border.color: Tokens.accent
            border.width: Tokens.strokeWidth
        }

        NinePatchArt {
            id: namePlateArt

            anchors.fill: parent
            key: "ui/plate/title-black"
        }

        Text {
            anchors.fill: parent
            anchors.leftMargin: 48 * Tokens.uiScale
            anchors.rightMargin: Tokens.gapLarge
            text: root.characterName
            color: Tokens.textOnPanel
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontSectionTitle
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    Gauge {
        x: 216 * Tokens.uiScale
        y: 96 * Tokens.uiScale
        width: 380 * Tokens.uiScale
        height: 32 * Tokens.uiScale
        kind: "health"
        value: root.hitPointsRatio
        label: root.hitPointsText
    }

    Gauge {
        x: 216 * Tokens.uiScale
        y: 138 * Tokens.uiScale
        width: 344 * Tokens.uiScale
        height: 32 * Tokens.uiScale
        kind: "experience"
        value: root.experienceRatio
        label: root.experienceText
    }

    // Pose apres les jauges : le medaillon mord sur leur embout gauche, comme sur la maquette.
    PortraitFrame {
        x: 20 * Tokens.uiScale
        y: 16 * Tokens.uiScale
        shape: "hud"
        size: 200 * Tokens.uiScale
        source: root.portrait
        level: root.level
    }

    // --- Membres (maquette : 28, 222 -> 142, 640) ---------------------------------------------------
    Column {
        x: 32 * Tokens.uiScale
        y: 252 * Tokens.uiScale
        spacing: 12 * Tokens.uiScale

        Repeater {
            model: root.party

            Item {
                id: member

                required property int index
                // Le modele entier, et non ses roles un par un : `ratio` est un role FACULTATIF,
                // que les lignes en attente de `PendingData.rows()` n'ont pas. Un role requis absent
                // ferait echouer la delegation ; lu ici, il vaut `undefined`, et la jauge reste vide.
                required property var model

                readonly property string label: member.model.label
                readonly property string value: member.model.value
                readonly property real ratio: member.model.ratio !== undefined ? member.model.ratio : 0

                // Le rail des points de vie mord sur le bas du portrait, comme sur la maquette.
                width: 112 * Tokens.uiScale
                height: 112 * Tokens.uiScale

                PortraitFrame {
                    id: memberPortrait

                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    shape: "square"
                    size: 96 * Tokens.uiScale
                    active: member.index === root.activeMember
                }

                Gauge {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 24 * Tokens.uiScale
                    kind: "health"
                    value: member.ratio
                    label: member.value.length > 0 ? member.value : member.label
                }

                // Le membre actif se signale par une marque, pas par la seule teinte de son cadre
                // (EX-IHM-071).
                FocusMark {
                    anchors.left: memberPortrait.right
                    anchors.verticalCenter: memberPortrait.verticalCenter
                    anchors.leftMargin: Tokens.gapSmall
                    visible: member.index === root.activeMember
                }
            }
        }
    }

    // --- Barre de boussole (maquette : 585, 35 -> 1082, 78) --------------------------------------------
    Item {
        id: compass

        x: 672 * Tokens.uiScale
        y: 40 * Tokens.uiScale
        width: 576 * Tokens.uiScale
        height: 50 * Tokens.uiScale

        Rectangle {
            anchors.fill: parent
            visible: !compassArt.delivered
            color: Tokens.panel
            border.color: Tokens.panelEdge
            border.width: Tokens.strokeWidth
        }

        NinePatchArt {
            id: compassArt

            anchors.fill: parent
            key: "ui/plate/compass-bar"
        }

        Row {
            anchors.centerIn: parent
            spacing: 160 * Tokens.uiScale

            Text { text: qsTr("O"); color: Tokens.textOnPanel; font.family: Tokens.titleFamily; font.pixelSize: Tokens.fontBody }
            Text { text: qsTr("N"); color: Tokens.goldLight; font.family: Tokens.titleFamily; font.pixelSize: Tokens.fontBody; font.weight: Font.Bold }
            Text { text: qsTr("E"); color: Tokens.textOnPanel; font.family: Tokens.titleFamily; font.pixelSize: Tokens.fontBody }
        }
    }

    // Pose sur un aplat : sous le cadre, c'est la scene, dont rien ne garantit le contraste.
    Rectangle {
        anchors.horizontalCenter: compass.horizontalCenter
        anchors.top: compass.bottom
        anchors.topMargin: Tokens.gapSmall
        width: pendingNote.implicitWidth + 2 * Tokens.gapMedium
        height: pendingNote.implicitHeight + Tokens.gapSmall
        visible: root.pending
        color: Tokens.panel

        Text {
            id: pendingNote

            anchors.centerIn: parent
            text: qsTr("HUD dessiné, données à venir")
            color: Tokens.textOnPanelMuted
            font.family: Tokens.loreFamily
            font.italic: true
            font.pixelSize: Tokens.fontCaption
        }
    }

    // --- Mini-carte (maquette : 1398, 22 -> 1658, 268) ------------------------------------------------
    Item {
        x: 1628 * Tokens.uiScale
        y: 28 * Tokens.uiScale
        width: 264 * Tokens.uiScale
        height: 264 * Tokens.uiScale

        // La carte, decoupee au rond sur un disque sombre ; l'anneau livre se pose par-dessus,
        // centre ajoure. Pas de PortraitFrame : son etat vide peint une silhouette de personnage.
        Rectangle {
            id: minimapMask

            anchors.fill: parent
            anchors.margins: parent.width * 0.18 // ouverture de l'anneau livre : rayon 0,29
            radius: width / 2
            color: Tokens.panel
            layer.enabled: true
        }

        Image {
            id: minimapImage

            anchors.fill: minimapMask
            visible: false
            source: root.minimap
            fillMode: Image.PreserveAspectCrop
            smooth: true
            mipmap: true
        }

        MultiEffect {
            anchors.fill: minimapMask
            visible: root.minimap.toString().length > 0
            source: minimapImage
            maskEnabled: true
            maskSource: minimapMask
        }

        FixedArt {
            id: ringArt

            anchors.fill: parent
            key: "ui/medallion/minimap-ring"
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: -Tokens.gapSmall
            text: qsTr("N")
            color: Tokens.goldLight
            style: Text.Outline
            styleColor: Tokens.panel
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontBody
            font.weight: Font.Bold
        }
    }

    // --- Quetes (maquette : 1398, 278 -> 1658, 485) ------------------------------------------------------
    PanelFrame {
        x: 1600 * Tokens.uiScale
        y: 312 * Tokens.uiScale
        width: 296 * Tokens.uiScale
        height: 244 * Tokens.uiScale
        subpanel: true

        Text {
            id: questsTitle

            anchors.left: parent.left
            anchors.top: parent.top
            text: qsTr("Quêtes")
            color: Tokens.textOnPanel
            font.family: Tokens.titleFamily
            font.pixelSize: Tokens.fontSectionTitle
            font.weight: Font.DemiBold
        }

        GoldDivider {
            id: questsDivider

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: questsTitle.bottom
            anchors.topMargin: Tokens.gapSmall
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: questsDivider.bottom
            anchors.bottom: parent.bottom
            anchors.topMargin: Tokens.gapSmall
            spacing: Tokens.gapSmall
            clip: true

            Repeater {
                model: root.quests

                Item {
                    id: quest

                    required property int index
                    required property string label
                    required property string value

                    readonly property bool tracked: quest.index === 0

                    width: parent.width
                    height: questTitle.implicitHeight + questObjective.implicitHeight

                    Item {
                        id: questMark

                        anchors.left: parent.left
                        anchors.verticalCenter: questTitle.verticalCenter
                        width: 20 * Tokens.uiScale
                        height: 20 * Tokens.uiScale

                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width * 0.7
                            height: parent.height * 0.7
                            visible: !questPipArt.delivered
                            rotation: quest.tracked ? 45 : 0
                            radius: quest.tracked ? 0 : width / 2
                            color: quest.tracked ? Tokens.goldLight : "transparent"
                            border.color: quest.tracked ? Tokens.panelEdge : Tokens.textOnPanel
                            border.width: Tokens.strokeWidth
                        }

                        FixedArt {
                            id: questPipArt

                            anchors.fill: parent
                            key: quest.tracked ? "ui/control/pip/filled" : "ui/control/pip/empty"
                        }
                    }

                    Text {
                        id: questTitle

                        anchors.left: questMark.right
                        anchors.right: parent.right
                        anchors.leftMargin: Tokens.gapSmall
                        text: quest.label
                        color: quest.tracked ? Tokens.goldLight : Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontBody
                        elide: Text.ElideRight
                    }

                    Text {
                        id: questObjective

                        anchors.left: questTitle.left
                        anchors.right: parent.right
                        anchors.top: questTitle.bottom
                        text: quest.value.length > 0 ? "•  " + quest.value : ""
                        color: Tokens.textOnPanel
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    // --- Barre d'etat : jour et lieu (maquette : 0, 870 -> 388, 915) ---------------------------------------
    Item {
        x: 0
        y: 1004 * Tokens.uiScale
        width: 480 * Tokens.uiScale
        height: 52 * Tokens.uiScale

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
                text: root.location
                color: Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
            }
        }
    }

    // --- Raccourcis de navigation (maquette : 1410, 790 -> 1660, 860) ---------------------------------------
    Row {
        x: 1596 * Tokens.uiScale
        y: 912 * Tokens.uiScale
        spacing: Tokens.gapSmall

        OrnateRoundButton {
            id: inventoryControl

            text: qsTr("Sac")
            iconKey: "ui/icon/nav/inventory"
            focusPolicy: Qt.NoFocus
        }

        OrnateRoundButton {
            id: journalControl

            text: qsTr("Quêtes")
            iconKey: "ui/icon/nav/quests"
            focusPolicy: Qt.NoFocus
        }

        OrnateRoundButton {
            id: mapControl

            text: qsTr("Carte")
            iconKey: "ui/icon/nav/map"
            focusPolicy: Qt.NoFocus
        }

        OrnateRoundButton {
            id: optionsControl

            text: qsTr("Options")
            iconKey: "ui/icon/nav/options"
            focusPolicy: Qt.NoFocus
        }
    }

    // --- Indicateur de mode (maquette : 1368, 872 -> 1662, 910) ---------------------------------------------
    Item {
        x: 1572 * Tokens.uiScale
        y: 1004 * Tokens.uiScale
        width: 336 * Tokens.uiScale
        height: 48 * Tokens.uiScale

        Rectangle {
            anchors.fill: parent
            visible: !ribbonArt.delivered
            color: Tokens.surface
            border.color: Tokens.border
            border.width: Tokens.strokeWidth
        }

        NinePatchArt {
            id: ribbonArt

            anchors.fill: parent
            key: "ui/plate/parchment-ribbon"
        }

        Row {
            anchors.centerIn: parent
            spacing: Tokens.gapSmall

            Text {
                text: qsTr("Exploration")
                color: root.mode === "exploration" ? Tokens.text : Tokens.textMuted
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontBody
                font.weight: root.mode === "exploration" ? Font.Bold : Font.Normal
                font.underline: root.mode === "exploration"
            }

            Text {
                text: "·"
                color: Tokens.textMuted
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontBody
            }

            Text {
                text: qsTr("Tactique")
                color: root.mode === "combat" ? Tokens.text : Tokens.textMuted
                font.family: Tokens.titleFamily
                font.pixelSize: Tokens.fontBody
                font.weight: root.mode === "combat" ? Font.Bold : Font.Normal
                font.underline: root.mode === "combat"
            }
        }
    }
}
