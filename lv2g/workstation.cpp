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

/* ************************************************************************************************* */

const enum ePrivilegeLevel cWorkstation::rights = PL_OPERATOR;

cWorkstation::cBatchBlocker::cBatchBlocker(cWorkstation *_po, tPSet _pFSet, cBatchBlocker * _par)
{
    counter = 0;
    isOk    = false;
    pOwner  = _po;
    pFSet   = _pFSet;
    pParent = _par;
    if (pParent != NULL) pParent->childList << this;
}

cWorkstation::cBatchBlocker::~cBatchBlocker()
{
    if (counter != 0) DERR() << counter << endl;
}

bool cWorkstation::cBatchBlocker::end(bool f)
{
    if (counter < 1) EXCEPTION(EPROGFAIL);
    --counter;
    return enforce(f);
}

bool cWorkstation::cBatchBlocker::test() const
{
    if (counter > 0)
        return false;
    if (pParent != NULL && !pParent->test())
        return false;
    return true;
}

bool cWorkstation::cBatchBlocker::enforce(bool f)
{
    if (f && test()) {
        setState();
        referState();
        return true;
    }
    return false;
}

void cWorkstation::cBatchBlocker::referState()
{
    cBatchBlocker *pRoot = this;
    while (pRoot->pParent != NULL) pRoot = pRoot->pParent;
    bool ok = getStat();
    pOwner->pUi->pushButtonSave->setEnabled(ok);
    QStringList l;
    l = getInfos();
    pOwner->pUi->textEditMsg->setHtml(l.join(sHtmlLine));
    l = getErrors();
    pOwner->pUi->textEditErr->setHtml(l.join(sHtmlLine));
}

void cWorkstation::cBatchBlocker::setState(bool f)
{
    if (!f) return;
    sErrors.clear(); sInfos.clear(); isOk = true;
    (pOwner->*pFSet)(f, sErrors, sInfos, isOk);
    foreach (cBatchBlocker *pbb, childList) {
        pbb->setState();
    }
}


const QStringList cWorkstation::cBatchBlocker::getInfos() const
{
    QStringList r;
    r << sInfos;
    foreach (cBatchBlocker *p, childList) {
        r << p->getInfos();
    }
    return r;
}
const QStringList cWorkstation::cBatchBlocker::getErrors() const
{
    QStringList r;
    r << sErrors;
    foreach (cBatchBlocker *p, childList) {
        r << p->getErrors();
    }
    return r;
}

bool cWorkstation::cBatchBlocker::getStat() const
{
    if (!isOk) return false;
    foreach (cBatchBlocker *p, childList) {
        if (!p->test()) EXCEPTION(EPROGFAIL);   // :-O
        if (!p->isOk) return false;
    }
    return true;
}

/* ------------------------------------------------------------------------------------------------- */

