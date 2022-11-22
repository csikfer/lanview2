#include "findbymac.h"
#include "srvdata.h"

#include "ui_basefind.h"
#include "ui_findbymac.h"

cFindByMac::cFindByMac(QMdiArea *parent) :
    cBaseFind(parent) ,
    pUiHead(new Ui::FindByMac)
{
    pUiHead->setupUi(pUi->widget);
    setAllMac();
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
    comboBoxMAC_currentTextChanged(pUiHead->comboBoxMAC->currentText());
    connect(pUiHead->comboBoxMAC,         SIGNAL(currentTextChanged(QString)), this, SLOT(comboBoxMAC_currentTextChanged(QString)));
    connect(pUiHead->comboBoxIP,          SIGNAL(currentTextChanged(QString)), this, SLOT(comboBoxIP_currentTextChanged(QString)));
    connect(pUiHead->pushButtonFindMac,   SIGNAL(clicked()),                   this, SLOT(pushButtonFindMac_clicked()));
    connect(pUiHead->toolButtonIP2MAC,    SIGNAL(clicked()),                   this, SLOT(toolButtonIP2MAC_clicked()));
    connect(pUiHead->toolButtonMAC2IP,    SIGNAL(clicked()),                   this, SLOT(toolButtonMAC2IP_clicked()));
    connect(pUiHead->pushButtonFindIp,    SIGNAL(clicked()),                   this, SLOT(pushButtonFindIp_clicked()));
    connect(pUiHead->pushButtonNMap,      SIGNAL(clicked()),                   this, SLOT(pushButtonNMap_clicked()));
    connect(pUiHead->pushButtonLocalhost, SIGNAL(clicked()),                   this, SLOT(pushButtonLocalhost_clicked()));
    connect(pUiHead->comboBoxLoIp,        SIGNAL(activated(QString)),          this, SLOT(comboBoxLoIp_activated(QString)));
    connect(pUiHead->comboBoxLoMac,       SIGNAL(activated(int)),              this, SLOT(comboBoxLoMac_activated(int)));
}

cFindByMac::~cFindByMac()
{
    ;
}

void cFindByMac::setMacAnIp(const cMac& mac, const QHostAddress& a)
{
    pUiHead->comboBoxIP->setCurrentText(a.toString());
    pUiHead->comboBoxMAC->setCurrentText(mac.toString());
}

void cFindByMac::setAllMac()
{
    // Query: All known MAC
    static const QString sql =
            "SELECT hwaddress FROM ("
               " SELECT DISTINCT(hwaddress) FROM arps"
              " UNION DISTINCT"
               " SELECT hwaddress FROM mactab"
              " UNION DISTINCT"
               " SELECT DISTINCT(hwaddress) FROM interfaces WHERE hwaddress IS NOT NULL) AS macs"
             " ORDER BY hwaddress ASC";
    QStringList items;
    if (execSql(*pq, sql)) do {
        // pUi->comboBox->addItem(pq->value(0).toString()); // SLOW !!
        items << pq->value(0).toString();
    } while (pq->next());
    pUiHead->comboBoxMAC->clear();
    pUiHead->comboBoxMAC->addItems(items);
}

void cFindByMac::setButtons()
{
    pUiHead->pushButtonFindMac->setEnabled(fMAC);
    pUiHead->pushButtonFindIp->setEnabled(fIP);
    pUiHead->pushButtonNMap->setEnabled(fIP);
    pUiHead->pushButtonExplore->setEnabled(fMAC && fIP && fSw);
    pUiHead->toolButtonMAC2IP->setEnabled(fMAC);
    pUiHead->toolButtonIP2MAC->setEnabled(fIP);
}

void cFindByMac::finishedPostWork()
{
    pUiHead->pushButtonFindMac->setDisabled(false);
    pUiHead->pushButtonExplore->setDisabled(false);
}

void cFindByMac::clearPostWork()
{
    pUiHead->pushButtonLocalhost->setEnabled(true);
    pUiHead->comboBoxLoMac->setDisabled(true);
    pUiHead->comboBoxLoMac->clear();
    pUiHead->comboBoxLoIp->setDisabled(true);
    pUiHead->comboBoxLoIp->clear();
    setAllMac();
    pUiHead->comboBoxIP->clear();
}

