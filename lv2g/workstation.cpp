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

cSetDialog::cSetDialog(QString _tn, bool _tristate, qlonglong _excl, qlonglong _def, QWidget * par)
    : QDialog(par)
    , enumType(cColEnumType::get(_tn))
{
    pLayout = new QVBoxLayout;
    setLayout(pLayout);
    tristate = _tristate;
    on = off = 0;
    def = _def;
    pCheckBoxs = new QButtonGroup(this);
    pCheckBoxs->setExclusive(false);
    pGrid = new QGridLayout;
    pLayout->addLayout(pGrid);
    pButtons = new cDialogButtons(ENUM2SET2(DBT_OK, DBT_CANCEL), 0, this);
    pLayout->addWidget(pButtons->pWidget());
    int i, n = enumType.enumValues.size(), row = 0, col = 0;
    qlonglong m = 1;
    int maxrow = lv2g::getInstance()->dialogRows;
    QCheckBox *p;
    for (i = 0; i < n; ++i, m <<= 1) {
        if (m & _excl) continue;
        QString e = enumType.enum2str(i);
        p = new QCheckBox(cEnumVal::viewShort(_tn, i, e));
        p->setTristate(tristate);
        p->setCheckState(tristate ? Qt::PartiallyChecked : Qt::Unchecked);
        pCheckBoxs->addButton(p, i);
        if (row >= maxrow) {
            ++col;
            row = 0;
        }
        pGrid->addWidget(p, row, col);
        ++row;
    }
    int dce = def == 0 ? DC_NULL : DC_DEFAULT;
    p = new QCheckBox(cEnumVal::viewShort(_sDatacharacter, dce, dataCharacter(dce)));
    dcSetD(p, dce);
    p->setTristate(false);
    p->setChecked(true);
    pCheckBoxs->addButton(p, i);
    if (row >= maxrow) {
        ++col;
        row = 0;
    }
    pGrid->addWidget(p, row, col);
    if (def != 0) {
        set(def);
    }

    connect(pCheckBoxs, SIGNAL(buttonClicked(int)), this, SLOT(clickedCheckBox(int)));
    connect(pButtons,   SIGNAL(buttonClicked(int)), this, SLOT(clickedButton(int)));
}

void cSetDialog::set(qlonglong _on, qlonglong _off)
{
    on  = _on;
    off = _off;
    int i, n = enumType.enumValues.size();
    QAbstractButton *pp;
    bool isNul = on == 0 && off == 0;
    if (isNul && def != 0) {
        on = def;
        isNul = false;
    }
    qlonglong m = 1;
    for (i = 0; i < n; ++i, m <<= 1) {
        pp = pCheckBoxs->button(i);
        if (pp == NULL) {
            if (m & on) EXCEPTION(EPROGFAIL, i);
            continue;
        }
        if (tristate) {
            QCheckBox *p = qobject_cast<QCheckBox *>(pp);
            if (p  == NULL) EXCEPTION(EPROGFAIL, i);
            if      (on  & m) p->setCheckState(Qt::Checked);
            else if (off & m) p->setCheckState(Qt::Unchecked);
            else              p->setCheckState(Qt::PartiallyChecked);
        }
        else {
            pp->setChecked(on & m);
        }
    }
    pp = pCheckBoxs->button(n);
    if (pp == NULL) EXCEPTION(EPROGFAIL, i);
    pp->setChecked(isNul);
}

QString cSetDialog::toString()
{
    QString r;
    if (tristate) {
        QStringList sl;
        if (on != 0) {
            sl = enumType.set2lst(on);
            for (int i = 0; i < sl.size(); ++i) sl[i].prepend(QChar('+'));
            r = sl.join(_sCommaSp);
            if (off != 0) r += _sCommaSp;
        }
        sl = enumType.set2lst(off);
        for (int i = 0; i < sl.size(); ++i) sl[i].prepend(QChar('-'));
        r += sl.join(_sCommaSp);
    }
    else {
        QStringList sl = enumType.set2lst(on);
        r = sl.join(_sCommaSp);
    }
    return r;
}

