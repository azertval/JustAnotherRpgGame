import QtQuick
import QtQuick.Controls
import Jadg.Ui

/*!
    Bouton rond de la charte v2 : raccourcis de navigation du HUD (LOT-87, T4.1).

    Piece `ui/button/round` (etats `normal`, `hover`, `pressed`, `disabled`), affichee a 64 px, et
    son icone (`ui/icon/nav/<entree>`). Un `Button` de Qt restyle, comme `OrnateButton` : le jumeau
    branche `clicked`, et le clavier et l'accessibilite viennent avec.

    Nomme `OrnateRoundButton` et non `RoundButton` : ce nom est deja celui d'un type de
    `QtQuick.Controls`, que l'import de ce module masquerait ou que ce type masquerait.

    Tant que l'icone n'est pas livree, le libelle s'ecrit dans le rond : un bouton sans icone ni
    libelle ne dirait pas ce qu'il ouvre.
*/
Button {
    id: root

    /// Cle de cahier de l'icone (`ui/icon/nav/inventory`), ou vide.
    property string iconKey: ""

    /// Etat impose (`normal`, `hover`, `pressed`, `disabled`), ou vide pour le deduire.
    property string forcedState: ""

    readonly property string visualState: root.forcedState.length > 0 ? root.forcedState
                                          : (!root.enabled ? "disabled"
                                          : (root.down ? "pressed" : (root.hovered || root.highlighted ? "hover" : "normal")))
    readonly property bool lit: root.visualState === "hover" || root.visualState === "pressed"
    readonly property bool disabledLook: root.visualState === "disabled"

    implicitWidth: 64 * Tokens.uiScale
    implicitHeight: 64 * Tokens.uiScale
    padding: 0

    Accessible.name: root.text

    background: Item {
        Rectangle {
            anchors.fill: parent
            visible: !art.delivered
            radius: width / 2
            color: root.lit ? Tokens.panelRaised : Tokens.panel
            border.color: root.disabledLook ? Tokens.textOnPanelMuted : (root.lit ? Tokens.goldLight : Tokens.panelEdge)
            border.width: Tokens.strokeWidth
        }

        FixedArt {
            id: art

            anchors.fill: parent
            key: "ui/button/round/" + root.visualState
        }
    }

    contentItem: Item {
        FixedArt {
            id: icon

            anchors.centerIn: parent
            width: parent.width * 0.6
            height: parent.height * 0.6
            key: root.iconKey
            opacity: root.disabledLook ? 0.5 : 1.0
        }

        Text {
            anchors.fill: parent
            anchors.margins: Tokens.strokeWidth * 2
            visible: !icon.delivered
            text: root.text
            color: root.disabledLook ? Tokens.textOnPanelMuted : Tokens.textOnPanel
            font.family: Tokens.bodyFamily
            font.pixelSize: Tokens.fontCaption
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: Tokens.fontCaption / 2
        }
    }
}
