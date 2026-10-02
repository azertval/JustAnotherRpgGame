import QtQuick
import QtQuick.Controls
import Jadg.Ui

/*!
    Case a cocher de la charte v2 (LOT-87, T2.7).

    Un `CheckBox` de Qt restyle : le jumeau lie `checked` a son reglage comme sur tout controle.
    Piece `ui/control/checkbox`, affichee a 28 px pour 1080p, etats `checked`, `unchecked`,
    `disabled` ; `forcedState` impose l'etat.
*/
CheckBox {
    id: root

    /// Etat impose (`checked`, `unchecked`, `disabled`), ou vide pour le deduire.
    property string forcedState: ""
    property string material: "dark"

    readonly property string visualState: root.forcedState.length > 0 ? root.forcedState
                                          : (!root.enabled ? "disabled" : (root.checked ? "checked" : "unchecked"))

    readonly property string key: "ui/control/checkbox/" + root.visualState

    spacing: Tokens.gapMedium
    padding: 0

    indicator: Item {
        x: root.leftPadding
        y: root.topPadding + (root.availableHeight - height) / 2
        implicitWidth: 28 * Tokens.uiScale
        implicitHeight: 28 * Tokens.uiScale

        Rectangle {
            anchors.fill: parent
            visible: !art.delivered
            color: Tokens.panel
            border.color: root.visualState === "disabled" ? Tokens.textOnPanelMuted : Tokens.panelEdge
            border.width: Tokens.strokeWidth

            Rectangle {
                anchors.fill: parent
                anchors.margins: parent.width * 0.25
                visible: root.visualState === "checked"
                color: Tokens.goldLight
            }
        }

        FixedArt {
            id: art

            anchors.fill: parent
            key: root.key
        }
    }

    contentItem: Text {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        color: root.material === "parchment"
               ? (root.visualState === "disabled" ? Tokens.textMuted : Tokens.text)
               : (root.visualState === "disabled" ? Tokens.textOnPanelMuted : Tokens.textOnPanel)
        font.family: Tokens.bodyFamily
        font.pixelSize: Tokens.fontBody
        verticalAlignment: Text.AlignVCenter
    }
}