cWorkstation::cWorkstation(QMdiArea *parent) :
    cIntSubObj(parent),
    bbNode(this, &cWorkstation::setStatNode),
     bbNodeName(this, &cWorkstation::setStatNodeName, &bbNode),
     bbNodeSerial(this, &cWorkstation::setStatNodeSerial, &bbNode),
     bbNodeInventory(this, &cWorkstation::setStatNodeInventory, &bbNode),
     bbNodeType(this, &cWorkstation::setStatNodeType, &bbNode),
     bbPort(this, &cWorkstation::setStatPort, &bbNode),
      bbPortName(this, &cWorkstation::setStatPortName, &bbPort),
      bbPortType(this, &cWorkstation::setStatPortType, &bbPort),
       bbPortMac(this, &cWorkstation::setStatPortMac, &bbPortType),
       bbIp(this, &cWorkstation::setStatIp, &bbPortType),
       bbLink(this, &cWorkstation::setStatLink, &bbPortType),
    pUi(new Ui::wstWidget),
    pq(newQuery())
{
    pSample = NULL;
    pnp = NULL;
    pif = NULL;
    pip = NULL;
    isModify = false;
    bbNode.begin();
    // Form
    pUi->setupUi(this);
    pIpEditWidget = new cIpEditWidget(iTab(AT_DYNAMIC, AT_FIXIP));
    pUi->verticalLayoutIp->insertWidget(1, pIpEditWidget);
    // small cosmetics
    QList<int> sizes;
    sizes << 300 << 500;
    pUi->splitterMsg->setSizes(sizes);
    sizes.pop_back();
    sizes << 300 << 300 << 300;
    pUi->splitterWorkstation->setSizes(sizes);
    // Button groups
    pModifyButtons = new QButtonGroup(this);
    pModifyButtons->addButton(pUi->radioButtonNew, 0);
    pModifyButtons->addButton(pUi->radioButtonMod, 1);
    pLinkTypeButtons = new QButtonGroup(this);
    pLinkTypeButtons->addButton(pUi->radioButtonLinkTerm,  LT_TERM);
    pLinkTypeButtons->addButton(pUi->radioButtonLinkFront, LT_FRONT);
    pLinkTypeButtons->addButton(pUi->radioButtonLinkBack,  LT_BACK);

    // Select objects
    pSelNode     = new cSelectNode(pUi->comboBoxFilterZone, pUi->comboBoxFilterPlace, pUi->comboBoxNode,
                                   pUi->lineEditFilterPlace, pUi->lineEditFilterPattern, _sNul, _sNul, this);
    cRecordListModel *pnm = new cRecordListModel(_sNodes, _sNul, this);
    pnm->nullable   = true; // van NULL
    pnm->only       = true; // csak a nodes tábla kell, SnmpDevices nem
    pnm->nullIdIsAll= true; // Ha place_id = NULL ==> teljes lista
    // csak az egy portos hálózati elemek kellenek
    static const QString sNodeConstFilt = "1 = (SELECT count(*) FROM nports AS p WHERE nodes.node_id = p.node_id)";
    pnm->setConstFilter(sNodeConstFilt, FT_SQL_WHERE);
    pSelNode->setNodeModel(pnm);
    pSelNode->refresh();
    pSelPlace    = new cSelectPlace(pUi->comboBoxZone, pUi->comboBoxPlace, pUi->lineEditPlacePat, _sNul, this);
    pSelLinked   = new cSelectLinkedPort(pUi->comboBoxLinkZone, pUi->comboBoxLinkPlace, pUi->comboBoxLinkNode,
                                   pUi->comboBoxLinkPort, pLinkTypeButtons, pUi->comboBoxLinkPortShare, iTab(ES_, ES_A, ES_B),
                                   pUi->lineEditLinkPlacePat, NULL, _sNul, _sNul, this);
    pSelPlace->refresh();
    pSelPlace->setSlave(pSelLinked);
    pSelLinked->refresh();
    // on_radioButtonMod_toggled(bool)
    connect(pSelNode,   SIGNAL(nodeIdChanged(qlonglong)),       this, SLOT(selectedNode(qlonglong)));
    // on_checkBoxPlaceEqu_toggled(bool)
    connect(pSelLinked, SIGNAL(changedLink(qlonglong,int,int)), this, SLOT(linkChanged(qlonglong,int,int)));
    // on_lineEditPName_textChanged(bool)
    // on_comboBoxPType_currentIndexChanged(QString)

    bbNode.end();
}

cWorkstation::~cWorkstation()
{
    delete pq;
}

void cWorkstation::msgEmpty(QLineEdit * pLineEdit, QLabel *pLabel, const QString& fn, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    QString s = pLineEdit->text();
    QString t = pLabel->text().simplified().toLower();
    if (t.endsWith(':')) t.chop(1);
    if (s.isEmpty()) {
        sErrs << htmlWarning(QObject::trUtf8("Nincs kitöltve a %1 mező.").arg(t));
        return;
    }
    if (!fn.isEmpty()) {   // Unique?
        cPatch n;
        n.setName(fn, s);
        if (n.completion(*pq) && !(isModify && node.getId() == n.getId())) {
            cPatch *pn = cPatch::getNodeObjById(*pq, n.getId());
            sErrs << htmlError(QObject::trUtf8("A megadott %1 nem egyedi!").arg(t));
            sInfs << htmlReportNode(*pq, *pn, trUtf8("A %2 (%3) eszköznek azonos a %1-a : ").arg(t), false);
            delete pn;
            isOk = false;
        }
    }
}

void cWorkstation::setStatNode(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    (void)f;
    (void)sErrs;
    (void)sInfs;
    (void)isOk;
}

