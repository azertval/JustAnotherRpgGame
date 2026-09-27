import QtQuick

/*!
    Doublure de `hmi::PartyModel` pour Qt Design Studio (LOT-138).

    Memes proprietes et methodes que le type C++ (`Source/HMI/Runtime/PartyModel.h`), avec les
    quatre fiches pre-tirees, pour que l'ecran de groupe et le HUD se dessinent dans l'atelier. Les
    methodes ne font rien.
*/
QtObject {
    readonly property var members: [
        { id: "heros-brawler", name: "Grom Tranche-Écaille", label: "Grom Tranche-Écaille", value: "15 / 15", ratio: 1, role: "Brawler", portrait: "", leader: true, rank: 0 },
        { id: "heros-mage", name: "Faelar Trace-Carte", label: "Faelar Trace-Carte", value: "8 / 8", ratio: 1, role: "Mage", portrait: "", leader: false, rank: 1 },
        { id: "heros-priest", name: "Helga Pierre-Sûre", label: "Helga Pierre-Sûre", value: "12 / 12", ratio: 1, role: "Priest", portrait: "", leader: false, rank: 2 },
        { id: "heros-scoundrel", name: "Nessa Double-Vie", label: "Nessa Double-Vie", value: "10 / 10", ratio: 1, role: "Scoundrel", portrait: "", leader: false, rank: 3 }
    ]
    readonly property var candidates: [
        { id: "heros-brawler", name: "Grom Tranche-Écaille", className: "Brawler", species: "Demi-orc", level: "1", value: "15 / 15", armorClass: "14", speed: "9 m", portrait: "", leader: true, rank: 0 },
        { id: "heros-mage", name: "Faelar Trace-Carte", className: "Mage", species: "Elfe d'automne", level: "1", value: "8 / 8", armorClass: "12", speed: "9 m", portrait: "", leader: false, rank: 1 },
        { id: "heros-priest", name: "Helga Pierre-Sûre", className: "Priest", species: "Nain des collines", level: "1", value: "12 / 12", armorClass: "17", speed: "7,5 m", portrait: "", leader: false, rank: 2 },
        { id: "heros-scoundrel", name: "Nessa Double-Vie", className: "Scoundrel", species: "Humain", level: "1", value: "10 / 10", armorClass: "14", speed: "12 m", portrait: "", leader: false, rank: -1 }
    ]
    readonly property string leaderName: "Grom Tranche-Écaille"
    readonly property string leaderLevel: "1"
    readonly property string leaderHitPoints: "15 / 15"
    readonly property int size: 3
    readonly property int maxSize: 4

    signal changed()

    function toggleMember(characterId) { return true }
    function setLeader(characterId) { return true }
    function moveMember(characterId, offset) { return true }
}
