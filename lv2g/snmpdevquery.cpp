#include "snmpdevquery.h"
#include "lv2validator.h"
#include "ui_snmpdevquery.h"
#include "report.h"

enum ePortTabIx {
    PTI_NAME, PTI_DESCR, PTI_IX, PTI_IFTYPE, PTI_TYPE, PTI_MAC, PTI_IP, PTI_IP_TYPE, PTI_SAVE, PTI_TRUNK, PTI_NOTE
};

cSnmpDevQPort::cSnmpDevQPort(cSnmpDevQuery * _par, int _r, cNPort *_pp)
    : QObject(_par)
    , parent(_par)
    , pTableWidget(_par->ui->tableWidget)
    , table(_par->table)
    , row(_r)
    , pPort(_pp)
    , mac(table[_sIfPhysAddress][row].toByteArray())
{
    pInterface = nullptr;
    pIfType = nullptr;
    pComboBoxIfType = nullptr;
    pIfTypeModel = nullptr;
    pComboBoxIpType = nullptr;
    pIpTypeModel = nullptr;
    pCheckBoxSave = nullptr;
    pComboBoxTrunk = nullptr;
    lastTrunkIx = 0;
    QString s;
    qlonglong id;

    if (pPort != nullptr && pPort->chkObjType<cInterface>(EX_IGNORE) >= 0) pInterface = pPort->reconvert<cInterface>();
    pTableWidget->setRowCount(row +1);
    name = pPort == nullptr ? table[_sIfName][row].toString() : pPort->getName();
    editItem(PTI_NAME,  name);
    tabItem(PTI_DESCR, _sIfDescr);
    tabItem(PTI_IX,    _sIfIndex, true);
    int ifType = table[_sIfType][row].toInt();
    pIfType = cIfType::fromIana(ifType, false, false);
    if (pIfType == nullptr) {
        s = QString::number(ifType);
    }
    else {
        s = pIfType->getName();
        s = QString("%1(%2)").arg(ifType).arg(s);
    }
    strItem(PTI_IFTYPE, s);
    pComboBoxIfType = new QComboBox;
    pIfTypeModel = new cIfTypesModel;
    pTableWidget->setCellWidget(row, PTI_TYPE, pComboBoxIfType);
    pIfTypeModel->joinWith(pComboBoxIfType);
    if (pPort != nullptr) {
        id = pPort->getId(_sIfTypeId);
        pIfType = &cIfType::ifType(id);
        note = pPort->getNote();
    }
    else {
        id = pIfType == nullptr ? NULL_ID : pIfType->getId();
    }
    pIfTypeModel->setCurrent(id);
    editItem(PTI_NOTE, note);
    s = mac.toString();
    strItem(PTI_MAC,  s);
    s = table[_sIpAdEntAddr][row].toString();
    strItem(PTI_IP,  s);
    pCheckBoxSave = new QCheckBox;
    bool save = pPort != nullptr;
    pCheckBoxSave->setEnabled(save);
    pCheckBoxSave->setChecked(save);
    pTableWidget->setCellWidget(row, PTI_SAVE, pCheckBoxSave);
    ipTypeItem();
    trunkItem();
    connect(pComboBoxIfType, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBoxIfType_currentIndexChenged(int)));
    connect(pCheckBoxSave, SIGNAL(toggled(bool)), this, SIGNAL(on_checkBoxSave_toggled(bool)));
}

void cSnmpDevQPort::ipTypeItem()
{
    if (pInterface != nullptr && !pInterface->addresses.isEmpty()) {
        pComboBoxIpType = new QComboBox;
        pIpTypeModel    = new cEnumListModel("addresstype");
        pIpTypeModel->joinWith(pComboBoxIpType);
        QString s = pInterface->addresses.first()->getName(_sIpAddressType);
        pIpTypeModel->setCurrent(s);
        pTableWidget->setCellWidget(row, PTI_IP_TYPE, pComboBoxIpType);
        connect(pComboBoxIpType, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBoxIpType_currentIndexChenged(int)));
    }
    else {
        pTableWidget->removeCellWidget(row, PTI_IP_TYPE);
        pComboBoxIpType = nullptr;
        pIpTypeModel    = nullptr;
        QTableWidgetItem *pi = new QTableWidgetItem;
        pi->setFlags(pi->flags() & ~Qt::ItemIsEditable);
        pTableWidget->setItem(row, PTI_IP_TYPE, pi);
    }
}