void cWorkstation::setStatNodeName(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    QString s;
    s = pUi->lineEditName->text();
    if (s.isEmpty()) {  // Missing workstation(/node) name.
        sErrs << htmlError(trUtf8("Nincs megadva a név! Az egyedi név megadása kötelező."));
        isOk = false;
        return;
    }
    cPatch *pn = cPatch::getNodeObjByName(*pq, s, EX_IGNORE);
    if (pn != NULL) {
        if (!(isModify && node.getId() == pn->getId())) { // Name is not unique
            sErrs << htmlError(trUtf8("A megadott néven már létezik egy eszköz! A névnek egyedinek kell lennie."));
            sInfs << htmlReportNode(*pq, *pn, trUtf8("A %1 megadott néven bejegyzett (%2 típusú) eszköz : "), false);
        }
        delete pn;
        return;
    }
}

void cWorkstation::setStatNodeSerial(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    msgEmpty(pUi->lineEditSerial,      pUi->labelSerial,    _sSerialNumber, sErrs, sInfs, isOk);
}

void cWorkstation::setStatNodeInventory(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    msgEmpty(pUi->lineEditInventory,   pUi->labelInventory, _sInventoryNumber, sErrs, sInfs, isOk);
}

void cWorkstation::setStatNodeType(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    msgEmpty(pUi->lineEditModelName,   pUi->labelModelName, _sNul, sErrs, sInfs, isOk);
    msgEmpty(pUi->lineEditModelNumber, pUi->labelModelNumber, _sNul, sErrs, sInfs, isOk);
}

void cWorkstation::setStatPort(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    (void)f;
    (void)sErrs;
    (void)sInfs;
    (void)isOk;
}

void cWorkstation::setStatPortName(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    (void)sInfs;
    QString s = pUi->lineEditPName->text();
    if (s.isEmpty()) {
        sErrs << htmlError(trUtf8("Nincs megadva a port név! Az név megadása kötelező."));
        isOk = false;
        return;
    }
}


void cWorkstation::setStatPortType(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;

}


void cWorkstation::setStatIp(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;

}

void cWorkstation::setStatLink(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    qlonglong lpid = pSelLinked->currentPortId();
    if (lpid == NULL_ID) {      // nincs cél port
        qlonglong nid = pSelLinked->currentNodeId();
        if (nid == NULL_ID) {   // cél eszköz se
            sErrs << htmlWarning(trUtf8("Nincs megadva link."));
            return;
        }
        cPatch *pn = cPatch::getNodeObjById(*pq, nid);
        sErrs << htmlError(trUtf8("Nincs megadva a linkelt eszköz portja."));
        sInfs << htmlReportNode(*pq, *pn, trUtf8("A következő eszköz egy portjára csatlakozna : %2 "));
        delete pn;
        isOk = false;
        return;
    }
    cPhsLink link;
    tRecordList<cPhsLink> list;
    ePhsLinkType type = (ePhsLinkType)pSelLinked->currentType();
    ePortShare   sh   = (ePortShare)pSelLinked->currentShare();
    if (link.collisions(*pq, list, lpid, type, sh)) {
        if (isModify) {   // Modosítás esetén saját magunkkal nem ütközünk
            int ix = -1;
            if (node.getId() != NULL_ID) ix = list.indexOf(_sPortId2, node.getId());
            if (ix >= 0) delete list.takeAt(ix);
        }
    }
    if (list.size() > 0) {
        sErrs << htmlWarning(trUtf8("A megadott link ütközik más linkekkel, ha menti, akkor ezek automatikusan törődnek. Visszaállításra nincs lehetőség."));
        sInfs << htmlInfo(trUtf8("A megadott link a következő link(ek)el ütközik:"))
               + linksHtmlTable(*pq, list);
    }
    if (type != LT_TERM) {  // További linkek (lánc)
        static const QString sql = "SELECT * from next_phs_link(NULL,?,?,?)";
        list.clear();
        link.clear();
        link.setId(_sPortId2,      lpid);
        link.setId(_sPhsLinkType2, type);
        QString ssh = portShare(sh);
        QString msg;
        while (execSql(*pq, sql, link.get(_sPortId2), link.get(_sPhsLinkType2), ssh)) {
            if (pq->value(0).isNull()) break; // vége (beolvassa a NULL rekordot is, mert függvényt hívunk)
            link.set(*pq);
            list << link;
            ssh = link.getName(_sPortShared);
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
            msg += linksHtmlTable(*pq, list);
            if (link.getId(_sPhsLinkType2) != LT_TERM && list.size() > 10) msg += htmlError("...");
        }
        if (!msg.isEmpty()) sInfs << msg;
    }
}

