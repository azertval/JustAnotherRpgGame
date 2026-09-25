pragma ComponentBehavior: Bound
import QtQuick
import Jadg.Ui

/*!
    Le calque tactique (LOT-24, LOT-118) : ce que l'interface ajoute par-dessus la scene rendue
    d'un combat -- cases atteignables, surbrillance et points de vie de chaque combattant, jauge
    des ennemis, chemin et curseur de ciblage. Une marque par combattant ou par case atteignable,
    jamais une par case de la grille.

    Le calque du combat sur la carte (`CombatHudForm`), ne en commun avec le Colisee retire :
    seules changent la surface qu'il recouvre et l'origine de la grille. Sur la carte, la grille
    tactique est une ZONE de la carte : `zoneColumn` et `zoneRow` ramenent chaque case de la grille
    sur la carte que la surface dessine ; au Colisee, la grille est la carte, et ils valent zero.

    Le cadrage (`gridTileWidth`, `gridOriginX`...) est celui que la surface de rendu publie : le
    calque ne recalcule jamais ou une case est dessinee.
*/
Item {
    id: root

    /// Les combattants : `column`, `row`, `footprint`, `side`, `active`, `down`, `hitPoints`,
    /// `hitPointsRatio`.
    property var fighters: []
    property var reachableCells: []
    property var pathCells: []
    property int cursorColumn: 0
    property int cursorRow: 0
    /// Le curseur se cache une fois le combat fini.
    property bool ended: false
    /// L'origine de la grille sur la surface : la case (0, 0) de la grille est la case
    /// (`zoneColumn`, `zoneRow`) de ce que la surface dessine.
    property int zoneColumn: 0
    property int zoneRow: 0

    property real gridTileWidth: 64
    property real gridTileHeight: 40
    property real gridOriginX: 0
    property real gridOriginY: 0

    /// La souris, en coordonnees du calque : le survol vise, le clic se deplace ou attaque.
    signal gridHovered(real x, real y)
    signal gridClicked(real x, real y)

    // Le coin du losange de la case (c, r) de la grille, sur la surface : la case (c + zoneColumn,
    // r + zoneRow) de ce qu'elle dessine -- la meme projection que la surface, ecrite en ligne
    // (un formulaire ne porte pas de fonction).
    readonly property real zoneOffsetX: (root.zoneColumn - root.zoneRow) * root.gridTileWidth / 2
    readonly property real zoneOffsetY: (root.zoneColumn + root.zoneRow) * root.gridTileHeight / 2

    // Cases ou le combattant actif peut finir son deplacement
    Repeater {
        model: root.reachableCells

        ArenaMark {
            required property var modelData

            kind: "reachable"
            x: root.gridOriginX + root.zoneOffsetX + (modelData.column - modelData.row) * root.gridTileWidth / 2
            y: root.gridOriginY + root.zoneOffsetY + (modelData.column + modelData.row) * root.gridTileHeight / 2
            width: root.gridTileWidth
            height: root.gridTileHeight
        }
    }

    // Les combattants : l'element est le losange de leur emprise (n cases de cote).
    Repeater {
        model: root.fighters

        Item {
            id: fighter

            required property var modelData

            readonly property int footprint: Math.max(1, fighter.modelData.footprint)
            readonly property bool ally: fighter.modelData.side === "allies"
            readonly property bool active: fighter.modelData.active

            x: root.gridOriginX + root.zoneOffsetX + (fighter.modelData.column - fighter.modelData.row - (fighter.footprint - 1)) * root.gridTileWidth / 2
            y: root.gridOriginY + root.zoneOffsetY + (fighter.modelData.column + fighter.modelData.row) * root.gridTileHeight / 2
            width: root.gridTileWidth * fighter.footprint
            height: root.gridTileHeight * fighter.footprint

            // La surbrillance : au tour (or), allie, ennemi
            Rectangle {
                id: highlight

                anchors.centerIn: parent
                width: fighter.width * 0.62
                height: width
                color: fighter.active ? Tokens.goldLight : (fighter.ally ? Tokens.textAlly : Tokens.textEnemy)
                opacity: fighter.active ? 0.5 : 0.36
                border.color: fighter.active ? Tokens.goldLight : (fighter.ally ? Tokens.textAlly : Tokens.textEnemy)
                border.width: fighter.active ? 3 : 2
                transform: [
                    Rotation { angle: 45; origin.x: highlight.width / 2; origin.y: highlight.height / 2 },
                    Scale { yScale: fighter.height / fighter.width; origin.y: highlight.height / 2 }
                ]
            }

            // La jauge d'un ennemi, au-dessus de sa figurine
            Gauge {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.top
                anchors.bottomMargin: fighter.height * 0.55
                visible: !fighter.ally && !fighter.modelData.down
                width: fighter.width * 0.5
                height: Math.max(5, root.gridTileWidth * 0.08)
                kind: "health"
                value: fighter.modelData.hitPointsRatio
            }

            // Les points de vie, sous la case
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.bottom
                anchors.topMargin: -root.gridTileHeight * 0.3
                visible: text.length > 0
                text: fighter.modelData.hitPoints
                color: fighter.ally ? Tokens.textAlly : Tokens.textOnPanel
                font.family: Tokens.bodyFamily
                font.pixelSize: Math.max(9, root.gridTileWidth * 0.2)
                style: Text.Outline
                styleColor: Tokens.panel
            }
        }
    }

    // Chemin de ciblage (LOT-24)
    Repeater {
        model: root.pathCells

        ArenaMark {
            required property var modelData

            kind: "path"
            x: root.gridOriginX + root.zoneOffsetX + (modelData.column - modelData.row) * root.gridTileWidth / 2
            y: root.gridOriginY + root.zoneOffsetY + (modelData.column + modelData.row) * root.gridTileHeight / 2
            width: root.gridTileWidth
            height: root.gridTileHeight
        }
    }

    // Curseur de ciblage (LOT-24)
    ArenaMark {
        kind: "cursor"
        visible: !root.ended
        x: root.gridOriginX + root.zoneOffsetX + (root.cursorColumn - root.cursorRow) * root.gridTileWidth / 2
        y: root.gridOriginY + root.zoneOffsetY + (root.cursorColumn + root.cursorRow) * root.gridTileHeight / 2
        width: root.gridTileWidth
        height: root.gridTileHeight
    }

    // La souris : le survol vise, le clic se deplace ou attaque.
    MouseArea {
        anchors.fill: parent
        enabled: !root.ended
        hoverEnabled: true
        onPositionChanged: (mouse) => root.gridHovered(mouse.x, mouse.y)
        onClicked: (mouse) => root.gridClicked(mouse.x, mouse.y)
    }
}
