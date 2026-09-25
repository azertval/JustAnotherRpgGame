import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Dialogue -- CABLAGE, cote developpeur (LOT-86, alimente au LOT-15).

    Chaque propriete du formulaire se lit de `DialogueModel`, chaque reponse cliquee devient
    `DialogueModel.choose`. Le modele ne decide rien de la conversation : c'est
    `core::DialogueRunner` qui la joue, et ce qui se voit ici est relu apres chaque geste.

    **Le dialogue ouvert est celui du PNJ a qui l'on parle** (LOT-09) : la carte le nomme, le
    routeur le transporte (`ScreenRouter.dialogueId`), et cet ecran n'a plus aucun identifiant
    ecrit en dur. Ouvert depuis le selecteur de developpement, sans PNJ, il n'a rien a jouer et le
    dit.

    Un PNJ peut ENVOYER SE BATTRE : l'action `startCombat` de son dialogue (le heraut du Colisee)
    devient `combatRequested`, et l'ecran ouvre alors le Colisee. On y joue sur la zone de combat
    de la carte, et l'on revient ici -- la carte est restee ce qu'elle etait.

    Un PNJ peut aussi ENGAGER LE COMBAT ICI MEME (LOT-118) : l'action `startEncounter` (le maitre
    d'arene) devient `encounterRequested`, la rencontre se monte sur la zone de combat de la carte
    (`EncounterModel.begin`) et l'affichage de combat s'ouvre par-dessus la carte gelee. Si la
    carte ne peut pas l'accueillir, le modele le dit et l'exploration continue.

    Un PNJ peut enfin TERMINER LA DEMO (LOT-119) : l'action `endDemo` (la mere, a la fin de la
    quete) devient `demoEnded`, et l'ecran « Fin de la demo » s'ouvre sur la voie nommee.

    `Echap` quitte la conversation ; `1` a `9` choisissent la reponse de ce rang. La conversation
    terminee, l'ecran se referme de lui-meme.
*/
DialogueForm {
    id: root

    focus: true

    readonly property DialogueModel conversation: DialogueModel {
        dialogueId: ScreenRouter.dialogueId

        onCombatRequested: function (arenaId) {
            // Le dialogue se referme AVANT d'ouvrir le sable : sans quoi l'on reviendrait du
            // combat sur une conversation finie.
            ScreenRouter.closeRpgScreen();
            ScreenRouter.openArena();
        }

        onEncounterRequested: function (encounterId) {
            ScreenRouter.closeRpgScreen();
            if (EncounterModel.begin(encounterId)) {
                ScreenRouter.openRpgScreen(ScreenRouter.CombatHud);
            }
        }

        // La demo se termine (LOT-119) : la conversation se referme, et l'ecran de fin dit la
        // voie que le dialogue a nommee.
        onDemoEnded: function (ending) {
            ScreenRouter.closeRpgScreen();
            ScreenRouter.openDemoEnd(ending);
        }
    }

    speakerName: conversation.speakerName
    attitude: conversation.attitude
    line: conversation.line
    checkOutcome: conversation.checkOutcome
    checkTitle: conversation.checkTitle
    checkDie: conversation.checkDie
    checkDetail: conversation.checkDetail
    checkVerdict: conversation.checkVerdict
    checkSucceeded: conversation.checkSucceeded
    replies: conversation.replies

    onReplyChosen: (rowId) => conversation.choose(rowId)

    Connections {
        target: root.conversation

        function onChanged() {
            if (root.conversation.finished) {
                ScreenRouter.closeRpgScreen();
            }
        }
    }

    Keys.onEscapePressed: ScreenRouter.closeRpgScreen()
    Keys.onPressed: (event) => {
        if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
            root.conversation.chooseAt(event.key - Qt.Key_1);
            event.accepted = true;
        }
    }
}
