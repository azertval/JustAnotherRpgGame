import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Carte, vue ville -- CABLAGE, cote developpeur (LOT-94, LOT-95, LOT-96).

    Troisieme niveau de l'ecran « Carte ». Ce jumeau ne charge rien : `WorldMap.qml` lui passe le
    plan (`WorldMapModel.city(...)`), la region et le lieu d'ou l'on vient, et l'indice du point
    choisi.
*/
CityMapForm {
    id: root

    /// Le plan montre : une table de `WorldMapModel.city(placeId)`, vide tant qu'aucun n'est ouvert.
    property var city: ({})
    /// La region de la ville, et la ville comme lieu de cette region (pour la mini-carte).
    property var region: null
    property var place: null
    /// Le point choisi, indice dans `city.points`, ou -1.
    property int pointIndex: -1

    readonly property var shownPoints: root.city.points !== undefined ? root.city.points : []

    cityImage: root.city.image !== undefined ? root.city.image : ""
    cityName: root.city.name !== undefined ? root.city.name : ""
    regionName: root.region ? root.region.name : ""
    regionImage: root.region ? root.region.image : ""
    cityX: root.place ? root.place.x : 0.5
    cityY: root.place ? root.place.y : 0.5
    // Tout point d'un plan est pose. Un quartier qui a sa carte s'ouvre sur sa vue (LOT-96) ; ceux
    // qu'on a parcourus portent leur point d'or ; ceux que la ville ne parcourt pas encore
    // s'annoncent grises, avec leur nom (LOT-121).
    points: root.shownPoints.map((point) => Object.assign({ placed: true }, point, {
        gateway: point.hasDistrictView === true && WorldModel.mapOfDistrict(point.pointId) !== "",
        visited: WorldModel.visitedDistricts.indexOf(point.pointId) >= 0,
        locked: WorldModel.mapOfDistrict(point.pointId) === ""
    }))
    // Le heros : son quartier, entoure d'or.
    readonly property var heroPoint: root.shownPoints.find((point) => point.pointId === WorldModel.districtId)
    here: root.heroPoint ? Qt.point(root.heroPoint.x, root.heroPoint.y) : Qt.point(-1, -1)
    labels: root.city.labels !== undefined ? root.city.labels : []
    selectedPoint: root.pointIndex
    pointDescription: root.pointIndex >= 0 && root.pointIndex < root.shownPoints.length
                      ? root.shownPoints[root.pointIndex].description : ""
}
