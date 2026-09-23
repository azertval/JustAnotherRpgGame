import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Competences et sorts -- CABLAGE, cote developpeur (LOT-87, T3.8).

    Aucune donnee avant le lot des sorts (`LOT-35`) : chaque liaison porte une CLE D'ATTRIBUTION
    `skills.*`, que `python scripts/i18n/list_pending_bindings.py` releve. Le jour ou le lot arrive, il
    remplace ici `PendingData` par sa vue-modele ; le formulaire ne bouge pas.

    Ouvert depuis la fiche de personnage ; le bouton Fiche y ramene. Les fleches designent une
    ecole de magie.
*/
SkillsForm {
    id: root

    focus: true

    spellSlots: PendingData.value("skills.spell_slots")
    attacks: PendingData.rows("skills.attacks", 2)
    cantrips: PendingData.rows("skills.cantrips", 3)

    schoolSlots: [
        PendingData.value("skills.school.abjuration.slots"),
        PendingData.value("skills.school.conjuration.slots"),
        PendingData.value("skills.school.divination.slots"),
        PendingData.value("skills.school.enchantment.slots"),
        PendingData.value("skills.school.evocation.slots"),
        PendingData.value("skills.school.illusion.slots"),
        PendingData.value("skills.school.necromancy.slots"),
        PendingData.value("skills.school.transmutation.slots")
    ]
    schoolSpells: [
        PendingData.value("skills.school.abjuration.spells"),
        PendingData.value("skills.school.conjuration.spells"),
        PendingData.value("skills.school.divination.spells"),
        PendingData.value("skills.school.enchantment.spells"),
        PendingData.value("skills.school.evocation.spells"),
        PendingData.value("skills.school.illusion.spells"),
        PendingData.value("skills.school.necromancy.spells"),
        PendingData.value("skills.school.transmutation.spells")
    ]

    spellName: PendingData.value("skills.spell.name")
    spellSchool: PendingData.value("skills.spell.school")
    spellDescription: PendingData.value("skills.spell.description")
    spellDamageType: PendingData.value("skills.spell.damage_type")
    spellRange: PendingData.value("skills.spell.range")
    spellDamage: PendingData.value("skills.spell.damage")
    spellCastingTime: PendingData.value("skills.spell.casting_time")
    spellComponents: PendingData.value("skills.spell.components")
    spellEffects: PendingData.value("skills.spell.effects")

    Keys.onUpPressed: root.currentSchool = (root.currentSchool + root.schools.length - 1) % root.schools.length
    Keys.onDownPressed: root.currentSchool = (root.currentSchool + 1) % root.schools.length

    Connections {
        target: root.sheetButton
        function onClicked() { ScreenRouter.openRpgScreen(ScreenRouter.CharacterSheet) }
    }
}
