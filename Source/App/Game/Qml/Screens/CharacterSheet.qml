import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Fiche de personnage — CÂBLAGE, côté développeur (LOT-86, LOT-87 T3.4, LOT-141).

    Ce fichier ne décrit aucune apparence : il relie le formulaire `CharacterSheetForm.ui.qml` à la
    vue-modèle C++. C'est la seule moitié que la conception n'ouvre jamais, et la seule où du code
    a le droit d'exister.

    Presque tout vient de `CharacterSheetModel`, dont la table `values` porte le score et le
    modificateur séparés de chaque caractéristique. Deux champs de la maquette n'ont pas encore de
    source et passent par `PendingData` : le **matricule**, et le **seuil du niveau suivant** qui
    remplit la jauge d'expérience (`LOT-74`). Le portrait est celui de la figurine de la classe,
    que la partie connaît.

    LA FICHE DE CHACUN (LOT-141). La fiche ouverte est celle du personnage que la partie désigne
    (`WorldModel.shownCharacterId` : celui de l'écran de groupe, sinon le meneur) ; `Tab` passe
    au membre suivant, `Maj+Tab` au précédent. Les trois onglets de la colonne droite se
    parcourent par `Page suiv.` / `Page préc.` ; dans l'onglet Classe ou
    Sorts, `Haut` / `Bas` (croix) font défiler la liste. `Échap` (`B`) referme.
*/
CharacterSheetForm {
    id: root

    focus: true
    currentTab: ScreenRouter.characterTab
    query: root.searchField.text
    selectedSpell: root.spellRows.length > 0 ? root.spellRows[Math.max(0, Math.min(root.spellList.currentIndex, root.spellRows.length - 1))].spell : ({})
    selectedCapacity: root.classRows.length > 0 ? root.classRows[Math.max(0, Math.min(root.classList.currentIndex, root.classRows.length - 1))] : ({})
    onTabRequested: (index) => {
        if (index === 3) ScreenRouter.openRpgScreen(ScreenRouter.Inventory)
        else root.currentTab = index
    }
    onAbilityTabRequested: (index) => { root.abilityTab = index; root.searchField.clear() }
    onSpellSelected: (index) => { root.spellList.currentIndex = index }
    onCapacitySelected: (index) => { root.classList.currentIndex = index }
    onMemberStepRequested: (step) => root.cycleMember(step)
    onCloseRequested: ScreenRouter.closeRpgScreen()

    readonly property CharacterSheetModel sheet: CharacterSheetModel {}
    /// Les membres du groupe, pour passer de l'un à l'autre.
    readonly property var members: WorldModel.partyMembers

    /// Une valeur de la fiche par sa clé, ou le tiret si la fiche ne la porte pas.
    function field(key) {
        const text = root.sheet.values[key];
        return text === undefined ? "—" : text;
    }

    characterName: sheet.name
    className: sheet.className
    level: sheet.level
    background: sheet.background
    species: sheet.species
    registration: PendingData.value("character_sheet.registration")
    portrait: root.portraitOf(sheet.characterId)

    hitPointsText: sheet.hitPoints
    // « 25 / 30 » : le courant se lit avant la barre, le maximum a sa propre clé.
    hitPointsRatio: {
        const current = parseInt(sheet.hitPoints, 10);
        const maximum = parseInt(sheet.hitPointsMax, 10);
        return maximum > 0 && !isNaN(current) ? Math.max(0, Math.min(1, current / maximum)) : 0;
    }

    experienceText: sheet.experience + " / " + root.nextLevelExperience
    experienceRatio: {
        const current = parseInt(sheet.experience, 10);
        const next = parseInt(root.nextLevelExperience, 10);
        return next > 0 && !isNaN(current) ? Math.max(0, Math.min(1, current / next)) : 0;
    }
    readonly property string nextLevelExperience: PendingData.value("character_sheet.experience.next_level")

    strengthScore: root.field("sheet.ability.strength.score")
    strengthModifier: root.field("sheet.ability.strength.modifier")
    dexterityScore: root.field("sheet.ability.dexterity.score")
    dexterityModifier: root.field("sheet.ability.dexterity.modifier")
    constitutionScore: root.field("sheet.ability.constitution.score")
    constitutionModifier: root.field("sheet.ability.constitution.modifier")
    intelligenceScore: root.field("sheet.ability.intelligence.score")
    intelligenceModifier: root.field("sheet.ability.intelligence.modifier")
    wisdomScore: root.field("sheet.ability.wisdom.score")
    wisdomModifier: root.field("sheet.ability.wisdom.modifier")
    charismaScore: root.field("sheet.ability.charisma.score")
    charismaModifier: root.field("sheet.ability.charisma.modifier")

    armorClass: sheet.armorClass
    initiative: sheet.initiative
    speed: sheet.speed
    proficiencyBonus: sheet.proficiencyBonus
    passivePerception: sheet.passivePerception

    skills: sheet.skills
    classRows: root.classRowsOf(sheet.capacities, sheet.upcomingCapacities).filter(row => row.label.toLocaleLowerCase().indexOf(root.query.toLocaleLowerCase()) >= 0)
    spellRows: root.spellRowsOf(sheet.spells).filter(row => row.label.toLocaleLowerCase().indexOf(root.query.toLocaleLowerCase()) >= 0)

    Component.onCompleted: sheet.loadShownCharacter()

    // Une montée de niveau, un combat : la fiche se relit quand la partie change le groupe.
    Connections {
        target: WorldModel
        function onPartyChanged() { root.sheet.loadShownCharacter() }
    }

    /// Le portrait de la figurine du membre `characterId`, ou vide.
    function portraitOf(characterId) {
        for (let i = 0; i < root.members.length; ++i) {
            if (root.members[i].id === characterId) {
                return root.members[i].portrait
            }
        }
        return ""
    }

    /// Passe au membre suivant (`step` 1) ou précédent (-1) du groupe.
    function cycleMember(step) {
        const count = root.members.length
        if (count === 0) {
            return
        }
        let index = 0
        for (let i = 0; i < count; ++i) {
            if (root.members[i].id === root.sheet.characterId) {
                index = i
            }
        }
        WorldModel.showCharacter(root.members[(index + step + count) % count].id)
    }

    function cycleTab(step) {
        root.currentTab = (root.currentTab + step + 3) % 3
    }

    /// Fait défiler la liste de l'onglet courant (classe, sorts) d'un cran.
    function scroll(step) {
        const view = root.currentTab === 1 ? (root.abilityTab === 1 ? root.spellList : root.classList) : null
        if (view === null) {
            return
        }
        view.currentIndex = Math.max(0, Math.min(view.count - 1, view.currentIndex + step))
        view.positionViewAtIndex(view.currentIndex, ListView.Contain)
    }

    function classRowsOf(capacities, upcoming) {
        const rows = []
        for (let i = 0; i < capacities.length; ++i) {
            rows.push({ rowId: capacities[i].id, label: capacities[i].name, value: capacities[i].iconKey,
                        detail: capacities[i].text, levelText: qsTr("Niveau %1").arg(capacities[i].level),
                        upcoming: false })
        }
        for (let i = 0; i < upcoming.length; ++i) {
            rows.push({ rowId: "next-" + upcoming[i].id + "-" + upcoming[i].level, label: upcoming[i].name,
                        value: upcoming[i].iconKey, detail: upcoming[i].text,
                        levelText: qsTr("À venir · niveau %1").arg(upcoming[i].level), upcoming: true })
        }
        return rows
    }

    function spellRowsOf(spells) {
        const rows = []
        for (let i = 0; i < spells.length; ++i) {
            rows.push({ rowId: spells[i].id, label: spells[i].name, value: spells[i].iconKey,
                        spell: spells[i], detail: spells[i].details, uses: spells[i].usesText,
                        levelText: spells[i].level === 0 ? qsTr("Sort mineur") : qsTr("Niveau %1").arg(spells[i].level) })
        }
        return rows
    }

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_PageDown: root.cycleTab(1); break
        case Qt.Key_PageUp: root.cycleTab(-1); break
        case Qt.Key_Tab: root.cycleMember(1); break
        case Qt.Key_Backtab: root.cycleMember(-1); break
        case Qt.Key_Down: root.scroll(1); break
        case Qt.Key_Up: root.scroll(-1); break
        case Qt.Key_Escape: ScreenRouter.closeRpgScreen(); break
        default: return
        }
        event.accepted = true
    }

}
