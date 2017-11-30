#include "snmpdevquery.h"
#include "lv2validator.h"
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
    cTableShapeField *pFieldShape = pDevShape->shapeFields.get(_sNodeType);
    pFieldShape->setName(_sFeatures, ":column=2:hide=patch,node,hub,host,snmp:");
    pTypeWidget = new cSetWidget(*pDevShape, *pFieldShape, (*pDev)[_sNodeType], 0, NULL);

    ui->setupUi(this);
    pButtobGroupSnmpV = new QButtonGroup(this);
    pButtobGroupSnmpV->addButton(ui->radioButtonSnmpV1);
    pButtobGroupSnmpV->addButton(ui->radioButtonSnmpV2c);
    QValidator *pVal = new cINetValidator(false);
    ui->comboBoxIp->setValidator(pVal);
    pSelectNode = new cSelectNode(ui->comboBoxZone, ui->comboBoxPlace, ui->comboBoxNode,
                                  ui->lineEditPatPlace, ui->lineEditPatName);
    cRecordListModel *pNodeModel = new cRecordListModel(_sSnmpDevices);
    pSelectNode->setNodeModel(pNodeModel, TS_TRUE);
    pSelectNode->reset();
    ui->formLayoutType->setWidget(0, QFormLayout::FieldRole, pTypeWidget->pWidget());

    // on_pushButtonSave_clicked()
    // on_pushButtonQuery_clicked()
    // on_lineEditName_textChanged(QString)
    connect(pSelectNode,            SIGNAL(nodeNameChanged(QString)),   this, SLOT(nodeNameChange(QString)));
    // on_comboBoxIp_currentIndexChanged(QString)
    // on_comboBoxIp_currentTextChanged(QString)

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
    qlonglong type = pTypeWidget->getId() | ENUM2SET2(NT_SNMP, NT_HOST);
    cError *pe = NULL;
    QString msg;
    bool isInsert = pDev->getId() == NULL_ID;
    if (isInsert) {
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
        if (isInsert) {
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
        ui->lineEditNodeId->setText(QString::number(pDev->getId()));
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
    QString msg;
    try {
        ui->pushButtonQuery->setDisabled(true);
        if (a.isNull()) {
            ui->textEdit->setHtml(htmlError(trUtf8("Nincs IP cím!")));
            return;
        }
        f = pDev->setBySnmp(comm, EX_ERROR, &msg, &a);
    } CATCHS(pe);
    if (pe != NULL) {
        pe->mDataMsg = msg;
        expError(pe->msg(), true);
        delete pe;
        f = false;
    }
    ui->textEdit->setHtml(cExportQueue::toText(false, true));
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
    if (!pDev->isEmpty_() && !listA.contains(newAddr)) {    // Töröljük mert kavarodás van belöle.
        pDev->clear();
        ui->lineEditNodeId->clear();
        pSelectNode->nodeSetNull();
    }
    a = newAddr;
    ui->pushButtonQuery->setEnabled(!a.isNull() && !ui->lineEditName->text().isEmpty());
}

void cSnmpDevQuery::nodeNameChange(const QString &name)
{
    ui->comboBoxIp->clear();
    if (name.isEmpty()) {
        ui->lineEditName->clear();
        pDev->clear();
        ui->lineEditNodeId->setText(_sNul);
        ui->pushButtonQuery->setDisabled(true);
        ui->pushButtonSave->setDisabled(true);
        pTypeWidget->setId(ENUM2SET2(NT_HOST, NT_SNMP));
        ui->radioButtonSnmpV2c->setChecked(true);
        return;
    }
    if (pDev->fetchByName(*pq, name)) {
        listA = pDev->fetchAllIpAddress(*pq);
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
        ui->lineEditNodeId->setText(QString::number(pDev->getId()));
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
