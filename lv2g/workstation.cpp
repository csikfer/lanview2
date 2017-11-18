#include "lv2widgets.h"
#include "srvdata.h"
#include "lv2user.h"
#include "lv2link.h"
#include "record_dialog.h"
#include "object_dialog.h"
#include "report.h"
#include "workstation.h"
#include "ui_wstform.h"
#include "ui_phslinkform.h"
#include "report.h"

/// \brief linkText
/// \return A link statusz (IS_OK vagy IS_COLLISION)
/// @param stat link status
/// @param modify modify status
/// @param pid  saját port ID (ha nem mentett port, akkor NULL_ID)
/// @param lpid Linkelt port ID
/// @param type Fizikai link típusa
/// @param sh   Megosztás
/// @param msg  Üzenet string referencia
static int _linkTest(QSqlQuery& q, int stat, bool modify, qlonglong _pid, qlonglong lpid, ePhsLinkType type, ePortShare _sh, QString& msg)
{
    DBGFN();
    msg.clear();
    if (stat != IS_OK && stat != IS_COLLISION) return stat;
    cPhsLink link;
    tRecordList<cPhsLink> list;
    int r;
    if (link.collisions(q, list, lpid, type, _sh)) {
        if (modify) {   // Modosítás esetén saját magunkkal nem ütközünk
            int ix = -1;
            if (_pid != NULL_ID) ix = list.indexOf(_sPortId2, _pid);
            if (ix >= 0) delete list.takeAt(ix);
        }
    }
    if (list.size() > 0) {
        msg += htmlInfo(QObject::trUtf8("A megadott link a következő link(ek)el ütközik:"));
        msg += linksHtmlTable(q, list);
        r = IS_COLLISION;
    }
    else {
        r = IS_OK;
    }
    if (type != LT_TERM) {  // További linkek
        static const QString sql = "SELECT * from next_phs_link(NULL,?,?,?)";
        list.clear();
        link.clear();
        link.setId(_sPortId2,      lpid);
        link.setId(_sPhsLinkType2, type);
        QString sh = portShare(_sh);
        while (execSql(q, sql, link.get(_sPortId2), link.get(_sPhsLinkType2), sh)) {
            if (q.value(0).isNull()) break; // vége (beolvassa a NULL rekordot is, mert függvényt hívunk)
            link.set(q);
            list << link;
            sh = link.getName(_sPortShared);
            if (list.size() > 10) break;
            if (link.getId(_sPhsLinkType2) == LT_TERM) break;
        }
        if (list.size() > 0) {
            if (link.getId(_sPhsLinkType2) == LT_TERM) {
                msg += htmlInfo(QObject::trUtf8("A linkel lánca, végponttól - végpontig:"));
            }
            else {
                msg += htmlInfo(QObject::trUtf8("A linkek lánca, csonka, nem ér el a másik végpontig :"));
            }
            msg += linksHtmlTable(q, list);
            if (link.getId(_sPhsLinkType2) != LT_TERM && list.size() > 10) msg += htmlError("...");
        }
    }
    _DBGFNL() << r << endl;
    return r;
}

/// A megadott radio button állapotának a beállítása
/// @param pGroup A QButtonGroup objektum pointere
/// @param id A kiválasztott radio button ID-je a group-ban
/// @param check Az állpot (true: checked)
/// @param enable Az engedályezése (true: engedélyezve)
static void buttonCheckEnable(QButtonGroup * pGroup, int id, bool check, bool enable)
{
    QAbstractButton *pButton = pGroup->button(id);
    pButton->setEnabled(enable);
    pButton->setCheckable(enable || check);
    pButton->setChecked(check);
}

/*static void buttonSets(QAbstractButton *p, bool e, bool c)
{
    p->setEnabled(e);
    p->setChecked(c);
}*/

/* ************************************************************************************************* */

const enum ePrivilegeLevel cWorkstation::rights = PL_OPERATOR;


