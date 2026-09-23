import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Equipe de mercenaires -- CABLAGE, cote developpeur (LOT-87, T3.7).

    Remplace les jumeaux de la feuille d'equipe et du tableau de la Guilde. Seuls les MEMBRES ont
    une source : les personnages existants, c'est-a-dire le personnage de demonstration, et des
    places libres autour de lui. Tout le reste attend les lots de la compagnie (`LOT-45`, `LOT-83`)
    et porte une CLE D'ATTRIBUTION `company.*`, que `python scripts/i18n/list_pending_bindings.py` releve.

    Onglets : `PageUp` et `PageDown`, ou le pointeur.
*/
CompanyForm {
    id: root

    focus: true

    readonly property CharacterSheetModel sheet: CharacterSheetModel {}

    /// Les six places de l'equipe. Le seul personnage qui existe l'occupe ; les autres sont libres.
    members: [
        { name: sheet.name, level: sheet.level, portrait: PendingData.image("company.member.portrait") },
        { name: "", level: "", portrait: "" },
        { name: "", level: "", portrait: "" },
        { name: "", level: "", portrait: "" },
        { name: "", level: "", portrait: "" },
        { name: "", level: "", portrait: "" }
    ]

    companyName: PendingData.value("company.name")
    motto: PendingData.value("company.motto")
    coatOfArms: PendingData.image("company.coat_of_arms")
    careerPoints: PendingData.value("company.career_points")
    companyLevel: PendingData.value("company.level")
    prestige: PendingData.value("company.prestige")
    fame: PendingData.value("company.fame")

    baseName: PendingData.value("company.base.name")
    baseLevel: PendingData.value("company.base.level")
    baseDescription: PendingData.value("company.base.description")
    baseCondition: PendingData.value("company.base.condition")
    buildings: PendingData.rows("company.base.buildings", 6)
    staff: PendingData.rows("company.base.staff", 4)

    achievements: PendingData.rows("company.achievements", 5)
    specialization: PendingData.value("company.specialization")
    style: PendingData.value("company.style")
    rank: PendingData.value("company.rank")
    treasures: PendingData.rows("company.treasures", 5)

    recruits: PendingData.rows("company.recruits", 8)
    contracts: PendingData.rows("company.contracts", 6)
    contract: PendingData.value("company.contract")
    contractGiver: PendingData.value("company.contract.giver")
    contractRank: PendingData.value("company.contract.rank")
    contractReward: PendingData.value("company.contract.reward")
    reserve: PendingData.rows("company.reserve", 8)

    Component.onCompleted: sheet.loadDemonstrationCharacter()

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_PageDown) {
            root.currentTab = (root.currentTab + 1) % 4
            event.accepted = true
        } else if (event.key === Qt.Key_PageUp) {
            root.currentTab = (root.currentTab + 3) % 4
            event.accepted = true
        }
    }

    Connections {
        target: root.teamTab
        function onClicked() { root.currentTab = 0 }
    }
    Connections {
        target: root.recruitmentTab
        function onClicked() { root.currentTab = 1 }
    }
    Connections {
        target: root.contractsTab
        function onClicked() { root.currentTab = 2 }
    }
    Connections {
        target: root.reserveTab
        function onClicked() { root.currentTab = 3 }
    }
}