void cSnmpDevQPort::trunkItem()
{
    bool trunk = pPort != nullptr && pIfType != nullptr && pIfType->isLinkage() && !parent->trunkPortList.isEmpty();
    if (trunk) {
        int ix = 0;
        int portIndex = int(pPort->getId(_sPortIndex));
        pComboBoxTrunk = new QComboBox;
        pComboBoxTrunk->addItem(_sNul);
        lastTrunkIx = 0;
        foreach (cInterface *p, parent->trunkPortList) {
            QString s = QString("%1(%2)").arg(p->getName(_sPortIndex), p->getName());
            pComboBoxTrunk->addItem(s);
            if (p->trunkMembers.contains(portIndex)) {
                if (lastTrunkIx == 0) {
                    lastTrunkIx = ix + 1;
                }
                else {
                    QString msg = tr("Egy port nem lehet két runk porthoz is hozzárendelve! port : %1, trunks : %2 and %3")
                            .arg(pPort->getName(), parent->trunkPortList.at(lastTrunkIx -1)->getName(), p->getName());
                    expError(msg);
                }
            }
            ++ix;
        }
        pComboBoxTrunk->setCurrentIndex(lastTrunkIx);
        pTableWidget->setCellWidget(row, PTI_TRUNK, pComboBoxTrunk);
        connect(pComboBoxTrunk, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBoxTrunk_currentIndexChenged()));
    }
    else {
        pTableWidget->removeCellWidget(row, PTI_TRUNK);
        pComboBoxTrunk = nullptr;
        QTableWidgetItem *pi = new QTableWidgetItem;
        pi->setFlags(pi->flags() & ~Qt::ItemIsEditable);
        pTableWidget->setItem(row, PTI_TRUNK, pi);
    }
}

void cSnmpDevQPort::on_comboBoxIfType_currentIndexChenged(int)
{
    pIfType = pIfTypeModel->current();
    qlonglong ifTypeId = pIfType->getId();
    QString objectType = pIfType->getName(_sIfTypeObjType);

    if (pPort != nullptr) { // Has been a real port so far.
        qlonglong portIfTypeId = pPort->getId(_sIfTypeId);
        QString portIfTypeObjType = cIfType::ifType(portIfTypeId).getName(_sIfTypeObjType);
        if (portIfTypeObjType == objectType) {  // port object type is unchanged
            pPort->setId(_sIfTypeId, ifTypeId);
            pCheckBoxSave->setEnabled(true);
            pCheckBoxSave->setChecked(true);
        }
        else {              // will not be a real port, or recreate port object
            int ixPortIndex = pPort->ixPortIndex();
            delete parent->pDev->ports.pull(ixPortIndex, pPort->get(ixPortIndex));
            pPort = nullptr;
            pInterface = nullptr;
            ipTypeItem();
        }
    }
    // Has not been a real port so far, or recreate. pPort is NULL.
    if (pPort == nullptr) {
        if (objectType != _sUnknown) {    // Will be a real port
            pPort = cNPort::newPortObj(*pIfType);
            pPort->setName(name);
            pPort->setNote(note);
            pPort->set(_sPortIndex, table[_sIfIndex][row]);
            if (objectType == _sInterface) {
                pInterface = pPort->reconvert<cInterface>();
                if (mac.isValid()) pInterface->setMac(_sHwAddress, mac);
                pInterface->set(_sIfDescr.toLower(), table[_sIfDescr][row]);
                QHostAddress addr(table[_sIpAdEntAddr][row].toString());
                if (!addr.isNull()) {
                    pInterface->addIpAddress(addr, AT_FIXIP);
                    ipTypeItem();
                }
            }
            parent->pDev->ports << pPort;
            pCheckBoxSave->setEnabled(true);
            pCheckBoxSave->setChecked(true);
        }
        else {
            pCheckBoxSave->setEnabled(false);
            pCheckBoxSave->setChecked(false);
        }
    }
    parent->refreshTrunks();
}

