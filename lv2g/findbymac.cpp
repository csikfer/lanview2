#include "findbymac.h"
#include "srvdata.h"
#include "lv2link.h"
#include "cerrormessagebox.h"
#include "report.h"
#include "scan.h"

#include "ui_findbymac.h"

const enum ePrivilegeLevel cFindByMac::rights = PL_VIEWER;

cFindByMac::cFindByMac(QMdiArea *parent) :
    cIntSubObj(parent),
    pUi(new Ui::FindByMac)
{
    pq = newQuery();
    fMAC = fIP = fSw = false;
    pUi->setupUi(this);
    setAllMac();
#ifdef SNMP_IS_EXISTS
    setSwitchs();
    connect(pUi->pushButtonExplore, SIGNAL(clicked()),                      this, SLOT(hit_explore()));
#else   // SNMP_IS_EXISTS
    pUi->comboBoxSw->hide();
    pUi->labelSw->hide();
    pUi->pushButtonExplore->hide();
    fSw = fale;
#endif  // SNMP_IS_EXISTS
    connect(pUi->comboBoxMAC,       SIGNAL(currentTextChanged(QString)),    this, SLOT(changeMAC(QString)));
    connect(pUi->comboBoxIP ,       SIGNAL(currentTextChanged(QString)),    this, SLOT(changeIP(QString)));
    connect(pUi->pushButtonClear,   SIGNAL(clicked()),                      this, SLOT(hit_clear()));
    connect(pUi->pushButtonFindMac, SIGNAL(clicked()),                      this, SLOT(hit_find()));
    connect(pUi->toolButtonIP2MAC,  SIGNAL(clicked()),                      this, SLOT(ip2mac()));
    connect(pUi->toolButtonMAC2IP,  SIGNAL(clicked()),                      this, SLOT(mac2ip()));
    changeMAC(pUi->comboBoxMAC->currentText());
}

cFindByMac::~cFindByMac()
{
    delete pq;
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
    pUi->comboBoxMAC->clear();
    pUi->comboBoxMAC->addItems(items);
}

void cFindByMac::setSwitchs()
{
#ifdef SNMP_IS_EXISTS
    static const QString sql = "SELECT node_name FROM snmpdevices WHERE 'switch' = ANY (node_type) ORDER BY node_name";
    QStringList items;
    if (execSql(*pq, sql)) do {
        // pUi->comboBox->addItem(pq->value(0).toString()); // SLOW !!
        items << pq->value(0).toString();
    } while (pq->next());
    pUi->comboBoxSw->clear();
    pUi->comboBoxSw->addItems(items);
    fSw = !items.isEmpty();
    if (fSw) pUi->comboBoxSw->setCurrentIndex(0);
#endif
}

void cFindByMac::setButtons()
{
    pUi->pushButtonFindMac->setEnabled(fMAC);
    pUi->pushButtonExplore->setEnabled(fMAC && fIP && fSw);
    pUi->toolButtonMAC2IP->setEnabled(fMAC);
    pUi->toolButtonIP2MAC->setEnabled(fIP);
}

void cFindByMac::changeMAC(const QString& s)
{
    fMAC = cMac::isValid(s);
    setButtons();
}

void cFindByMac::changeIP(const QString& s)
{
    fIP = !QHostAddress(s).isNull();
    setButtons();
}

void cFindByMac::hit_clear()
{
    setAllMac();
    pUi->comboBoxIP->clear();
}

void cFindByMac::hit_find()
{
    QString sMac = pUi->comboBoxMAC->currentText();
    QString text = reportByMac(*pq, sMac);
    pUi->textEdit->setHtml(text);
}

void cFindByMac::hit_explore()
{
#ifdef SNMP_IS_EXISTS
    QSqlQuery q = getQuery();
    QHostAddress ip(pUi->comboBoxIP->currentText());
    cMac mac(pUi->comboBoxMAC->currentText());
    QString swName = pUi->comboBoxSw->currentText();
    cSnmpDevice sw;
    if (ip.isNull() || !mac.isValid() || !sw.fetchByName(q, swName)) {
        pUi->textEdit->setHtml(htmlError(trUtf8("Hibás adatok.")));
        return;
    }
    cExportQueue::init(false);
    exploreByAddress(mac, ip, sw);
    QString text = cExportQueue::toText(false, true);   // Töröljük is
    pUi->textEdit->setHtml(text);
#endif
}

void cFindByMac::ip2mac()
{
    QHostAddress ip(pUi->comboBoxIP->currentText());
    cMac mac = cArp::ip2mac(*pq, ip, EX_IGNORE);
    if (mac.isValid()) pUi->comboBoxMAC->setCurrentText(mac.toString());
}

void cFindByMac::mac2ip()
{
    cMac mac(pUi->comboBoxMAC->currentText());
    QList<QHostAddress> ipl = cArp::mac2ips(*pq, mac);
    pUi->comboBoxIP->clear();
    foreach (QHostAddress a, ipl) {
        pUi->comboBoxIP->addItem(a.toString());
    }
    pUi->comboBoxIP->setCurrentIndex(0);
}
