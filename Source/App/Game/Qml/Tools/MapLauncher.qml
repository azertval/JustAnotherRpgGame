pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Jadg.Ui
import Jadg.Runtime

/*!
    Le lanceur de cartes -- un OUTIL DE DEBUG, pas un ecran du jeu (`--screen=MapLauncher`).

    Il remplace l'ecran du Colisee (LOT-50) : depuis que le combat se joue sur la carte (LOT-118),
    eprouver une fonctionnalite, c'est ouvrir la carte qu'on vient de dessiner, au bon endroit, dans
    le bon etat. La liste est celle des cartes du contenu (`Levels/`), precedee des dossiers qu'on
    ajoute -- les brouillons de l'editeur, comme `--levels=`. Les champs de droite sont ceux de la
    ligne de commande (`--map=@arrivee`, `--at=`, `--flags=`, `--hero-figure=`), et lancer passe par
    les memes fonctions qu'elle (`MapLauncherModel.launch`).

    Il vit dans Tools/ et non Screens/ : sans formulaire dans Source/Ui, l'atelier n'a rien a y
    dessiner, et rien ici n'est du dessin de conception -- la palette est celle des panneaux
    sombres, les controles ceux de Qt.

    | Geste | Souris | Clavier |
    |---|---|---|
    | Choisir une carte | clic | fleches haut, bas |
    | Lancer | bouton « Lancer » | Entree (hors d'un champ de texte : Ctrl+Entree) |
    | Quitter l'outil | bouton « Fermer » | Echap |
*/
Rectangle {
    id: root

    color: Tokens.panel
    focus: true

    /// Emis quand l'outil veut se refermer : la pile d'ecrans rend la main au routeur.
    signal toolClosed()

    readonly property MapLauncherModel launcher: MapLauncherModel {}

    /// La carte choisie, ou -1.
    property int selectedIndex: -1
    readonly property var selectedMap: root.selectedIndex >= 0 && root.selectedIndex < root.launcher.maps.length
                                       ? root.launcher.maps[root.selectedIndex] : null

    // `toolClosed` fait detruire cet ecran : jamais depuis le gestionnaire lui-meme.
    Keys.onEscapePressed: Qt.callLater(function () { root.toolClosed() })
    Keys.onUpPressed: root.select(root.selectedIndex - 1)
    Keys.onDownPressed: root.select(root.selectedIndex + 1)
    Keys.onReturnPressed: root.launch()
    Keys.onEnterPressed: root.launch()

    function select(index) {
        if (root.launcher.maps.length === 0)
            return
        root.selectedIndex = Math.max(0, Math.min(root.launcher.maps.length - 1, index))
        list.positionViewAtIndex(root.selectedIndex, ListView.Contain)
    }

    function launch() {
        if (root.selectedMap === null)
            return
        if (root.launcher.launch(root.selectedMap.id, arrivalField.text, atField.text,
                                 flagsField.text, figureField.text)) {
            // DIFFERE : le routeur fait remplacer cet ecran par la pile, et le detruire
            // pendant le gestionnaire (clic, touche) qui a demande le lancement plante Qt.
            Qt.callLater(function () { ScreenRouter.jumpToGame() })
        }
    }

    // Le modele a deja lu ses cartes a sa construction, avant que la liaison ci-dessous existe :
    // la premiere carte se choisit ici.
    Component.onCompleted: root.select(0)

    // La liste se vide et se remplit : la selection suit, sans sortir des bornes.
    Connections {
        target: root.launcher
        function onChanged() {
            if (root.selectedIndex >= root.launcher.maps.length)
                root.selectedIndex = root.launcher.maps.length - 1
            if (root.selectedIndex < 0 && root.launcher.maps.length > 0)
                root.selectedIndex = 0
        }
    }

    /// Un bouton de la barre : un libelle, un etat enfonce.
    component ToolButton: Rectangle {
        id: button

        property string text
        property bool active: false
        signal clicked()

        implicitWidth: label.implicitWidth + Tokens.gapMedium * 2
        implicitHeight: Tokens.gapLarge
        radius: Tokens.strokeWidth * 2
        color: active ? Tokens.panelEdge : Tokens.panelRaised
        border.width: 1
        border.color: area.containsMouse || active ? Tokens.panelEdge : Tokens.textOnPanelMuted

        Text {
            id: label
            anchors.centerIn: parent
            text: button.text
            color: button.active ? Tokens.panel : Tokens.textOnPanel
            font.pixelSize: Tokens.fontCaption
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: button.clicked()
        }
    }

    /// Un intitule de section.
    component SectionTitle: Text {
        color: Tokens.textOnPanelMuted
        font.pixelSize: Tokens.fontCaption
        font.bold: true
        font.capitalization: Font.AllUppercase
    }

    /// Un champ de saisie sur fond sombre, avec son libelle a gauche.
    component Field: Row {
        id: field

        property string label
        property string placeholder
        property alias text: input.text

        spacing: Tokens.gapSmall
        width: parent.width

        Text {
            width: Tokens.gapLarge * 3
            anchors.verticalCenter: parent.verticalCenter
            text: field.label
            color: Tokens.textOnPanelMuted
            font.pixelSize: Tokens.fontCaption
        }
        TextField {
            id: input
            width: field.width - Tokens.gapLarge * 3 - field.spacing
            placeholderText: field.placeholder
            font.pixelSize: Tokens.fontCaption
            // Ctrl+Entree lance depuis un champ ; Entree seule y reste, pour ne pas lancer sur une
            // valeur a moitie tapee.
            Keys.onPressed: (event) => {
                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                        && (event.modifiers & Qt.ControlModifier)) {
                    root.launch()
                    event.accepted = true
                } else if (event.key === Qt.Key_Escape) {
                    root.forceActiveFocus()
                    event.accepted = true
                }
            }
        }
    }

    // --- La barre du haut --------------------------------------------------------------------
    Rectangle {
        id: toolbar

        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Tokens.gapLarge * 1.75
        color: Tokens.panelRaised

        Row {
            anchors { left: parent.left; leftMargin: Tokens.gapMedium; verticalCenter: parent.verticalCenter }
            spacing: Tokens.gapSmall

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "Lanceur de cartes"
                color: Tokens.textOnPanel
                font.pixelSize: Tokens.fontBody
                font.bold: true
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "--screen=MapLauncher · " + root.launcher.maps.length + " carte(s)"
                color: Tokens.textOnPanelMuted
                font.pixelSize: Tokens.fontCaption
            }
        }

        Row {
            anchors { right: parent.right; rightMargin: Tokens.gapMedium; verticalCenter: parent.verticalCenter }
            spacing: Tokens.gapSmall

            ToolButton {
                text: "Actualiser"
                onClicked: root.launcher.refresh()
            }
            ToolButton {
                text: "Fermer (Echap)"
                onClicked: Qt.callLater(function () { root.toolClosed() })
            }
        }
    }

    // --- La liste des cartes -------------------------------------------------------------------
    Rectangle {
        id: listPane

        anchors { left: parent.left; top: toolbar.bottom; bottom: statusBar.top }
        anchors.margins: Tokens.gapMedium
        width: Math.round(parent.width * 0.55)
        color: Tokens.panelRaised
        border.width: 1
        border.color: Tokens.textOnPanelMuted

        ListView {
            id: list

            anchors.fill: parent
            anchors.margins: Tokens.strokeWidth
            clip: true
            model: root.launcher.maps
            currentIndex: root.selectedIndex
            ScrollBar.vertical: ScrollBar {}

            delegate: Rectangle {
                id: row

                required property int index
                required property var modelData

                readonly property bool broken: modelData.error.length > 0
                readonly property bool current: index === root.selectedIndex

                width: list.width
                height: idText.implicitHeight + detailText.implicitHeight + Tokens.gapSmall * 1.5
                color: current ? Tokens.panelEdge : (index % 2 === 0 ? Tokens.panelRaised : Tokens.panel)

                Column {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                    anchors.leftMargin: Tokens.gapSmall
                    anchors.rightMargin: Tokens.gapSmall

                    Text {
                        id: idText
                        width: parent.width
                        elide: Text.ElideMiddle
                        text: row.modelData.id + (row.modelData.name.length > 0 ? "  —  " + row.modelData.name : "")
                        color: row.current ? Tokens.panel : (row.broken ? Tokens.textEnemy : Tokens.textOnPanel)
                        font.pixelSize: Tokens.fontBody
                    }
                    Text {
                        id: detailText
                        width: parent.width
                        elide: Text.ElideMiddle
                        text: row.broken ? row.modelData.error
                                         : row.modelData.columns + " × " + row.modelData.rows + "  ·  " + row.modelData.directory
                        color: row.current ? Tokens.panel : Tokens.textOnPanelMuted
                        font.pixelSize: Tokens.fontCaption
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.forceActiveFocus()
                        root.select(row.index)
                    }
                    onDoubleClicked: {
                        root.select(row.index)
                        root.launch()
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: "Aucune carte sous " + root.launcher.levelsRoot
                color: Tokens.textOnPanelMuted
                font.pixelSize: Tokens.fontCaption
            }
        }
    }

    // --- Les reglages du lancement -------------------------------------------------------------
    Column {
        id: settings

        anchors { left: listPane.right; right: parent.right; top: toolbar.bottom }
        anchors.margins: Tokens.gapMedium
        spacing: Tokens.gapSmall

        SectionTitle { text: "Carte choisie" }
        Text {
            width: parent.width
            wrapMode: Text.Wrap
            text: root.selectedMap === null ? "—" : root.selectedMap.file
            color: Tokens.textOnPanel
            font.pixelSize: Tokens.fontCaption
        }

        Item { width: 1; height: Tokens.gapSmall }
        SectionTitle { text: "Options du lancement (celles de la ligne de commande)" }

        Field { id: arrivalField; label: "@arrivee"; placeholder: "point d'arrivee nomme, vide = l'entree" }
        Field { id: atField; label: "--at="; placeholder: "<colonne>,<ligne>" }
        Field { id: flagsField; label: "--flags="; placeholder: "quest/x/started,cle=valeur" }
        Field { id: figureField; label: "--hero-figure="; placeholder: "Common/Characters/Heroes/brawler" }

        Row {
            spacing: Tokens.gapSmall
            ToolButton {
                text: "Lancer (Entree)"
                active: root.selectedMap !== null && root.selectedMap.error.length === 0
                onClicked: root.launch()
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.selectedMap !== null ? "--map=" + root.selectedMap.id
                                                  + (arrivalField.text.length > 0 ? "@" + arrivalField.text : "")
                                                : ""
                color: Tokens.textOnPanelMuted
                font.pixelSize: Tokens.fontCaption
            }
        }

        Item { width: 1; height: Tokens.gapMedium }
        SectionTitle { text: "Dossiers de cartes (lus avant " + root.launcher.levelsRoot + ")" }

        Repeater {
            model: root.launcher.directories
            Row {
                id: directoryRow
                required property int index
                required property string modelData
                spacing: Tokens.gapSmall
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: directoryRow.modelData
                    color: Tokens.textOnPanel
                    font.pixelSize: Tokens.fontCaption
                }
                ToolButton {
                    text: "Retirer"
                    onClicked: root.launcher.removeDirectory(directoryRow.index)
                }
            }
        }

        Row {
            width: parent.width
            spacing: Tokens.gapSmall
            TextField {
                id: directoryField
                width: parent.width - addButton.width - parent.spacing
                placeholderText: "chemin d'un dossier de cartes (brouillons de l'editeur)"
                font.pixelSize: Tokens.fontCaption
                onAccepted: addButton.clicked()
            }
            ToolButton {
                id: addButton
                text: "Ajouter"
                onClicked: {
                    if (root.launcher.addDirectory(directoryField.text))
                        directoryField.clear()
                }
            }
        }
    }

    // --- La ligne d'etat ---------------------------------------------------------------------
    Rectangle {
        id: statusBar

        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: Tokens.gapLarge
        color: Tokens.panelRaised

        Text {
            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
            anchors.leftMargin: Tokens.gapMedium
            anchors.rightMargin: Tokens.gapMedium
            elide: Text.ElideRight
            text: root.launcher.status
            color: Tokens.textOnPanel
            font.pixelSize: Tokens.fontCaption
        }
    }
}