cWorkstation::cWorkstation(QMdiArea *parent) :
    cIntSubObj(parent),
    pUi(new Ui::wstWidget),
    pq(newQuery())
{
    pSample = NULL;
    pnp = NULL;
    pif = NULL;
    pip = NULL;
    // Set states
    memset(&states, 0, sizeof(states)); // set all members: IS_EMPTY, false
    states.portName = IS_OK;    // A form-ban van egy alapértelmezett név: "ethernet"
    states.subNetNeed = true;   // A form-ban dinamikus cím van megadva, ahoz kell subnet;
    states.macNeed = true;      // Alapértelmezetten MAC is kel az első port (interfész)-re
    // Form
    pUi->setupUi(this);
    pIpEditWidget = new cIpEditWidget(ENUM2SET2(AT_DYNAMIC, AT_FIXIP));
    pUi->verticalLayoutIp->insertWidget(1, pIpEditWidget);
    // small cosmetics
    QList<int> sizes;
    sizes << 300 << 300;
    pUi->splitterMsg->setSizes(sizes);
    sizes << 300;
    pUi->splitterWorkstation->setSizes(sizes);
    // Button group
    pModifyButtons = new QButtonGroup(this);
    pModifyButtons->addButton(pUi->radioButtonNew, 0);
    pModifyButtons->addButton(pUi->radioButtonMod, 1);

    // Csak az egy portos eszközöket keressük, és persze nem patch paneleket (A cSelectNode a patchs táblában keres)
    static const QString sNodeConstFilt = "(NOT 'patch'::nodetype = ANY (node_type) AND 1 = (SELECT count(*) FROM nports AS p WHERE patchs.node_id = p.node_id))";
    pSelNode     = new cSelectNode(pUi->comboBoxFilterZone, pUi->comboBoxFilterPlace, pUi->comboBoxNode,
                                   pUi->lineEditFilterPlace, pUi->lineEditName, _sNul, sNodeConstFilt, this);
    pSelPlace    = new cSelectPlace(pUi->comboBoxZone, pUi->comboBoxPlace, pUi->lineEditPlacePat, _sNul, this);
    pSelNodeLink = new cSelectNode(pUi->comboBoxLinkZone, pUi->comboBoxLinkPlace, pUi->comboBoxLinkNode,
                                   pUi->lineEditLinkPlacePat, NULL, _sNul, _sNul, this);
    pSelNodeLink->refresh();
    pSelPlace->setSlave(pSelNodeLink);
    pSelNode->refresh();
    pSelPlace->refresh();
}

cWorkstation::~cWorkstation()
{
    ;
}

void cWorkstation::node2gui()
{
    pnp = NULL;
    pif = NULL;
    pip = NULL;
    pSelPlace->setCurrentPlace(node.getId(_sPlaceId));
    pUi->lineEditName->setText(node.getName());
    pUi->lineEditNote->setText(node.getNote());
    pUi->lineEditSerial->setText(node.getName(_sSerialNumber));
    pUi->lineEditInventory->setText(node.getName(_sInventoryNumber));
    pUi->lineEditModelName->setText(node.getName(_sModelName));
    pUi->lineEditModelNumber->setText(node.getName(_sModelNumber));
    pUi->lineEditNodeType->setText(node.getName(_sNodeType));
    // Display ports
    if (node.ports.isEmpty()) node.addPort(_sEthernet, _sEthernet, _sNul, NULL_IX);
    pnp = node.ports.first();
    if (0 == pnp->chkObjType<cInterface>(EX_IGNORE)) pif = pnp->reconvert<cInterface>();
    pUi->lineEditPName->setText(pnp->getName());
    pUi->lineEditPNote->setText(pnp->getNote());
    QString sIfType = cIfType::ifTypeName(pnp->getId(__sIfTypeId));
    _setCurrentIndex(sIfType, pUi->comboBoxPType, EX_IGNORE);
    pUi->lineEditPTag->setText(pnp->getName(_sPortTag));
    pUi->lineEditPMAC->setText(pif == NULL ? _sNul : pif->getName(_sHwAddress));
    // Ip
    if (pif != NULL) {
        if (pif->addresses.isEmpty()) pif->addIpAddress(QHostAddress(), AT_DYNAMIC);
        pip = pif->addresses.first();
    }
}

// SLOTS:

void cWorkstation::on_checkBoxPlaceEqu_toggled(bool checked)
{
    if (checked) {
        pSelPlace->setSlave(pSelNodeLink);
    }
    else {
        pSelPlace->disconnectSlave();
    }
}

void cWorkstation::on_comboBoxNode_currentIndexChanged(int index)
{
    pDelete(pSample);
    if (index <= 0) {   // NULL
        ;
    }
    else {
        qlonglong nid = pSelNode->currentNodeId();
        pSample = cPatch::getNodeObjById(*pq, nid)->reconvert<cNode>();
        node.clear();
        node.set(*pSample);
        node.fetchPorts(*pq, CV_PORTS_ADDRESSES);
        if (node.ports.size() != 1) EXCEPTION(EPROGFAIL);   // Elvileg az egy portosokat olvastuk be.
        // Display fields
        node2gui();
    }
}
