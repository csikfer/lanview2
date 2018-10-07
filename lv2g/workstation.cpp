#include "lv2widgets.h"
#include "srvdata.h"
#include "lv2user.h"
#include "lv2link.h"
#include "record_dialog.h"
#include "object_dialog.h"
#include "popupreport.h"
#include "workstation.h"
#include "ui_wstform.h"
#include "ui_phslinkform.h"
#include "menu.h"
#include "mainwindow.h"
#include "hsoperate.h"
#include "cerrormessagebox.h"

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

    pEditNote            = new cLineWidget;
    pEditSerialNumber    = new cLineWidget;
    pEditInventoryNumber = new cLineWidget;
    pEditModelNumber     = new cLineWidget;
    pEditModelName       = new cLineWidget;
    setFormEditWidget(pUi->formLayoutNode, pUi->labelNote,       pEditNote);
    setFormEditWidget(pUi->formLayoutNode, pUi->labelSerial,     pEditSerialNumber);
    setFormEditWidget(pUi->formLayoutNode, pUi->labelInventory,  pEditInventoryNumber);
    setFormEditWidget(pUi->formLayoutNode, pUi->labelModelName,  pEditModelName);
    setFormEditWidget(pUi->formLayoutNode, pUi->labelModelNumber,pEditModelNumber);
    pEditPNote          = new cLineWidget;
    pEditPTag           = new cLineWidget;
    setFormEditWidget(pUi->formLayoutPort, pUi->labelPNote,      pEditPNote);
    setFormEditWidget(pUi->formLayoutPort, pUi->labelPTag,       pEditPTag);

    pIpEditWidget = new cIpEditWidget(iTab(AT_DYNAMIC, AT_FIXIP));
    pUi->verticalLayoutIp->insertWidget(1, pIpEditWidget);
    pIpEditWidget->showToolBoxs(true, true, true);
    pIpEditWidget->enableToolButtons(true, true, true);
    pIpEditWidget->setInfoToolTip(trUtf8("Infó az IP címről."));
    pIpEditWidget->setGoToolTip(trUtf8("Eszköz (minta) kiválasztása az IP cím alapján."));
    pIpEditWidget->setQueryToolTip(trUtf8("Az IP megállpítása a MAC alapján."));
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
    pSelNode->refresh(false);
    pSelPlace    = new cSelectPlace(pUi->comboBoxZone, pUi->comboBoxPlace, pUi->lineEditPlacePat, _sNul, this);
    pSelNode->setSlave(pSelPlace, false);
    pSelLinked   = new cSelectLinkedPort(pUi->comboBoxLinkZone, pUi->comboBoxLinkPlace, pUi->comboBoxLinkNode,
                                   pUi->comboBoxLinkPort, pLinkTypeButtons, pUi->comboBoxLinkPortShare, iTab(ES_, ES_A, ES_B),
                                   pUi->lineEditLinkPlacePat, NULL, _sNul, _sNul, this);
    pSelPlace->refresh(false);
    pSelLinked->refresh(false);

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
    connect(pEditSerialNumber, SIGNAL(changed(QVariant)),          this, SLOT(serialNumberChanged(QVariant)));
    connect(pEditInventoryNumber, SIGNAL(changed(QVariant)),       this, SLOT(inventoryNumberChanged(QVariant)));
    connect(pEditModelName, SIGNAL(changed(QVariant)),             this, SLOT(modelNameChanged(QVariant)));
    connect(pEditModelNumber, SIGNAL(changed(QVariant)),           this, SLOT(modelNumberChanged(QVariant)));
    connect(pSelLinked,    SIGNAL(changedLink(qlonglong,int,int)), this, SLOT(linkChanged(qlonglong,int,int)));
    connect(pIpEditWidget, SIGNAL(changed(QHostAddress,int)),      this, SLOT(addressChanged(QHostAddress,int)));
    connect(pIpEditWidget, SIGNAL(info_clicked()),                 this, SLOT(ip_info()));
    connect(pIpEditWidget, SIGNAL(query_clicked()),                this, SLOT(ip_query()));
    connect(pIpEditWidget, SIGNAL(go_clicked()),                   this, SLOT(ip_go()));
}

cWorkstation::~cWorkstation()
{
    delete pq;
}

