import QtQuick
import QtQuick.Layouts
import Jadg.Ui

/*!
    Options -- FORMULAIRE, cote conception (LOT-87, T3.2 ; maquette 05).

    Transcrit de la maquette `05_Options_Mockup.png` (1536 x 1024), cotes ramenees a 1920 x 1080 puis
    multipliees par `Tokens.uiScale` : une colonne d'onglets a gauche sous la plaque de titre, un
    grand panneau sombre a droite, ses sections, et une barre de pied Par defaut, Annuler, Appliquer.

    **Trois onglets, et non six.** `EX-IHM-072` retire tout reglage inoperant, et la decision du
    14 septembre 2026 l'a garde : seuls les onglets qui portent un reglage BRANCHE sont dessines --
    General (langue, journaux), Graphismes (plein ecran, synchronisation verticale, compteur de
    diagnostic), Audio (volume). Jeu, Commandes et Accessibilite reviendront avec leurs reglages.

    **Rien ne s'ecrit avant « Appliquer ».** Les controles tiennent les valeurs en attente ; le
    jumeau les ecrit dans `OptionsModel` a « Appliquer », les abandonne a « Annuler » (qui ferme
    l'ecran), et « Par defaut » y pose les valeurs d'usine sans les ecrire. `dirty`, calcule par le
    jumeau, allume « Appliquer » : un bouton qui n'aurait rien a appliquer serait inoperant.

    Les controles sont exposes par `property alias` : c'est le jumeau qui leur attache une reaction,
    un formulaire ne pouvant pas contenir de code.
*/
Item {
    id: root

    /// L'onglet affiche : 0 General, 1 Graphismes, 2 Audio. Le jumeau le tient.
    property int currentTab: 0

    /// Vrai si une valeur en attente differe du reglage enregistre.
    property bool dirty: false

    property string logsMessage: ""
    property bool logsEnabled: true

    property alias generalTab: generalTabControl
    property alias graphicsTab: graphicsTabControl
    property alias audioTab: audioTabControl

    property alias fullscreenCheck: fullscreenControl
    property alias vsyncCheck: vsyncControl
    property alias diagnosticsCheck: diagnosticsControl
    property alias volumeSlider: volumeControl
    property alias hudScaleSlider: hudScaleControl
    property alias languageBox: languageControl
    property alias saveLogsButton: saveLogsControl

    property alias defaultButton: defaultControl
    property alias cancelButton: cancelControl
    property alias applyButton: applyControl

    width: 1920
    height: 1080

    // --- Le fond : un decor voile, qui ne dispute rien aux reglages -------------------------------
    Rectangle {
        anchors.fill: parent
        color: Tokens.panel
    }

    // Tant que le decor n'est pas livre, l'aplat `panel` ci-dessus en tient lieu : plus aucune
    // image du corpus ne sert de repli (LOT-94, EX-IHM-076).
    CoverArt {
        id: backdropArt

        anchors.fill: parent
        key: "ui/background/options-backdrop"
    }

    // --- Colonne des onglets (maquette : 100, 70 -> 420, 890) ---------------------------------------
    PanelFrame {
        id: rail

        x: 125 * Tokens.uiScale
        y: 150 * Tokens.uiScale
        width: 420 * Tokens.uiScale
        height: 790 * Tokens.uiScale
        padding: 0

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 90 * Tokens.uiScale
            spacing: Tokens.gapSmall

            OrnateButton {
                id: generalTabControl
                width: 372 * Tokens.uiScale
                kind: "menu"
                iconKey: "ui/icon/options-tab/general"
                text: qsTr("Général")
                highlighted: root.currentTab === 0
                focusPolicy: Qt.NoFocus
            }

            OrnateButton {
                id: graphicsTabControl
                width: 372 * Tokens.uiScale
                kind: "menu"
                iconKey: "ui/icon/options-tab/graphics"
                text: qsTr("Graphismes")
                highlighted: root.currentTab === 1
                focusPolicy: Qt.NoFocus
            }

            OrnateButton {
                id: audioTabControl
                width: 372 * Tokens.uiScale
                kind: "menu"
                iconKey: "ui/icon/options-tab/audio"
                text: qsTr("Audio")
                highlighted: root.currentTab === 2
                focusPolicy: Qt.NoFocus
            }
        }

        // La rose des vents en filigrane, au pied de la colonne (maquette : 150, 640 -> 370, 860).
        FixedArt {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Tokens.gapLarge
            width: 260 * Tokens.uiScale
            height: 260 * Tokens.uiScale
            key: "ui/ornament/compass-watermark/dark"
        }
    }

    // La plaque de titre deborde la colonne, comme sur la maquette (95, 65 -> 450, 160).
    TitlePlate {
        x: 119 * Tokens.uiScale
        y: 74 * Tokens.uiScale
        width: 440 * Tokens.uiScale
        material: "garnet"
        text: qsTr("Options")
    }

    // --- Panneau des reglages (maquette : 430, 115 -> 1425, 935) -----------------------------------
    PanelFrame {
        id: settingsPanel

        x: 538 * Tokens.uiScale
        y: 121 * Tokens.uiScale
        width: 1243 * Tokens.uiScale
        height: 865 * Tokens.uiScale

        StackLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: footer.top
            anchors.bottomMargin: Tokens.gapLarge
            currentIndex: root.currentTab

            // --- General ------------------------------------------------------------------------
            RowLayout {
                spacing: Tokens.gapLarge

                PanelFrame {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.preferredHeight: 224 * Tokens.uiScale
                    subpanel: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Tokens.gapMedium

                        SectionBanner {
                            Layout.fillWidth: true
                            material: "dark"
                            text: qsTr("Langue")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Tokens.gapMedium

                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Langue du jeu")
                                color: Tokens.textOnPanel
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                            }

                            OrnateCombo {
                                id: languageControl
                                Layout.preferredWidth: 240 * Tokens.uiScale
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                PanelFrame {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.preferredHeight: 224 * Tokens.uiScale
                    subpanel: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Tokens.gapMedium

                        SectionBanner {
                            Layout.fillWidth: true
                            material: "dark"
                            text: qsTr("Journaux")
                        }

                        OrnateButton {
                            id: saveLogsControl
                            Layout.fillWidth: true
                            kind: "secondary"
                            enabled: root.logsEnabled
                            text: qsTr("Enregistrer les journaux de session")
                        }

                        Text {
                            Layout.fillWidth: true
                            text: root.logsMessage
                            color: Tokens.textOnPanelMuted
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontCaption
                            wrapMode: Text.WrapAnywhere
                            visible: root.logsMessage.length > 0
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // --- Graphismes ---------------------------------------------------------------------
            RowLayout {
                spacing: Tokens.gapLarge

                PanelFrame {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.preferredHeight: 300 * Tokens.uiScale
                    subpanel: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Tokens.gapMedium

                        SectionBanner {
                            Layout.fillWidth: true
                            material: "dark"
                            text: qsTr("Affichage")
                        }

                        OrnateCheck {
                            id: fullscreenControl
                            text: qsTr("Plein écran")
                        }

                        OrnateCheck {
                            id: vsyncControl
                            text: qsTr("Synchronisation verticale")
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.leftMargin: 44 * Tokens.uiScale
                            text: qsTr("Appliquée au prochain lancement.")
                            color: Tokens.textOnPanelMuted
                            font.family: Tokens.loreFamily
                            font.italic: true
                            font.pixelSize: Tokens.fontCaption
                        }

                        OrnateCheck {
                            id: diagnosticsControl
                            text: qsTr("Compteur de diagnostic")
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                PanelFrame {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.preferredHeight: 350 * Tokens.uiScale
                    subpanel: true
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Tokens.gapMedium
                        SectionBanner {
                            Layout.fillWidth: true
                            material: "dark"
                            text: qsTr("Interface en jeu")
                        }
                        Text {
                            text: qsTr("Taille du HUD")
                            color: Tokens.textOnPanel
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontSectionTitle
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            OrnateSlider {
                                id: hudScaleControl
                                objectName: "hudScaleSlider"
                                Layout.fillWidth: true
                                from: 75
                                to: 130
                                stepSize: 5
                                value: 100
                                Accessible.name: qsTr("Taille du HUD")
                            }
                            Text {
                                Layout.preferredWidth: 76 * Tokens.uiScale
                                text: Math.round(hudScaleControl.value) + " %"
                                color: Tokens.textOnPanel
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontSectionTitle
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Ajuste les portraits, commandes et informations en exploration et en combat. La taille du terrain et des menus reste inchangée.\nPrend effet avec Appliquer et reste enregistrée.")
                            color: Tokens.textOnPanel
                            font.family: Tokens.bodyFamily
                            font.pixelSize: Tokens.fontBody
                            wrapMode: Text.WordWrap
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // --- Audio --------------------------------------------------------------------------
            RowLayout {
                spacing: Tokens.gapLarge

                PanelFrame {
                    Layout.alignment: Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.preferredHeight: 224 * Tokens.uiScale
                    subpanel: true

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Tokens.gapMedium

                        SectionBanner {
                            Layout.fillWidth: true
                            material: "dark"
                            text: qsTr("Volume")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Tokens.gapMedium

                            Text {
                                text: qsTr("Volume général")
                                color: Tokens.textOnPanel
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                            }

                            OrnateSlider {
                                id: volumeControl
                                Layout.fillWidth: true
                                from: 0
                                to: 100
                                stepSize: 1
                            }

                            Text {
                                Layout.preferredWidth: 64 * Tokens.uiScale
                                horizontalAlignment: Text.AlignRight
                                text: Math.round(volumeControl.value) + " %"
                                color: Tokens.textOnPanel
                                font.family: Tokens.bodyFamily
                                font.pixelSize: Tokens.fontBody
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    // Meme largeur implicite qu'un panneau : la colonne vide partage la ligne a parts egales.
                    implicitWidth: 480 * Tokens.uiScale
                }
            }
        }

        // --- Barre de pied (maquette : 465, 865 -> 1400, 925) ------------------------------------
        Item {
            id: footer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 56 * Tokens.uiScale

            GoldDivider {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.top
                anchors.bottomMargin: Tokens.gapMedium
            }

            OrnateButton {
                id: defaultControl
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                kind: "default"
                text: qsTr("Par défaut")
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: Tokens.gapMedium

                OrnateButton {
                    id: cancelControl
                    kind: "cancel"
                    text: qsTr("Annuler")
                }

                OrnateButton {
                    id: applyControl
                    kind: "apply"
                    enabled: root.dirty
                    text: qsTr("Appliquer")
                }
            }
        }
    }
}
