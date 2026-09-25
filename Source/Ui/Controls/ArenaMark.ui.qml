import QtQuick
import Jadg.Ui

/*!
    La marque de ciblage du calque tactique (`TacticalLayer`), posee sur une case de la grille
    isometrique du combat sur la carte (LOT-24, LOT-118).

    L'element EST le losange de la case : le calque le place par la meme projection que ses
    cases. Il ne dessine qu'un losange de jetons :

    - `cursor` : le contour d'or epais de la case visee par le curseur de ciblage ;
    - `path` : un petit losange d'or plein sur une case que le deplacement traverserait ;
    - `reachable` : un losange pale sur une case ou le combattant actif peut finir son deplacement.

    Les marques vivent dans leur propre calque, au-dessus de la scene rendue : deplacer le curseur
    ne reconstruit que ces quelques marques.
*/
Item {
    id: root

    /// `cursor`, `path` ou `reachable`.
    property string kind: "cursor"

    readonly property bool cursor: root.kind === "cursor"
    readonly property bool reachable: root.kind === "reachable"

    Rectangle {
        id: diamond

        anchors.centerIn: parent
        width: root.width * (root.cursor ? 0.7 : (root.reachable ? 0.62 : 0.26))
        height: width
        color: root.cursor ? "transparent" : (root.reachable ? Tokens.textAlly : Tokens.goldLight)
        opacity: root.cursor ? 1 : (root.reachable ? 0.28 : 0.7)
        border.color: root.reachable ? Tokens.textAlly : Tokens.goldLight
        border.width: root.cursor ? 4 : (root.reachable ? 2 : 0)
        transform: [
            Rotation { angle: 45; origin.x: diamond.width / 2; origin.y: diamond.height / 2 },
            Scale { yScale: root.height / root.width; origin.y: diamond.height / 2 }
        ]
    }
}
