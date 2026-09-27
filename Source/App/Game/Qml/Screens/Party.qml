import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Groupe -- CABLAGE, cote developpeur (LOT-138).

    Chaque propriete du formulaire se lit de `PartyModel`, qui ne garde aucune composition : le
    groupe est a la partie (`WorldModel`), et chaque geste lui passe. Le personnage designe l'est au
    clavier ou a la manette, et reste le meme personnage quand l'ordre de marche change.

    Au clavier : `Haut` et `Bas` designent, `Entree` prend ou laisse, `M` fait mener, `Page
    precedente` et `Page suivante` avancent et reculent dans l'ordre de marche, `Echap` referme.
    A la manette : la croix designe, `A` prend ou laisse, `X` fait mener, `LB` et `RB` avancent et
    reculent, `B` referme.
*/
PartyForm {
    id: root

    focus: true

    readonly property PartyModel party: PartyModel {}
    readonly property var current: root.candidates.length > 0 ? root.candidates[root.currentIndex] : null
    readonly property bool currentIsMember: root.current !== null && root.current.rank >= 0

    members: party.members
    candidates: party.candidates
    maxSize: party.maxSize
    toggleText: root.currentIsMember ? qsTr("Laisser") : qsTr("Prendre")
    // Le dernier ne se laisse pas, un cinquieme ne se prend pas : le bouton le dit avant le refus.
    canToggle: root.current !== null && (root.currentIsMember ? root.party.size > 1
                                                              : root.party.size < root.party.maxSize)
    canLead: root.currentIsMember && !root.current.leader
    canMove: root.currentIsMember && root.party.size > 1

    function select(step) {
        if (root.candidates.length === 0)
            return;
        root.currentIndex = (root.currentIndex + step + root.candidates.length) % root.candidates.length;
    }
    function toggle() {
        if (root.current !== null)
            root.party.toggleMember(root.current.id);
    }
    function lead() {
        if (root.current !== null)
            root.party.setLeader(root.current.id);
    }
    function move(offset) {
        if (root.current !== null)
            root.party.moveMember(root.current.id, offset);
    }

    onCandidateClicked: (index) => { root.currentIndex = index }
    onToggleClicked: root.toggle()
    onLeadClicked: root.lead()
    onMoveClicked: (offset) => root.move(offset)

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_Up: root.select(-1); break
        case Qt.Key_Down: root.select(1); break
        case Qt.Key_Return:
        case Qt.Key_Enter:
        case Qt.Key_Space: root.toggle(); break
        case Qt.Key_M: root.lead(); break
        case Qt.Key_PageUp: root.move(-1); break
        case Qt.Key_PageDown: root.move(1); break
        case Qt.Key_Escape: ScreenRouter.closeRpgScreen(); break
        default: return
        }
        event.accepted = true
    }

    GamepadNavigator {
        active: root.visible
        onPressed: (button) => {
            switch (button) {
            case "up": root.select(-1); break
            case "down": root.select(1); break
            case "a": root.toggle(); break
            case "x": root.lead(); break
            case "lb": root.move(-1); break
            case "rb": root.move(1); break
            case "b": ScreenRouter.closeRpgScreen(); break
            }
        }
    }
}
