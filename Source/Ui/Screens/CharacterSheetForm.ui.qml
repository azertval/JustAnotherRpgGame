import QtQuick
import QtQuick.Layouts
import Jadg.Ui

/*!
    Fiche de personnage -- FORMULAIRE, cote conception (LOT-87, T3.4 ; maquette 03 ; LOT-141).

    Transcrit de la maquette `03_Character_Sheet_Mockup.png` (1536 x 1024), cotes ramenees a
    1920 x 1080 puis multipliees par `Tokens.uiScale`. Un grand parchemin relie, en trois colonnes :

    - a gauche, le portrait rond entoure des six medaillons de caracteristiques, et sous eux les
      constantes de combat en medaillons derives (sur la rose des vents en filigrane) ;
    - au centre, l'identite du personnage : nom, classe, niveau, origine, espece, matricule, les
      jauges de vie et d'experience, le sceau, l'ecusson et le paraphe ;
    - a droite, trois onglets (LOT-141) : les dix-huit competences, pastille de maitrise, icone,
      nom et modificateur ; la CLASSE, capacites acquises avec le niveau qui les donne et celles a
      venir ; les SORTS connus et leurs lancers restants.

    Les proprietes portent des VALEURS D'EXEMPLE, pour que la mise en page se juge dans l'atelier ;
    le jumeau les remplace par celles de la vue-modele.
*/
Item {
    id: root

    // --- Identite ---------------------------------------------------------------------------------
    property string characterName: "Brenna Pierrefonte"
    property string className: "Guerrière"
    property string level: "3"
    property string background: "Artisane de guilde"
    property string species: "Naine des collines"
    property string registration: "—"
    property url portrait: ""

    // --- Jauges : le texte, et le remplissage de 0 a 1 --------------------------------------------
    property string hitPointsText: "25 / 30"
    property real hitPointsRatio: 25 / 30
    property string experienceText: "900 / —"
    property real experienceRatio: 0

    // --- Caracteristiques : score et modificateur, separes ----------------------------------------
    property string strengthScore: "16"
    property string strengthModifier: "+3"
    property string dexterityScore: "12"
    property string dexterityModifier: "+1"
    property string constitutionScore: "15"
    property string constitutionModifier: "+2"
    property string intelligenceScore: "10"
    property string intelligenceModifier: "+0"
    property string wisdomScore: "13"
    property string wisdomModifier: "+1"
    property string charismaScore: "8"
    property string charismaModifier: "-1"

    // --- Constantes de combat ---------------------------------------------------------------------
    property string armorClass: "16"
    property string initiative: "+1"
    property string speed: "9 m"
    property string proficiencyBonus: "+2"
    property string passivePerception: "11"

    /// Les competences : un modele aux roles `rowId`, `label`, `value` (`+5 •` si maitrisee).
    property var skills: exampleSkills

    // Un `ListModel` et non un tableau : il a la forme exacte du vrai modele (des ROLES), sans quoi
    // un ecran valide sur l'exemple s'afficherait vide une fois branche.
    /// Le bouton qui ouvre les competences et sorts (LOT-87, T3.8), branche par le jumeau.
    property alias skillsButton: skillsControl

    // --- Les onglets de la colonne droite (LOT-141) -------------------------------------------------
    /// L'onglet courant : 0 competences, 1 classe, 2 sorts.
    property int currentTab: 0
    property alias skillsTab: skillsTabControl
    property alias classTab: classTabControl
    property alias spellsTab: spellsTabControl
    /// Les listes de la classe et des sorts, pour que le jumeau les fasse defiler au clavier.
    property alias classList: classView
    property alias spellList: spellView
    /// Les capacites, acquises puis a venir : `label` le nom, `value` la cle de l'icone, `detail`
    /// la regle, `levelText` (« Niveau 3 »), `upcoming`.
    property var classRows: exampleClassRows
    /// Les sorts connus : `label` le nom, `value` l'icone, `detail` (portee, duree, des),
    /// `uses` (« 1 / 2 », « a volonte »), `levelText`.
    property var spellRows: exampleSpellRows

    readonly property ListModel exampleClassRows: ListModel {
        ListElement { rowId: "tough"; label: "Tough as Nails"; value: "ui/icon/capacity/tough-as-nails"; detail: "Player's Guide to Tanares, p. 193. Résistance à tous les dégâts."; levelText: "Niveau 1"; upcoming: false }
        ListElement { rowId: "mark"; label: "Hit the Mark"; value: "ui/icon/capacity/hit-the-mark"; detail: "Player's Guide to Tanares, p. 193. +2 aux jets d'attaque."; levelText: "À venir · niveau 3"; upcoming: true }
        ListElement { rowId: "extra"; label: "Extra Attack"; value: "ui/icon/capacity/extra-attack"; detail: "Player's Guide to Tanares, p. 193. Deux attaques par action."; levelText: "À venir · niveau 5"; upcoming: true }
    }

    readonly property ListModel exampleSpellRows: ListModel {
        ListElement { rowId: "fire-bolt"; label: "Trait de feu"; value: "ui/icon/spell/fire-bolt"; detail: "Portée : 36 mètres, dégâts : 1d10 feu"; uses: "à volonté"; levelText: "Sort mineur" }
        ListElement { rowId: "magic-missile"; label: "Projectile magique"; value: "ui/icon/spell/magic-missile"; detail: "Portée : 36 mètres, dégâts : 1d4+1 force"; uses: "1 / 2"; levelText: "Niveau 1" }
    }

    readonly property ListModel exampleSkills: ListModel {
        ListElement { rowId: "athletics"; label: "Athlétisme"; value: "+5 •" }
        ListElement { rowId: "stealth"; label: "Discrétion"; value: "+1" }
        ListElement { rowId: "perception"; label: "Perception"; value: "+3 •" }
        ListElement { rowId: "insight"; label: "Intuition"; value: "+1" }
    }

    width: 1920
    height: 1080

    Rectangle {
        anchors.fill: parent
        color: Tokens.frameEdge
    }

    // --- Le parchemin relie (maquette : 20, 15 -> 1515, 1010) --------------------------------------
    PanelFrame {
        id: sheet

        x: 24 * Tokens.uiScale
        y: 16 * Tokens.uiScale
        width: 1872 * Tokens.uiScale
        height: 1048 * Tokens.uiScale
        material: "parchment"
        bound: true
        padding: 0

        // Les deux separations verticales entre colonnes (maquette : x 550 et 1090).
        Rectangle {
            x: (688 - 24) * Tokens.uiScale
            y: 40 * Tokens.uiScale
            width: Tokens.strokeWidth
            height: parent.height - 80 * Tokens.uiScale
            color: Tokens.border
        }

        Rectangle {
            x: (1360 - 24) * Tokens.uiScale
            y: 40 * Tokens.uiScale
            width: Tokens.strokeWidth
            height: parent.height - 80 * Tokens.uiScale
            color: Tokens.border
        }
    }

    // === Colonne gauche : portrait et caracteristiques ============================================

    // Fanion d'ecusson, en haut de la colonne (maquette : 255, 15 -> 370, 140).
    FixedArt {
        x: 319 * Tokens.uiScale
        y: 16 * Tokens.uiScale
        width: 128 * Tokens.uiScale
        height: 160 * Tokens.uiScale
        key: "ui/ornament/crest-pennant"
    }

    // Rose des vents en filigrane (maquette : 130, 590 -> 470, 930).
    FixedArt {
        x: 175 * Tokens.uiScale
        y: 620 * Tokens.uiScale
        width: 400 * Tokens.uiScale
        height: 400 * Tokens.uiScale
        key: "ui/ornament/compass-watermark/parchment"
    }

    // Portrait rond (maquette : centre 310, 355).
    PortraitFrame {
        x: (388 - 150) * Tokens.uiScale
        y: (374 - 150) * Tokens.uiScale
        shape: "round"
        size: 300 * Tokens.uiScale
        source: root.portrait
    }

    // Les six medaillons, aux centres releves sur la maquette.
    StatMedallion {
        x: (246 - 70) * Tokens.uiScale
        y: (174 - 70) * Tokens.uiScale
        width: 140 * Tokens.uiScale
        height: 140 * Tokens.uiScale
        label: qsTr("FOR")
        iconKey: "ui/icon/ability/strength"
        value: root.strengthScore
        modifier: root.strengthModifier
    }

    StatMedallion {
        x: (531 - 70) * Tokens.uiScale
        y: (174 - 70) * Tokens.uiScale
        width: 140 * Tokens.uiScale
        height: 140 * Tokens.uiScale
        label: qsTr("DEX")
        iconKey: "ui/icon/ability/dexterity"
        value: root.dexterityScore
        modifier: root.dexterityModifier
    }

    StatMedallion {
        x: (129 - 70) * Tokens.uiScale
        y: (359 - 70) * Tokens.uiScale
        width: 140 * Tokens.uiScale
        height: 140 * Tokens.uiScale
        label: qsTr("CON")
        iconKey: "ui/icon/ability/constitution"
        value: root.constitutionScore
        modifier: root.constitutionModifier
    }

    StatMedallion {
        x: (635 - 70) * Tokens.uiScale
        y: (359 - 70) * Tokens.uiScale
        width: 140 * Tokens.uiScale
        height: 140 * Tokens.uiScale
        label: qsTr("INT")
        iconKey: "ui/icon/ability/intelligence"
        value: root.intelligenceScore
        modifier: root.intelligenceModifier
    }

    StatMedallion {
        x: (215 - 70) * Tokens.uiScale
        y: (533 - 70) * Tokens.uiScale
        width: 140 * Tokens.uiScale
        height: 140 * Tokens.uiScale
        label: qsTr("SAG")
        iconKey: "ui/icon/ability/wisdom"
        value: root.wisdomScore
        modifier: root.wisdomModifier
    }

    StatMedallion {
        x: (531 - 70) * Tokens.uiScale
        y: (533 - 70) * Tokens.uiScale
        width: 140 * Tokens.uiScale
        height: 140 * Tokens.uiScale
        label: qsTr("CHA")
        iconKey: "ui/icon/ability/charisma"
        value: root.charismaScore
        modifier: root.charismaModifier
    }

    // Constantes de combat, en medaillons derives, sur le filigrane (ecart a la maquette : elle ne
    // les montre pas, mais la fiche les porte, et les retirer aurait perdu une donnee reelle).
    Grid {
        anchors.horizontalCenter: parent.left
        anchors.horizontalCenterOffset: 375 * Tokens.uiScale
        y: 700 * Tokens.uiScale
        columns: 3
        spacing: Tokens.gapMedium

        StatMedallion { kind: "derived"; label: qsTr("CA"); value: root.armorClass }
        StatMedallion { kind: "derived"; label: qsTr("INIT."); value: root.initiative }
        StatMedallion { kind: "derived"; label: qsTr("VITESSE"); value: root.speed }
        StatMedallion { kind: "derived"; label: qsTr("MAÎTRISE"); value: root.proficiencyBonus }
        StatMedallion { kind: "derived"; label: qsTr("PERC. PASS."); value: root.passivePerception }
    }

    // === Colonne centrale : identite =================================================================

    TitlePlate {
        anchors.horizontalCenter: parent.left
        anchors.horizontalCenterOffset: 1010 * Tokens.uiScale
        y: 58 * Tokens.uiScale
        // Largeur bornee a la colonne : la plaque suit son titre, et deborderait sur les competences.
        width: 640 * Tokens.uiScale
        material: "black"
        text: qsTr("Identité du personnage")
    }

    Column {
        x: 719 * Tokens.uiScale
        y: 168 * Tokens.uiScale
        width: 580 * Tokens.uiScale
        spacing: 10 * Tokens.uiScale

        FieldRow { width: parent.width; label: qsTr("Nom"); value: root.characterName }
        FieldRow { width: parent.width; label: qsTr("Classe"); value: root.className }
        FieldRow { width: parent.width; label: qsTr("Niveau"); value: root.level }
        FieldRow { width: parent.width; label: qsTr("Origine"); value: root.background }
        FieldRow { width: parent.width; label: qsTr("Espèce"); value: root.species }
        FieldRow { width: parent.width; label: qsTr("Matricule"); value: root.registration }
    }

    // Jauges (maquette : vie 580, 550 -> 1030, 640 ; experience 580, 670 -> 1030, 760).
    Column {
        x: 725 * Tokens.uiScale
        y: 590 * Tokens.uiScale
        width: 560 * Tokens.uiScale
        spacing: Tokens.gapSmall

        Item {
            width: parent.width
            height: 40 * Tokens.uiScale

            FieldRow {
                anchors.left: parent.left
                width: 220 * Tokens.uiScale
                tagWidth: 220 * Tokens.uiScale
                label: qsTr("Points de vie")
                value: ""
            }

            Text {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: root.hitPointsText
                color: Tokens.text
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
            }
        }

        Gauge {
            width: parent.width
            kind: "health"
            value: root.hitPointsRatio
        }

        Item {
            width: parent.width
            height: 56 * Tokens.uiScale

            FieldRow {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                width: 220 * Tokens.uiScale
                tagWidth: 220 * Tokens.uiScale
                label: qsTr("Expérience")
                value: ""
            }

            Text {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Tokens.gapSmall
                text: root.experienceText
                color: Tokens.text
                font.family: Tokens.bodyFamily
                font.pixelSize: Tokens.fontBody
            }
        }

        Gauge {
            width: parent.width
            kind: "experience"
            value: root.experienceRatio
        }
    }

    // Le pied de l'identite : sceau, ecusson, paraphe (maquette : 555, 835 -> 1080, 1005).
    PanelFrame {
        x: 700 * Tokens.uiScale
        y: 872 * Tokens.uiScale
        width: 648 * Tokens.uiScale
        height: 170 * Tokens.uiScale
        material: "parchment"
        subpanel: true
        padding: 0

        FixedArt {
            x: 60 * Tokens.uiScale
            anchors.verticalCenter: parent.verticalCenter
            width: 120 * Tokens.uiScale
            height: 120 * Tokens.uiScale
            key: "ui/ornament/wax-seal"
        }

        FixedArt {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: -40 * Tokens.uiScale
            width: 112 * Tokens.uiScale
            height: 112 * Tokens.uiScale
            key: "ui/ornament/crossed-crest"
        }

        // Le paraphe est une piece sans lettres ; le nom s'ecrit au-dessus par le jeu, a la plume
        // (`signatureFamily`), et le paraphe le souligne : centre sur lui, sa boucle barrait le nom.
        Item {
            anchors.right: parent.right
            anchors.rightMargin: 40 * Tokens.uiScale
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 24 * Tokens.uiScale
            width: 256 * Tokens.uiScale
            height: 64 * Tokens.uiScale

            FixedArt {
                anchors.fill: parent
                key: "ui/ornament/signature-flourish"
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.verticalCenter
                text: root.characterName
                color: Tokens.text
                font.family: Tokens.signatureFamily
                font.pixelSize: Tokens.fontScreenTitle
            }
        }
    }

    // === Colonne droite : competences, classe, sorts (LOT-141) ==========================================
    // Trois onglets sur la meme colonne : les competences de la maquette, la classe (capacites
    // acquises, a venir), les sorts (lancers restants). Le jumeau fait tourner `currentTab` (Page
    // suiv. / prec., RB / LB) ; les onglets se cliquent aussi.

    TitlePlate {
        anchors.horizontalCenter: parent.left
        anchors.horizontalCenterOffset: 1625 * Tokens.uiScale
        y: 58 * Tokens.uiScale
        width: 460 * Tokens.uiScale
        material: "black"
        text: root.currentTab === 0 ? qsTr("Compétences") : (root.currentTab === 1 ? qsTr("Classe") : qsTr("Sorts"))
    }

    Row {
        x: 1400 * Tokens.uiScale
        y: 150 * Tokens.uiScale
        spacing: Tokens.gapSmall

        OrnateTab {
            id: skillsTabControl
            width: 174 * Tokens.uiScale
            material: "parchment"
            text: qsTr("Compétences")
            checkable: false
            checked: root.currentTab === 0
            focusPolicy: Qt.NoFocus
        }
        OrnateTab {
            id: classTabControl
            width: 130 * Tokens.uiScale
            material: "parchment"
            text: qsTr("Classe")
            checkable: false
            checked: root.currentTab === 1
            focusPolicy: Qt.NoFocus
        }
        OrnateTab {
            id: spellsTabControl
            width: 130 * Tokens.uiScale
            material: "parchment"
            text: qsTr("Sorts")
            checkable: false
            checked: root.currentTab === 2
            focusPolicy: Qt.NoFocus
        }
    }

    // L'entree des sorts (LOT-87, T3.8) : sous les competences, qui en sont le voisin naturel.
    OrnateButton {
        id: skillsControl

        x: 1480 * Tokens.uiScale
        y: 990 * Tokens.uiScale
        width: 380 * Tokens.uiScale
        kind: "secondary"
        text: qsTr("Compétences et sorts")
    }

    StackLayout {
        x: 1400 * Tokens.uiScale
        y: 220 * Tokens.uiScale
        width: 450 * Tokens.uiScale
        height: 760 * Tokens.uiScale
        currentIndex: root.currentTab

        // ===== Onglet Competences ==========================================================================
        ListView {
            clip: true
            interactive: false
            model: root.skills

            // Une enveloppe et non `SkillRow` directement : la brique porte deja `label` et `value`, et un
            // delegue ne peut pas redeclarer en `required` une propriete de son type.
            delegate: Item {
                id: skillEntry

                required property string rowId
                required property string label
                required property string value

                width: ListView.view.width
                height: skillLine.implicitHeight

                SkillRow {
                    id: skillLine

                    anchors.fill: parent
                    skillId: skillEntry.rowId
                    label: skillEntry.label
                    // La maitrise est portee par la pastille : le point de la ligne de texte s'efface.
                    value: skillEntry.value.replace(" •", "")
                    proficient: skillEntry.value.indexOf("•") >= 0
                }
            }
        }

        // ===== Onglet Classe : acquis, puis a venir ========================================================
        ListView {
            id: classView

            clip: true
            spacing: Tokens.gapSmall
            model: root.classRows

            delegate: Item {
                id: capacityEntry

                required property string label
                required property string value
                required property string detail
                required property string levelText
                required property bool upcoming

                width: ListView.view.width
                height: Math.max(capacityIcon.height, capacityColumn.implicitHeight) + Tokens.gapSmall
                opacity: capacityEntry.upcoming ? 0.6 : 1

                Item {
                    id: capacityIcon

                    anchors.left: parent.left
                    anchors.top: parent.top
                    width: 64 * Tokens.uiScale
                    height: 64 * Tokens.uiScale

                    Rectangle {
                        anchors.fill: parent
                        visible: !capacityRing.delivered
                        radius: width / 2
                        color: Tokens.panel
                        border.color: Tokens.accent
                        border.width: 2 * Tokens.strokeWidth
                    }

                    FixedArt {
                        id: capacityRing

                        anchors.fill: parent
                        key: "ui/medallion/icon-ring"
                    }

                    FixedArt {
                        anchors.fill: parent
                        anchors.margins: 11 * Tokens.uiScale
                        key: capacityEntry.value
                    }
                }

                Column {
                    id: capacityColumn

                    anchors.left: capacityIcon.right
                    anchors.right: parent.right
                    anchors.leftMargin: Tokens.gapMedium
                    anchors.top: parent.top

                    Text {
                        width: parent.width
                        text: capacityEntry.label
                        color: Tokens.text
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontBody
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        text: capacityEntry.levelText
                        color: capacityEntry.upcoming ? Tokens.textMuted : Tokens.accent
                        font.family: Tokens.titleFamily
                        font.pixelSize: Tokens.fontCaption
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        text: capacityEntry.detail
                        color: Tokens.textMuted
                        font.family: Tokens.bodyFamily
                        font.pixelSize: Tokens.fontCaption
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // ===== Onglet Sorts : les sorts connus et leurs lancers ===========================================
        ListView {
            id: spellView

            clip: true
            spacing: Tokens.gapSmall
            model: root.spellRows

            delegate: Item {
                id: spellRow

                required property string label
                required property string value
                required property string detail
                required property string uses
                required property string levelText

                width: ListView.view.width
                height: spellLine.implicitHeight + Tokens.gapSmall

                SpellEntry {
                    id: spellLine

                    anchors.left: parent.left
                    anchors.right: parent.right
                    name: spellRow.label
                    iconKey: spellRow.value
                    details: spellRow.levelText + "\n" + spellRow.detail
                }

                // Les lancers restants, a droite du nom : ce que le combat en a laisse (LOT-139).
                Text {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    text: spellRow.uses
                    color: Tokens.accent
                    font.family: Tokens.titleFamily
                    font.pixelSize: Tokens.fontBody
                    font.weight: Font.DemiBold
                }
            }
        }
    }
}
