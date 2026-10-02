import QtQuick
import QtTest
import Jadg.Ui

TestCase {
    id: tests
    name: "HudScaling"
    width: 1280
    height: 720
    visible: true
    when: windowShown
    Item { id: stage; anchors.fill: parent }
    SignalSpy { id: clicks; signalName: "clicked" }

    function cleanup() {
        clicks.target = null
        Tokens.hudScale = 1
        Tokens.uiScale = 1
    }
    function test_scaling_data() {
        const rows = []
        for (const size of [960, 1280, 1920]) {
            for (const factor of [0.75, 1, 1.3]) {
                for (const form of ["GameViewForm", "CombatHudForm"])
                    rows.push({tag: form + "-" + size + "-" + factor, form: form, size: size, factor: factor})
            }
        }
        return rows
    }
    function bounds(item, screen) {
        const top = item.mapToItem(screen, 0, 0)
        const bottom = item.mapToItem(screen, item.width, item.height)
        return {x: top.x, y: top.y, right: bottom.x, bottom: bottom.y}
    }
    function contained(item, screen) {
        const rect = bounds(item, screen)
        verify(rect.x >= -1 && rect.y >= -1 && rect.right <= screen.width + 1 && rect.bottom <= screen.height + 1,
               item.objectName + " escapes viewport: " + JSON.stringify(rect))
        return rect
    }
    function test_scaling(data) {
        tests.width = data.size
        tests.height = data.size * 9 / 16
        Tokens.uiScale = data.size / 1920
        Tokens.hudScale = data.factor
        const screen = createTemporaryQmlObject("import QtQuick; import Jadg.Ui; " + data.form + " { anchors.fill: parent }", stage)
        verify(screen !== null)
        wait(30)
        compare(screen.viewportHost.width, data.size)
        compare(screen.viewportHost.height, data.size * 9 / 16)
        const nav = findChild(screen, "hudNavigation")
        contained(nav, screen)
        if (data.form === "CombatHudForm") {
            contained(findChild(screen, "combatInitiative"), screen)
            contained(findChild(screen, "combatActionDock"), screen)
        } else {
            const party = contained(findChild(screen, "explorationPartyDock"), screen)
            const navigation = bounds(nav, screen)
            verify(party.right < navigation.x, "party and navigation overlap")
        }
        clicks.target = screen.characterButton
        clicks.clear()
        mouseClick(screen.characterButton, screen.characterButton.width / 2, screen.characterButton.height / 2)
        compare(clicks.count, 1, "scaled button must remain clickable")
    }
}
