pragma Singleton
import QtQuick

/*!
    Doublure de `hmi::WorldModel` pour Qt Design Studio (LOT-09).

    Memes proprietes et methodes que le type C++ (`Source/HMI/Runtime/WorldModel.h`), avec une
    carte d'exemple chargee : l'ecran de jeu se dessine ainsi dans l'atelier, HUD compris. Les
    methodes ne font rien -- dans l'atelier, personne ne marche.
*/
QtObject {
    readonly property string mapId: "capital/martpart"
    readonly property string mapName: "Martpart"
    readonly property string status: ""
    readonly property bool loaded: true
    readonly property int columns: 48
    readonly property int rows: 40
    readonly property real heroColumn: 20.5
    readonly property real heroRow: 15.5
    property string heroFigure: "Common/Characters/Heroes/brawler"
    property bool frozen: false
    readonly property string cityLocation: "central-empire-the-capital-city"
    readonly property string districtId: "central-empire-the-capital-city-martpart"
    readonly property var visitedDistricts: ["central-empire-the-capital-city-martpart"]
    // Le groupe (LOT-138) : le Brawler en tete, comme au depart d'une partie.
    readonly property var partyMembers: []
    readonly property var partyCandidates: []
    readonly property string leaderId: "heros-brawler"
    readonly property string leaderName: "Grom Tranche-Écaille"
    readonly property url leaderPortrait: ""
    readonly property int maxPartySize: 4
    readonly property string shownCharacterId: "heros-brawler"

    signal partyChanged()

    function startNewGame() { return true }
    function endGame() {}
    function enterMap(mapId, arrival) { return true }
    function mapOfDistrict(districtId) {
        return districtId.endsWith("-martpart") ? "capital/martpart"
             : districtId.endsWith("-arenarea") ? "capital/arenarea" : ""
    }
    function setMove(x, y) {}
    function interact() {}
    function setLeader(characterId) { return true }
    function rotateLeader() { return true }
    function toggleMember(characterId) { return true }
    function moveMember(characterId, offset) { return true }
    function levelUp(characterId) { return true }
    function rest(characterId) { return true }
    function showCharacter(characterId) {}
}
