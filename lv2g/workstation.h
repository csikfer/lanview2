#ifndef CWORKSTATION_H
#define CWORKSTATION_H

#include <QWidget>
#include "lv2g.h"
#include "lv2models.h"
#include "lv2link.h"
#include "lv2widgets.h"
#include "object_dialog.h"

namespace Ui {
    class wstWidget;
}
class cDialogButtons;

enum {
    IS_EMPTY = 0,   ///< Üres, nincs megadva
    IS_OK,          ///< OK
    IS_COLLISION,   ///< Más értékkel ütközik
    IS_INVALID,     ///< Nem megengedett érték (értéktattományon kívül)
    IS_LOOPBACK,    ///< Egy loopback cím lett megadva.
    IS_EXTERNAL,
    IS_SOFT_COLL,   ///< Cím ütközés, dinamikus cím, törölhető
    IS_IMPERFECT = IS_INVALID,
    IS_NONEXIST = 0,
    IS_EXIST,
    IS_ENABLE,
    IS_NOT_POSSIBLE = 0,
    IS_POSSIBLE,
    IS_MUST

};

class LV2GSHARED_EXPORT cWorkstation : public cIntSubObj
{
    Q_OBJECT
public:
    cWorkstation(QMdiArea *parent = 0);
    ~cWorkstation();
    static const enum ePrivilegeLevel rights;
private:
    void node2gui();

    Ui::wstWidget   *pUi;
    cSelectNode     *pSelNode;      /// A szerkesztendő, vagy minta eszköz kiválasztása
    cSelectPlace    *pSelPlace;     /// Az eszköz helyének a megadása
    cSelectNode     *pSelNodeLink; /// A linkelt port, ill. az eszköz kiválasztáse
    QButtonGroup    *pModifyButtons;
    cIpEditWidget   *pIpEditWidget;

    struct cStates {
        unsigned    modify:1;           // bool (false: new object, true: modify existing object)
        unsigned    nodeName:2;         // EMPTY, OK, COLLISION
        unsigned    nodePlace:1;        // EMPTY, OK
        unsigned    serialNumber:2;     // EMPTY, OK, COLLISION
        unsigned    inventoryNumber:2;  // EMPTY, OK, COLLISION
        unsigned    portName:1;         // EMPTY, OK
        unsigned    macNeed:1;          // bool
        unsigned    mac:2;              // EMPTY, OK, COLLISION, INVALID
        unsigned    ipNeed:2;           //
        unsigned    ipAddr:3;           // EMPTY, OK, COLLISION, INVALID, LOOPBACK
        unsigned    subNetNeed:1;       // bool
        unsigned    subNet:1;           // EMPTY, OK
        unsigned    linkPossible:1;     // bool
        unsigned    link:2;             // EMPTY, OK, COLLISION, IMPERFECT
    }   states;
    QSqlQuery *pq;
    /// Kiválasztott workstation objektum (modosítandó eredetije vagy minta)
    cNode      *pSample;
    /// A szerkesztett workstation objektum
    cNode       node;   ///< A munkaállomás objektum
    cNPort *    pnp;
    cInterface *pif;
    cIpAddress *pip;
private slots:
    void on_checkBoxPlaceEqu_toggled(bool checked);
    void on_comboBoxNode_currentIndexChanged(int index);
};

#endif // CWORKSTATION_H