/// Az állapot alapján egy szűrési feltételstringet állít elő.
/// @param _fn A mező neve a feltételben
/// @param _addOff Az off maszkhoz hozzáadandó bitek
/// @param _addOn Az on maszkhoz hozzáadandó bitek
QString cSetDialog::toWhere(const QString& _fn, qlonglong _addOff, qlonglong _addOn)
{
    QStringList wl;
    qlonglong m = on | _addOn;
    if (m != 0) {
        QString ons = quoted(stringListToSql(enumType.set2lst(m)).toString());
        wl << QString("%1 @> %2::%3[]").arg(_fn, ons, enumType);
    }
    m = off | _addOff;
    if (m != 0) {
        QString offs = quoted(stringListToSql(enumType.set2lst(m)).toString());
        wl << QString("NOT %1 && %2::%3[]").arg(_fn, offs, enumType);
    }
    return wl.join(" AND ");
}

void cSetDialog::clickedCheckBox(int id)
{
    int n = enumType.enumValues.size();
    if (id == n) { // NULL
        set(0, 0);
    }
    else {
        QAbstractButton *pp = pCheckBoxs->button(id);
        if (pp == NULL) EXCEPTION(EPROGFAIL, id);
        qlonglong m = enum2set(id);
        if (tristate) {
            QCheckBox *p = qobject_cast<QCheckBox *>(pp);
            if (p  == NULL) EXCEPTION(EPROGFAIL, id);
            switch (p->checkState()) {
            case Qt::Checked:           on |=  m; off &= ~m; break;
            case Qt::Unchecked:         on &= ~m; off |=  m; break;
            case Qt::PartiallyChecked:  on &= ~m; off &= ~m; break;
            default:                    EXCEPTION(EPROGFAIL, id);
            }
        }
        else {
            if (pp->isChecked()) {
                on |=  m;
                if (collisoins.find(id) != collisoins.end()) {
                    on &= ~collisoins[id];
                    autoset();
                    set(on);
                }
                else {
                    bool corr = false;
                    foreach (int e, collisoins.keys()) {
                        if (m & collisoins[e]) {    // rule?
                            if (on & enum2set(e)) { // collision (reverse)
                                on &= ~enum2set(e);
                                corr = true;
                            }
                        }
                    }
                    if (corr) {
                        autoset();
                        set(on);
                    }
                }
            }
            else {
                on &= ~m;
                if (autoset()) set(on);
            }
        }
        bool isNull = on == 0 && off == 0;
        if (isNull && def != 0) {
            set(def);
        }
        else {
            pp = pCheckBoxs->button(n);
            if (pp == NULL) EXCEPTION(EPROGFAIL, id);
            pp->setChecked(isNull);
        }
    }
    changedState(on, off);
}

bool cSetDialog::autoset()
{
    foreach (int e, autosets.keys()) {
        if (0 == (autosets[e] & on)) {
            on |= ENUM2SET(e);
            return true;
        }
    }
    return false;
}

void cSetDialog::clickedButton(int id)
{
    switch (id) {
    case DBT_OK:        accept();   break;
    case DBT_CANCEL:    reject();   break;
    }
}


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
    bool ok = pRoot->getStat();
    pOwner->pUi->pushButtonSave->setEnabled(ok);
    QStringList l;
    l = pRoot->getInfos();
    pOwner->pUi->textEditMsg->setHtml(l.join(sHtmlLine));
    l = pRoot->getErrors();
    pOwner->pUi->textEditErr->setHtml(l.join(sHtmlLine));
}

