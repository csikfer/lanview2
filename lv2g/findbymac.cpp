#include "findbymac.h"
#include "lv2link.h"
#include "ui_findbymac.h"
#include "cerrormessagebox.h"

cFindByMac::cFindByMac(QWidget *parent) :
    cOwnTab(parent),
    pUi(new Ui::FindByMac)
{
    pq = newQuery();
    pUi->setupUi(this);
    QString sql =
            "SELECT hwaddress FROM ("
               " SELECT DISTINCT(hwaddress) FROM arps"
              " UNION DISTINCT"
               " SELECT hwaddress FROM mactab"
              " UNION DISTINCT"
               " SELECT DISTINCT(hwaddress) FROM interfaces) AS macs"
             " ORDER BY hwaddress ASC";
    if (execSql(*pq, sql)) do {
        QString mac = pq->value(0).toString();
        if (!mac.isEmpty() && mac != "00:00:00:00:00:00") {
            pUi->comboBox->addItem(mac);
        }
    } while (pq->next());
    connect(pUi->pushButtonClose,   SIGNAL(clicked()), this, SLOT(endIt()));
    connect(pUi->pushButtonFindMac, SIGNAL(clicked()), this, SLOT(find()));
}

cFindByMac::~cFindByMac()
{
    delete pUi;
    delete pq;
}

void cFindByMac::find()
{
    static const QString th   = "<th> %1 </th>";
    static const QString td   = "<td> %1 </td>";
    static const QString bold = "<b>%1</b>";
    static const QString nil  = " - ";
    static const QString sep  = ", ";
#define SEP_SIZE 2
    QString text;
    pUi->textEdit->clear();
    QString sMac = pUi->comboBox->currentText();
    cMac mac(sMac);
    if (!mac) {
        text = trUtf8("A '%1' nem valós MC!").arg(sMac);
        pUi->textEdit->setPlainText(text);
        return;
    }
    /* ** OUI ** */
    cOui oui;
    if (oui.fetchByMac(*pq, mac)) {
        text += "OUI rekord : " + oui.getName() + "<br>";
        text += oui.getNote() + "<br><br>";
    }
    else {
        text += "Nincs OUI rekord.<br><br>";
    }
    /* ** NODE ** */
    cNode node;
    if (node.fetchByMac(*pq, mac)) {
        node.fetchPorts(*pq);
        node.fetchParams(*pq);
        QString tablename = node.getOriginalDescr(*pq)->tableName();
        text += trUtf8("Bejegyzett hálózati elem (%1) : <b> %2</b><br>").arg(tablename, node.getName());
        text += "<style>";
        text +=  "table { border-spacing: 5px; }";
        text +=  "table, th, td { border: 2px solid black; } ";
        text += "</style>";
        text += "<table> <tr>";
        text += th.arg("port");
        text += th.arg("típus");
        text += th.arg("ip cím(ek)");
        text += th.arg("DNS név");
        text += th.arg("Fizikai link");
        text += th.arg("Logikai link");
        text += th.arg("LLDP link");
        text += "</tr>\n";
        /* -- PORTS -- */
        QListIterator<cNPort *> li(node.ports);
        while (li.hasNext()) {
             cNPort * p = li.next();
             text += "<tr>";
             text += td.arg(p->getName());
             text += td.arg(cIfType::ifTypeName(p->getId(_sIfTypeId)));
             if (p->descr() == cInterface::_descr_cInterface()) {  // Interface
                 QString ips, dns;
                 cInterface *i = p->reconvert<cInterface>();
                 QListIterator<cIpAddress *> ii(i->addresses);
                 while (ii.hasNext()) {
                      cIpAddress * ia = ii.next();
                      ips += bold.arg(ia->view(*pq, _sAddress)) + "/" + ia->getName(_sIpAddressType) + sep;
                      QHostInfo hi = QHostInfo::fromName(ia->address().toString());
                      dns += hi.hostName() + sep;
                 }
                 ips.chop(SEP_SIZE);
                 dns.chop(SEP_SIZE);
                 text += td.arg(ips);
                 text += td.arg(dns);
             }
             else {
                 text += td.arg(nil);
                 text += td.arg(nil);
             }
             qlonglong id, lp;
             id = p->getId();
             lp = LinkGetLinked<cPhsLink>(*pq, id);
             text += td.arg(lp == NULL_ID ? nil : cNPort::getFullNameById(*pq, lp));
             lp = LinkGetLinked<cLogLink>(*pq, id);
             text += td.arg(lp == NULL_ID ? nil : cNPort::getFullNameById(*pq, lp));
             lp = LinkGetLinked<cLldpLink>(*pq, id);
             text += td.arg(lp == NULL_ID ? nil : cNPort::getFullNameById(*pq, lp));


             text += "</tr>\n";
        }
        text += "</table><br><br>";
    }
    else {
        text += trUtf8("Nincs bejegyzett hálózati elem ezzel a MAC-el<br><br>");
    }
    /* ** ARP ** */
    QList<QHostAddress> al = cArp::mac2ips(*pq, mac);
    if (al.isEmpty()) {
        text += trUtf8("Nincs találat az arps táblában a megadott MAC-el<br><br>");
    }
    else {
        QString ips;
        foreach (QHostAddress a, al) {
            ips += a.toString() + ", ";
        }
        ips.chop(2);
        text += trUtf8("Az arps táblában a megadott MAC-hez talált IP címek : <b>%1</b><br><br>").arg(ips);
    }
    /* ** MAC TAB** */
    cMacTab mt;
    mt.setMac(_sHwAddress, mac);
    if (mt.completion(*pq)) {
        text += trUtf8("Találat a switch cím táblában : <b>%1</b> (%2 - %3; %4) <br><br>").arg(
                    cNPort::getFullNameById(*pq, mt.getId(_sPortId)),
                    mt.view(*pq, _sFirstTime),
                    mt.view(*pq, _sLastTime),
                    mt.view(*pq, _sMacTabState));
    }
    else {
        text += trUtf8("Nincs találat a switch címtábákban a megadott MAC-el<br><br>");
    }
    pUi->textEdit->setHtml(text);
}