void cFindByMac::comboBoxMAC_currentTextChanged(const QString& s)
{
    fMAC = cMac::isValid(s);
    setButtons();
}

void cFindByMac::comboBoxIP_currentTextChanged(const QString& s)
{
    fIP = !QHostAddress(s).isNull();
    setButtons();
}

void cFindByMac::pushButtonFindMac_clicked()
{
    QString sMac = pUiHead->comboBoxMAC->currentText();
    QString text = htmlReportByMac(*pq, sMac);
    appendReport(text);
}

void cFindByMac::pushButtonExplore_clicked()
{
#ifdef SNMP_IS_EXISTS
    QHostAddress ip(pUiHead->comboBoxIP->currentText());
    cMac mac(pUiHead->comboBoxMAC->currentText());
    QString swName = pUiHead->comboBoxSw->currentText();
    discover(ip, mac, swName);
    pUiHead->pushButtonExplore->setDisabled(true);
    pUiHead->pushButtonFindMac->setDisabled(true);
#endif
}

void cFindByMac::toolButtonIP2MAC_clicked()
{
    QHostAddress ip(pUiHead->comboBoxIP->currentText());
    cMac mac = cArp::ip2mac(*pq, ip, EX_IGNORE);
    if (mac.isValid()) pUiHead->comboBoxMAC->setCurrentText(mac.toString());
}

void cFindByMac::toolButtonMAC2IP_clicked()
{
    cMac mac(pUiHead->comboBoxMAC->currentText());
    QList<QHostAddress> ipl = cArp::mac2ips(*pq, mac);
    pUiHead->comboBoxIP->clear();
    foreach (QHostAddress a, ipl) {
        pUiHead->comboBoxIP->addItem(a.toString());
    }
    pUiHead->comboBoxIP->setCurrentIndex(0);
}

void cFindByMac::pushButtonFindIp_clicked()
{
    QString sIp = pUiHead->comboBoxIP->currentText();
    QString text = htmlReportByIp(*pq, sIp);
    appendReport(text);
}

void cFindByMac::pushButtonNMap_clicked()
{
    NMap(pUiHead->comboBoxIP->currentText());
}

void cFindByMac::pushButtonLocalhost_clicked()
{
    pUiHead->pushButtonLocalhost->setDisabled(true);
    myInterfaces = QNetworkInterface::allInterfaces();
    QMutableListIterator<QNetworkInterface>    i(myInterfaces);
    cMac    mac;
    while (i.hasNext()) {
        QNetworkInterface &iface = i.next();
        mac = iface.hardwareAddress();
        if (mac.isValid()) {
            pUiHead->comboBoxLoMac->addItem(mac.toString());
        }
        else {
            i.remove();
        }
    }
    bool e = !myInterfaces.isEmpty();
    pUiHead->comboBoxLoMac->setEnabled(e);
    if (e) {
        pUiHead->comboBoxLoMac->setCurrentIndex(0);
        comboBoxLoMac_activated(0);
    }
}

void cFindByMac::comboBoxLoMac_activated(int index)
{
    pUiHead->comboBoxMAC->setCurrentText(myInterfaces.at(index).hardwareAddress());
    bool e = false;
    foreach (QNetworkAddressEntry ae, myInterfaces.at(index).addressEntries()) {
        QHostAddress a = ae.ip();
        if (!a.isNull()) {
            pUiHead->comboBoxLoIp->addItem(a.toString());
            e = true;
        }
    }
    pUiHead->comboBoxLoIp->setEnabled(e);
    if (e) {
        pUiHead->comboBoxLoIp->setCurrentIndex(0);
        comboBoxLoIp_activated(pUiHead->comboBoxLoIp->currentText());
    }
}

void cFindByMac::comboBoxLoIp_activated(const QString &arg1)
{
    pUiHead->comboBoxIP->setCurrentText(arg1);
}

void cFindByMac::pushButtonRDP_clicked()
{
#if defined(Q_OS_WINDOWS)
    rdpClientAddr = rdpClientAddress();
    pUi->comboBoxIP->setCurrentText(rdpClientAddr.toString());
    on_toolButtonIP2MAC_clicked();
#endif  // defined(Q_OS_WINDOWS)
}
