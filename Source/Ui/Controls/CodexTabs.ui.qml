import QtQuick
import Jadg.Ui

/*! Navigation commune des pages du mercenaire. Le contenu et les gestes restent dans les écrans. */
Row {
    id: root
    property int currentIndex: 0
    signal requested(int index)
    spacing: 6 * Tokens.uiScale
    OrnateTab { id: characterTab; width: 250 * Tokens.uiScale; text: qsTr("Personnage"); checked: root.currentIndex === 0; checkable: false }
    Connections { target: characterTab; function onClicked() { root.requested(0) } }
    OrnateTab { id: classTab; width: 340 * Tokens.uiScale; text: qsTr("Capacités de classe"); checked: root.currentIndex === 1; checkable: false }
    Connections { target: classTab; function onClicked() { root.requested(1) } }
    OrnateTab { id: skillsTab; width: 270 * Tokens.uiScale; text: qsTr("Compétences"); checked: root.currentIndex === 2; checkable: false }
    Connections { target: skillsTab; function onClicked() { root.requested(2) } }
    OrnateTab { id: equipmentTab; width: 270 * Tokens.uiScale; text: qsTr("Équipement"); checked: root.currentIndex === 3; checkable: false }
    Connections { target: equipmentTab; function onClicked() { root.requested(3) } }
}