void cSnmpDevQPort::on_checkBoxSave_toggled(bool checkState)
{
    if (checkState) {
        if (pPort == nullptr) {
            on_comboBoxIfType_currentIndexChenged(0);
        }
    }
    else {
        if (pPort != nullptr) {
            int ixPortIndex = pPort->ixPortIndex();
            delete parent->pDev->ports.pull(ixPortIndex, pPort->get(ixPortIndex));
            pPort = nullptr;
        }
    }
}

void cSnmpDevQPort::on_comboBoxIpType_currentIndexChenged(int)
{
    if (pInterface == nullptr || pInterface->addresses.isEmpty()) return;
    QString s = pIpTypeModel->current();
    pInterface->addresses.first()->setName(_sIpAddressType, s);
}

void cSnmpDevQPort::on_tableWidget_itemChanged(const QTableWidgetItem * pi)
{
    int col = pTableWidget->column(pi);
    switch (col) {
    case PTI_NAME:
        name = pi->text();
        if (pPort) pPort->setName(name);
        break;
    case PTI_NOTE:
        note = pi->text();
        if (pPort) pPort->setNote(note);
        break;
    default:
        break;
    }
}

void cSnmpDevQPort::on_comboBoxTrunk_currentIndexChenged(int ix)
{
    if (pPort == nullptr || lastTrunkIx == ix) return;
    int portIndex = int(pPort->getId(_sPortIndex));
    cInterface *p;
    if (lastTrunkIx > 0) {
        p = parent->trunkPortList.at(lastTrunkIx -1);
        p->trunkMembers.removeAll(portIndex);
    }
    lastTrunkIx = ix;
    bool trunk = ix > 0;
    if (trunk) {
        p = parent->trunkPortList.at(lastTrunkIx -1);
        p->trunkMembers << portIndex;
    }
}

/* ---------------------------------------------------------------------------------- */

const enum ePrivilegeLevel cSnmpDevQuery::rights = PL_OPERATOR;
const QString cSnmpDevQuery::sqlSnmp = "'snmp' = ANY (node_type)";
const QString cSnmpDevQuery::sqlHost = "'host' = ANY (node_type)";

cSnmpDevQuery::cSnmpDevQuery(QMdiArea *parent) :
    cIntSubObj(parent),
    ui(new Ui::cSnmpDevQuery)
{
    pq = newQuery();
    pDev  = nullptr;
    pHost = nullptr;
    pDevShape = new cTableShape();              // A típus widgethez kellenek a leírók (shape)
    pDevShape->setByName(*pq, _sSnmpDevices);
    pDevShape->fetchFields(*pq, true);
    cTableShapeField *pFieldShape = pDevShape->shapeFields.get(_sNodeType);
    pFieldShape->setName(_sFeatures, ":column=2:hide=patch,node,hub,host,snmp:");
    pTypeWidget = new cSetWidget(*pDevShape, *pFieldShape, o[_sNodeType], 0, nullptr);

    ui->setupUi(this);
    ui->labelInventory->  setText(pDevShape->shapeFields.get(_sInventoryNumber)->getText(cTableShapeField::LTX_DIALOG_TITLE));
    ui->labelSerial->     setText(pDevShape->shapeFields.get(_sSerialNumber)->   getText(cTableShapeField::LTX_DIALOG_TITLE));
    ui->labelModelNumber->setText(pDevShape->shapeFields.get(_sModelNumber)->    getText(cTableShapeField::LTX_DIALOG_TITLE));
    ui->labelModelName->  setText(pDevShape->shapeFields.get(_sModelName)->      getText(cTableShapeField::LTX_DIALOG_TITLE));
    ui->labelOsName->     setText(pDevShape->shapeFields.get(_sOsName)->         getText(cTableShapeField::LTX_DIALOG_TITLE));
    ui->labelOsVersion->  setText(pDevShape->shapeFields.get(_sOsVersion)->      getText(cTableShapeField::LTX_DIALOG_TITLE));
    pButtobGroupSnmpV = new QButtonGroup(this);
    pButtobGroupSnmpV->addButton(ui->radioButtonSnmpV1);
    pButtobGroupSnmpV->addButton(ui->radioButtonSnmpV2c);
    QValidator *pVal = new cINetValidator(false);
    ui->comboBoxIp->setValidator(pVal);
    pSelectNode = new cSelectNode(ui->comboBoxZone, ui->comboBoxPlace, ui->comboBoxNode,
                                  ui->lineEditPatPlace, ui->lineEditPatName,
                                  _sNul, sqlSnmp, this);
    pSelectNode->reset();
    ui->verticalLayoutType->addWidget(pTypeWidget);
    convertToSnmp = false;

    // connected: on_pushButtonSave_clicked()
    // connected: on_pushButtonQuery_clicked()
    // connected: on_lineEditName_textChanged(QString)
    connect(pSelectNode,            SIGNAL(nodeNameChanged(QString)),   this, SLOT(nodeNameChange(QString)));
    // connected: on_comboBoxIp_currentIndexChanged(QString)
    // connected: on_comboBoxIp_currentTextChanged(QString)
    // connected: on_checkBoxToSnmp_toggled(bool)

}

