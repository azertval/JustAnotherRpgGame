import QtQuick

/*!
    Doublure de `hmi::CharacterSheetModel` pour Qt Design Studio (LOT-87).

    Mêmes propriétés que le type C++ (`Source/HMI/Runtime/CharacterSheetModel.h`), avec les valeurs
    du personnage de démonstration ; les listes sont des `ListModel` aux rôles de `SheetRowModel`
    (`rowId`, `label`, `value`), parce qu'un tableau n'exposerait que `modelData`.
*/
QtObject {
    readonly property string name: "Brenna Pierrefonte"
    readonly property string species: "Naine des collines"
    readonly property string background: "Artisane de guilde"
    readonly property string level: "3"
    readonly property string experience: "900"
    readonly property string hitPoints: "25 / 30"
    readonly property string hitPointsMax: "30"
    readonly property string hitDice: "3d10"
    readonly property string armorClass: "16"
    readonly property string initiative: "+1"
    readonly property string speed: "9 m"
    readonly property string proficiencyBonus: "+2"
    readonly property string passivePerception: "11"

    readonly property var values: ({
        "sheet.name": "Brenna Pierrefonte", "sheet.class": "Guerrière", "sheet.level": "3",
        "sheet.species": "Naine des collines", "sheet.background": "Artisane de guilde",
        "sheet.experience": "900", "sheet.hit_points": "25 / 30", "sheet.hit_points_max": "30",
        "sheet.ability.strength.score": "16", "sheet.ability.strength.modifier": "+3",
        "sheet.ability.dexterity.score": "12", "sheet.ability.dexterity.modifier": "+1",
        "sheet.ability.constitution.score": "15", "sheet.ability.constitution.modifier": "+2",
        "sheet.ability.intelligence.score": "10", "sheet.ability.intelligence.modifier": "+0",
        "sheet.ability.wisdom.score": "13", "sheet.ability.wisdom.modifier": "+1",
        "sheet.ability.charisma.score": "8", "sheet.ability.charisma.modifier": "-1"
    })

    readonly property ListModel abilities: ListModel {
        ListElement { rowId: "strength"; label: "Force"; value: "16 (+3)" }
        ListElement { rowId: "dexterity"; label: "Dextérité"; value: "12 (+1)" }
        ListElement { rowId: "constitution"; label: "Constitution"; value: "15 (+2)" }
        ListElement { rowId: "intelligence"; label: "Intelligence"; value: "10 (+0)" }
        ListElement { rowId: "wisdom"; label: "Sagesse"; value: "13 (+1)" }
        ListElement { rowId: "charisma"; label: "Charisme"; value: "8 (-1)" }
    }

    readonly property ListModel skills: ListModel {
        ListElement { rowId: "athletics"; label: "Athlétisme"; value: "+5 •" }
        ListElement { rowId: "stealth"; label: "Discrétion"; value: "+1" }
        ListElement { rowId: "perception"; label: "Perception"; value: "+1" }
        ListElement { rowId: "insight"; label: "Intuition"; value: "+1" }
    }

    function loadDemonstrationCharacter() {}

    signal changed()
}
