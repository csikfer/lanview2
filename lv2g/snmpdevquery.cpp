#include "snmpdevquery.h"
#include "lv2validator.h"
#include "ui_snmpdevquery.h"
#include "report.h"

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
    pButtobGroupSnmpV = new QButtonGroup(this);
    pButtobGroupSnmpV->addButton(ui->radioButtonSnmpV1);
    pButtobGroupSnmpV->addButton(ui->radioButtonSnmpV2c);
    QValidator *pVal = new cINetValidator(false);
    ui->comboBoxIp->setValidator(pVal);
    pSelectNode = new cSelectNode(ui->comboBoxZone, ui->comboBoxPlace, ui->comboBoxNode,
                                  ui->lineEditPatPlace, ui->lineEditPatName,
                                  _sNul, sqlSnmp, this);
    pSelectNode->reset();
    ui->formLayoutType->setWidget(0, QFormLayout::FieldRole, pTypeWidget);
    convertToSnmp = false;

    // on_pushButtonSave_clicked()
    // on_pushButtonQuery_clicked()
    // on_lineEditName_textChanged(QString)
    connect(pSelectNode,            SIGNAL(nodeNameChanged(QString)),   this, SLOT(nodeNameChange(QString)));
    // on_comboBoxIp_currentIndexChanged(QString)
    // on_comboBoxIp_currentTextChanged(QString)
    // on_checkBoxToSnmp_toggled(bool)

}

cSnmpDevQuery::~cSnmpDevQuery()
{
    delete ui;
    clearHost();
    delete pq;
    delete pDevShape;
}

void cSnmpDevQuery::on_pushButtonSave_clicked()
{
    if (pDev == nullptr) return;
    QString msg;
    QString name = ui->lineEditName->text();
    bool isInsert = pHost == nullptr;
    bool isConvert = pDev != pHost;
    if (isConvert) {
        if (!convertToSnmp || isInsert) EXCEPTION(EPROGFAIL);
        msg = trUtf8("Nem SNMP eszköz konvertálása SNMP eszközzé.\n");
    }
    if (isInsert) {
        msg = trUtf8("A %1 nevű eszköz beillesztése az adatbázisba.").arg(name);
        pDev->setName(name);
    }
    else if (name == pDev->getName()) {
        msg += trUtf8("A %1 nevű eszköz frissítése az adatbázisban.").arg(name);
    }
    else {
        msg += trUtf8("A %1 nevű eszköz frissítése az adatbázisban, új név %2.").arg(pDev->getName(), name);
        pDev->setName(name);
    }

    QString note = ui->lineEditNote->text();
    qlonglong type = pTypeWidget->getId() | ENUM2SET2(NT_SNMP, NT_HOST);
    cError *pe = nullptr;
    ui->textEdit->append(htmlWarning(msg));
    static const QString tn = "SnmpDevQuerySave";
    try {
        ui->pushButtonSave->setDisabled(true);
        pDev->setNote(note);
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
    } CATCHS(pe);
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
    QString name = ui->lineEditName->text();
    if (name.isEmpty()) return;
    bool f = false;
    cError * pe = nullptr;
    QString comm = ui->lineEditCom->text();
    if (pDev == nullptr) {
        pDev = new cSnmpDevice;
        if (pHost != nullptr) pDev->set_(*pHost);
    }
    pDev->setName(_sSnmpVer, ui->radioButtonSnmpV1->isChecked() ? "1" : "2c");
    qlonglong type = pTypeWidget->getId() | ENUM2SET2(NT_SNMP, NT_HOST);
    pDev->setId(_sNodeType, type);
    cExportQueue::init(false);
    ui->textEdit->setHtml(htmlWarning(trUtf8("Lekérdezés indítása, kérem várjon!")));
    QString msg;
    try {
        ui->pushButtonQuery->setDisabled(true);
        if (a.isNull()) {
            ui->textEdit->setHtml(htmlError(trUtf8("Nincs IP cím!")));
            return;
        }
        f = pDev->setBySnmp(comm, EX_ERROR, &msg, &a);
    } CATCHS(pe);
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
    ui->pushButtonQuery->setDisabled(a.isNull());
    ui->pushButtonSave->setDisabled(true);
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