void cWorkstation::cBatchBlocker::setState(bool f)
{
    if (!f) return;
    sErrors.clear(); sInfos.clear(); isOk = true;
    (pOwner->*pFSet)(true, sErrors, sInfos, isOk);
    foreach (cBatchBlocker *pbb, childList) {
        pbb->setState(true);
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
        if (!p->getStat()) return false;
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
    bbNodeModel(this, &cWorkstation::setStatNodeType, &bbNode),
    bbPort(this, &cWorkstation::setStatPort, &bbNode),
    bbPortName(this, &cWorkstation::setStatPortName, &bbPort),
    bbPortType(this, &cWorkstation::setStatPortType, &bbPort),
    bbIp(this, &cWorkstation::setStatIp, &bbPortType),
    bbPortMac(this, &cWorkstation::setStatPortMac, &bbPortType),
    bbLink(this, &cWorkstation::setStatLink, &bbPortType),
    pUi(new Ui::wstWidget),
    pq(newQuery())
{
    filtTypeOn = ENUM2SET(NT_WORKSTATION);
    filtTypeOff= 0;
    nodeType   = ENUM2SET2(NT_HOST, NT_WORKSTATION);
    excludedNodeType = ENUM2SET4(NT_PATCH, NT_HUB, NT_SNMP, NT_VIRTUAL) | ENUM2SET3(NT_SWITCH, NT_GATEWAY, NT_CLUSTER);
    pSetDialogFiltType = new cSetDialog(_sNodetype, true, excludedNodeType, 0, this);
    pSetDialogFiltType->set(filtTypeOn, filtTypeOff);
    pSetDialogType     = new cSetDialog(_sNodetype, false, excludedNodeType, ENUM2SET2(NT_HOST, NT_WORKSTATION), this);
    pSetDialogType->set(nodeType);
    pSetDialogType->addCollision(NT_HOST, ENUM2SET(NT_NODE));
    pSetDialogType->addCollision(NT_NODE, ENUM2SET4(NT_HOST, NT_AP, NT_SERVER, NT_WINDOWS));
    pSetDialogType->addAutoset(NT_HOST, ENUM2SET2(NT_NODE, NT_HOST));
    excludedNodeType &= ~ENUM2SET(NT_PATCH); // nem kell
    pSample = NULL;
    pnp = NULL;
    pit = NULL;
    pif = NULL;
    pip = NULL;
    isModify = false;
    bbNode.begin();
    // Form
    pUi->setupUi(this);
    pIpEditWidget = new cIpEditWidget(iTab(AT_DYNAMIC, AT_FIXIP));
    pUi->verticalLayoutIp->insertWidget(1, pIpEditWidget);
    pIpEditWidget->showToolBoxs(true, true, true);
    pIpEditWidget->enableToolBoxs(true, true, true);
    pMacValidator = new cMacValidator(true, this);
    pUi->lineEditPMAC->setValidator(pMacValidator);
    // small cosmetics
    QList<int> sizes;
    sizes << 450 << 750;
    pUi->splitterMsg->setSizes(sizes);
    sizes.clear();
    sizes << 300 << 300 << 300 << 300;
    pUi->splitterWorkstation->setSizes(sizes);
    // Button groups
    pModifyButtons = new QButtonGroup(this);
    pModifyButtons->addButton(pUi->radioButtonNew, 0);
    pModifyButtons->addButton(pUi->radioButtonMod, 1);
    pLinkTypeButtons = new QButtonGroup(this);
    pLinkTypeButtons->addButton(pUi->radioButtonLinkTerm,  LT_TERM);
    pLinkTypeButtons->addButton(pUi->radioButtonLinkFront, LT_FRONT);
    pLinkTypeButtons->addButton(pUi->radioButtonLinkBack,  LT_BACK);

    // Set
    pUi->lineEditNodeTypeFilt->setText(pSetDialogFiltType->toString());
    pUi->lineEditNodeType->setText(pSetDialogType->toString());
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
    pnm->setFilter(pSetDialogFiltType->toWhere(_sNodeType), OT_DEFAULT, FT_SQL_WHERE);
    pSelNode->setNodeModel(pnm);
    pSelNode->refresh();
    pSelPlace    = new cSelectPlace(pUi->comboBoxZone, pUi->comboBoxPlace, pUi->lineEditPlacePat, _sNul, this);
    pSelNode->setSlave(pSelPlace, false);
    pSelLinked   = new cSelectLinkedPort(pUi->comboBoxLinkZone, pUi->comboBoxLinkPlace, pUi->comboBoxLinkNode,
                                   pUi->comboBoxLinkPort, pLinkTypeButtons, pUi->comboBoxLinkPortShare, iTab(ES_, ES_A, ES_B),
                                   pUi->lineEditLinkPlacePat, NULL, _sNul, _sNul, this);
    pSelPlace->refresh();
    pSelPlace->setSlave(pSelLinked);
    pSelLinked->refresh();

    pif = new cInterface;
    pnp = pif;
    node.ports << pnp;
    pit = &cIfType::ifType(pUi->comboBoxPType->currentText());
    pif->setId(_sIfTypeId, pit->getId());
    pif->setName(pUi->lineEditPName->text());
    pip = pIpEditWidget->get();
    pif->addresses << pip;
    portIsLinkage = pit->isLinkage();
    bbNode.end();

    connect(pSelNode,      SIGNAL(nodeIdChanged(qlonglong)),       this, SLOT(selectedNode(qlonglong)));
    connect(pSelLinked,    SIGNAL(changedLink(qlonglong,int,int)), this, SLOT(linkChanged(qlonglong,int,int)));
    connect(pIpEditWidget, SIGNAL(changed(QHostAddress,int)),      this, SLOT(addressChanged(QHostAddress,int)));
    connect(pIpEditWidget, SIGNAL(info_clicked()),                 this, SLOT(ip_info()));
    connect(pIpEditWidget, SIGNAL(query_clicked()),                this, SLOT(ip_query()));
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
            sInfs << htmlReportNode(*pq, *pn, trUtf8("A %2 (%3) eszköznek azonos a %1-a.").arg(t), false);
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
            sInfs << htmlReportNode(*pq, *pn, trUtf8("A %1 megadott néven bejegyzett (%2 típusú) eszköz : "), true);
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
    (void)f;
    (void)sErrs;
    (void)sInfs;
    (void)isOk;
}


void cWorkstation::setStatIp(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f || pip == NULL) return;
    int state = pIpEditWidget->state();
    if (state & cIpEditWidget::IES_SUBNET_IS_NULL) {
        sErrs << htmlError(trUtf8("Nincs megadva alhálózat, vagy VLAN."));
    }
    if (state & cIpEditWidget::IES_ADDRESS_IS_INVALID) {
        sErrs << htmlError(trUtf8("Hibás, vagy hiányos IP címet adott meg."));
        isOk = false;
        return;
    }
    if (state & cIpEditWidget::IES_ADDRESS_IS_NULL) {
        QString msg = trUtf8("Nincs megadva IP cím.");
        if (pip->getId(_sIpAddressType) == AT_DYNAMIC) {
            sErrs << htmlInfo(msg);
        }
        else {
            msg += trUtf8(" A cím megadása a 'dynamic' típus kivételével kötelező.");
            sErrs << htmlError(msg);
            isOk = false;
        }
        return;
    }
    cNode n;
    if (pip->getId(_sIpAddressType) != AT_PRIVATE && n.fetchByIp(*pq, pip->address(), EX_IGNORE)) {
        if (node.getId() == NULL_ID || node.getId() != n.getId()) {
            sErrs << htmlError(trUtf8("A megadott IP címmel már létezik egy eszköz! A címnek, a 'privat' típus kivépelével, egyedinek kell lennie."));
            sInfs << htmlReportNode(*pq, n, trUtf8("A megadott címmel %1 nevű (%2 típusú) eszköz van bejegyezve."), false);
            isOk = false;
        }
    }
}

void cWorkstation::setStatPortMac(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f || pif == NULL) return;
    QString sMac = pUi->lineEditPMAC->text();
    if (sMac.isEmpty()) {
        sErrs << htmlWarning(trUtf8("Nincs megadva a MAC."));
        return;
    }
    cMac mac = pif->mac();
    if (mac.isEmpty()) {
        sErrs << htmlWarning(trUtf8("A megadott MAC hibás."));
        isOk = false;
        return;
    }
    cNode n;
    if (n.fetchByMac(*pq, mac)) {
        if (node.getId() == NULL_ID || node.getId() != n.getId()) {
            sErrs << htmlError(trUtf8("A megadott MAC-kal már létezik egy eszköz! Két különböző eszköznek nem lehet azonos MAC-je."));
            sInfs << htmlReportNode(*pq, n, trUtf8("A megadott MAC-kel a %1 nevű (%2 típusú) eszköz van bejegyezve."), false);
            isOk = false;
        }
    }
    QHostAddress a = cArp::mac2ip(*pq, mac, EX_IGNORE);
    if (!a.isNull() && pIpEditWidget->state() & cIpEditWidget::IES_ADDRESS_IS_NULL) pIpEditWidget->set(a);
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
            if (pnp->getId() != NULL_ID) {
                ix = list.indexOf(_sPortId2, pnp->get(pnp->idIndex()));
            }
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
    pit = &cIfType::ifType(pnp->getId(__sIfTypeId));
    QString sIfType = pit->getName();
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
    pSelLinked->setExcludedNode(node.getId());
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
    pUi->radioButtonMod->setChecked(true);
    bbNode.end(true);
}