void cWorkstation::node2gui()
{
    pnp = NULL;
    pif = NULL;
    pip = NULL;
    bbNode.begin();
    pl.clear();
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
    qlonglong pid = pnp->getId();
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
        pIpEditWidget->set(pip);
    }
    // link
    bool link = pl.isExist(*pq, pid, LT_TERM);
    pSelLinked->setLink(pl);
    if (link) {
        qlonglong plid = pSelLinked->currentPlaceId();
        if (pSelNode->currentPlaceId() == plid) {
            pUi->checkBoxPlaceEqu->setChecked(true);
        }
        else {
            pUi->checkBoxPlaceEqu->setChecked(false);
        }
    }
    bbNode.end(true);
}

// SLOTS:

void cWorkstation::on_radioButtonMod_toggled(bool checked)
{
    isModify = checked;
    bbNode.enforce();
}

void cWorkstation::selectedNode(qlonglong id)
{
    pDelete(pSample);
    if (id == NULL_ID) return;
    qlonglong nid = pSelNode->currentNodeId();
    pSample = cPatch::getNodeObjById(*pq, nid)->reconvert<cNode>();
    node.clear();
    node.set(*pSample);
    node.fetchPorts(*pq, CV_PORTS_ADDRESSES);
    if (node.ports.size() != 1) EXCEPTION(EPROGFAIL);   // Elvileg az egy portosokat olvastuk be.
    pUi->radioButtonMod->setEnabled(true);
    // Display fields
    node2gui();
}

void cWorkstation::on_checkBoxPlaceEqu_toggled(bool checked)
{
    pSelPlace->setSlave(checked ? pSelLinked : NULL);   // -> linkedNodeIdChanged();
}


void cWorkstation::linkChanged(qlonglong _pid, int _lt, int _sh)
{
    (void)_pid;
    (void)_lt;
    (void)_sh;
    bbLink.enforce(true);
}


void cWorkstation::on_lineEditPName_textChanged(const QString &arg1)
{
    (void)arg1;
    bbPortName.enforce(true);
}

void cWorkstation::on_comboBoxPType_currentIndexChanged(const QString &arg1)
{
    const cIfType& ifType = cIfType::ifType(arg1);  // New port type (ifType)
    QString ot = ifType.getName(_sIfTypeObjType);  // New object type name
    bool isInterface;
    if      (ot == _sNPort)     isInterface = false;    // new type is interface
    else if (ot == _sInterface) isInterface = true;     // new type is passive port
    else                        EXCEPTION(EDATA, 0, ifType.identifying(false));
    bool changeType = isInterface == (pif == NULL);     // Changed interface object típe?
    int  linkType   = (int)ifType.getId(_sIfTypeLinkType);  // New port link type
    bool isLinkage  = linkType == LT_PTP || linkType == LT_BUS; // Linkable?
    bool togleLink  = isLinkage != pl.isNull();         // Change link ? !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    pnp->setId(_sIfTypeId, ifType.getId());
    if (!(togleLink || changeType)) return;             // No any other change
    bbPortType.begin();
    if (changeType) { // Change port object type
        if (isInterface) {  // cNPort => cInterface
            pif = new cInterface;
            pif->set(*pnp);
            node.ports[0] = pif;
            delete pnp;
            pnp = pif;
            pip = new cIpAddress;
            pif->addresses << pip;
            pIpEditWidget->set(pip);
        }
        else {              // cInterface => cNPort
            pnp = new cNPort;
            pnp->set(*pnp);
            node.ports[0] = pif;
            delete pif;
            pif = NULL;
            pip = NULL;
        }
        pUi->lineEditPMAC->setEnabled(isInterface);
        pIpEditWidget->setAllDisabled(!isInterface);
    }
    if (togleLink) {
        pl.clear();
        if (isLinkage) {    // Enable link
        }
        else {              // Disable link
        }
        pSelLinked->setEnabled(isLinkage);
    }

    bbPortType.end();
}