cSnmpDevQuery::~cSnmpDevQuery()
{
    clearPorts();
    delete ui;
    clearHost();
    delete pq;
    delete pDevShape;

}

inline void setTextField(cRecord *po, const QString fn, QLineEdit *pLE)
{
    QString txt = pLE->text();
    if (txt.isEmpty()) po->clear(fn);
    else               po->setName(fn, txt);
}

void cSnmpDevQuery::on_pushButtonSave_clicked()
{
    if (pDev == nullptr) return;
    QString msg;
    QString name = ui->lineEditName->text();
    bool isInsert = pHost == nullptr;
    bool isConvert = pHost != nullptr && pDev != pHost;
    if (isConvert) {
        if (!convertToSnmp || isInsert) EXCEPTION(EPROGFAIL);
        msg = tr("Nem SNMP eszköz konvertálása SNMP eszközzé.\n");
    }
    if (isInsert) {
        msg = tr("A %1 nevű eszköz beillesztése az adatbázisba.").arg(name);
        pDev->setName(name);
    }
    else if (name == pDev->getName()) {
        msg += tr("A %1 nevű eszköz frissítése az adatbázisban.").arg(name);
    }
    else {
        msg += tr("A %1 nevű eszköz frissítése az adatbázisban, új név %2.").arg(pDev->getName(), name);
        pDev->setName(name);
    }

    qlonglong type = pTypeWidget->getId() | ENUM2SET2(NT_SNMP, NT_HOST);
    cError *pe = nullptr;
    ui->textEdit->append(htmlWarning(msg));
    static const QString tn = "SnmpDevQuerySave";
    try {
        ui->pushButtonSave->setDisabled(true);
        setTextField(pDev, _sNodeNote,        ui->lineEditNote);
        setTextField(pDev, _sInventoryNumber, ui->lineEditInventory);
        setTextField(pDev, _sSerialNumber,    ui->lineEditSerial);
        setTextField(pDev, _sModelNumber,     ui->lineEditModelNumber);
        setTextField(pDev, _sModelName,       ui->lineEditModelName);
        setTextField(pDev, _sOsName,          ui->lineEditOsName);
        setTextField(pDev, _sOsVersion,       ui->lineEditOsVersion);
        pDev->setId(_sNodeType, type);
        pDev->setId(_sPlaceId, pSelectNode->currentPlaceId());
        sqlBegin(*pq, tn);
        if (isConvert) {
            static const QString sql = "SELECT node2snmpdevice(?)";
            execSql(*pq, sql, pHost->getId());
            pDev->setId(pHost->getId());
        }
        if (isInsert) {
            pDev->insert(*pq);
        }
        else {
            pDev->rewriteById(*pq);
        }
        sqlCommit(*pq, tn);
    } CATCHS(pe)
    if (pe != nullptr) {
        ui->textEdit->append(htmlError(pe->msg(), true));
        sqlRollback(*pq, tn);
        delete pe;
    }
    else {
        ui->textEdit->append(_sOk);
        ui->lineEditNodeId->setText(QString::number(pDev->getId()));
    }
}