void cWorkstation::msgEmpty(const QVariant& val, QLabel *pLabel, const QString& fn, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    bool isNull = val.isNull();
    bool unique = !fn.isEmpty();    // Ha van mező név, akkor egyediségre is vizsgálunk
    QString sVal = val.toString();
    QString n = pLabel->text().simplified().toLower();
    if (n.endsWith(':')) {
        n.chop(1);
        n = n.simplified();
    }
    if (sVal.isEmpty()) {
        QString msg = trUtf8("Nincs kitöltve a %1 mező.").arg(n);
        if (isNull || !unique) {
            sErrs << htmlWarning(msg);
        }
        else {
            sErrs << htmlError(msg);
            isOk = false;
         }
        return;
    }
    if (unique) {
        cPatch node;
        node.setName(fn, sVal); // Azonos mezőérték keresése
        if (node.completion(*pq) && !(isModify && node.getId() == node.getId())) {  // önmagunkkal nem ütközünk
            cPatch *pn = cPatch::getNodeObjById(*pq, node.getId());
            sErrs << htmlError(QObject::trUtf8("A megadott %1 nem egyedi!").arg(n));
            QString t = trUtf8("A %2 (%3) eszköznek azonos a %1-a.").arg(n);
            sInfs << htmlPair2Text(htmlReportNode(*pq, *pn, t, 0));
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
    isModify = pUi->radioButtonMod->isChecked();    // nem mindíg frissül
    if (pn != NULL) {
        if (!(isModify && node.getId() == pn->getId())) { // Name is not unique
            sErrs << htmlError(trUtf8("A megadott néven már létezik egy eszköz! A névnek egyedinek kell lennie."));
            QString t = trUtf8("A %1 megadott néven bejegyzett (%2 típusú) eszköz : ");
            sInfs << htmlPair2Text(htmlReportNode(*pq, *pn, t, CV_PORTS));
        }
        delete pn;
        return;
    }
}

void cWorkstation::setStatNodeSerial(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    msgEmpty(pEditSerialNumber->get(), pUi->labelSerial, _sSerialNumber, sErrs, sInfs, isOk);
}

void cWorkstation::setStatNodeInventory(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    msgEmpty(pEditInventoryNumber->get(), pUi->labelInventory, _sInventoryNumber, sErrs, sInfs, isOk);
}

void cWorkstation::setStatNodeType(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk)
{
    if (!f) return;
    msgEmpty(pEditModelName->get(),   pUi->labelModelName, _sNul, sErrs, sInfs, isOk);
    msgEmpty(pEditModelNumber->get(), pUi->labelModelNumber, _sNul, sErrs, sInfs, isOk);
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
//  if (!f) return;
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
        if (node.getId() != NULL_ID && (!isModify || node.getId() != n.getId())) {
            sErrs << htmlError(trUtf8("A megadott IP címmel már létezik egy eszköz! A címnek, a 'privat' típus kivépelével, egyedinek kell lennie."));
            QString t = trUtf8("A megadott címmel %1 nevű (%2 típusú) eszköz van bejegyezve.");
            sInfs << htmlPair2Text(htmlReportNode(*pq, n, t, 0));
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
        if (node.getId() != NULL_ID && (!isModify || node.getId() != n.getId())) {
            sErrs << htmlError(trUtf8("A megadott MAC-kal már létezik egy eszköz! Két különböző eszköznek nem lehet azonos MAC-je."));
            QString t = trUtf8("A megadott MAC-kel a %1 nevű (%2 típusú) eszköz van bejegyezve.");
            sInfs << htmlPair2Text(htmlReportNode(*pq, n, t, 0));
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
    qlonglong lnid = pSelLinked->currentNodeId();
    pl.clear();
    bool exist = lnid != NULL_ID;
    pUi->lineEditLinkNodeType->setEnabled(exist);
    QString lnt;
    if (exist) {
        cPatch n;
        n.setById(*pq, lnid);
        lnt = n.getName(_sNodeType);
    }
    pUi->lineEditLinkNodeType->setText(lnt);
    exist = lpid != NULL_ID;
    pUi->lineEditLinkNote->setEnabled(exist);
    if (!exist) {      // nincs cél port
        if (lnid == NULL_ID) {   // cél eszköz se
            sErrs << htmlWarning(trUtf8("Nincs megadva link."));
            return;
        }
        cPatch *pn = cPatch::getNodeObjById(*pq, lnid);
        sErrs << htmlError(trUtf8("Nincs megadva a linkelt eszköz portja."));
        QString t = trUtf8("A következő eszköz egy portjára csatlakozna : %2 ");
        sInfs << htmlPair2Text(htmlReportNode(*pq, *pn, t));
        delete pn;
        isOk = false;
        return;
    }
    cPhsLink link;
    tRecordList<cPhsLink> list;
    ePhsLinkType type = (ePhsLinkType)pSelLinked->currentType();
    ePortShare   sh   = (ePortShare)pSelLinked->currentShare();
    pl.setId(_sPortId2, lpid);
    pl.setId(_sPhsLinkType2, type);
    pl.setId(_sPortShared, sh);
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
    link.clear();
    link.setId(_sPortId2,      lpid);   // vissza tesszük, ha végpont, akkor kell
    if (type != LT_TERM) {  // További linkek (lánc)
        static const QString sql = "SELECT * FROM next_patch(?,?,?)";
        link.setId(_sPhsLinkType2, type);
        list.clear();
        QString ssh = portShare(sh);
        QString msg;
        while (execSql(*pq, sql, link.get(_sPortId2), link.get(_sPhsLinkType2), ssh)) {
            if (pq->value(0).isNull()) break; // vége (beolvassa a NULL rekordot is, mert függvényt hívunk)
            link.set(*pq);
            list << link;
            type = ePhsLinkType(link.getId(_sPhsLinkType2));
            ssh = link.getName(_sPortShared);
            if (list.size() > 10) break;
            if (type == LT_TERM) break;
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
    if (type == LT_TERM) {  // Van végpont ?
        qlonglong ppid  = pnp->getId();     // Saját port ID
        lpid = link.getId(_sPortId2);       // Másik végponti port ID
        qlonglong epid;
        if (ppid != NULL_ID) {
            cLogLink  llnk;
            PDEB(VERBOSE) << "New LogLink : " << cNPort::getFullNameById(*pq, ppid) << " <==> " << cNPort::getFullNameById(*pq, lpid) << endl;
            llnk.setId(_sPortId2, lpid);
            if (llnk.completion(*pq)) {
                epid = llnk.getId(_sPortId1);
                PDEB(VERBOSE) << "Loaded LogLink : " << cNPort::getFullNameById(*pq, epid) << " <==> " << cNPort::getFullNameById(*pq, lpid) << endl;
                if (epid != ppid) {
                    sInfs << htmlError(trUtf8("Az ellen oldali végpont (törlendő) logikai linkje : %1").arg(cNPort::getFullNameById(*pq, epid)));
                }
            }
            cLldpLink ldnl;
            ldnl.setId(_sPortId2, lpid);
            bool eq = false;
            if (ldnl.completion(*pq)) {
                epid = ldnl.getId(_sPortId1);
                if (epid != ppid) {
                    sInfs << htmlError(trUtf8("Az ellen oldali végpont ütköző linkje az LLDP alapján : %1").arg(cNPort::getFullNameById(*pq, epid)));
                }
                else {
                    eq = true;
                    sInfs << htmlGrInf(trUtf8("Megeggyező LLDP link."));
                }
            }
            if (!eq) {
                ldnl.clear();
                ldnl.setId(_sPortId1, ppid);
                if (ldnl.completion(*pq)) {
                    epid = ldnl.getId(_sPortId2);
                    if (epid != lpid) {
                        sInfs << htmlError(trUtf8("Az ütköző link az LLDP alapján : %1").arg(cNPort::getFullNameById(*pq, epid)));
                    }
                }
            }
        }
        if (pif != NULL && pif->getMac(_sHwAddress).isValid()) {
            cMacTab mt;
            mt.setMac(_sHwAddress, pif->getMac(_sHwAddress));
            if (mt.completion(*pq)) {
                qlonglong mtp = mt.getId(_sPortId);
                if (lpid == mtp) {
                    sInfs << htmlGrInf(trUtf8("A Link megerősítve a MAC címtáblák alapján."));
                }
                else {
                    sInfs << htmlError(trUtf8("A Link ütközik a MAC címtáblákkal, talált port %1 .").arg(cNPort::getFullNameById(*pq, mtp)));
                }
            }
            else {
                sInfs << htmlInfo(trUtf8("A Link nincs megerősítve a MAC címtáblák alapján."));
            }
        }
    }
}

void cWorkstation::node2gui(bool setModOn)
{
    pnp = NULL;
    pif = NULL;
    pip = NULL;
    bbNode.begin();
    pl.clear();
    pSelPlace->setCurrentPlace(node.getId(_sPlaceId));
    pUi->lineEditName->setText(node.getName());
    pEditNote->set(node.get(node.noteIndex()));
    pEditSerialNumber->set(node.get(_sSerialNumber));
    pEditInventoryNumber->set(node.get(_sInventoryNumber));
    pEditModelName->set(node.get(_sModelName));
    pEditModelNumber->set(node.get(_sModelNumber));
    pUi->lineEditNodeType->setText(node.getName(_sNodeType));
    // Display ports
    if (node.ports.isEmpty()) node.addPort(_sEthernet, _sEthernet, _sNul, NULL_IX);
    pnp = node.ports.first();
    qlonglong pid = pnp->getId();
    if (0 == pnp->chkObjType<cInterface>(EX_IGNORE)) pif = pnp->reconvert<cInterface>();
    pUi->lineEditPName->setText(pnp->getName());
    pEditPNote->set(pnp->get(pnp->noteIndex()));
    pEditPTag->set(pnp->get(_sPortTag));
    pit = &cIfType::ifType(pnp->getId(__sIfTypeId));
    QString sIfType = pit->getName();
    _setCurrentIndex(sIfType, pUi->comboBoxPType, EX_IGNORE);
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
    if (link) {
        qlonglong plid = pSelLinked->currentPlaceId();
        if (pSelNode->currentPlaceId() == plid) {
            pSelLinked->cSelectPlace::copyCurrents(*pSelPlace);
        }
    }
    pSelLinked->setLink(pl);
    bool f = setModButtons(node.getId());
    pUi->radioButtonMod->setChecked(f && setModOn);
    bbNode.end(true);
}

bool cWorkstation::setModButtons(qlonglong _id)
{
    bool f = _id != NULL_ID;
    if (!f) {
        pUi->radioButtonNew->setChecked(true);
        pUi->radioButtonMod->setEnabled(false);
        pUi->lineEditID->setText(_sNul);
    }
    else {
        pUi->lineEditID->setText(QString::number(_id));
        pUi->radioButtonMod->setEnabled(true);
    }
    pUi->lineEditID->setEnabled(f);
    pUi->pushButtonDelete->setEnabled(f);
    pUi->pushButtonGoServices->setEnabled(f);
    return f;
}

// SLOTS:

void cWorkstation::selectedNode(qlonglong id)
{
    pDelete(pSample);
    if (id == NULL_ID) {
        return;
    }
    bbNode.begin();
    qlonglong nid = pSelNode->currentNodeId();
    pSample = cPatch::getNodeObjById(*pq, nid)->reconvert<cNode>();
    node.clear();
    pnp = NULL;
    pif = NULL;
    pip = NULL;
    node.set(*pSample);
    node.fetchPorts(*pq, CV_PORTS_ADDRESSES);
    if (node.ports.size() != 1) EXCEPTION(EPROGFAIL);   // Elvileg az egy portosokat olvastuk be.
    bbNode.end(false);  // set state by node2gui()
    // Display fields
    node2gui(true);
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
    popupReportByIp(this, *pq, a.toString());
}

void cWorkstation::ip_go()
{
    if (pip != NULL) {
        bool set = false;
        QHostAddress ip = pip->address();
        int cnt = -1;
        if (!ip.isNull()) {
            cNode n;
            cnt = n.fetchByIp(*pq, ip, EX_IGNORE);
            if (1 == cnt) {
                if (1 != n.fetchPorts(*pq, CV_PORTS_ADDRESSES)) {
                    pUi->textEditMsg->append(htmlError(trUtf8("A %1 IP alapján kiválasztott %2 eszköznek nem egy portja van, ezzel az űrlappal nem kezelhető.")
                                                       .arg(ip.toString(), n.getName())));
                    return;
                }
                cNPort *p = n.ports.first();
                cInterface *pi = p->reconvert<cInterface>();
                if (pi->addresses.size() > 1) {                 // Max egy cím
                    pUi->textEditMsg->append(htmlError(trUtf8("A %1 IP alapján kiválasztott %2 eszköznek nem egy csak egy IP címe van, ezzel az űrlappal nem kezelhető.")
                                                       .arg(ip.toString(), n.getName())));
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
                if (set) {
                    pUi->textEditMsg->append(htmlInfo(trUtf8("A %1 IP alapján beolvasott az eszköz : ").arg(ip.toString(), n.getName())));
                }
            }
        }
        if (!set) {
            QString msg;
            switch (cnt) {
            case -1:
                msg = trUtf8("A IP nem valós cím.");
                break;
            case 1:
                msg = trUtf8("A %1 IP alapján nem sikerült beolvasni az eszköz adatokat.").arg(ip.toString());
                break;
            case 0:
                msg = trUtf8("A %1 IP címmel nincs bejegyzett eszköz.").arg(ip.toString());
                break;
            default:
                msg = trUtf8("A %1 IP címmel több bejegyzett eszköz is van.").arg(ip.toString());
                break;
            }
            pUi->textEditMsg->append(htmlError(msg));
        }
    }
}

void cWorkstation::ip_query()
{
    if (pif != NULL) {
        cMac mac = pif->mac();
        if (mac.isValid()) {
            QList<QHostAddress> al = cArp::mac2ips(*pq, mac);
            int n = al.size();
            if (n > 1) { // Több cím van! Ezt csak jelezzük
                QString msg = trUtf8("A MAC-hez tőbb cím is tartozik, az első lessz beállítva, eldobott címek : ");
                foreach (QHostAddress a, al.mid(1)) {
                    msg += a.toString() + ", ";
                }
                msg.chop(2);
                pUi->textEditMsg->append(msg);
            }
            if (n >= 1) {   // Az első címet állítjuk be
                pIpEditWidget->set(al.first());     // Type??
                addressChanged(al.first(), AT_DYNAMIC);
                pUi->textEditMsg->append(trUtf8("A %1 MAC alapján beállított IP cím : %2").arg(mac.toString(), al.first().toString()));
                return;
            }
        }
        pUi->textEditMsg->append(trUtf8("A %1 MAC-hez tartozó IP cím megállpítása sikertelen.").arg(mac.toString()));
    }
}


void cWorkstation::on_radioButtonMod_toggled(bool checked)
{
    bbNode.begin();
    if (checked) {
        isModify = setModButtons(node.getId());
    }
    else {
        isModify = false;
        pUi->lineEditID->setDisabled(true);
        pUi->pushButtonDelete->setDisabled(true);
        pUi->pushButtonGoServices->setDisabled(true);
    }
    bbNode.end();
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
            node.ports.clear();
            node.ports << pif;
            pnp = pif;
            pip = new cIpAddress;
            pif->addresses << pip;
            pIpEditWidget->set(pip);
        }
        else {              // cInterface => cNPort
            pnp = new cNPort;
            pnp->set(*pnp);
            node.ports.clear();
            node.ports << pnp;
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

void cWorkstation::serialNumberChanged(const QVariant &arg1)
{
    (void)arg1;
    bbNodeSerial.enforce(true);
}

void cWorkstation::inventoryNumberChanged(const QVariant &arg1)
{
    (void)arg1;
    bbNodeInventory.enforce(true);
}

void cWorkstation::modelNameChanged(const QVariant &arg1)
{
    (void)arg1;
    bbNodeModel.enforce(true);
}

void cWorkstation::modelNumberChanged(const QVariant &arg1)
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
    popupReportByMAC(this, *pq, sMac);
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
    pSetDialogType->set(nodeType);
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
    pSelNode->refresh(false);
    pSelLinked->refresh(false);
    pSelPlace->setCurrentPlace(pid);
}

void cWorkstation::on_toolButtonSelectByMAC_clicked()
{
    if (pif != NULL) {
        bool set = false;
        cMac mac = pif->mac();
        int cnt = -1;
        if (mac.isValid()) {
            cNode n;
            cnt = n.fetchByMac(*pq, mac, EX_ERROR);
            if (1 == cnt) {        // Egy port
                if (1 != n.fetchPorts(*pq, CV_PORTS_ADDRESSES)) {
                    pUi->textEditMsg->append(htmlError(trUtf8("A %1 MAC alapján kiválasztott %2 eszköznek nem egy portja van, ezzel az űrlappal nem kezelhető.")
                                                       .arg(mac.toString(), n.getName())));
                    return;
                }
                cNPort *p = n.ports.first();
                cInterface *pi = p->reconvert<cInterface>();
                if (pi->addresses.size() > 1) {                 // Max egy cím
                    pUi->textEditMsg->append(htmlError(trUtf8("A %1 MAC alapján kiválasztott %2 eszköznek nem egy csak egy IP címe van, ezzel az űrlappal nem kezelhető.")
                                                       .arg(mac.toString(), n.getName())));
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
                if (set) {
                    pUi->textEditMsg->append(htmlInfo(trUtf8("A %1 MAC alapján beolvasott az eszköz : ").arg(mac.toString(), n.getName())));
                }
            }
        }
        if (!set) {
            QString msg;
            switch (cnt) {
            case -1:
                msg = trUtf8("A MAC nem valós cím.");
                break;
            case 1:
                msg = trUtf8("A %1 MAC alapján nem sikerült beolvasni az eszköz adatokat.").arg(mac.toString());
                break;
            case 0:
                msg = trUtf8("A %1 MAC címmel nincs bejegyzett eszköz.").arg(mac.toString());
                break;
            default:
                msg = trUtf8("A %1 MAC címmel több bejegyzett eszköz is van.").arg(mac.toString());
                break;
            }
            pUi->textEditMsg->append(htmlError(msg));
        }
    }
}

void cWorkstation::on_toolButtonInfoLnkNode_clicked()
{
    qlonglong lnid = pSelLinked->currentNodeId();
    if (lnid != NULL_ID) {
        popupReportNode(this, *pq, lnid);
    }
}

void cWorkstation::on_toolButtonAddLnkNode_clicked()
{
    cPatch sample;
    sample.setId(_sPlaceId, pSelLinked->currentPlaceId());
    pSelLinked->insertPatch(&sample);
}

void cWorkstation::on_pushButtonDelete_clicked()
{
    bbNode.begin();
    if (node.getId() != NULL_ID) {
        int b = QMessageBox::warning(this,
                             trUtf8("A %1 nevű eszköz törlése.").arg(node.getName()),
                             trUtf8("Valóban törölni akarja az eszközt? Teljes visszavonásra nincs lehetőség!"),
                             QMessageBox::Ok, QMessageBox::Cancel);
        if (b != QMessageBox::Ok) {
            bbNode.end(false);   // nincs változás
            return;
        }
        if (cErrorMessageBox::condMsgBox(node.tryRemove(*pq))) {
            // Az adatok maradnak, de az ID-ket töröljük
            if (pip != NULL) pip->clearId();
            node.ports.clearId();
            node.clearId(); // A konténerekben töröltük az ID-ket először, igy azokat nem törli!
            pSelNode->refresh(false);
            pSelNode->setCurrentNode(NULL_ID);
            setModButtons(NULL_ID);
        }
    }
    bbNode.end(false);
    node2gui(false);
}

void cWorkstation::on_pushButtonGoServices_clicked()
{
    if (node.getId() != NULL_ID) {
        const QString _sHSOperate = "hsop";
        QMap<QString, QAction *>::iterator i = cMenuAction::actionsMap.find(_sHSOperate);
        if (i != cMenuAction::actionsMap.end()) {
            (*i)->triggered();
        }
        else {
            QString msg = trUtf8("Nincs szervízpéldányt manipuláló adatlap a menüben.");
            pUi->textEditMsg->append(htmlError(msg));
            return;
        }
        QWidget *pw = lv2g::getInstance()->pMainWindow->pMdiArea->currentSubWindow()->widget();
        cHSOperate *pHSOp = dynamic_cast<cHSOperate *>(pw);
        if (pHSOp == NULL) {
            QString msg = trUtf8("A szervízpéldányt manipuláló adatlap kiválasztása sikertelen.");
            pUi->textEditMsg->append(htmlError(msg));
            return;
        }
        pHSOp->queryNodeServices(node.getId());
    }
}

void cWorkstation::on_pushButtonSave_clicked()
{
    bool ok;
    ok = node.ports.size() == 1 && node.ports.first() == pnp;
    node.setName(pUi->lineEditName->text());
    node.set(node.noteIndex(), pEditNote->get());
    node.set(_sSerialNumber,    pEditSerialNumber->get());
    node.set(_sInventoryNumber, pEditInventoryNumber->get());
    node.set(_sModelName,       pEditModelName->get());
    node.set(_sModelNumber,     pEditModelNumber->get());
    node.setId(_sNodeType, nodeType);
    node.setId(_sPlaceId, pSelPlace->currentPlaceId());
    pnp->setName(pUi->lineEditPName->text());
    pnp->set(pnp->noteIndex(), pEditPNote->get());
    pnp->set(_sPortTag, pEditPTag->get());
    pnp->setId(_sIfTypeId, pit->getId());
    if (pif != NULL) {
        pif->setName(_sHwAddress, pUi->lineEditPMAC->text());
        pIpEditWidget->get(pip);
        ok = ok && pif->addresses.size() == 1 && pif->addresses.first() == pip;
    }
    if (!ok) {
        EXCEPTION(EPROGFAIL, 0, node.toString());
    }
    sqlBegin(*pq, _sWorkstation);
    if (isModify) {
        ok = cErrorMessageBox::condMsgBox(node.tryUpdateById(*pq), this, trUtf8("Az eszköz modosítása sikertelen."));
    }
    else {
        if (pip != NULL) pip->clearId();
        node.ports.clearId();
        node.clearId(); // A konténerekben töröltük az ID-ket először, igy azokat nem törli!
        node.clear(_sNodeStat);
        ok = cErrorMessageBox::condMsgBox(node.tryInsert(*pq), this, trUtf8("Az eszköz regisztrálása sikertelen."));
    }
    if (ok) {
        qlonglong pid = pnp->getId();
        QString msgPost = trUtf8(" Az egész művelet visszavonásra kerül.");
        if (!pl.isNull(_sPortId2)) {                // Van link
            pl.setId(_sPortId1,      pid);
            pl.setId(_sPhsLinkType1, LT_TERM);
            pl.setId(_sLinkType,     LT_PTP);
            pl.setNote(pUi->lineEditLinkNote->text());
            ok = cErrorMessageBox::condMsgBox(pl.tryReplace(*pq), this, trUtf8("Az eszköz fizikai linkjjének (patch) rögzítépse sikertelen.") + msgPost);
        }
        else if (isModify && pl.isExist(*pq, pid, LT_TERM)) {   // Ha volt link, törlendő
            ok = cErrorMessageBox::condMsgBox(pl.tryRemove(*pq), this, trUtf8("Az eszköz fizikai linkjjének (patch) törlése sikertelen.") + msgPost);
        }
    }
    if (ok) {
        sqlCommit(*pq, _sWorkstation);
    }
    else {
        sqlRollback(*pq, _sWorkstation);
    }
    pSelNode->refresh(false);
    pSelLinked->refresh(false);
    node2gui(true);
    pUi->textEditMsg->append(trUtf8("A bevitt adatok sikeresen el lettek mentve."));
}

void cWorkstation::on_pushButtonPlaceEqu_clicked()
{
    pSelLinked->cSelectPlace::copyCurrents(*pSelPlace);
}

void cWorkstation::on_toolButtonEditLinkNode_clicked()
{
    qlonglong id = pSelLinked->currentNodeId();
    if (id != NULL_ID && pSelLinked->isPatch()) {
        cPatch patch;
        patch.setById(*pq, id);
        patch.fetchPorts(*pq);
        cPatch *p = patchEditDialog(*pq, this, &patch);
        if (p != NULL) {
            pSelLinked->refresh();
            pDelete(p);
        }
    }
}

void cWorkstation::on_toolButtonEditPlace_clicked()
{
    pSelPlace->editCurrentPlace();
}

void cWorkstation::on_toolButtonInfoNode_clicked()
{
    qlonglong nid = pSelNode->currentNodeId();
    if (nid != NULL_ID) {
        popupReportNode(this, *pq, nid);
    }
}
