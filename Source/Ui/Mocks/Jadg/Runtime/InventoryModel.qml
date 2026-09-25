import QtQuick

/*!
    Doublure de `hmi::InventoryModel` pour Qt Design Studio (LOT-87).

    Mêmes propriétés que le type C++ (`Source/HMI/Runtime/InventoryModel.h`), avec l'équipement du
    personnage de démonstration.
    Les actions (`equipSelected`, `dropSelected`…) ne font rien : l'atelier dessine, il ne joue pas.
*/
QtObject {
    readonly property string purse: "37 po"
    readonly property string carried: "24 kg"
    readonly property string capacity: "80 kg"
    readonly property string backpack: "Corde (15 m), torches (5), rations (3)"

    readonly property var equipped: ({
        "torso": { itemId: "cuir-cloute", name: "Cuir clouté" },
        "main-hand": { itemId: "epee-longue", name: "Épée longue" },
        "off-hand": { itemId: "bouclier", name: "Bouclier" }
    })
    property int filter: 0
    readonly property var cells: [
        { itemId: "dague", name: "Dague", quantity: 1 },
        { itemId: "torche", name: "Torche", quantity: 5 },
        { itemId: "rations-1-jour", name: "Rations (1 jour)", quantity: 4 }
    ]
    readonly property string selectedItem: "dague"
    readonly property string selectedSlot: ""
    readonly property var selection: ({
        itemId: "dague", name: "Dague", kind: "Arme courante", damage: "1d4", armor: "",
        weight: "0,5 kg", text: "Finesse, légère, lancer", canEquip: true, canUnequip: false,
        canDrop: true
    })
    readonly property real loadRatio: 0.3
    readonly property string gold: "42"
    readonly property string armorClass: "16"
    readonly property string initiative: "+1"
    readonly property string speed: "9 m"
    readonly property string passivePerception: "13"

    function loadDemonstrationCharacter() {}
    function selectItem(itemId) {}
    function selectSlot(slot) {}
    function equipSelected() {}
    function unequipSelected() {}
    function dropSelected() {}
    function sortBackpack() {}

    signal changed()
}