void cSnmpDevQuery::on_pushButtonQuery_clicked()
{
    clearPorts();
    QString name = ui->lineEditName->text();
    if (name.isEmpty()) return;
    bool f = false;
    cError * pe = nullptr;
    QString comm = ui->lineEditCom->text();
    if (pDev  == nullptr) pDev = new cSnmpDevice;
    if (pHost != nullptr) pDev->set_(*pHost);
    pDev->setNote(ui->lineEditNote->text());
    pDev->setName(_sSnmpVer, ui->radioButtonSnmpV1->isChecked() ? "1" : "2c");
    qlonglong type = pTypeWidget->getId() | ENUM2SET2(NT_SNMP, NT_HOST);
    pDev->setId(_sNodeType, type);
    cExportQueue::init(false);
    ui->textEdit->setHtml(htmlWarning(tr("Lekérdezés indítása, kérem várjon!")));
    QString msg;
    try {
        ui->pushButtonQuery->setDisabled(true);
        if (a.isNull()) {
            ui->textEdit->setHtml(htmlError(tr("Nincs IP cím!")));
            return;
        }
        table.clear();
        f = pDev->setBySnmp(comm, EX_ERROR, &msg, &a, &table);
        if (f) {    // refresh port table
            int rows = table.first().size();    // all ports number
            int validPorts = 0;
            setTrunkPortList();
            for (int row = 0; row < rows; ++row) {
                QString name = table[_sIfName][row].toString();
                cNPort *p = pDev->ports.get(name, EX_IGNORE);
                portRows << new cSnmpDevQPort(this, row, p);
                if (p != nullptr) validPorts++;
            }
            ui->tableWidget->resizeColumnsToContents();
            if (validPorts != pDev->ports.size()) {
                expError(trUtf8("validPorts : %1 != ports.size() : %2").arg(validPorts).arg(pDev->ports.size()));
            }
        }
    } CATCHS(pe)
    if (pe != nullptr) {
        pe->mDataMsg = msg;
        expError(pe->msg(), true);
        delete pe;
        f = false;
    }
    ui->textEdit->setHtml(cExportQueue::toText(false, true));
    type = pDev->getId(_sNodeType);
    pTypeWidget->setId(type);
    ui->pushButtonSave->setEnabled(f);
}

void cSnmpDevQuery::ipTextChanged(const QString &s)
{
    if (s.isEmpty()) {
        a.clear();
        return;
    }
    QHostAddress newAddr;
    newAddr.setAddress(s);
    if (pHost != nullptr && !listA.contains(newAddr)) {    // Töröljük mert kavarodás van belöle.
        clearHost();
        ui->lineEditNodeId->clear();
        pSelectNode->nodeSetNull();
    }
    a = newAddr;
    ui->pushButtonQuery->setEnabled(!a.isNull() && !ui->lineEditName->text().isEmpty());
}

void cSnmpDevQuery::on_lineEditName_textChanged(const QString &s)
{
    ui->pushButtonQuery->setEnabled(!a.isNull() && !s.isEmpty());
}

void cSnmpDevQuery::on_comboBoxIp_currentIndexChanged(const QString &arg1)
{
    ipTextChanged(arg1);
}

void cSnmpDevQuery::on_comboBoxIp_currentTextChanged(const QString &arg1)
{
    ipTextChanged(arg1);
}

void cSnmpDevQuery::on_checkBoxToSnmp_toggled(bool checked)
{
    convertToSnmp = checked;
    if (convertToSnmp) {
        pSelectNode->changeNodeConstFilter(sqlHost);
    }
    else {
        pSelectNode->changeNodeConstFilter(sqlSnmp);
    }
    pSelectNode->refresh();
}

void cSnmpDevQuery::clearPorts()
{
    trunkPortList.clear();
    ui->tableWidget->setRowCount(0);
    while (!portRows.isEmpty()) delete portRows.takeLast();
}
void cSnmpDevQuery::setTrunkPortList()
{
    int ixIfTypeId = cNPort().ixIfTypeId();
    qlonglong trunkTypeId = cIfType::ifTypeId(_sMultiplexor);
    trunkPortList.clear();
    foreach (cNPort *p, pDev->ports.list()) {
        qlonglong ifTypeId = p->getId(ixIfTypeId);
        if (ifTypeId == trunkTypeId) trunkPortList << p->reconvert<cInterface>();
    }
}

