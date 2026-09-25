pragma ComponentBehavior: Bound
import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Carte -- CABLAGE, cote developpeur (LOT-94, LOT-95, LOT-96).

    Cinq niveaux, cinq formulaires empiles : le monde (`WorldMapForm`, cable ici), une region
    (`RegionMap.qml`), le plan d'une ville (`CityMap.qml`), un quartier et ses ilots
    (`DistrictMap.qml`), un ilot (`BlockMap.qml`). Une sous-zone d'un quartier (l'Arena of Fate dans
    Arenarea, LOT-121) s'ouvre DANS la vue du quartier (`zoneIndex`), sur sa carte ; « remonter »
    rend le quartier. Ce jumeau tient le niveau courant et la selection
    de chacun, et la passe aux autres ; les cartes et leurs reperes viennent de `WorldMapModel`
    (atlas du `LOT-37` joint a `world-maps.json`), les quartiers de `CityDistrictModel` (leur carte
    de niveau), la place du heros de `WorldModel`, l'encart du personnage de la fiche de
    demonstration.

    L'ecran s'ouvre la ou l'on est : sur le plan de la ville, le quartier du heros choisi, quand il
    parcourt un quartier (LOT-96) ; sur l'Empire central sinon.

    On ne se DEPLACE pas sur ces cartes : elles servent a s'orienter (feuille de route, §8). Restent
    en attente, jusqu'aux quetes (`LOT-16`) et au voyage (`LOT-42`) : le jour et l'heure
    (`PendingData`, cles `world_map.*`).

    Commandes -- les memes a tous les niveaux :
    - fleches / croix : repere voisin dans cette direction ; Tab, PagePrec/PageSuiv, LB/RB : lieu
      suivant de la liste (y compris ceux que la carte ne situe pas) ;
    - Entree, clic, A : ouvrir le niveau suivant ; Retour arriere, Echap, clic droit, B : remonter
      (au niveau du monde, Echap ferme l'ecran) ;
    - + / -, molette, X / Y : agrandir ; glisser : deplacer la carte.
    - Ctrl+clic journalise la fraction sous le pointeur : c'est ainsi qu'on releve une position
      pour `world-maps.json`.
*/
Item {
    id: root

    focus: true

    readonly property WorldMapModel atlas: WorldMapModel {}
    readonly property CharacterSheetModel sheet: CharacterSheetModel {}
    readonly property CityDistrictModel districts: CityDistrictModel {}

    /// 0 : le monde ; 1 : une region ; 2 : le plan d'une ville ; 3 : un quartier ; 4 : un ilot.
    property int level: 0
    property int regionIndex: 0
    property int placeIndex: 0
    property int pointIndex: 0
    property int blockIndex: 0
    /// La sous-zone ouverte dans le quartier (LOT-121, D-16), ou -1 : le quartier lui-meme.
    property int zoneIndex: -1
    property var city: ({})
    /// Le quartier ouvert : une table de `CityDistrictModel.district(mapId)`.
    property var district: ({})

    readonly property var regions: atlas.regions
    readonly property var region: root.regionIndex >= 0 && root.regionIndex < root.regions.length
                                  ? root.regions[root.regionIndex] : null
    readonly property var places: root.region ? root.region.places : []
    readonly property var place: root.placeIndex >= 0 && root.placeIndex < root.places.length
                                 ? root.places[root.placeIndex] : null
    readonly property var points: root.city.points !== undefined ? root.city.points : []
    readonly property var point: root.pointIndex >= 0 && root.pointIndex < root.points.length
                                 ? root.points[root.pointIndex] : null
    readonly property var blocks: root.district.blocks !== undefined ? root.district.blocks : []
    readonly property var block: root.blockIndex >= 0 && root.blockIndex < root.blocks.length
                                 ? root.blocks[root.blockIndex] : null
    /// Vrai si le heros parcourt le quartier ouvert.
    readonly property bool heroInDistrict: WorldModel.loaded && root.district.mapId !== undefined
                                           && root.district.mapId === WorldModel.mapId

    readonly property var forms: [worldForm, regionForm, cityForm, districtForm, blockForm]
    /// La carte qu'on deplace et qu'on agrandit ; l'ilot n'en a pas (c'est une image).
    readonly property var canvas: root.forms[root.level].canvas !== undefined
                                  ? root.forms[root.level].canvas : null

    WorldMapForm {
        id: worldForm

        anchors.fill: parent
        opacity: root.level === 0 ? 1 : 0
        visible: opacity > 0
        enabled: root.level === 0
        scale: root.level === 0 ? 1 : 1.35

        worldImage: root.atlas.worldImage
        regions: root.regions
        selectedRegion: root.regionIndex

        Behavior on opacity { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
    }

    RegionMap {
        id: regionForm

        anchors.fill: parent
        opacity: root.level === 1 ? 1 : 0
        visible: opacity > 0
        enabled: root.level === 1
        scale: root.level === 1 ? 1 : (root.level === 0 ? 0.8 : 1.35)

        region: root.region
        placeIndex: root.placeIndex
        worldImage: root.atlas.worldImage

        Behavior on opacity { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
    }

    CityMap {
        id: cityForm

        anchors.fill: parent
        opacity: root.level === 2 ? 1 : 0
        visible: opacity > 0
        enabled: root.level === 2
        scale: root.level === 2 ? 1 : (root.level < 2 ? 0.8 : 1.35)

        city: root.city
        region: root.region
        place: root.place
        pointIndex: root.pointIndex

        Behavior on opacity { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
    }

    DistrictMap {
        id: districtForm

        anchors.fill: parent
        opacity: root.level === 3 ? 1 : 0
        visible: opacity > 0
        enabled: root.level === 3
        scale: root.level === 3 ? 1 : (root.level < 3 ? 0.8 : 1.35)

        point: root.point
        district: root.district
        city: root.city
        region: root.region
        blockIndex: root.blockIndex
        zoneIndex: root.zoneIndex

        Behavior on opacity { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
    }

    BlockMap {
        id: blockForm

        anchors.fill: parent
        opacity: root.level === 4 ? 1 : 0
        visible: opacity > 0
        enabled: root.level === 4
        scale: root.level === 4 ? 1 : 0.8

        point: root.point
        block: root.block
        city: root.city
        region: root.region
        heroInBlock: root.heroInDistrict && root.block !== null
                     && root.districts.blockAt(root.district.mapId, Math.floor(WorldModel.heroColumn),
                                               Math.floor(WorldModel.heroRow)) === root.block.blockId
        // L'image ne se demande qu'a l'ouverture de l'ilot (`openBlock`) : elle se dessine hors
        // ecran, et la redemander a chaque pas du heros serait un rendu par image.
        imageSource: ""

        Behavior on opacity { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.InOutQuad } }
    }

    // --- Niveaux ------------------------------------------------------------------------------------

    function openRegion(index) {
        if (index < 0 || index >= root.regions.length)
            return
        root.regionIndex = index
        root.placeIndex = root.regions[index].places.length > 0 ? 0 : -1
        root.resetView(regionForm.canvas)
        root.level = 1
    }

    function openCity(index) {
        if (index < 0 || index >= root.places.length || !root.places[index].hasCityMap)
            return
        root.placeIndex = index
        root.city = root.atlas.city(root.places[index].placeId)
        root.pointIndex = root.points.length > 0 ? 0 : -1
        root.resetView(cityForm.canvas)
        root.level = 2
    }

    /// Un quartier qui a sa carte s'ouvre sur sa vue (LOT-96) ; les autres restent des reperes.
    function openDistrict(index) {
        if (index < 0 || index >= root.points.length)
            return
        const chosen = root.points[index]
        const mapId = WorldModel.mapOfDistrict(chosen.pointId)
        if (chosen.hasDistrictView !== true || mapId === "")
            return
        root.pointIndex = index
        root.zoneIndex = -1
        root.district = root.districts.district(mapId)
        // Le heros dans ce quartier : on ouvre sur son ilot.
        const here = mapId === WorldModel.mapId
                     ? root.districts.blockAt(mapId, Math.floor(WorldModel.heroColumn),
                                              Math.floor(WorldModel.heroRow)) : ""
        const heroBlock = root.blocks.findIndex((block) => block.blockId === here)
        // Le heros dans une sous-zone (ou dessous) : on ouvre sur son entree (LOT-121).
        const heroZone = districtForm.zones.findIndex((zone) => WorldModel.mapId === mapId + "/" + zone.zoneId
            || WorldModel.mapId.startsWith(mapId + "/" + zone.zoneId + "/"))
        root.blockIndex = heroBlock >= 0 ? heroBlock
                        : heroZone >= 0 ? root.blocks.length + heroZone
                        : (districtForm.markers.length > 0 ? 0 : -1)
        root.resetView(districtForm.canvas)
        root.level = 3
    }

    /// L'ilot choisi : la carte du quartier telle que le jeu la dessine, cadree sur lui. Apres les
    /// ilots viennent les sous-zones du quartier, qui s'ouvrent sur leur carte (LOT-121).
    function openBlock(index) {
        if (index >= root.blocks.length && index < districtForm.markers.length) {
            root.openZone(index - root.blocks.length)
            return
        }
        if (index < 0 || index >= root.blocks.length)
            return
        root.blockIndex = index
        blockForm.imageSource = root.districts.blockImage(
            root.district.mapId, root.blocks[index].blockId,
            root.heroInDistrict ? WorldModel.heroFigure : "",
            WorldModel.heroColumn, WorldModel.heroRow)
        root.level = 4
    }

    /// La sous-zone choisie (l'Arena of Fate dans Arenarea) : sa carte, a la place du quartier.
    function openZone(index) {
        if (index < 0 || index >= districtForm.zones.length || districtForm.zones[index].image === "")
            return
        root.blockIndex = root.blocks.length + index
        root.zoneIndex = index
        root.resetView(districtForm.canvas)
    }

    /// Le geste « ouvrir » : la region choisie, la ville si elle a un plan, le quartier s'il a sa
    /// carte, puis l'ilot.
    function confirm() {
        if (root.level === 0)
            root.openRegion(root.regionIndex)
        else if (root.level === 1)
            root.openCity(root.placeIndex)
        else if (root.level === 2)
            root.openDistrict(root.pointIndex)
        else if (root.level === 3 && root.zoneIndex < 0)
            root.openBlock(root.blockIndex)
    }

    /// Le geste « remonter » : d'un niveau, et hors de l'ecran depuis le monde.
    function back() {
        if (root.level === 3 && root.zoneIndex >= 0) {
            // De la sous-zone, on remonte a son quartier, son repere choisi.
            root.zoneIndex = -1
            root.resetView(districtForm.canvas)
        } else if (root.level > 0)
            root.level -= 1
        else
            ScreenRouter.closeRpgScreen()
    }

    /// Ouverture directe d'un niveau, pour verifier une vue sans la parcourir :
    /// `--map-region=<region>` et, avec elle, `--map-city=<lieu>`, puis `--map-district=<quartier>`
    /// et `--map-block=<ilot>` (LOT-96) ou `--map-zone=<sous-zone>` (LOT-121). @return Vrai si un
    /// niveau a ete demande.
    function openFromArguments() {
        let regionId = ""
        let cityId = ""
        let districtId = ""
        let blockId = ""
        let zoneId = ""
        for (const argument of Qt.application.arguments) {
            if (argument.startsWith("--map-region="))
                regionId = argument.substring(13)
            else if (argument.startsWith("--map-city="))
                cityId = argument.substring(11)
            else if (argument.startsWith("--map-district="))
                districtId = argument.substring(15)
            else if (argument.startsWith("--map-block="))
                blockId = argument.substring(12)
            else if (argument.startsWith("--map-zone="))
                zoneId = argument.substring(11)
        }
        if (regionId === "" || atlas.regionIndex(regionId) < 0)
            return false
        root.openRegion(atlas.regionIndex(regionId))
        const index = root.places.findIndex((place) => place.placeId === cityId)
        if (index < 0)
            return true
        root.openCity(index)
        const chosen = root.points.findIndex((point) => point.pointId === districtId)
        if (chosen < 0)
            return true
        root.select(chosen)
        root.openDistrict(chosen)
        const block = root.blocks.findIndex((block) => block.blockId === blockId)
        if (block >= 0)
            root.openBlock(block)
        const zone = districtForm.zones.findIndex((zone) => zone.zoneId === zoneId)
        if (zone >= 0)
            root.openZone(zone)
        return true
    }

    /// L'ecran s'ouvre la ou l'on est : sur le plan de la ville, le quartier du heros choisi.
    function openWhereTheHeroIs() {
        const cityId = WorldModel.cityLocation
        if (WorldModel.districtId === "" || cityId === "")
            return
        const regionIndex = root.regions.findIndex(
            (region) => region.places.some((place) => place.placeId === cityId))
        if (regionIndex < 0)
            return
        root.openRegion(regionIndex)
        root.openCity(root.places.findIndex((place) => place.placeId === cityId))
        const chosen = root.points.findIndex((point) => point.pointId === WorldModel.districtId)
        if (chosen >= 0)
            root.select(chosen)
    }

    // --- Selection ----------------------------------------------------------------------------------

    function markersOf(level) {
        if (level === 0)
            return root.regions
        if (level === 1)
            return root.places.filter((place) => place.placed)
        if (level === 2)
            return root.points
        return level === 3 && root.zoneIndex < 0 ? districtForm.markers : []
    }

    function selection() {
        return root.level === 0 ? root.regionIndex : root.level === 1 ? root.placeIndex
             : root.level === 2 ? root.pointIndex
             : root.level === 3 && root.zoneIndex < 0 ? root.blockIndex : -1
    }

    function select(index) {
        if (root.level === 0)
            root.regionIndex = index
        else if (root.level === 1)
            root.placeIndex = index
        else if (root.level === 2)
            root.pointIndex = index
        else if (root.level === 3)
            root.blockIndex = index
        root.reveal(index)
    }

    /// Le repere voisin dans la direction (dx, dy) : le plus proche dans l'axe, l'ecart lateral
    /// comptant double -- sans quoi « droite » sauterait a un repere presque a la verticale.
    function step(dx, dy) {
        const markers = root.markersOf(root.level)
        const current = root.selection()
        if (markers.length === 0)
            return
        if (current < 0 || current >= markers.length) {
            root.select(0)
            return
        }
        let best = -1
        let bestCost = Infinity
        for (let index = 0; index < markers.length; ++index) {
            const along = (markers[index].x - markers[current].x) * dx
                        + (markers[index].y - markers[current].y) * dy
            const across = Math.abs((markers[index].x - markers[current].x) * dy)
                         + Math.abs((markers[index].y - markers[current].y) * dx)
            if (index === current || along <= 0.005)
                continue
            const cost = along + 2 * across
            if (cost < bestCost) {
                bestCost = cost
                best = index
            }
        }
        if (best >= 0)
            root.select(best)
    }

    /// Le lieu suivant ou precedent de la LISTE : le seul chemin vers un lieu sans repere.
    function cycle(delta) {
        const count = root.level === 1 ? root.places.length : root.markersOf(root.level).length
        if (count > 0)
            root.select((root.selection() + delta + count) % count)
    }

    // --- Vue ----------------------------------------------------------------------------------------

    function clampView(canvas) {
        if (!canvas)
            return
        const flick = canvas.flick
        flick.contentX = Math.max(-flick.leftMargin,
                                  Math.min(flick.contentWidth - flick.width, flick.contentX))
        flick.contentY = Math.max(-flick.topMargin,
                                  Math.min(flick.contentHeight - flick.height, flick.contentY))
    }

    function resetView(canvas) {
        if (!canvas)
            return
        canvas.zoom = 1
        canvas.flick.contentX = -canvas.flick.leftMargin
        canvas.flick.contentY = -canvas.flick.topMargin
    }

    /// Agrandit d'un pas, en gardant au meme endroit de la carte le point (px, py) de l'ecran.
    function zoomBy(factor, px, py) {
        const canvas = root.canvas
        if (!canvas)
            return
        const flick = canvas.flick
        const next = Math.max(1, Math.min(canvas.maximumZoom, canvas.zoom * factor))
        const fx = (flick.contentX + px) / canvas.mapWidth
        const fy = (flick.contentY + py) / canvas.mapHeight
        canvas.zoom = next
        flick.contentX = fx * canvas.mapWidth - px
        flick.contentY = fy * canvas.mapHeight - py
        root.clampView(canvas)
    }

    /// Amene le repere choisi a l'ecran quand la carte agrandie l'a laisse dehors.
    function reveal(index) {
        const markers = root.markersOf(root.level)
        if (index < 0 || index >= markers.length)
            return
        const canvas = root.canvas
        if (!canvas)
            return
        const flick = canvas.flick
        const x = markers[index].x * canvas.mapWidth - flick.contentX
        const y = markers[index].y * canvas.mapHeight - flick.contentY
        const margin = 120 * Tokens.uiScale
        if (x < flick.leftMargin + margin || x > flick.width - margin
                || y < margin || y > flick.height - margin) {
            flick.contentX = markers[index].x * canvas.mapWidth - (flick.width + flick.leftMargin) / 2
            flick.contentY = markers[index].y * canvas.mapHeight - flick.height / 2
            root.clampView(canvas)
        }
    }

    // --- Cablage des formulaires ---------------------------------------------------------------------

    function wire(form, level) {
        root.wireHud(form, level)
        form.canvas.backRequested.connect(root.back)
        form.canvas.positionMarked.connect((x, y) => {
            if (root.level === level)
                console.info("Carte : [" + x.toFixed(3) + ", " + y.toFixed(3) + "] sur " + form.canvas.image)
        })
        form.canvas.markerHovered.connect((index, inside) => {
            if (inside && root.level === level)
                root.select(index)
        })
        form.canvas.markerActivated.connect((index) => {
            if (root.level !== level)
                return
            root.select(index)
            root.confirm()
            root.forceActiveFocus()
        })
    }

    function wireHud(form, level) {
        const hud = form.hud
        hud.characterName = Qt.binding(() => root.sheet.name)
        hud.levelText = Qt.binding(() => qsTr("Niv. %1").arg(root.sheet.level))
        hud.hitPointsText = Qt.binding(() => root.sheet.hitPoints)
        hud.hitPointsRatio = Qt.binding(() => {
            const current = parseInt(root.sheet.hitPoints, 10)
            const maximum = parseInt(root.sheet.hitPointsMax, 10)
            return maximum > 0 && !isNaN(current) ? Math.max(0, Math.min(1, current / maximum)) : 0
        })
        hud.experienceText = Qt.binding(
            () => root.sheet.experience + " / " + PendingData.value("world_map.experience.next_level"))
        hud.experienceRatio = 0
        hud.portrait = PendingData.image("world_map.character.portrait")
        hud.clock = PendingData.value("world_map.clock")
        // Ou se tient le groupe (LOT-96) : la carte que parcourt le heros.
        hud.partyLocation = Qt.binding(() => WorldModel.loaded
                                       ? WorldModel.mapName
                                       : PendingData.value("world_map.party.location"))
        hud.hint = Qt.binding(() => level === 4
            ? qsTr("Retour : remonter")
            : qsTr("Flèches : choisir  ·  Entrée : ouvrir  ·  Retour : remonter  ·  + / − : agrandir"))

        hud.zoomInRequested.connect(() => root.zoomBy(1.25, root.width / 2, root.height / 2))
        hud.zoomOutRequested.connect(() => root.zoomBy(1 / 1.25, root.width / 2, root.height / 2))
        hud.backRequested.connect(root.back)
        hud.questsRequested.connect(() => ScreenRouter.openRpgScreen(ScreenRouter.QuestJournal))
        hud.inventoryRequested.connect(() => ScreenRouter.openRpgScreen(ScreenRouter.Inventory))
        hud.companyRequested.connect(() => ScreenRouter.openRpgScreen(ScreenRouter.Company))
        hud.optionsRequested.connect(() => ScreenRouter.openOptions())
    }

    Component.onCompleted: {
        atlas.load()
        sheet.loadDemonstrationCharacter()
        root.wire(worldForm, 0)
        root.wire(regionForm, 1)
        root.wire(cityForm, 2)
        root.wire(districtForm, 3)
        root.wireHud(blockForm, 4)
        // La carte s'ouvre sur l'Empire central, coeur de l'atlas et du premier jalon -- ou la ou
        // se tient le heros, s'il parcourt un quartier.
        root.regionIndex = Math.max(0, atlas.regionIndex("central-empire"))
        if (!root.openFromArguments())
            root.openWhereTheHeroIs()
    }

    Connections {
        target: regionForm.panel
        function onEntryChosen(index) { root.select(index); root.forceActiveFocus() }
        function onEntryActivated(index) { root.select(index); root.confirm(); root.forceActiveFocus() }
    }
    Connections {
        target: cityForm.panel
        function onEntryChosen(index) { root.select(index); root.forceActiveFocus() }
        function onEntryActivated(index) { root.select(index); root.confirm(); root.forceActiveFocus() }
    }
    Connections {
        target: districtForm.panel
        function onEntryChosen(index) { root.select(index); root.forceActiveFocus() }
        function onEntryActivated(index) { root.select(index); root.confirm(); root.forceActiveFocus() }
    }

    WheelHandler {
        target: null
        onWheel: (event) => root.zoomBy(event.angleDelta.y > 0 ? 1.15 : 1 / 1.15,
                                        point.position.x, point.position.y)
    }

    // Le releve de position (Ctrl+clic) part de `MapCanvas` (voir sa note en tete de fichier) et
    // rejoint le journal via `wire()`, comme `backRequested` et les reperes.

    GamepadNavigator {
        active: root.visible
        onPressed: (button) => {
            switch (button) {
            case "up": root.step(0, -1); break
            case "down": root.step(0, 1); break
            case "left": root.step(-1, 0); break
            case "right": root.step(1, 0); break
            case "a": root.confirm(); break
            case "b": root.back(); break
            case "x": root.zoomBy(1.25, root.width / 2, root.height / 2); break
            case "y": root.zoomBy(1 / 1.25, root.width / 2, root.height / 2); break
            case "lb": root.cycle(-1); break
            case "rb": root.cycle(1); break
            }
        }
    }

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_Up: root.step(0, -1); break
        case Qt.Key_Down: root.step(0, 1); break
        case Qt.Key_Left: root.step(-1, 0); break
        case Qt.Key_Right: root.step(1, 0); break
        case Qt.Key_Tab:
        case Qt.Key_PageDown: root.cycle(1); break
        case Qt.Key_Backtab:
        case Qt.Key_PageUp: root.cycle(-1); break
        case Qt.Key_Return:
        case Qt.Key_Enter: root.confirm(); break
        case Qt.Key_Backspace:
        case Qt.Key_Escape: root.back(); break
        case Qt.Key_Plus:
        case Qt.Key_Equal: root.zoomBy(1.25, root.width / 2, root.height / 2); break
        case Qt.Key_Minus: root.zoomBy(1 / 1.25, root.width / 2, root.height / 2); break
        default: return
        }
        event.accepted = true
    }
}
