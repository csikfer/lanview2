#include "snmpdevquery.h"
#include "ui_snmpdevquery.h"
#include "report.h"

const enum ePrivilegeLevel cSnmpDevQuery::rights = PL_OPERATOR;

cSnmpDevQuery::cSnmpDevQuery(QMdiArea *parent) :
    cIntSubObj(parent),
    ui(new Ui::cSnmpDevQuery)
{
    pq = newQuery();
    pDev = new cSnmpDevice;
    pDev->setParent(this);
    pDevShape = new cTableShape();              // A típus widgethez kellenek a leírók (shape)
    pDevShape->setByName(*pq, _sSnmpDevices);
    pDevShape->fetchFields(*pq);
    pTypeWidget = new cSetWidget(*pDevShape, *pDevShape->shapeFields.get(_sNodeType), (*pDev)[_sNodeType], 0, NULL);

    ui->setupUi(this);
    pButtobGroupSnmpV = new QButtonGroup(this);
    pButtobGroupSnmpV->addButton(ui->radioButtonSnmpV1);
    pButtobGroupSnmpV->addButton(ui->radioButtonSnmpV2c);
    pSelectNode = new cSelectNode(ui->comboBoxZone, ui->comboBoxPlace, ui->comboBoxNode,
                                  ui->lineEditPatPlace, ui->lineEditPatName);
    cRecordListModel *pNodeModel = new cRecordListModel(_sSnmpDevices);
    pSelectNode->setNodeModel(pNodeModel, TS_TRUE);
    pSelectNode->reset();
    ui->formLayoutType->setWidget(0, QFormLayout::FieldRole, pTypeWidget->pWidget());

    // connect(ui->pushButtonSave,     SIGNAL(clicked()),                  this, SLOT(on_pushButtonSave_clicked()));
    // connect(ui->pushButtonQuery,    SIGNAL(clicked()),                  this, SLOT(on_pushButtonQuery_clicked()));
    // connect(ui->lineEditIp,         SIGNAL(textChanged(QString)),       this, SLOT(on_lineEditIp_textChanged(QString)));
    connect(pSelectNode,            SIGNAL(nodeNameChanged(QString)),   this, SLOT(nodeNameChange(QString)));
}

cSnmpDevQuery::~cSnmpDevQuery()
{
    delete ui;
    delete pDev;
    delete pq;
    delete pDevShape;
}

void cSnmpDevQuery::on_pushButtonSave_clicked()
{
    QString name = ui->lineEditName->text();
    QString note = ui->lineEditNote->text();
    qlonglong type = pTypeWidget->getId();
    cError *pe = NULL;
    QString msg;
    if (pDev->getId() == NULL_ID) {
        msg = trUtf8("A %1 nevű eszköz beillesztése az adatbázisba.").arg(name);
        pDev->setName(name);
    }
    else if (name == pDev->getName()) {
        msg = trUtf8("A %1 nevű eszköz frissítése az adatbázisban.").arg(name);
    }
    else {
        msg = trUtf8("A %1 nevű eszköz frissítése az adatbázisban, új név %2.").arg(pDev->getName(), name);
        pDev->setName(name);
    }
    ui->textEdit->append(htmlWarning(msg));
    static const QString tn = "SnmpDevQuerySave";
    try {
        ui->pushButtonSave->setDisabled(true);
        pDev->setNote(note);
        pDev->setId(_sNodeType, type);
        pDev->setId(_sPlaceId, pSelectNode->currentPlaceId());
        sqlBegin(*pq, tn);
        if (pDev->getId() == NULL_ID) {
            pDev->insert(*pq);
        }
        else {
            pDev->rewriteById(*pq);
        }
        sqlCommit(*pq, tn);
    } CATCHS(pe);
    if (pe != NULL) {
        ui->textEdit->append(htmlError(pe->msg(), true));
        sqlRollback(*pq, tn);
        delete pe;
    }
    else {
        ui->textEdit->append(_sOk);
    }
}

void cSnmpDevQuery::on_pushButtonQuery_clicked()
{
    bool f = false;
    cError * pe = NULL;
    QString name = ui->lineEditName->text();
    QString comm = ui->lineEditCom->text();
    if (name.isEmpty()) return;
    cExportQueue::init(false);
    ui->textEdit->setHtml(htmlWarning(trUtf8("Lekérdezés indítása, kérem várjon!")));
    try {
        ui->pushButtonQuery->setDisabled(true);
        if (pDev->getId() == NULL_ID) {
            QStringPair ip;
            ip.first = a.toString();
            ip.second= _sFixIp;
            pDev->asmbNode(*pq, name, NULL, &ip, &_sARP, _sNul, NULL_ID);
            QString v = ui->radioButtonSnmpV1->isChecked() ? "1" : "2c";
            pDev->setName(_sSnmpVer, v);
        }
        else if (pDev->getIpAddress().isNull()) {
            ui->textEdit->setHtml(htmlError(trUtf8("Nincs IP cím!")));
            return;
        }
        f = pDev->setBySnmp(comm);
    } CATCHS(pe);
    if (pe != NULL) {
        expError(pe->msg(), true);
        delete pe;
        f = false;
    }
    ui->textEdit->setHtml(cExportQueue::toText(false, true));
    ui->pushButtonSave->setEnabled(f);
}

void cSnmpDevQuery::on_lineEditIp_textChanged(const QString &s)
{
    QHostAddress newAddr;
    newAddr.setAddress(s);
    if (!pDev->isEmpty_() && a != newAddr) {    // Töröljük mert kavarodás van belöle.
        pDev->clear();
        pSelectNode->nodeSetNull();
    }
    a = newAddr;
    ui->pushButtonQuery->setEnabled(!a.isNull() && !ui->lineEditName->text().isEmpty());
}

void cSnmpDevQuery::on_lineEditName_textChanged(const QString &s)
{
    ui->pushButtonQuery->setEnabled(!a.isNull() && !s.isEmpty());
}

void cSnmpDevQuery::nodeNameChange(const QString &name)
{
    if (name.isEmpty()) {
        ui->lineEditIp->clear();
        ui->lineEditName->clear();
        pDev->clear();
        ui->pushButtonQuery->setDisabled(true);
        ui->pushButtonSave->setDisabled(true);
        pTypeWidget->setId(ENUM2SET2(NT_HOST, NT_SNMP));
        ui->radioButtonSnmpV2c->setChecked(true);
        return;
    }
    if (pDev->fetchByName(*pq, name)) {
        a = pDev->getIpAddress(*pq);
        ui->lineEditName->setText(name);
        ui->lineEditIp->setText(a.toString());
        ui->lineEditCom->setText(pDev->getName(_sCommunityRd));
        pTypeWidget->setId(pDev->getId(_sNodeType));
        ui->pushButtonQuery->setDisabled(true);
        ui->pushButtonSave->setDisabled(false);
        switch (pDev->snmpVersion()) {
        case SNMP_VERSION_1:    ui->radioButtonSnmpV1->setChecked(true);    break;
        case SNMP_VERSION_2c:   ui->radioButtonSnmpV2c->setChecked(true);   break;
        default:
            ui->radioButtonSnmpV1->setChecked(false);
            ui->radioButtonSnmpV2c->setChecked(false);
        }
        ui->pushButtonQuery->setDisabled(a.isNull());
    }
    ui->pushButtonSave->setDisabled(true);
}
