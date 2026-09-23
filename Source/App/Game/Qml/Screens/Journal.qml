import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Journal de quetes -- CABLAGE, cote developpeur (LOT-86).

    Alimente par le lot des quetes, qui n'est pas ecrit.

    Chaque liaison porte une CLE D'ATTRIBUTION nommee, qui aboutit a l'ancre `PendingData`. Le
    jour ou le lot fonctionnel arrive, il remplace ici `PendingData` par sa vraie vue-modele :
    le formulaire ne bouge pas, et la mise en page decidee aujourd'hui est conservee telle quelle.

    `python scripts/i18n/list_pending_bindings.py` releve ces cles depuis le QML : l'inventaire de ce
    qu'il reste a brancher est DERIVE du code, donc toujours exact.
*/
JournalForm {
    pending: true

    quests: PendingData.rows("journal.quests", 8)
    detail: PendingData.value("journal.quest_detail")
    objectives: PendingData.rows("journal.objectives", 4)
}