void cSnmpDevQuery::nodeNameChange(const QString &name)
{
    ui->comboBoxIp->clear();
    clearHost();
    if (name.isEmpty()) {
        ui->lineEditName->clear();
        ui->lineEditNodeId->setText(_sNul);
        ui->pushButtonQuery->setDisabled(true);
        ui->pushButtonSave->setDisabled(true);
        pTypeWidget->setId(ENUM2SET2(NT_HOST, NT_SNMP));
        ui->radioButtonSnmpV2c->setChecked(true);
        return;
    }
    cPatch *p = cPatch::getNodeObjByName(*pq, name);
    pHost = p->reconvert<cNode>();
    if (0 == p->chkObjType<cSnmpDevice>(EX_IGNORE)) {   // Host is SNMP device
        pDev = static_cast<cSnmpDevice *>(p);
    }
    else {
        if (!convertToSnmp) EXCEPTION(EPROGFAIL);
    }
    listA = pHost->fetchAllIpAddress(*pq);
    a.clear();
    if (!listA.isEmpty()) {
        QStringList items;
        foreach (QHostAddress ha, listA) {
            items << ha.toString();
        }
        ui->comboBoxIp->addItems(items);
        a = listA.first();
        ui->comboBoxIp->setCurrentIndex(0);
    }
    ui->lineEditName->setText(name);
    ui->lineEditNodeId->setText(QString::number(pHost->getId()));
    pTypeWidget->setId(pHost->getId(_sNodeType));
    qlonglong pid = pHost->getId(_sPlaceId);
    if (pSelectNode->currentPlaceId() != pid) {
        qlonglong nid = pHost->getId();
        pSelectNode->bbNode.begin();
        pSelectNode->setCurrentPlace(pid);
        pSelectNode->setCurrentNode(nid);
        pSelectNode->bbNode.end(false);
    }
    QString community = pDev == nullptr ? _sPublic : pDev->getName(_sCommunityRd);
    ui->lineEditCom->setText(community);
    int snmpversion = pDev == nullptr ? SNMP_VERSION_2c : pDev->snmpVersion();
    switch (snmpversion) {
    case SNMP_VERSION_1:
        ui->radioButtonSnmpV1->setChecked(true);
        break;
    case SNMP_VERSION_2c:
    default:
        ui->radioButtonSnmpV2c->setChecked(true);
        break;
    }
    ui->lineEditNote->       setText(pHost->getNote());
    ui->lineEditInventory->  setText(pHost->getName(_sInventoryNumber));
    ui->lineEditSerial->     setText(pHost->getName(_sSerialNumber));
    ui->lineEditModelNumber->setText(pHost->getName(_sModelNumber));
    ui->lineEditModelName->  setText(pHost->getName(_sModelName));
    ui->lineEditOsName->     setText(pHost->getName(_sOsName));
    ui->lineEditOsVersion->  setText(pHost->getName(_sOsVersion));
    ui->pushButtonQuery->setDisabled(a.isNull());
    ui->pushButtonSave->setDisabled(true);
}

void cSnmpDevQuery::refreshTrunks()
{
    setTrunkPortList();
    foreach (cSnmpDevQPort * pRow, portRows) {
        pRow->trunkItem();
    }
}

void cSnmpDevQuery::on_pushButtonLocalhost_clicked()
{
    cNode *pSelf = cNode::getSelfNodeObjByMac(*pq);
    if (pSelf == nullptr) {
        QString name = ui->lineEditName->text();
        ui->comboBoxIp->clear();
        ui->comboBoxIp->addItem("127.0.0.1");
        if (name.isEmpty()) return;
        ui->lineEditName->setText(name);
        if (ui->lineEditCom->text().isEmpty()) ui->lineEditCom->setText(_sPublic);
    }
    else {
        bool notSnmp = pSelf->tableoid() == cNode().tableoid();
        ui->checkBoxToSnmp->setChecked(notSnmp);
        pSelectNode->setCurrentNode(pSelf->getId());
    }
    on_pushButtonQuery_clicked();
}
