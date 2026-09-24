import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Journal de quetes -- CABLAGE, cote developpeur (LOT-86, alimente au LOT-116).

    Chaque propriete du formulaire se lit de `QuestJournalModel`, qui ne garde aucun etat de quete :
    il relit les drapeaux de la partie, a l'ouverture et a chaque etape atteinte. Les quetes
    commencees a gauche, l'entree la plus recente et les etapes atteintes de la quete choisie a
    droite ; la choisie porte la marque `›`.

    Au clavier et a la manette : `Haut` et `Bas` changent de quete, `Echap` referme le journal.
*/
JournalForm {
    id: root

    focus: true

    readonly property QuestJournalModel journal: QuestJournalModel {}

    quests: journal.quests
    detail: journal.detail
    objectives: journal.objectives

    Keys.onUpPressed: root.journal.selectNeighbour(-1)
    Keys.onDownPressed: root.journal.selectNeighbour(1)
    Keys.onEscapePressed: ScreenRouter.closeRpgScreen()
}