// SLOTS:

void cWorkstation::selectedNode(qlonglong id)
{
    pDelete(pSample);
    if (id == NULL_ID) {
        pUi->radioButtonNew->setChecked(true);
        return;
    }
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

void cWorkstation::linkChanged(qlonglong _pid, int _lt, int _sh)
{
    (void)_pid;
    (void)_lt;
    (void)_sh;
    bbLink.enforce(true);
}

void cWorkstation::addressChanged(const QHostAddress& _a, int _st)
{
    (void)_a;
    (void)_st;
    if (pip != NULL) {
        pIpEditWidget->get(pip);
        bbIp.enforce();
    }
    else {
        if (!pIpEditWidget->isDisabled()) EXCEPTION(EPROGFAIL);
    }
}

void cWorkstation::ip_info()
{
    const QHostAddress& a = pIpEditWidget->getAddress();
    QString info = htmlReportByIp(*pq, a.toString());
    pUi->textEditMsg->setHtml(info);
}

void cWorkstation::ip_go()
{

}

void cWorkstation::ip_query()
{
    if (pif != NULL) {
        cMac mac = pif->mac();
        if (mac.isValid()) {
            QList<QHostAddress> al = cArp::mac2ips(*pq, mac);
            switch (al.size()) {
            default:
                pUi->textEditMsg->append(trUtf8("A MAC-hez tőbb cím is tartozik, az első lessz beállítva."));
            case 1:
                pIpEditWidget->set(al.first());
                addressChanged(al.first(), AT_DYNAMIC);
                return;
            case 0:
                break;
            }
        }
        pUi->textEditMsg->append(trUtf8("A MAC-hez tartozó IP cím megállpítása sikertelen."));
    }
}


void cWorkstation::on_radioButtonMod_toggled(bool checked)
{
    isModify = checked;
    bbNode.enforce();
}

void cWorkstation::on_checkBoxPlaceEqu_toggled(bool checked)
{
    pSelPlace->setSlave(checked ? pSelLinked : NULL);   // -> linkedNodeIdChanged();
}

void cWorkstation::on_lineEditPName_textChanged(const QString &arg1)
{
    (void)arg1;
    bbPortName.enforce(true);
}

void cWorkstation::on_comboBoxPType_currentIndexChanged(const QString &arg1)
{
    pit = &cIfType::ifType(arg1);  // New port type (ifType)
    QString ot = pit->getName(_sIfTypeObjType);  // New object type name
    bool isInterface;
    if      (ot == _sNPort)     isInterface = false;    // new type is interface
    else if (ot == _sInterface) isInterface = true;     // new type is passive port
    else                        EXCEPTION(EDATA, 0, pit->identifying(false));
    bool changeType = isInterface == (pif == NULL);     // Changed interface object típe?
    bool isLinkage  = pit->isLinkage();               // New port type is linkable?
    bool togleLink  = isLinkage != portIsLinkage;       // Change link ?
    pnp->setId(_sIfTypeId, pit->getId());
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
        pUi->toolButtonIP2MAC->setEnabled(isInterface);
        pIpEditWidget->setAllDisabled(!isInterface);
    }
    if (togleLink) {
        portIsLinkage = isLinkage;
        pl.clear();
        if (isLinkage) {    // Enable link
        }
        else {              // Disable link
        }
        pSelLinked->setEnabled(isLinkage);
    }
    bbPortType.end();
}

