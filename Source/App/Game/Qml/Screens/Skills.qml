import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Competences et sorts -- CABLAGE, cote developpeur (LOT-87, T3.8 ; LOT-141).

    Tout vient de `CharacterSheetModel`, pour le personnage que la partie designe (le meme que la
    fiche) : ses attaques d'arme, ses sorts mineurs, ses sorts connus ranges par ecole de magie,
    et le detail du sort designe. Les fleches Haut / Bas designent une ecole, Gauche / Droite un
    sort dans cette ecole ; `Tab` passe au membre suivant ; `Echap` (`B`) referme.

    Ouvert depuis la fiche de personnage ; le bouton Fiche y ramene.
*/
SkillsForm {
    id: root

    focus: true

    readonly property CharacterSheetModel sheet: CharacterSheetModel {}
    /// Le sort designe dans l'ecole courante (indice dans `spellsOfSchool(currentSchool)`).
    property int currentSpell: 0
    readonly property var schoolSpellLists: root.spellsBySchool(sheet.spells)
    readonly property var current: root.spellAt(root.currentSchool, root.currentSpell)

    spellSlots: root.slotsText(sheet.spells)
    attacks: sheet.attacks
    cantrips: root.cantripRows(sheet.spells)

    schoolSlots: root.schoolTexts(true)
    schoolSpells: root.schoolTexts(false)

    spellName: root.current !== null ? root.current.name : "—"
    spellSchool: root.current !== null ? root.schoolName(root.current.school) : "—"
    spellDescription: root.current !== null ? root.current.text : "—"
    spellDamageType: root.current !== null && root.current.damageType.length > 0 ? root.current.damageType : "—"
    spellRange: root.current !== null ? root.current.range : "—"
    spellDamage: root.current !== null && root.current.damage.length > 0 ? root.current.damage : "—"
    spellCastingTime: root.current !== null ? root.current.castingTime : "—"
    spellComponents: root.current !== null && root.current.components.length > 0 ? root.current.components : "—"
    spellEffects: root.current !== null ? qsTr("Durée : %1 · Lancers : %2").arg(root.current.duration).arg(root.current.usesText) : "—"

    Component.onCompleted: sheet.loadShownCharacter()

    Connections {
        target: WorldModel
        function onPartyChanged() { root.sheet.loadShownCharacter() }
    }

    onCurrentSchoolChanged: root.currentSpell = 0

    /// Les sorts connus, par ecole, dans l'ordre des ecoles du formulaire.
    function spellsBySchool(spells) {
        const lists = []
        for (let s = 0; s < root.schools.length; ++s) {
            const list = []
            for (let i = 0; i < spells.length; ++i) {
                if (spells[i].school === root.schools[s].schoolId) {
                    list.push(spells[i])
                }
            }
            lists.push(list)
        }
        return lists
    }

    function spellAt(school, index) {
        const list = root.schoolSpellLists[school]
        if (!list || list.length === 0) {
            return null
        }
        return list[Math.max(0, Math.min(list.length - 1, index))]
    }

    function schoolName(schoolId) {
        for (let s = 0; s < root.schools.length; ++s) {
            if (root.schools[s].schoolId === schoolId) {
                return root.schools[s].name
            }
        }
        return schoolId
    }

    /// Par ecole : le nombre de sorts connus (`counts`), ou leurs noms.
    function schoolTexts(counts) {
        const texts = []
        for (let s = 0; s < root.schools.length; ++s) {
            const list = root.schoolSpellLists[s]
            if (counts) {
                texts.push(list.length === 0 ? "—" : qsTr("%1 sort(s)").arg(list.length))
            } else {
                const names = []
                for (let i = 0; i < list.length; ++i) {
                    names.push(list[i].name + (list[i].perDay > 0 ? " (" + list[i].usesText + ")" : ""))
                }
                texts.push(names.length === 0 ? "—" : names.join(", "))
            }
        }
        return texts
    }

    function cantripRows(spells) {
        const rows = []
        for (let i = 0; i < spells.length; ++i) {
            if (spells[i].level === 0) {
                rows.push({ rowId: spells[i].id, label: spells[i].name, value: spells[i].usesText })
            }
        }
        return rows
    }

    /// Les lancers du jour : « 3 / 4 » sur l'ensemble des sorts a lancers, ou « — » sans sort.
    function slotsText(spells) {
        let remaining = 0
        let perDay = 0
        for (let i = 0; i < spells.length; ++i) {
            remaining += spells[i].remaining
            perDay += spells[i].perDay
        }
        return perDay > 0 ? remaining + " / " + perDay : "—"
    }

    function cycleSpell(step) {
        const list = root.schoolSpellLists[root.currentSchool]
        if (list && list.length > 0) {
            root.currentSpell = (root.currentSpell + step + list.length) % list.length
        }
    }

    function cycleMember(step) {
        const members = WorldModel.partyMembers
        if (members.length === 0) {
            return
        }
        let index = 0
        for (let i = 0; i < members.length; ++i) {
            if (members[i].id === root.sheet.characterId) {
                index = i
            }
        }
        WorldModel.showCharacter(members[(index + step + members.length) % members.length].id)
    }

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_Up: root.currentSchool = (root.currentSchool + root.schools.length - 1) % root.schools.length; break
        case Qt.Key_Down: root.currentSchool = (root.currentSchool + 1) % root.schools.length; break
        case Qt.Key_Left: root.cycleSpell(-1); break
        case Qt.Key_Right: root.cycleSpell(1); break
        case Qt.Key_Tab: root.cycleMember(1); break
        case Qt.Key_Backtab: root.cycleMember(-1); break
        case Qt.Key_Escape: ScreenRouter.openRpgScreen(ScreenRouter.CharacterSheet); break
        default: return
        }
        event.accepted = true
    }

    GamepadNavigator {
        active: root.visible
        onPressed: (button) => {
            switch (button) {
            case "up": root.currentSchool = (root.currentSchool + root.schools.length - 1) % root.schools.length; break
            case "down": root.currentSchool = (root.currentSchool + 1) % root.schools.length; break
            case "left": root.cycleSpell(-1); break
            case "right": root.cycleSpell(1); break
            case "x": root.cycleMember(1); break
            case "b": ScreenRouter.openRpgScreen(ScreenRouter.CharacterSheet); break
            }
        }
    }

    Connections {
        target: root.sheetButton
        function onClicked() { ScreenRouter.openRpgScreen(ScreenRouter.CharacterSheet) }
    }
}
