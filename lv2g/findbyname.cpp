#include "findbyname.h"
#include "ui_basefind.h"
#include "ui_findbyname.h"


cFindByName::cFindByName(QMdiArea *parent) :
    cBaseFind(parent),
    pUiHead(new Ui::FindByName)
{
    pSelectNode = nullptr;
    fN = false;
    pUiHead->setupUi(pUi->widget);
    // RDP Windows-on, ha ez egy RDP kliens
    if (rdpClientAddr.isNull()) {
        pUiHead->pushButtonRDP->hide();
    }
    else {
        connect(pUiHead->pushButtonRDP, SIGNAL(clicked()), this, SLOT(pushButtonRDP_clicked()));
    }

    if (fSw) {
        pUiHead->comboBoxSw->addItems(switchItems);
        pUiHead->comboBoxSw->setCurrentIndex(centralSwitchIndex);
        connect(pUiHead->pushButtonExplore, SIGNAL(clicked()), this, SLOT(pushButtonExplore_clicked()));
    }
    else {
        pUiHead->comboBoxSw->hide();
        pUiHead->labelSw->hide();
        pUiHead->pushButtonExplore->hide();
    }
    if (nmapExists == TS_FALSE) {
        pUiHead->pushButtonNMap->hide();
    }
    pSelectNode = new cSelectNode(pUiHead->comboBoxZone, pUiHead->comboBoxPlace, pUiHead->comboBoxNode, pUiHead->lineEditPlacePattern, pUiHead->lineEditNodePattern);
    connect(pSelectNode,                 SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(selectedNodeChanged(qlonglong)));
    connect(pUiHead->pushButtonExplore,  SIGNAL(clicked()),                this, SLOT(pushButtonExplore_clicked()));
    connect(pUiHead->pushButtonRDP,      SIGNAL(clicked()),                this, SLOT(pushButtonRDP_clicked()));
    connect(pUiHead->pushButtonNMap,     SIGNAL(clicked()),                this, SLOT(pushButtonNMap_clicked()));
    connect(pUiHead->pushButtonLocalhost,SIGNAL(clicked()),                this, SLOT(pushButtonLocalhost_clicked()));
    connect(pUiHead->pushButtonReport,   SIGNAL(clicked()),                this, SLOT(pushButtonReport_clicked()));

}

cFindByName::~cFindByName()
{
    delete pSelectNode;
}

void cFindByName::setButtons()
{
    pUiHead->pushButtonReport->setEnabled(fN);
    pUiHead->pushButtonNMap->setEnabled(fIP);
}

void cFindByName::selectedNodeChanged(qlonglong nid)
{
    fN = fIP = nid != NULL_ID;
    if (fN) {
        cNode node;
        addressList = node.fetchAllIpAddress(*pq, nid);
        fIP = !addressList.isEmpty();
    }
    QStringList addresses;
    if (fIP) {
        for (QHostAddress& a: addressList) {
            if (!a.isNull()) addresses << a.toString();
        }
        fIP = !addresses.isEmpty();
    }
    pUiHead->comboBoxIP->clear();
    pUiHead->comboBoxIP->setEnabled(fIP);
    if (fIP) pUiHead->comboBoxIP->addItems(addresses);
    setButtons();
}

void cFindByName::clearPostWork()
{
    pUiHead->comboBoxIP->clear();
    pSelectNode->setCurrentZone(lv2g::getInstance()->zoneId);
    pSelectNode->setCurrentPlace(NULL_ID);
    pSelectNode->setCurrentNode(NULL_ID);
    fN = fIP = false;
    setButtons();
}

void cFindByName::pushButtonExplore_clicked()
{
#ifdef SNMP_IS_EXISTS
    QHostAddress ip(pUiHead->comboBoxIP->currentText());
    cMac mac; //  *****
    QString swName = pUiHead->comboBoxSw->currentText();
    if (ip.isNull() || !mac.isValid()) {
        appendReport(htmlError(tr("Hibás adatok.")));
        return;
    }
    discover(ip, mac, swName);
    pUiHead->pushButtonExplore->setDisabled(true);
#endif
}

void cFindByName::finishedPostWork()
{
    pUiHead->pushButtonExplore->setDisabled(false);
}

void cFindByName::pushButtonNMap_clicked()
{
    NMap(pUiHead->comboBoxIP->currentText());
}

void cFindByName::pushButtonLocalhost_clicked()
{
    pSelectNode->setCurrentNode(lanView::getInstance()->selfNode().getId());
}

void cFindByName::pushButtonReport_clicked()
{
    cRecord *pNode = cNode::getNodeObjByName(*pq, pSelectNode->currentNodeName(), EX_IGNORE);
    if (pNode != nullptr) {
        tStringPair texts = htmlReportNode(*pq, *pNode);
        appendReport(texts.first);
        appendReport(texts.second);
        delete pNode;
    }
}