void cWorkstation::on_lineEditPMAC_textChanged(const QString &arg1)
{
    (void)arg1;
    if (pif != NULL) {
        if (arg1.isEmpty()) pif->clear(_sHwAddress);
        else pif->setMac(_sHwAddress, cMac(arg1));
        bbPortMac.enforce();
    }
    else {
        if (pUi->lineEditPMAC->isEnabled()) EXCEPTION(EPROGFAIL);
    }
}

void cWorkstation::on_lineEditName_textChanged(const QString &arg1)
{
    (void)arg1;
    bbNodeName.enforce(true);
}

void cWorkstation::on_lineEditSerial_textChanged(const QString &arg1)
{
    (void)arg1;
    bbNodeSerial.enforce(true);
}

void cWorkstation::on_lineEditInventory_textChanged(const QString &arg1)
{
    (void)arg1;
    bbNodeInventory.enforce(true);
}

void cWorkstation::on_lineEditModelName_textChanged(const QString &arg1)
{
    (void)arg1;
    bbNodeModel.enforce(true);
}

void cWorkstation::on_lineEditModelNumber_textChanged(const QString &arg1)
{
    (void)arg1;
    bbNodeModel.enforce(true);
}

void cWorkstation::on_toolButtonErrRefr_clicked()
{
    bbNode.enforce(true);
}

void cWorkstation::on_toolButtonInfRefr_clicked()
{
    bbNode.referState();
}

