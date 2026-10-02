import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Options -- CABLAGE, cote developpeur (LOT-86, LOT-87 T3.2).

    Les reglages sont REELS. `EX-IHM-083` l'exige : un reglage expose doit atteindre le moteur, et
    une case a cocher qui ne fait rien est pire qu'une case absente -- elle fait croire a un
    reglage, et l'utilisateur cherche ensuite pourquoi il n'a pas d'effet.

    **Les valeurs en attente sont les controles eux-memes.** Ils partent des reglages enregistres ;
    « Appliquer » les ecrit dans `OptionsModel` (qui persiste et publie, et l'application branche
    chaque signal sur ce qu'il atteint) ; « Annuler » et `Echap` ferment l'ecran sans rien ecrire ;
    « Par defaut » pose les valeurs d'usine du modele, sans les ecrire non plus. Aucune copie des
    reglages ici : une seconde copie aurait fini par diverger des controles qu'elle doublait.

    Onglets : `PageUp` et `PageDown`, ou le pointeur.
*/
OptionsForm {
    id: root

    focus: true
    logsEnabled: OptionsModel.logsAvailable
    logsMessage: root.lastLogsMessage

    // `dirty` compare chaque controle a son reglage enregistre : il retombe tout seul apres
    // « Appliquer », puisque le modele prend alors la valeur du controle.
    dirty: root.fullscreenCheck.checked !== OptionsModel.fullscreen
           || root.vsyncCheck.checked !== OptionsModel.vsync
           || root.diagnosticsCheck.checked !== OptionsModel.diagnostics
           || Math.round(root.volumeSlider.value) !== OptionsModel.volume
           || Math.round(root.hudScaleSlider.value) !== OptionsModel.hudScalePercent
           || OptionsModel.languages[root.languageBox.currentIndex] !== OptionsModel.language

    property string lastLogsMessage: ""

    Component.onCompleted: {
        // Le NOM s'affiche, le CODE se pose : les deux listes ont le meme ordre, et c'est le rang
        // choisi qui fait le lien entre elles.
        root.languageBox.model = OptionsModel.languageNames
        root.load(OptionsModel)
    }

    /// Pose dans les controles un jeu de valeurs : les reglages enregistres (`OptionsModel`) ou
    /// les valeurs d'usine (`OptionsModel.defaults`) -- les deux portent les memes noms.
    function load(values) {
        root.fullscreenCheck.checked = values.fullscreen
        root.vsyncCheck.checked = values.vsync
        root.diagnosticsCheck.checked = values.diagnostics
        root.volumeSlider.value = values.volume
        root.hudScaleSlider.value = values.hudScalePercent
        root.languageBox.currentIndex = OptionsModel.languages.indexOf(values.language)
    }

    function apply() {
        OptionsModel.fullscreen = root.fullscreenCheck.checked
        OptionsModel.vsync = root.vsyncCheck.checked
        OptionsModel.diagnostics = root.diagnosticsCheck.checked
        OptionsModel.volume = Math.round(root.volumeSlider.value)
        OptionsModel.hudScalePercent = Math.round(root.hudScaleSlider.value)
        OptionsModel.language = OptionsModel.languages[root.languageBox.currentIndex]
    }

    Keys.onEscapePressed: ScreenRouter.closeOptions()
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_PageDown) {
            root.currentTab = (root.currentTab + 1) % 3
            event.accepted = true
        } else if (event.key === Qt.Key_PageUp) {
            root.currentTab = (root.currentTab + 2) % 3
            event.accepted = true
        }
    }

    Connections {
        target: root.generalTab
        function onClicked() { root.currentTab = 0 }
    }
    Connections {
        target: root.graphicsTab
        function onClicked() { root.currentTab = 1 }
    }
    Connections {
        target: root.audioTab
        function onClicked() { root.currentTab = 2 }
    }

    Connections {
        target: root.saveLogsButton
        function onClicked() { root.lastLogsMessage = OptionsModel.saveLogs() }
    }
    Connections {
        target: root.defaultButton
        function onClicked() { root.load(OptionsModel.defaults) }
    }
    Connections {
        target: root.cancelButton
        function onClicked() { ScreenRouter.closeOptions() }
    }
    Connections {
        target: root.applyButton
        function onClicked() { root.apply() }
    }
}
