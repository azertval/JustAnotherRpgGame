import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Carte, vue quartier -- CABLAGE, cote developpeur (LOT-96, LOT-121).

    Quatrieme niveau de l'ecran « Carte ». Ce jumeau ne charge rien : `WorldMap.qml` lui passe le
    point du quartier sur le plan de la ville (une table de `WorldMapModel.city(...).points`), le
    quartier lu par `CityDistrictModel.district(...)`, l'indice du repere choisi et la sous-zone
    ouverte ; il en tire ce que le formulaire montre.

    Les reperes du quartier sont ses ilots, puis ses SOUS-ZONES (LOT-121, D-16 : l'Arena of Fate
    dans Arenarea), posees a leur entree. Une carte RENDUE (`LevelEditor --render --canvas`) porte
    sa grille (`grid`) : la case (c, r) tombe en `origin + c × column + r × row` de l'image, et le
    heros, les ilots et les entrees s'y posent en isometrie. Sans grille -- le plan agrandi, ou une
    carte peinte --, la carte est tracee nord en haut et les cases se lisent en fractions.

    Le heros est marque ou il est : a sa case sur la carte ou il marche ; a l'entree de sa sous-zone
    sur la carte du quartier, quand il est dans l'Arena of Fate ou dessous.
*/
DistrictMapForm {
    id: root

    /// Le point du quartier sur le plan de sa ville, ou `null`.
    property var point: null
    /// Le quartier : une table de `CityDistrictModel.district(mapId)`.
    property var district: ({})
    property var city: ({})
    property var region: null
    /// Le repere choisi, indice dans `markers` (ilots puis sous-zones), ou -1.
    property int blockIndex: -1
    /// La sous-zone ouverte, indice dans `zones`, ou -1 pour le quartier lui-meme.
    property int zoneIndex: -1

    readonly property var shownBlocks: root.district.blocks !== undefined ? root.district.blocks : []
    readonly property var zones: root.point && root.point.zones !== undefined ? root.point.zones : []
    readonly property var zone: root.zoneIndex >= 0 && root.zoneIndex < root.zones.length
                                ? root.zones[root.zoneIndex] : null
    readonly property var grid: root.point && root.point.grid !== undefined ? root.point.grid : null
    readonly property int columns: root.district.columns !== undefined ? root.district.columns : 0
    readonly property int rows: root.district.rows !== undefined ? root.district.rows : 0
    readonly property string mapId: root.district.mapId !== undefined ? root.district.mapId : ""

    /// Les reperes du quartier : ses ilots numerotes, puis ses sous-zones, qui s'ouvrent aussi.
    readonly property var markers: root.shownBlocks.map((block, index) => Object.assign(
        { kind: "point-of-interest", number: index + 1, gateway: true, placed: true }, block,
        root.cellPoint(block.column + block.width / 2, block.row + block.height / 2,
                       block.x, block.y)))
        .concat(root.zones.map((zone, index) => Object.assign(
            { kind: "point-of-interest", number: root.shownBlocks.length + index + 1,
              gateway: zone.image !== "", placed: true, name: zone.name, zoneId: zone.zoneId },
            root.cellPoint(zone.entrance.x + 0.5, zone.entrance.y + 0.5, -1, -1))))

    /// La case (c, r) du quartier sur son image : par la grille, a defaut en fractions ; `fx`, `fy`
    /// sont les fractions deja connues (un ilot), -1 sinon.
    function cellPoint(column, row, fx, fy) {
        if (root.grid)
            return { x: root.grid.origin.x + column * root.grid.column.x + row * root.grid.row.x,
                     y: root.grid.origin.y + column * root.grid.column.y + row * root.grid.row.y }
        if (fx >= 0)
            return { x: fx, y: fy }
        return { x: root.columns > 0 ? column / root.columns : -1,
                 y: root.rows > 0 ? row / root.rows : -1 }
    }

    /// Le heros sur la carte montree, `x < 0` s'il n'y est pas.
    function heroPoint() {
        if (!WorldModel.loaded || root.mapId === "")
            return Qt.point(-1, -1)
        const heroMap = WorldModel.mapId
        if (root.zone) {
            // La sous-zone : le heros a sa case, s'il y marche (dessous, il n'y a pas de place).
            const g = root.zone.grid
            if (!g || heroMap !== root.mapId + "/" + root.zone.zoneId)
                return Qt.point(-1, -1)
            return Qt.point(g.origin.x + WorldModel.heroColumn * g.column.x + WorldModel.heroRow * g.row.x,
                            g.origin.y + WorldModel.heroColumn * g.column.y + WorldModel.heroRow * g.row.y)
        }
        if (heroMap === root.mapId) {
            const at = root.cellPoint(WorldModel.heroColumn, WorldModel.heroRow, -1, -1)
            return Qt.point(at.x, at.y)
        }
        // Dans une sous-zone, ou sous elle : a son entree.
        const inside = root.markers.find((marker) => marker.zoneId !== undefined
            && (heroMap === root.mapId + "/" + marker.zoneId
                || heroMap.startsWith(root.mapId + "/" + marker.zoneId + "/")))
        return inside ? Qt.point(inside.x, inside.y) : Qt.point(-1, -1)
    }

    cityImage: root.city.image !== undefined ? root.city.image : ""
    districtImage: root.zone ? root.zone.image
                   : root.point && root.point.districtImage !== undefined ? root.point.districtImage : ""
    frame: root.point && root.point.frame !== undefined ? root.point.frame : Qt.rect(0, 0, 1, 1)
    districtName: root.point ? root.point.name : ""
    zoneName: root.zone ? root.zone.name : ""
    cityName: root.city.name !== undefined ? root.city.name : ""
    regionName: root.region ? root.region.name : ""
    blocks: root.zone ? [] : root.markers
    selectedBlock: root.zone ? -1 : root.blockIndex
    blockDescription: ""
    here: root.heroPoint()
}
