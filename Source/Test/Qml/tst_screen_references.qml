import QtQuick
import QtTest
import Jadg.Ui

/*!
    Chaque ecran de Jadg.Ui ressemble a sa capture de reference (regression visuelle).

    Le pack UI et les pieces 9-patch de la charte v2 sont sensibles au pixel : un jeton de couleur
    change, une marge qui glisse, une piece qui ne se charge plus -- rien de cela ne fait echouer
    qmllint ni un test de comportement. L'ecran est rendu a 960 x 540 (echelle 0,5 de la conception
    1080p, pour des references legeres), en rendu logiciel, puis compare a References/<Ecran>.png.

    Une modification VOULUE d'un ecran se valide en regenerant sa reference et en relisant l'image
    dans le diff de la PR : JADG_UPDATE_REFERENCES=1, puis ctest -R QmlTests.
*/
TestCase {
    id: testCase

    name: "ScreenReferences"
    width: 960
    height: 540
    visible: true
    when: windowShown

    Item {
        id: stage

        width: 960
        height: 540
    }

    function initTestCase() {
        Tokens.uiScale = 0.5
    }

    function cleanupTestCase() {
        Tokens.uiScale = 1
        Tokens.hudScale = 1
    }

    function test_ecran_data() {
        return jadgUiFiles.filter(function (file) {
            return file.indexOf("Screens/") === 0
        }).map(function (file) {
            const type = file.substring(file.lastIndexOf("/") + 1)
            return { tag: type, type: type }
        }).concat([
            {tag: "OptionsHud", type: "OptionsForm", properties: {currentTab: 1}},
            {tag: "CombatHudLarge", type: "CombatHudForm", factor: 1.3},
            {tag: "ExplorationHudSmall", type: "GameViewForm", factor: 0.75}
        ])
    }

    function test_ecran(data) {
        Tokens.hudScale = data.factor || 1
        const screen = createTemporaryQmlObject("import QtQuick\nimport Jadg.Ui\n" + data.type
                                                + " { anchors.fill: parent }", stage, data.tag)
        verify(screen !== null)
        if (data.properties) {
            for (const key of Object.keys(data.properties)) screen[key] = data.properties[key]
        }
        // Images chargees (asynchrones ou non) et une image rendue apres elles.
        tryVerify(function () { return allImagesReady(screen) }, 5000, "images non chargées")
        waitForRendering(screen, 5000)
        const file = captureDirectory + "/" + data.tag + ".capture.png"
        let saved = undefined
        verify(stage.grabToImage(function (grab) { saved = grab.saveToFile(file) }))
        tryVerify(function () { return saved !== undefined }, 5000, "capture non rendue")
        verify(saved, "capture non écrite : " + file)
        const result = referenceImages.compare(file, data.tag)
        verify(result.ok, data.type + " : " + result.message)
    }

    function allImagesReady(item) {
        if (item.status !== undefined && item.progress !== undefined && item.status === Image.Loading)
            return false
        for (let i = 0; i < item.children.length; ++i) {
            if (!allImagesReady(item.children[i]))
                return false
        }
        return true
    }
}