void cWorkstation::on_toolButtonReportMAC_clicked()
{
    QString sMac = pUi->lineEditPMAC->text();
    if (sMac.isEmpty()) return;
    pUi->textEditMsg->setHtml(htmlReportByMac(*pq, sMac));
}

void cWorkstation::on_toolButtonNodeTypeFilt_clicked()
{
    pSetDialogFiltType->set(filtTypeOn, filtTypeOff);
    if (pSetDialogFiltType->exec() == QDialog::Accepted) {
        pSelNode->setNodeFilter(pSetDialogFiltType->toWhere(_sNodeType, excludedNodeType), OT_DEFAULT, FT_SQL_WHERE);
        filtTypeOn = pSetDialogFiltType->getOn();
        filtTypeOff= pSetDialogFiltType->getOff();
        pUi->lineEditNodeTypeFilt->setText(pSetDialogFiltType->toString());
    }
}

void cWorkstation::on_toolButtonNodeType_clicked()
{
    pSetDialogFiltType->set(nodeType);
    if (pSetDialogType->exec() == QDialog::Accepted) {
        nodeType = pSetDialogType->getOn();
        pUi->lineEditNodeType->setText(pSetDialogType->toString());
    }
}

void cWorkstation::on_toolButtonIP2MAC_clicked()
{
    if (pip != NULL) {
        QHostAddress a = pip->address();
        if (!a.isNull()) {
            cMac mac = cArp::ip2mac(*pq, a);
            if (mac.isValid()) {
                pUi->lineEditPMAC->setText(mac.toString());
                return;
            }
        }
        pUi->textEditMsg->append(trUtf8("A MAC cím megállpítása az IP címből sikertelen."));
    }
}

void cWorkstation::on_toolButtonAddPlace_clicked()
{
    qlonglong pid = pSelPlace->insertPlace();
    if (pid == NULL_ID) return;
    pSelNode->refresh();
    pUi->checkBoxPlaceEqu->setChecked(false);
    pSelLinked->refresh();
    pSelPlace->setCurrentPlace(pid);
}

void cWorkstation::on_toolButtonSelectByMAC_clicked()
{
    if (pif != NULL) {
        bool set = false;
        cMac mac = pif->mac();
        if (mac.isValid()) {
            cNode n;
            if (1 == n.fetchByMac(*pq, mac, EX_ERROR)) {        // Egy port
                if (1 != n.fetchPorts(*pq, CV_PORTS_ADDRESSES)) {
                    pUi->textEditMsg->append(htmlError("A MAC alapján kiválasztott eszköznek nem egy portja van, ezzel az űrlappal nem kezelhető."));
                    return;
                }
                cNPort *p = n.ports.first();
                cInterface *pi = p->reconvert<cInterface>();
                if (pi->addresses.size() > 1) {                 // Max egy cím
                    pUi->textEditMsg->append(htmlError("A MAC alapján kiválasztott eszköznek nem egy csak egy IP címe van, ezzel az űrlappal nem kezelhető."));
                    return;
                }
                pSelNode->setCurrentNode(n.getId());
                set = n.getId() == pSelNode->currentNodeId();
                if (!set) {     // Mégegy próba, töröljük a típus szűrőt
                    pUi->lineEditNodeTypeFilt->setText(_sNul);
                    pSelNode->setNodeFilter(pSetDialogFiltType->toWhere(_sNul, excludedNodeType), OT_DEFAULT, FT_NO);
                    filtTypeOn = filtTypeOff = 0;
                    pSelNode->setCurrentNode(n.getId());
                    set = n.getId() == pSelNode->currentNodeId();
                }
            }
        }
        if (!set) {
            pUi->textEditMsg->append("A MAC alapján nem sikerült beolvasni az eszköz adatokat.");
        }
    }
}

void cWorkstation::on_toolButtonInfoLnkNode_clicked()
{
    qlonglong lnid = pSelLinked->currentNodeId();
    if (lnid != NULL_ID) {
        cPatch *pn = cPatch::getNodeObjById(*pq, lnid, EX_IGNORE);
        if (pn != NULL) {
            QString s = htmlReportNode(*pq, *pn);
            pUi->textEditMsg->setHtml(s);
            delete pn;
        }
    }
}

void cWorkstation::on_toolButtonAddLnkNode_clicked()
{
    cPatch sample;
    sample.setId(_sPlaceId, pSelLinked->currentPlaceId());
    pSelLinked->insertPatch(&sample);
}
