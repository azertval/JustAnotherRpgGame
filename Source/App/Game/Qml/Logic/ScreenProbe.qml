import QtQuick
import QtQuick.Controls
import Jadg.Runtime

/*!
    Sélecteur d'écrans -- OUTIL DE VÉRIFICATION, côté développeur (LOT-86).

    Deux boutons posés par-dessus l'interface, qui font défiler les écrans et la galerie. Ils existent
    parce que le viewport du jeu n'affiche encore aucune scène (`Source/Elements/Levels/` ne porte
    que la carte de l'arène du `LOT-50`, que l'écran du Colisée dessine lui-même) : sans eux, les sept écrans dessinés mais pas encore
    alimentés ne sont atteignables par aucun chemin de jeu, et ne se vérifient donc pas.

    **Ce n'est pas une fonctionnalité, et le code le garantit** : `visible` se lie à
    `ScreenRouter.developerBuild`, faux dans un binaire livré. Une consigne de relecture aurait
    laissé ces boutons partir avec le jeu le jour où quelqu'un oublie de les retirer.

    Son apparence n'est **pas décrite** : elle emprunte la palette ambiante, comme tout contrôle Qt.
    Ce n'est pas de la paresse mais ce qui lui évite d'écrire la moindre couleur -- `EX-IHM-105`
    tient alors sans qu'on lui invente d'exception, et une exception dans un contrôle automatique
    finit toujours par en couvrir une deuxième. Ce qui le distingue de l'interface n'est donc pas sa
    teinte mais son libellé, « n/15 · NomDÉcran », qui ne peut être pris pour du contenu de jeu.

    Il se pose en **bas** de la fenêtre : le haut porte le bandeau de titre de chaque écran, et une
    barre en travers y rendait les captures de vérification inutilisables -- justement celles qu'il
    sert à produire.

    **Il cède la main au routeur.** Sa sélection est effacée dès que le jeu navigue de lui-même
    (`clear()`, appelé par `ScreenStack`). Sans cela, un écran choisi ici serait resté épinglé :
    `Échap` n'aurait plus rien fermé, et la navigation -- ce qu'on veut justement vérifier -- aurait
    paru cassée par l'outil censé permettre de la vérifier.

    Il vit dans `Logic/` et non dans `Screens/` : rien ici n'appartient à la conception.
*/
Item {
    id: root

    /// Les écrans à parcourir, dans l'ordre du sélecteur.
    required property var screenNames

    /// L'écran choisi, ou chaîne vide tant que le sélecteur n'a pas servi -- auquel cas c'est le
    /// routeur qui décide, et le jeu s'ouvre sur son menu comme il le doit.
    property string selectedScreen: ""

    /// Rang de l'écran choisi, `-1` avant tout usage.
    property int index: -1

    readonly property int barHeight: 26

    visible: ScreenRouter.developerBuild

    /// Rend la main au routeur, en gardant le rang : ◀ et ▶ repartent d'où l'on en était.
    function clear() {
        root.selectedScreen = "";
    }

    /// Ouvre l'écran nommé, s'il est de la liste : le menu de développement (F9) passe par ici, si
    /// bien qu'un écran choisi au menu se comporte exactement comme un écran choisi aux boutons --
    /// épinglé jusqu'à ce que le routeur reprenne la main, et ◀ ▶ repartent de lui.
    function select(name) {
        const rank = root.screenNames.indexOf(name);
        if (rank < 0) {
            return;
        }
        root.index = rank;
        root.selectedScreen = name;
    }

    /// Ouvre l'écran suivant (`delta > 0`) ou précédent, en bouclant aux extrémités.
    function step(delta) {
        const count = root.screenNames.length;
        // Avant tout usage, `-1` n'est le symétrique de « position 0 » que pour `delta > 0`
        // ((-1 + 1) % count fait 0) : reculer depuis `-1` retombait un rang trop loin. Le premier
        // pas part donc d'une position explicite plutôt que du seul calcul modulo.
        root.index = root.index < 0 ? (delta > 0 ? 0 : count - 1)
                                     : (root.index + delta + count) % count;
        root.selectedScreen = root.screenNames[root.index];
    }

    // La barre est le seul endroit sensible à la souris : `root` remplit l'écran, et y poser le
    // survol aurait rendu la barre opaque dès que le pointeur entre dans la fenêtre.
    Row {
        id: bar

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 4
        spacing: 4
        height: root.barHeight
        // Effacée tant qu'on ne s'en approche pas : elle recouvre l'interface qu'elle sert à
        // regarder, et une capture d'écran ne doit pas en garder une barre opaque en travers.
        opacity: hover.hovered ? 1.0 : 0.4

        Behavior on opacity {
            NumberAnimation {
                duration: 120
            }
        }

        HoverHandler {
            id: hover
        }

        Button {
            width: 34
            height: root.barHeight
            // Le sélecteur ne prend jamais le focus : les écrans se pilotent au clavier et à la
            // manette, et un outil qui capterait les touches empêcherait de vérifier cela même.
            focusPolicy: Qt.NoFocus
            text: "◀"
            onClicked: root.step(-1)
        }

        Label {
            width: 200
            height: root.barHeight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            // Vide dès que le routeur a repris la main : afficher le dernier écran choisi aurait
            // nommé autre chose que ce qui est à l'écran.
            text: root.selectedScreen.length === 0
                  ? "écrans (dév.)"
                  : (root.index + 1) + "/" + root.screenNames.length + " · " + root.selectedScreen

            // Un fond est nécessaire : sans lui, le libellé se lirait par-dessus le parchemin
            // d'un écran, et deviendrait illisible sur la moitié d'entre eux.
            background: Rectangle {
                color: root.palette.window
                border.color: root.palette.mid
                border.width: 1
            }
        }

        Button {
            width: 34
            height: root.barHeight
            focusPolicy: Qt.NoFocus
            text: "▶"
            onClicked: root.step(1)
        }
    }
}
