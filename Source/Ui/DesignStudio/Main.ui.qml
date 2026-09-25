import QtQuick
import QtQuick.Layouts
import Jadg.Ui

/*!
    Galerie des briques de la charte v2 (LOT-87, T2.7).

    Chaque brique de `Controls/` posee au moins une fois, dans chacun de ses etats, sur la matiere
    qui la porte : les panneaux sombres a gauche, le parchemin a droite. C'est ce que Qt Design
    Studio ouvre en premier (`mainFile` du .qmlproject), et ce que le jeu affiche sous
    `--screen=Gallery` : la meme page, lue par les deux, est la preuve que les briques se resolvent
    dans l'atelier ET a l'execution.

    Dessinee a 1080p, la definition de conception. Tant qu'aucune image du cahier n'est livree,
    chaque brique montre son aplat de repli ; la galerie ne change pas le jour ou elles arrivent --
    ce sont les briques qui les posent.

    N'importe que Jadg.Ui : jamais Jadg.Runtime, et donc ouvrable sans rien construire. Les libelles
    ne sont pas traduits : c'est un outil de verification, pas un ecran du jeu.
*/
Rectangle {
    id: root

    width: 1920
    height: 1080
    color: Tokens.panel

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: sheet.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Row {
            id: sheet

            width: root.width
            height: Math.max(darkColumn.implicitHeight, parchmentColumn.implicitHeight) + 2 * Tokens.gapLarge

            // --- Panneaux sombres -------------------------------------------------------------
            Rectangle {
                width: root.width / 2
                height: sheet.height
                color: Tokens.panel

                ColumnLayout {
                    id: darkColumn

                    x: Tokens.gapLarge
                    y: Tokens.gapLarge
                    width: parent.width - 2 * Tokens.gapLarge
                    spacing: Tokens.gapMedium

                    TitlePlate {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Charte v2"
                        material: "garnet"
                    }

                    SectionBanner {
                        Layout.fillWidth: true
                        material: "dark"
                        text: "Panneaux"
                    }

                    RowLayout {
                        spacing: Tokens.gapMedium

                        PanelFrame {
                            Layout.preferredWidth: 400 * Tokens.uiScale
                            Layout.preferredHeight: 240 * Tokens.uiScale
                            material: "dark"

                            Text {
                                text: "PanelFrame"
                                color: Tokens.textOnPanel
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                            }
                        }

                        PanelFrame {
                            Layout.preferredWidth: 300 * Tokens.uiScale
                            Layout.preferredHeight: 240 * Tokens.uiScale
                            material: "dark"
                            subpanel: true

                            Text {
                                text: "subpanel"
                                color: Tokens.textOnPanelMuted
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                            }
                        }
                    }

                    SectionBanner {
                        Layout.fillWidth: true
                        material: "dark"
                        text: "Entrées de menu"
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: Tokens.gapMedium
                        rowSpacing: Tokens.gapSmall

                        // Deux entrees de 480 px ne tiennent pas cote a cote dans la demi-page : la
                        // galerie les resserre, la piece s'etire sur sa marge droite.
                        OrnateButton { Layout.preferredWidth: 420 * Tokens.uiScale; kind: "menu"; text: "Continuer"; forcedState: "active" }
                        OrnateButton { Layout.preferredWidth: 420 * Tokens.uiScale; kind: "menu"; text: "Nouvelle partie"; forcedState: "hover" }
                        OrnateButton { Layout.preferredWidth: 420 * Tokens.uiScale; kind: "menu"; text: "Options" }
                        OrnateButton { Layout.preferredWidth: 420 * Tokens.uiScale; kind: "menu"; text: "Charger une partie"; enabled: false }
                    }

                    SectionBanner {
                        Layout.fillWidth: true
                        material: "dark"
                        text: "Boutons"
                    }

                    GridLayout {
                        columns: 3
                        columnSpacing: Tokens.gapMedium
                        rowSpacing: Tokens.gapSmall

                        OrnateButton { kind: "primary"; text: "Primaire" }
                        OrnateButton { kind: "primary"; text: "Survol"; forcedState: "hover" }
                        OrnateButton { kind: "primary"; text: "Enfoncé"; forcedState: "pressed" }
                        OrnateButton { kind: "secondary"; text: "Secondaire" }
                        OrnateButton { kind: "primary"; text: "Désactivé"; enabled: false }
                        OrnateButton { kind: "back"; text: "Retour" }
                        OrnateButton { kind: "default"; text: "Par défaut" }
                        OrnateButton { kind: "cancel"; text: "Annuler" }
                        OrnateButton { kind: "apply"; text: "Appliquer" }
                    }

                    SectionBanner {
                        Layout.fillWidth: true
                        material: "dark"
                        text: "Onglets et réglages"
                    }

                    RowLayout {
                        spacing: Tokens.gapLarge

                        ColumnLayout {
                            Layout.alignment: Qt.AlignTop
                            spacing: Tokens.gapSmall

                            OrnateTab { material: "dark"; text: "Général"; forcedState: "active" }
                            OrnateTab { material: "dark"; text: "Graphismes" }
                            OrnateTab { material: "dark"; text: "Multijoueur"; enabled: false }
                        }

                        ColumnLayout {
                            Layout.alignment: Qt.AlignTop
                            spacing: Tokens.gapMedium

                            OrnateCheck { text: "Plein écran"; checked: true }
                            OrnateCheck { text: "Synchronisation verticale" }
                            OrnateCheck { text: "Non disponible"; enabled: false }
                            OrnateSlider { value: 0.7 }
                            OrnateCombo { model: ["Français", "English"] }
                            GoldDivider { Layout.fillWidth: true }
                        }
                    }

                    SectionBanner {
                        Layout.fillWidth: true
                        material: "dark"
                        text: "HUD"
                    }

                    RowLayout {
                        spacing: Tokens.gapLarge

                        PortraitFrame { shape: "hud"; level: "5" }
                        PortraitFrame { shape: "square"; size: 120 * Tokens.uiScale }
                        PortraitFrame { shape: "square"; size: 120 * Tokens.uiScale; active: true }

                        ColumnLayout {
                            spacing: Tokens.gapSmall

                            Gauge { kind: "health"; value: 0.7; label: "32 / 45" }
                            Gauge { kind: "experience"; value: 0.4 }
                        }
                    }

                    RowLayout {
                        spacing: Tokens.gapSmall

                        ActionSlot { label: "Arc long"; shortcut: "1"; active: true }
                        ActionSlot { label: "Potion"; quantity: "3"; shortcut: "2" }
                        ActionSlot { label: "Piège"; shortcut: "3"; enabled: false }

                        Item { Layout.preferredWidth: Tokens.gapLarge }

                        OrnateRoundButton { text: "Sac" }
                        OrnateRoundButton { text: "Carte"; forcedState: "hover" }
                        OrnateRoundButton { text: "Options"; forcedState: "pressed" }
                        OrnateRoundButton { text: "Quêtes"; enabled: false }
                    }

                    SectionBanner {
                        Layout.fillWidth: true
                        material: "dark"
                        text: "Menu"
                    }

                    RowLayout {
                        spacing: Tokens.gapLarge

                        LogoPlate { Layout.preferredWidth: 320 * Tokens.uiScale }

                        ColumnLayout {
                            spacing: Tokens.gapMedium

                            Row {
                                spacing: Tokens.gapSmall

                                FocusMark { anchors.verticalCenter: parent.verticalCenter }
                                FocusMark { anchors.verticalCenter: parent.verticalCenter; size: 40 * Tokens.uiScale }
                            }

                            QuotePlate { text: "« Chaque choix façonne une nouvelle histoire. »" }

                            CreditSection {
                                Layout.preferredWidth: 420 * Tokens.uiScale
                                title: "Section"
                                iconKey: "ui/icon/credits-section/development"
                                lines: [{ role: "Rôle", names: "Nom\nNom" }]
                            }

                            // Un fond couvrant, cadre dans une vignette : invisible tant que la scene
                            // n'est pas livree -- la vignette montre alors le panneau dessous.
                            Rectangle {
                                Layout.preferredWidth: 320 * Tokens.uiScale
                                Layout.preferredHeight: 180 * Tokens.uiScale
                                color: Tokens.panelRaised
                                border.color: Tokens.panelEdge
                                border.width: Tokens.strokeWidth

                                CoverArt {
                                    anchors.fill: parent
                                    key: "ui/background/menu-scene"
                                }
                            }
                        }
                    }
                }
            }

            // --- Parchemin --------------------------------------------------------------------
            Rectangle {
                width: root.width / 2
                height: sheet.height
                color: Tokens.background

                ColumnLayout {
                    id: parchmentColumn

                    x: Tokens.gapLarge
                    y: Tokens.gapLarge
                    width: parent.width - 2 * Tokens.gapLarge
                    spacing: Tokens.gapMedium

                    TitlePlate {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Fiche"
                        material: "black"
                    }

                    SectionBanner {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Panneaux"
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: Tokens.gapMedium
                        rowSpacing: Tokens.gapMedium

                        PanelFrame {
                            Layout.preferredWidth: 400 * Tokens.uiScale
                            Layout.preferredHeight: 260 * Tokens.uiScale
                            material: "parchment"

                            Text {
                                text: "PanelFrame"
                                color: Tokens.text
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                            }
                        }

                        PanelFrame {
                            Layout.preferredWidth: 400 * Tokens.uiScale
                            Layout.preferredHeight: 260 * Tokens.uiScale
                            material: "parchment"
                            bound: true
                            padding: 2 * Tokens.gapLarge

                            Text {
                                text: "bound"
                                color: Tokens.text
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                            }
                        }

                        PanelFrame {
                            Layout.preferredWidth: 400 * Tokens.uiScale
                            Layout.preferredHeight: 120 * Tokens.uiScale
                            material: "parchment"
                            subpanel: true

                            Text {
                                text: "subpanel"
                                color: Tokens.text
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                            }
                        }

                        PanelFrame {
                            Layout.preferredWidth: 400 * Tokens.uiScale
                            Layout.preferredHeight: 120 * Tokens.uiScale
                            material: "parchment"
                            subpanel: true
                            empty: true

                            Text {
                                text: "empty"
                                color: Tokens.textMuted
                                font.family: Tokens.loreFamily
                                font.italic: true
                                font.pixelSize: Tokens.fontBody
                            }
                        }
                    }

                    SectionBanner {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Médaillons"
                    }

                    RowLayout {
                        spacing: Tokens.gapMedium

                        PortraitFrame { shape: "round"; size: 160 * Tokens.uiScale }
                        StatMedallion { label: "FOR"; value: "16"; modifier: "+3" }
                        StatMedallion { label: "DEX"; value: "14"; modifier: "+2" }
                        StatMedallion { kind: "derived"; label: "CA"; value: "15" }
                    }

                    ColumnLayout {
                        spacing: Tokens.gapSmall

                        FieldRow { Layout.preferredWidth: 560 * Tokens.uiScale; label: "Nom"; value: "Brenna" }
                        SkillRow { Layout.preferredWidth: 480 * Tokens.uiScale; skillId: "athletics"; label: "Athlétisme"; value: "+5"; proficient: true }
                        SkillRow { Layout.preferredWidth: 480 * Tokens.uiScale; skillId: "stealth"; label: "Discrétion"; value: "+1" }
                    }

                    SectionBanner {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Filtres"
                    }

                    RowLayout {
                        spacing: 0

                        OrnateTab { Layout.preferredWidth: 280 * Tokens.uiScale; material: "parchment"; text: "Armes"; forcedState: "active" }
                        OrnateTab { Layout.preferredWidth: 280 * Tokens.uiScale; material: "parchment"; text: "Armures" }
                        OrnateTab { Layout.preferredWidth: 280 * Tokens.uiScale; material: "parchment"; text: "Divers"; enabled: false }
                    }

                    SectionBanner {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Emplacements"
                    }

                    RowLayout {
                        spacing: Tokens.gapSmall

                        ItemSlot { }
                        ItemSlot { forcedState: "hover" }
                        ItemSlot { selected: true; quantity: 3 }
                        ItemSlot { equipped: true; rarity: "rare" }
                        ItemSlot { rarity: "legendary"; quantity: 12 }
                    }

                    SectionBanner {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Jauges"
                    }

                    Gauge {
                        Layout.preferredWidth: 480 * Tokens.uiScale
                        kind: "weight"
                        value: 0.85
                        label: "68 / 80"
                    }
                }
            }
        }
    }
}
