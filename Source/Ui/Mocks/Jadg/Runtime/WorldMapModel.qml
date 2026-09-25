import QtQuick

/*!
    Doublure de `hmi::WorldMapModel` pour Qt Design Studio (LOT-94, LOT-95).

    Mêmes propriétés et méthodes que le type C++ (`Source/HMI/Runtime/WorldMapModel.h`) : deux
    régions de l'atlas à leurs repères réels, et le plan de la Capitale, pour juger les trois vues
    dans l'atelier.
*/
QtObject {
    readonly property string worldImage: "world.jpg"
    readonly property var regions: [
        { regionId: "central-empire", name: "Central Empire", image: "region-central-empire.jpg",
          x: 0.634, y: 0.42, kind: "city", number: 0, gateway: true,
          frame: { x: 0.39, y: 0.19, width: 0.48, height: 0.48 },
          government: "Absolute monarchy", faction: "Tanarean Empire", population: 2300000,
          grades: [0, 1, 4, 3, 1, 1, 3], labels: [],
          places: [
              { placeId: "central-empire-the-capital-city", name: "The Capital City",
                description: "The largest city on the continent.", placed: true, x: 0.59, y: 0.565,
                hasCityMap: true, kind: "city", number: 0, gateway: true },
              { placeId: "central-empire-hajal-city", name: "Hajal City",
                description: "A global financial center.", placed: true, x: 0.63, y: 0.19,
                hasCityMap: false, kind: "point-of-interest", number: 0, gateway: false }
          ] },
        { regionId: "seashores", name: "Seashores", image: "region-seashores.jpg",
          x: 0.557, y: 0.563, kind: "city", number: 0, gateway: true,
          frame: { x: 0.29, y: 0.384, width: 0.406, height: 0.406 },
          government: "Pirate Lords", faction: "Tanarean Empire", population: 617000,
          grades: [3, 4, 1, 4, 2, 3, 1], labels: [], places: [] }
    ]

    function load() {}

    function city(placeId) {
        if (placeId !== "central-empire-the-capital-city")
            return ({})
        return {
            cityId: placeId, name: "The Capital City", regionId: "central-empire",
            image: "city-central-empire-the-capital-city.jpg", labels: [],
            points: [
                { pointId: "central-empire-the-capital-city-martpart", number: 6, name: "Martpart",
                  description: "The Capital's market district.", x: 0.702, y: 0.505,
                  kind: "point-of-interest", gateway: false }
            ]
        }
    }

    function regionIndex(regionId) {
        for (let index = 0; index < regions.length; ++index) {
            if (regions[index].regionId === regionId)
                return index
        }
        return -1
    }

    signal changed()
}
