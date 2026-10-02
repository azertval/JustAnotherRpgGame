import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Jadg.Ui

/*!
    Bouton de la charte v2 : entree de menu, action, validation, retour (LOT-87, T2.7).

    Un `Button` de Qt restyle, et non un dessin qui imiterait un bouton : le clavier,
    le pointeur et l'accessibilite viennent avec, et le jumeau branche `clicked` comme sur tout
    bouton. Sept pieces du cahier (`ui/button/<kind>`), une propriete pour choisir.

    **Les etats sont des proprietes.** L'etat visuel se deduit de ce que le bouton sait deja
    (`enabled`, `down`, `hovered`, `highlighted`), et `forcedState` l'impose : c'est ce qui permet a
    la galerie -- et a l'atelier, ou l'on ne survole rien -- de montrer chaque etat cote a cote.

    Entree de menu (`kind: "menu"`) : l'etat `active` est l'entree COURANTE (`highlighted`), celle
    que le clavier designe, et non celle qu'on enfonce. Les autres boutons n'ont pas
    d'entree courante ; `highlighted` y vaut un survol, pour que le focus au clavier se voie.
*/
Button {
    id: root

    /// `menu`, `primary`, `secondary`, `apply`, `cancel`, `default` ou `back`.
    property string kind: "primary"

    /// Cle de cahier de l'icone posee a gauche du libelle (`ui/icon/menu/new-game`), ou vide.
    property string iconKey: ""

    /// Etat impose (`normal`, `hover`, `pressed`, `active`, `disabled`), ou vide pour le deduire.
    property string forcedState: ""

    readonly property bool menu: root.kind === "menu"

    readonly property string visualState: root.forcedState.length > 0 ? root.forcedState
                                          : (!root.enabled ? "disabled"
                                          : (root.menu ? (root.highlighted || root.down ? "active" : (root.hovered ? "hover" : "normal"))
                                                       : (root.down ? "pressed" : (root.hovered || root.highlighted ? "hover" : "normal"))))

    readonly property bool lit: root.visualState === "hover" || root.visualState === "pressed"
                                || root.visualState === "active"
    readonly property bool disabledLook: root.visualState === "disabled"

    readonly property string key: "ui/button/" + root.kind + "/" + root.visualState

    // Tailles de production du cahier, a 1080p : 480 x 72 (menu), 320 x 72 (retour), 280 x 56.
    implicitWidth: (root.menu ? 480 : (root.kind === "back" ? 320 : 280)) * Tokens.uiScale
    implicitHeight: (root.menu || root.kind === "back" ? 72 : 56) * Tokens.uiScale

    // La marge gauche des pieces de menu et de retour (96 px) porte leur ornement et l'icone.
    leftPadding: root.menu || root.kind === "back" ? 96 * Tokens.uiScale : Math.min(48 * Tokens.uiScale, width * 0.09)
    rightPadding: root.menu || root.kind === "back" ? 48 * Tokens.uiScale : Math.min(48 * Tokens.uiScale, width * 0.09)
    topPadding: 0
    bottomPadding: 0

    background: Item {
        Rectangle {
            anchors.fill: parent
            visible: !art.delivered
            color: root.disabledLook ? Tokens.panel
                   : (root.kind === "apply" ? Tokens.success
                   : (root.kind === "cancel" ? Tokens.danger
                   : (root.kind === "default" ? Tokens.info
                   : (root.kind === "primary" || root.kind === "back" ? (root.lit ? Tokens.gemLight : Tokens.gem)
                   : (root.menu ? (root.visualState === "active" ? Tokens.gemLight : (root.lit ? Tokens.panelRaised : Tokens.panel))
                   : (root.lit ? Tokens.panelRaised : Tokens.panel))))))
            border.color: root.disabledLook ? Tokens.textOnPanelMuted
                          : (root.lit ? Tokens.goldLight : Tokens.panelEdge)
            border.width: Tokens.strokeWidth
        }

        NinePatchArt {
            id: art

            anchors.fill: parent
            key: root.key
        }
    }

    contentItem: RowLayout {
        spacing: Tokens.gapSmall

        Image {
            Layout.preferredWidth: 40 * Tokens.uiScale
            Layout.preferredHeight: 40 * Tokens.uiScale
            Layout.alignment: Qt.AlignVCenter
            visible: source.toString().length > 0
            source: Artwork.delivered[root.iconKey] ? Artwork.baseUrl + Artwork.delivered[root.iconKey].file : ""
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            opacity: root.disabledLook ? 0.5 : 1.0
        }

        Text {
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.text
            color: root.disabledLook ? Tokens.textOnPanelMuted : Tokens.textOnPanel
            font.family: Tokens.titleFamily
            font.pixelSize: root.menu ? Tokens.fontSectionTitle : Tokens.fontBody
            horizontalAlignment: root.menu ? Text.AlignLeft : Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
}
