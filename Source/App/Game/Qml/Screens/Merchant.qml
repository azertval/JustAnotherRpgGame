import QtQuick
import Jadg.Ui
import Jadg.Runtime

/*!
    Marchand -- CABLAGE, cote developpeur (LOT-86).

    Alimente par le lot du peuplement civil : marchands et leurs stocks.

    Chaque liaison porte une CLE D'ATTRIBUTION nommee, qui aboutit a l'ancre `PendingData`. Le
    jour ou le lot fonctionnel arrive, il remplace ici `PendingData` par sa vraie vue-modele :
    le formulaire ne bouge pas, et la mise en page decidee aujourd'hui est conservee telle quelle.

    `python scripts/i18n/list_pending_bindings.py` releve ces cles depuis le QML : l'inventaire de ce
    qu'il reste a brancher est DERIVE du code, donc toujours exact.
*/
MerchantForm {
    pending: true

    goods: PendingData.rows("merchant.goods", 8)
    gold: PendingData.value("merchant.purse.gold")
    bag: PendingData.rows("merchant.bag", 8)
}
