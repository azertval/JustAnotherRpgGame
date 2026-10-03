pragma ComponentBehavior: Bound

import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    La galerie des assets -- un OUTIL DE DEBUG, pas un ecran du jeu (`--screen=AssetGallery`).

    Tous les assets livres, disposes en blocs (l'emprise plus une case de marge), toutes leurs
    formes et toutes leurs animations a la fois. Seuls les blocs a l'ecran sont dessines ; ceux de
    l'anneau d'un bloc autour sont precharges ; le reste est libere (`AssetGalleryViewport`).

    Il vit dans Tools/ et non Screens/ : sans formulaire dans Source/Ui, l'atelier n'a rien a y dessiner, et ce qui s'y voit
    n'est que le relais des proprietes de la surface.

    | Geste | Souris | Clavier |
    |---|---|---|
    | Se deplacer | clic maintenu et glisser | fleches |
    | Inspecter un bloc | clic | -- |
    | Zoomer | molette | + et - |
    | Lecture, pause | -- | Espace |
    | Image precedente, suivante | -- | , et . |
    | Aller a une famille | clic dans la liste | -- |
    | Centrer ailleurs | clic sur la minicarte | -- |
*/
Rectangle {
    id: root

    color: Tokens.panel
    focus: true

    Keys.onSpacePressed: gallery.playing = !gallery.playing
    Keys.onLeftPressed: gallery.panBy(gallery.cellSize, 0)
    Keys.onRightPressed: gallery.panBy(-gallery.cellSize, 0)
    Keys.onUpPressed: gallery.panBy(0, gallery.cellSize)
    Keys.onDownPressed: gallery.panBy(0, -gallery.cellSize)
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Plus) {
            gallery.zoom = gallery.zoom * 2
        } else if (event.key === Qt.Key_Minus) {
            gallery.zoom = gallery.zoom / 2
        } else if (event.key === Qt.Key_Comma) {
            gallery.step(-1)
        } else if (event.key === Qt.Key_Period) {
            gallery.step(1)
        } else {
            return
        }
        event.accepted = true
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

    /// Un intitule de section, dans les colonnes laterales.
    component SectionTitle: Text {
        color: Tokens.textOnPanelMuted
        font.pixelSize: Tokens.fontCaption
        font.bold: true
        font.capitalization: Font.AllUppercase
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
                text: "Galerie des assets"
                color: Tokens.textOnPanel
                font.pixelSize: Tokens.fontBody
                font.bold: true
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "--screen=AssetGallery"
                color: Tokens.textOnPanelMuted
                font.pixelSize: Tokens.fontCaption
            }
        }

        Row {
            anchors { right: parent.right; rightMargin: Tokens.gapMedium; verticalCenter: parent.verticalCenter }
            spacing: Tokens.gapSmall

            Repeater {
                model: [0.5, 1, 2, 3, 4]
                ToolButton {
                    required property real modelData
                    text: "×" + String(modelData).replace(".", ",")
                    active: gallery.zoom === modelData
                    onClicked: gallery.zoom = modelData
                }
            }
            Item { width: Tokens.gapMedium; height: 1 }
            ToolButton {
                text: "Cases"
                active: gallery.showGrid
                onClicked: gallery.showGrid = !gallery.showGrid
            }
            ToolButton {
                text: "Emprise"
                active: gallery.showFootprint
                onClicked: gallery.showFootprint = !gallery.showFootprint
            }
            Item { width: Tokens.gapMedium; height: 1 }
            ToolButton {
                text: gallery.playing ? "Pause" : "Lecture"
                onClicked: gallery.playing = !gallery.playing
            }
            Repeater {
                model: [0.25, 1, 2]
                ToolButton {
                    required property real modelData
                    text: modelData === 0.25 ? "×¼" : "×" + modelData
                    active: gallery.speed === modelData
                    onClicked: gallery.speed = modelData
                }
            }
        }
    }

    // --- Les familles --------------------------------------------------------------------------
    Rectangle {
        id: families

        anchors { left: parent.left; top: toolbar.bottom; bottom: statusBar.top }
        width: Tokens.gapLarge * 8
        color: Tokens.panelRaised

        Column {
            anchors { fill: parent; margins: Tokens.gapMedium }
            spacing: Tokens.gapSmall

            SectionTitle { text: "Familles" }

            Repeater {
                model: gallery.bands
                Rectangle {
                    id: bandRow
                    required property var modelData
                    required property int index
                    width: parent.width
                    height: bandText.implicitHeight + Tokens.gapSmall * 2
                    radius: Tokens.strokeWidth * 2
                    color: bandArea.containsMouse ? Tokens.panel : "transparent"

                    Column {
                        id: bandText
                        anchors { left: parent.left; right: parent.right; margins: Tokens.gapSmall; verticalCenter: parent.verticalCenter }
                        Text {
                            text: bandRow.modelData.title
                            color: Tokens.textOnPanel
                            font.pixelSize: Tokens.fontBody
                        }
                        Text {
                            text: bandRow.modelData.directory + " · " + bandRow.modelData.count
                            color: Tokens.textOnPanelMuted
                            font.pixelSize: Tokens.fontCaption
                        }
                    }

                    MouseArea {
                        id: bandArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: gallery.showBand(bandRow.index)
                    }
                }
            }
        }
    }

    // --- La vue ----------------------------------------------------------------------------------
    Item {
        id: viewport

        anchors { left: families.right; right: inspector.left; top: toolbar.bottom; bottom: statusBar.top }
        clip: true

        AssetGalleryViewport {
            id: gallery
            anchors.fill: parent
            clearColor: Tokens.panel
            // L'anticrenelage des options : un asset se juge ici comme il parait en jeu.
            sampleCount: OptionsModel.antialiasing
        }

        Repeater {
            model: gallery.bands
            Text {
                required property var modelData
                x: gallery.offsetX + Tokens.gapSmall
                y: gallery.offsetY + modelData.row * gallery.cellSize + (gallery.cellSize - height) / 2
                visible: y > -height && y < viewport.height
                text: modelData.title + "   " + modelData.directory
                color: Tokens.goldLight
                font.pixelSize: Tokens.fontSectionTitle
            }
        }

        Repeater {
            model: gallery.zoom >= 1 ? gallery.labels : []
            Column {
                id: label
                required property var modelData
                x: modelData.x + Tokens.gapSmall
                y: modelData.y + modelData.height - height - Tokens.gapSmall / 2
                width: modelData.width - Tokens.gapMedium
                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: label.modelData.title
                    color: Tokens.textOnPanel
                    font.pixelSize: Tokens.fontCaption
                }
                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: label.modelData.meta
                    color: Tokens.textOnPanelMuted
                    font.pixelSize: Tokens.fontCaption
                }
            }
        }

        MouseArea {
            id: pointer

            property real lastX: 0
            property real lastY: 0
            property bool moved: false

            anchors.fill: parent
            cursorShape: pressed && moved ? Qt.ClosedHandCursor : Qt.OpenHandCursor

            onPressed: (mouse) => {
                lastX = mouse.x
                lastY = mouse.y
                moved = false
                root.forceActiveFocus()
            }
            onPositionChanged: (mouse) => {
                const dx = mouse.x - lastX
                const dy = mouse.y - lastY
                if (!moved && Math.abs(dx) + Math.abs(dy) < Tokens.strokeWidth * 2)
                    return
                moved = true
                gallery.panBy(dx, dy)
                lastX = mouse.x
                lastY = mouse.y
            }
            onReleased: (mouse) => {
                if (!moved)
                    gallery.select(gallery.blocAt(mouse.x, mouse.y))
            }
            onWheel: (wheel) => {
                gallery.zoomAt(wheel.angleDelta.y > 0 ? gallery.zoom * 2 : gallery.zoom / 2,
                               wheel.x, wheel.y)
            }
        }

        // La minicarte : toute la galerie, l'etat de chaque bloc et le cadre de la vue.
        Rectangle {
            id: minimap

            readonly property real cellScale: Math.min((Tokens.gapLarge * 5) / Math.max(1, gallery.layoutColumns),
                                                   (viewport.height / 3) / Math.max(1, gallery.layoutRows))

            anchors { right: parent.right; bottom: parent.bottom; margins: Tokens.gapMedium }
            width: gallery.layoutColumns * cellScale + Tokens.gapSmall
            height: gallery.layoutRows * cellScale + Tokens.gapSmall
            color: Tokens.panelRaised
            border.width: 1
            border.color: Tokens.textOnPanelMuted

            function stateColor(bloc) {
                const ring = gallery.ringCells
                const overlaps = (margin) => bloc.column < gallery.viewColumn + gallery.viewColumns + margin
                        && bloc.column + bloc.columns > gallery.viewColumn - margin
                        && bloc.row < gallery.viewRow + gallery.viewRows + margin
                        && bloc.row + bloc.rows > gallery.viewRow - margin
                if (overlaps(0))
                    return Tokens.panelEdge
                return overlaps(ring) ? Tokens.textAlly : Tokens.textOnPanelMuted
            }

            Item {
                anchors { fill: parent; margins: Tokens.gapSmall / 2 }

                Repeater {
                    model: gallery.minimap
                    Rectangle {
                        required property var modelData
                        x: modelData.column * minimap.cellScale
                        y: modelData.row * minimap.cellScale
                        width: Math.max(1, modelData.columns * minimap.cellScale - 1)
                        height: Math.max(1, modelData.rows * minimap.cellScale - 1)
                        color: minimap.stateColor(modelData)
                    }
                }

                Rectangle {
                    x: gallery.viewColumn * minimap.cellScale
                    y: gallery.viewRow * minimap.cellScale
                    width: gallery.viewColumns * minimap.cellScale
                    height: gallery.viewRows * minimap.cellScale
                    color: "transparent"
                    border.width: Tokens.strokeWidth
                    border.color: Tokens.goldLight
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: (mouse) => gallery.centerOnCell(mouse.x / minimap.cellScale, mouse.y / minimap.cellScale)
                    onPositionChanged: (mouse) => gallery.centerOnCell(mouse.x / minimap.cellScale, mouse.y / minimap.cellScale)
                }
            }
        }
    }

    // --- L'inspecteur --------------------------------------------------------------------------
    Rectangle {
        id: inspector

        readonly property var entry: gallery.selected

        anchors { right: parent.right; top: toolbar.bottom; bottom: statusBar.top }
        width: Tokens.gapLarge * 10
        color: Tokens.panelRaised

        Column {
            anchors { fill: parent; margins: Tokens.gapMedium }
            spacing: Tokens.gapMedium
            visible: inspector.entry.path !== undefined

            Column {
                width: parent.width
                SectionTitle { text: "Inspecteur" }
                Text {
                    text: inspector.entry.model || ""
                    color: Tokens.textOnPanel
                    font.pixelSize: Tokens.fontSectionTitle
                }
                Text {
                    text: inspector.entry.form || ""
                    color: Tokens.goldLight
                    font.pixelSize: Tokens.fontBody
                }
                Text {
                    width: parent.width
                    wrapMode: Text.WrapAnywhere
                    text: inspector.entry.path || ""
                    color: Tokens.textOnPanelMuted
                    font.pixelSize: Tokens.fontCaption
                }
            }

            Grid {
                columns: 2
                columnSpacing: Tokens.gapMedium
                rowSpacing: Tokens.gapSmall / 2

                Repeater {
                    // Libelle, valeur, libelle, valeur... : la grille range deux par ligne.
                    model: [
                        "Famille", inspector.entry.family,
                        "Image", inspector.entry.frameSize,
                        "Images", inspector.entry.frameCount,
                        "Durée", inspector.entry.duration,
                        "Boucle", inspector.entry.loop,
                        "Emprise", inspector.entry.footprint,
                        "Ancre", inspector.entry.anchor,
                        "Bloc", inspector.entry.bloc,
                        "État", inspector.entry.state
                    ]
                    Text {
                        required property var modelData
                        required property int index
                        text: modelData === undefined ? "" : String(modelData)
                        color: index % 2 === 0 ? Tokens.textOnPanelMuted : Tokens.textOnPanel
                        font.pixelSize: Tokens.fontCaption
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Tokens.gapSmall

                SectionTitle { text: "Images" }
                Flow {
                    width: parent.width
                    spacing: Tokens.gapSmall / 2
                    Repeater {
                        model: inspector.entry.frameCount || 0
                        Rectangle {
                            id: frameCell
                            required property int index
                            width: Tokens.gapLarge * 0.8
                            height: Tokens.gapLarge
                            radius: Tokens.strokeWidth
                            color: frameCell.index === gallery.selectedFrame ? Tokens.panelEdge : Tokens.panel
                            Text {
                                anchors.centerIn: parent
                                text: frameCell.index
                                color: frameCell.index === gallery.selectedFrame ? Tokens.panel : Tokens.textOnPanelMuted
                                font.pixelSize: Tokens.fontCaption
                            }
                        }
                    }
                }
                Row {
                    spacing: Tokens.gapSmall
                    ToolButton { text: "‹"; onClicked: gallery.step(-1) }
                    ToolButton {
                        text: gallery.playing ? "Pause" : "Lecture"
                        onClicked: gallery.playing = !gallery.playing
                    }
                    ToolButton { text: "›"; onClicked: gallery.step(1) }
                }
            }

            Column {
                width: parent.width
                spacing: Tokens.gapSmall

                SectionTitle { text: "Formes du modèle" }
                Flow {
                    width: parent.width
                    spacing: Tokens.gapSmall / 2
                    Repeater {
                        model: gallery.siblings
                        ToolButton {
                            required property var modelData
                            text: modelData.form
                            active: modelData.index === gallery.selectedIndex
                            onClicked: {
                                gallery.select(modelData.index)
                                gallery.centerOn(modelData.index)
                            }
                        }
                    }
                }
            }
        }
    }

    // --- La barre d'etat -----------------------------------------------------------------------
    Rectangle {
        id: statusBar

        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: Tokens.gapLarge * 1.1
        color: Tokens.panelRaised

        Row {
            anchors { left: parent.left; leftMargin: Tokens.gapMedium; verticalCenter: parent.verticalCenter }
            spacing: Tokens.gapMedium

            Text {
                text: "%1 dessinés".arg(gallery.drawnCount)
                color: Tokens.panelEdge
                font.pixelSize: Tokens.fontCaption
            }
            Text {
                text: "%1 préchargés".arg(gallery.preloadedCount)
                color: Tokens.textAlly
                font.pixelSize: Tokens.fontCaption
            }
            Text {
                text: "%1 déchargés".arg(gallery.unloadedCount)
                color: Tokens.textOnPanelMuted
                font.pixelSize: Tokens.fontCaption
            }
            Text {
                text: "%1 formes au total".arg(gallery.blocCount)
                color: Tokens.textOnPanel
                font.pixelSize: Tokens.fontCaption
            }
            Text {
                visible: gallery.errors.length > 0
                text: "%1 problème(s) : %2".arg(gallery.errors.length).arg(gallery.errors.join(" ; "))
                color: Tokens.textEnemy
                font.pixelSize: Tokens.fontCaption
            }
        }

        Text {
            anchors { right: parent.right; rightMargin: Tokens.gapMedium; verticalCenter: parent.verticalCenter }
            text: "Clic maintenu + glisser : se déplacer · clic : inspecter · molette : zoom"
            color: Tokens.textOnPanelMuted
            font.pixelSize: Tokens.fontCaption
        }
    }
}
